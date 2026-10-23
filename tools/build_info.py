"""Release identity shared by firmware, companion assets, and packages."""
import json
import os
from pathlib import Path
import re
import subprocess


def build_info(root=None):
    root = Path(root or Path(__file__).resolve().parents[1])
    version = os.environ.get('MESHIT_VERSION') or json.loads((root / 'package.json').read_text())['version']
    if not re.fullmatch(r'v?(?:0|[1-9][0-9]*)\.(?:0|[1-9][0-9]*)\.(?:0|[1-9][0-9]*)(?:[.+-][a-zA-Z0-9.+-]+)?', version):
        raise ValueError('MESHIT_VERSION must be a version such as v0.1.0 or v0.1.0.abcdef0-beta')
    top = subprocess.run(['git', 'rev-parse', '--show-toplevel'], cwd=root, capture_output=True, text=True)
    # This workspace can be nested inside an unrelated parent checkout.
    result = subprocess.run(['git', 'rev-parse', 'HEAD'], cwd=root, capture_output=True, text=True)
    own_checkout = top.returncode == 0 and Path(top.stdout.strip()).resolve() == root.resolve()
    return {'version': version, 'commit': result.stdout.strip() if own_checkout and result.returncode == 0 else 'unknown'}


def android_apk_name(info):
    version = info['version'].removeprefix('v')
    return f"meshit-android-v{version}-{info['commit'][:7]}-debug.apk"


if __name__ == '__main__':
    print(json.dumps(build_info()))
