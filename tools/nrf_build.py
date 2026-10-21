from pathlib import Path
import json
import sys
Import('env')
root = Path(env.subst('$PROJECT_DIR'))
sys.path.insert(0, str(root / 'tools'))
from build_info import build_info
build = Path(env.subst('$BUILD_DIR'))
build.mkdir(parents=True, exist_ok=True)
(build / 'build-info.json').write_text(json.dumps(build_info(root), indent=2)+'\n')
framework = Path(env.PioPlatform().get_package_dir('framework-arduinoadafruitnrf52'))
env.Append(LIBPATH=[str(framework / 'cores/nRF5/linker')])
env.Append(CPPPATH=[str(root / 'firmware/nrf52')])
