#!/usr/bin/env python3
from pathlib import Path
import subprocess
root=Path(__file__).resolve().parents[1]
crypto=root/'.pio/libdeps/heltec-v3/Crypto'
json=root/'.pio/libdeps/heltec-v3/ArduinoJson/src'
if not crypto.exists() or not json.exists():
    raise SystemExit('Install firmware dependencies first: pio pkg install -e heltec-v3')
out=root/'output';out.mkdir(exist_ok=True)
sources=['Crypto','Cipher','AuthenticatedCipher','BlockCipher','ChaCha','ChaChaPoly','Poly1305','SHA256','Hash','AES128','AESCommon']
subprocess.run(['c++','-std=c++17','-O1','-I'+str(crypto),'-I'+str(json),'-I'+str(root/'lib/Meshit/src'),str(root/'tools/simulator.cpp'),*[str(crypto/(s+'.cpp')) for s in sources],'-o',str(out/'simulator')],check=True)
