#!/usr/bin/env python3
"""Generate, build, lint, and package the Android development companion."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
from build_info import build_info, android_apk_name

ROOT = Path(__file__).resolve().parents[1]


def build(offline=False):
    env = os.environ.copy()
    java = Path('/Applications/Android Studio.app/Contents/jbr/Contents/Home')
    if not env.get('JAVA_HOME') and java.exists():
        env['JAVA_HOME'] = str(java)
    sdk = Path.home()/'Library/Android/sdk'
    if not env.get('ANDROID_HOME') and sdk.exists():
        env['ANDROID_HOME'] = str(sdk)
    subprocess.run(['npm', 'run', 'mobile:android'], cwd=ROOT, env=env, check=True)
    command = ['gradlew.bat' if os.name == 'nt' else './gradlew', ':app:assembleDebug', ':app:lintDebug', ':app:testDebugUnitTest']
    if offline:
        command.append('--offline')
    subprocess.run(command, cwd=ROOT/'android', env=env, check=True)
    output = ROOT/'dist/companions'
    output.mkdir(parents=True, exist_ok=True)
    info = build_info(ROOT)
    apk = output/android_apk_name(info)
    shutil.copyfile(ROOT/'android/app/build/outputs/apk/debug/app-debug.apk', apk)
    (output/'build-info.json').write_text(json.dumps(info, indent=2)+'\n')
    apk.with_suffix('.apk.sha256').write_text(f'{hashlib.sha256(apk.read_bytes()).hexdigest()}  {apk.name}\n')
    # Retire the old unversioned download so it cannot be mistaken for this build.
    for legacy in ('meshit-android-debug.apk', 'meshit-android-debug.apk.sha256'):
        (output/legacy).unlink(missing_ok=True)
    print(f'Android debug APK: {apk}')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--offline', action='store_true')
    build(parser.parse_args().offline)
