#!/usr/bin/env python3
"""Compile the real firmware core with ASan/UBSan, then verify independent vectors."""
import os
from pathlib import Path
import subprocess
root = Path(__file__).resolve().parents[1]
crypto = Path(os.environ.get('MESHIT_CRYPTO_DIR', root / '.pio/libdeps/heltec-v3/Crypto'))
if not crypto.exists():
    raise SystemExit('Install firmware dependencies first: pio pkg install -e heltec-v3')
out = root / 'output'
out.mkdir(exist_ok=True)
sources = ['Crypto','Cipher','AuthenticatedCipher','BlockCipher','ChaCha','ChaChaPoly','Poly1305','SHA256','Hash','AES128','AESCommon']
subprocess.run(['c++','-std=c++17','-g','-O1','-fsanitize=address,undefined','-fno-omit-frame-pointer','-I'+str(crypto),'-I'+str(root/'lib/Meshit/src'),str(root/'tests/core.cpp'),*[str(crypto/(s+'.cpp')) for s in sources],'-o',str(out/'test-core')],check=True)
result = subprocess.run([str(out/'test-core')],check=True,capture_output=True,text=True)
(out/'vectors.json').write_text(result.stdout)
subprocess.run(['node',str(root/'tests/vectors.mjs'),str(out/'vectors.json')],check=True)
print('Core behavior, bounded history, recovery, snapshots, UTF-8, carriers and encryption passed (ASan/UBSan).')
