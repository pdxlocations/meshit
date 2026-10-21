#!/usr/bin/env python3
"""Package all built targets, including ESP full-flash images and validated RAK UF2."""
from pathlib import Path
import csv
import hashlib
import json
import os
import shutil
import struct
import subprocess
import tempfile
from build_info import build_info
from check_firmware_targets import check

ROOT = Path(__file__).resolve().parents[1]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def metadata(path, **extra):
    return {'file': path.name, 'bytes': path.stat().st_size, 'sha256': digest(path), **extra}


def validate_uf2(path, target):
    image = path.read_bytes()
    if not image or len(image) % 512:
        raise ValueError('Invalid UF2 block length')
    start, end = int(target['applicationStart'], 0), int(target['applicationEnd'], 0)
    occupied, vectors = set(), {}
    for at in range(0, len(image), 512):
        m1, m2, flags, address, size, number, count, family = struct.unpack('<8I', image[at:at+32])
        if (m1, m2, flags, size, number, count, family, struct.unpack_from('<I', image, at+508)[0]) != (
                0x0a324655, 0x9e5d5157, 0x2000, 256, at//512, len(image)//512, 0xada52840, 0x0ab16f30):
            raise ValueError('Invalid nRF52840 UF2 header')
        if address < start or address+size > end or address in occupied:
            raise ValueError('UF2 overlaps reserved memory or duplicate application blocks')
        occupied.add(address)
        if address == start:
            vectors['sp'], vectors['pc'] = struct.unpack_from('<II', image, at+32)
    if min(occupied) != start or not (0x20008000 < vectors.get('sp', 0) <= 0x20040000):
        raise ValueError('UF2 application offset or stack pointer does not match the selected SoftDevice')
    pc = vectors.get('pc', 0)
    if not (pc & 1 and start <= pc-1 < end):
        raise ValueError('UF2 reset vector is outside the application')


def package(root=ROOT):
    esp, nrf = check(root)
    info = build_info(root)
    dist = root / 'dist'
    dist.mkdir(exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='firmware-', dir=dist) as tmp:
        stage = Path(tmp)
        manifest = []
        for target in esp:
            name = target['environment']
            build = root / '.pio/build' / name
            source = build / 'firmware.bin'
            layout = json.loads((build / 'flash-layout.json').read_text())
            if layout['chip'] != 'esp32s3' or layout['size'] != f"{target['flashMiB']}MB":
                raise ValueError(f'Unexpected flash processor/size for {name}')
            if layout['build'] != info:
                raise ValueError(f'Rebuild {name} with the selected release identity before packaging')
            rows = []
            app_offset = app_size = None
            for row in csv.reader(line for line in (root / target['partitions']).read_text().splitlines()
                                  if line.strip() and not line.lstrip().startswith('#')):
                offset, size = int(row[3], 0), int(row[4], 0)
                if (rows and offset < rows[-1][0] + rows[-1][1]) or offset + size > target['flashMiB']*1024*1024:
                    raise ValueError('Overlapping or oversized flash partitions')
                rows.append((offset, size))
                if row[0] == 'app0':
                    app_offset, app_size = offset, size
            if app_offset != 0x10000 or source.stat().st_size > app_size:
                raise ValueError('Firmware exceeds the declared application partition')
            app = stage / f'meshit-{name}.bin'
            shutil.copyfile(source, app)
            factory = stage / f'meshit-{name}-factory.bin'
            command = [layout['python'], layout['esptool'], '--chip', layout['chip'], 'merge_bin',
                       '-o', str(factory), '--flash_mode', layout['mode'], '--flash_freq', layout['frequency'],
                       '--flash_size', layout['size'], '--fill-flash-size', layout['size']]
            offsets = set()
            for offset, image in layout['images']:
                if offset in offsets or offset < 0 or offset + Path(image).stat().st_size > app_offset:
                    raise ValueError('Invalid boot/partition/OTA flash inputs')
                offsets.add(offset)
                command.extend([hex(offset), image])
            if not {0, 0x8000, 0xe000}.issubset(offsets):
                raise ValueError('Missing bootloader, partition table, or OTA initialization image')
            command.extend([hex(app_offset), str(source)])
            subprocess.run(command, cwd=root, check=True)
            if factory.stat().st_size != target['flashMiB']*1024*1024:
                raise ValueError('Fresh-install image does not span the entire flash')
            merged = factory.read_bytes()
            if merged[app_offset:app_offset+source.stat().st_size] != source.read_bytes():
                raise ValueError('Factory application bytes differ from app-only update')
            if merged[app_offset+source.stat().st_size:] != b'\xff'*(len(merged)-app_offset-source.stat().st_size):
                raise ValueError('Fresh-install image contains unexpected data in the cleared storage area')
            manifest.append({**target, **metadata(app, kind='app-only', flashOffset='0x10000'),
                             'hardwareValidated': False,
                             'factory': metadata(factory, kind='factory', flashOffset='0x0', erasesData=True)})
        pio_core = Path(os.environ.get('PLATFORMIO_CORE_DIR', Path.home()/'.platformio'))
        converter = pio_core/'packages/framework-arduinoadafruitnrf52/tools/uf2conv/uf2conv.py'
        nrf_images = []
        for target in nrf:
            name = target['environment']
            build = root/'.pio/build'/name
            if json.loads((build/'build-info.json').read_text()) != info:
                raise ValueError(f'Rebuild {name} with the selected release identity before packaging')
            hex_image = stage/f'meshit-{name}.hex'
            shutil.copyfile(build/'firmware.hex', hex_image)
            uf2 = stage/f'meshit-{name}.uf2'
            subprocess.run([shutil.which('python3') or 'python', str(converter), str(hex_image), '-c',
                            '-f', '0xADA52840', '-o', str(uf2)], check=True)
            validate_uf2(uf2, target)
            nrf_images.extend([hex_image, uf2])
        (stage/'manifest.json').write_text(json.dumps(manifest, indent=2)+'\n')
        (stage/'nrf52-targets.json').write_text(json.dumps(nrf, indent=2)+'\n')
        (stage/'build-info.json').write_text(json.dumps(info, indent=2)+'\n')
        esp_images = [stage/image['file'] for t in manifest for image in [t, t['factory']]]
        (stage/'SHA256SUMS-ESP32').write_text(''.join(f'{digest(p)}  {p.name}\n' for p in esp_images))
        (stage/'SHA256SUMS-NRF52').write_text(''.join(f'{digest(p)}  {p.name}\n' for p in nrf_images))
        (stage/'SHA256SUMS').write_text(''.join(f'{digest(p)}  {p.name}\n' for p in esp_images+nrf_images))
        output = dist/'firmware'
        if output.exists():
            shutil.rmtree(output)
        shutil.copytree(stage, output)
    print(f'Packaged {len(esp)} ESP and {len(nrf)} nRF targets: {output}')


if __name__ == '__main__':
    package()
