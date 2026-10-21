"""Export PlatformIO's actual flash inputs for the browser factory image packager."""
import json
import sys
from pathlib import Path
Import('env')

board = env.BoardConfig()
root = Path(env.subst('$PROJECT_DIR'))
sys.path.insert(0, str(root / 'tools'))
from build_info import build_info
mode = env.subst('$BOARD_FLASH_MODE')
memory = board.get('build.arduino.memory_type', '')
if memory in ('opi_opi', 'opi_qspi'):
    mode = 'dout'
elif mode in ('qio', 'qout'):
    mode = 'dio'
frequency = int(env.subst('$BOARD_F_FLASH').rstrip('Ll')) // 1000000
layout = {
    'build': build_info(root),
    'chip': board.get('build.mcu'),
    'mode': mode,
    'frequency': f'{frequency}m',
    'size': board.get('upload.flash_size'),
    'python': env.subst('$PYTHONEXE'),
    'esptool': str(Path(env.PioPlatform().get_package_dir('tool-esptoolpy')) / 'esptool.py'),
    'images': [[int(str(offset), 0), str(Path(env.subst(path)).resolve())]
               for offset, path in env.get('FLASH_EXTRA_IMAGES', [])],
}
output = Path(env.subst('$BUILD_DIR')) / 'flash-layout.json'
output.parent.mkdir(parents=True, exist_ok=True)
output.write_text(json.dumps(layout, indent=2) + '\n')
