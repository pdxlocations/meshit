"""Embed gzip assets in the ESP application; no separate filesystem upload."""
from pathlib import Path
import gzip
import json
import sys
try:
    Import('env')
    root = Path(env.subst('$PROJECT_DIR'))
except NameError:
    root = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(root / 'tools'))
from build_info import build_info
info = build_info(root)
lines = ['#pragma once', '#include <Arduino.h>', 'static const char FIRMWARE_VERSION[] = ' + json.dumps(info['version']) + ';']
presets = json.dumps(json.loads((root / 'config/presets.json').read_text()), separators=(',', ':'))
lines += ['static const char PRESETS[] = R"PRESETS(' + presets + ')PRESETS";']
lines += ['#if defined(MESHIT_ESP32)', 'struct WebAsset {const char* path;const char* mime;const uint8_t* data;size_t size;};']
assets = []
for i, (name, mime) in enumerate([('index.html','text/html'),('app.js','text/javascript'),('transport.js','text/javascript'),('style.css','text/css')]):
    data = gzip.compress((root / 'web' / name).read_bytes(), mtime=0)
    lines += [f'static const uint8_t asset{i}[] PROGMEM = {{' + ','.join(map(str,data)) + '};']
    assets.append(f'{{"/{name}","{mime}",asset{i},sizeof(asset{i})}}')
lines += ['static const WebAsset WEB_ASSETS[] = {' + ','.join(assets) + '};', '#endif']
output = root / 'include/BuildAssets.h'
text = '\n'.join(lines) + '\n'
if not output.exists() or output.read_text() != text:
    output.write_text(text)
