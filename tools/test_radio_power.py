#!/usr/bin/env python3
"""Check TX power selection against the resolved flags of every firmware environment."""
import configparser
import re
import shlex
import subprocess
from pathlib import Path
root = Path(__file__).resolve().parents[1]
ini = configparser.ConfigParser(interpolation=None)
ini.read(root / 'platformio.ini')
def flags(section):
    if 'build_flags' in ini[section]:
        value = ini[section]['build_flags']
    elif 'extends' in ini[section]:
        return flags(ini[section]['extends'])
    else:
        return flags('env')
    return re.sub(r'\$\{([^}]+)\}', lambda match: flags(match[1].rsplit('.', 1)[0]), value)
for section in ini.sections():
    if not section.startswith('env:'):
        continue
    name = section[4:]
    maximum = 28 if name in ('heltec-v4', 'heltec-v4-3') else 22
    source = f'''#include "RadioPower.h"
using namespace meshit;
static_assert(maxTxPower(boardPowerProfile)=={maximum});
static_assert(selectTxPower(boardPowerProfile,30).output=={maximum});
static_assert(selectTxPower(boardPowerProfile,27).output=={min(maximum,27)});
static_assert(selectTxPower(boardPowerProfile,10).output==10);
static_assert(selectTxPower(boardPowerProfile,30).drive<={21 if maximum==28 else 22});
'''
    defines = [flag for flag in shlex.split(flags(section)) if flag.startswith('-D')]
    subprocess.run(['c++', '-std=c++17', '-fsyntax-only', '-x', 'c++', '-I'+str(root/'include'), *defines, '-'], input=source, text=True, check=True)
    print(f'{name}: TX power selection passed')
