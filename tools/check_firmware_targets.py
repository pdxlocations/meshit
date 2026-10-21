#!/usr/bin/env python3
"""Fail when a PlatformIO environment is missing from the downloadable catalogs."""
import configparser
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def check(root=ROOT):
    ini = configparser.ConfigParser(interpolation=None)
    ini.read(root / 'platformio.ini')
    environments = {s[4:] for s in ini.sections() if s.startswith('env:')}
    esp = json.loads((root / 'config/firmware-targets.json').read_text())
    nrf = json.loads((root / 'config/nrf52-targets.json').read_text())
    listed = [t['environment'] for t in esp + nrf]
    if len(set(listed)) != len(listed) or set(listed) != environments:
        raise ValueError(f'Catalog/PlatformIO mismatch: environments={sorted(environments)}, catalog={listed}')
    for t in esp:
        if t['chipFamily'] != 'ESP32-S3' or not (root / t['partitions']).is_file():
            raise ValueError(f'Invalid ESP target: {t}')
    for t in nrf:
        expected = {'6.1.1':'0x26000', '7.3.0':'0x27000'}.get(t['softdevice'])
        if t['applicationStart'] != expected or t['applicationEnd'] != '0x80000':
            raise ValueError(f'Invalid nRF memory layout: {t}')
        name = t['environment']
        linker = ini[f'env:{name}'].get('board_build.ldscript', ini['nrf52']['board_build.ldscript'])
        text = (root / linker).read_text()
        if f'ORIGIN = {expected}' not in text or '0x80000 -' not in text:
            raise ValueError(f'Linker/catalog mismatch: {name}')
    return esp, nrf


if __name__ == '__main__':
    esp, nrf = check()
    print(f'All {len(esp)} ESP and {len(nrf)} nRF target profiles match PlatformIO and linker layouts.')
