#!/usr/bin/env python3
"""Create the foldered release bundle consumed by the PHP web flasher."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import tempfile
from zipfile import ZipFile, ZIP_DEFLATED
from build_info import build_info, android_apk_name
from check_firmware_targets import check

ROOT = Path(__file__).resolve().parents[1]


def copy_verified(source, destination, expected):
    if hashlib.sha256(source.read_bytes()).hexdigest() != expected:
        raise ValueError(f'Checksum mismatch: {source.name}')
    shutil.copyfile(source, destination)


def package(root=ROOT, require_android=False):
    dist, firmware = root/'dist', root/'dist/firmware'
    info = build_info(root)
    if json.loads((firmware/'build-info.json').read_text()) != info:
        raise ValueError('Firmware and release versions/commits differ')
    esp_catalog, nrf_catalog = check(root)
    manifest = json.loads((firmware/'manifest.json').read_text())
    if sorted(t['environment'] for t in manifest) != sorted(t['environment'] for t in esp_catalog):
        raise ValueError('Release is missing ESP profiles or contains duplicate/unknown profiles')
    if json.loads((firmware/'nrf52-targets.json').read_text()) != nrf_catalog:
        raise ValueError('Release nRF profiles differ from the source catalog')
    nrf_sums = [line.split() for line in (firmware/'SHA256SUMS-NRF52').read_text().splitlines()]
    expected_nrf = {f'meshit-{t["environment"]}.{ext}' for t in nrf_catalog for ext in ['uf2','hex']}
    if any(len(line) != 2 for line in nrf_sums) or len(nrf_sums) != len(expected_nrf) or {line[1] for line in nrf_sums} != expected_nrf:
        raise ValueError('Release is missing nRF images or contains duplicate/unknown images')
    with tempfile.TemporaryDirectory(prefix='release-', dir=dist) as tmp:
        tree = Path(tmp)/'bundle'
        esp, nrf = tree/'esp32', tree/'nrf52'
        esp.mkdir(parents=True)
        nrf.mkdir()
        for target in manifest:
            for image in [target, target['factory']]:
                name = image['file']
                if Path(name).name != name:
                    raise ValueError('Invalid firmware filename')
                copy_verified(firmware/name, esp/name, image['sha256'])
        for name in ['manifest.json', 'build-info.json']:
            shutil.copyfile(firmware/name, esp/name)
        shutil.copyfile(firmware/'SHA256SUMS-ESP32', esp/'SHA256SUMS')
        for digest, name in nrf_sums:
            if Path(name).name != name or Path(name).suffix not in ('.uf2', '.hex'):
                raise ValueError('Invalid nRF checksum entry')
            copy_verified(firmware/name, nrf/name, digest)
        for name in ['nrf52-targets.json', 'SHA256SUMS-NRF52']:
            shutil.copyfile(firmware/name, nrf/name)
        shutil.copytree(root/'webflasher', tree/'webflasher')
        (tree/'build-info.json').write_text(json.dumps(info, indent=2)+'\n')
        (tree/'README.txt').write_text(
            f'meshit {info["version"]}\nCommit: {info["commit"]}\n\n'
            'esp32/      App-only updates at 0x10000; full-flash factory images at 0x0.\n'
            'nrf52/      RAK4631 / T114 application UF2/HEX; match the installed S140 version.\n'
            'webflasher/ Upload these contents to a PHP 8.1+ HTTPS web host.\n\n'
            'ESP updates: leave Erase device unchecked to preserve existing meshit storage.\n'
            'Factory images replace ALL flash, including messages and Wi-Fi settings.\n'
            'RAK S140 6.1.1: meshit-rak4631-s140-6-1-1.uf2.\n'
            'RAK S140 7.3.0: meshit-rak4631.uf2. These do not replace the SoftDevice.\n'
            'T114 S140 7.3.0: meshit-heltec-t114.uf2. No SoftDevice replacement.\n'
            'The Android development APK is a separate asset on the same GitHub release.\n'
            'Setup: https://github.com/pdxlocations/meshit#readme\n')
        checks = []
        for p in sorted(tree.rglob('*')):
            if p.is_file():
                checks.append(f'{hashlib.sha256(p.read_bytes()).hexdigest()}  {p.relative_to(tree).as_posix()}')
        (tree/'SHA256SUMS').write_text('\n'.join(checks)+'\n')
        archive = Path(tmp)/'meshit-release.zip'
        with ZipFile(archive, 'w', ZIP_DEFLATED) as bundle:
            for p in sorted(tree.rglob('*')):
                if p.is_file():
                    bundle.write(p, p.relative_to(tree).as_posix())
        with ZipFile(archive) as bundle:
            if bundle.testzip() is not None:
                raise ValueError('Invalid release ZIP')
        if require_android:
            apk = dist/'companions'/android_apk_name(info)
            if not apk.is_file() or json.loads((apk.parent/'build-info.json').read_text()) != info:
                raise ValueError('Build the matching Android companion before releasing')
            shutil.copyfile(apk, dist/apk.name)
            for legacy in ('meshit-android-debug.apk', 'meshit-android-debug.apk.sha256'):
                (dist/legacy).unlink(missing_ok=True)
            (dist/(apk.name+'.sha256')).write_text(f'{hashlib.sha256(apk.read_bytes()).hexdigest()}  {apk.name}\n')
        output = dist/'release'
        if output.exists():
            shutil.rmtree(output)
        shutil.copytree(tree, output)
        shutil.copyfile(archive, dist/archive.name)
        (dist/'meshit-release.zip.sha256').write_text(f'{hashlib.sha256(archive.read_bytes()).hexdigest()}  meshit-release.zip\n')
    print(f'Release folder: {output}\nGitHub bundle: {dist / "meshit-release.zip"}')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--require-android', action='store_true')
    args = parser.parse_args()
    package(require_android=args.require_android)
