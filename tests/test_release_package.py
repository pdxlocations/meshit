import json
import os
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch
from zipfile import ZipFile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'tools'))
from build_info import build_info
from package_release import package
from check_firmware_targets import check


class ReleaseTests(unittest.TestCase):
    def test_versions_reject_shell_or_path_input(self):
        for version in ['1.0.0', 'v1.0.0', 'v1.0.0.abcdef0-beta']:
            with patch.dict(os.environ, {'MESHIT_VERSION': version}):
                self.assertEqual(build_info(ROOT)['version'], version)
        for version in ['../file', '0.1.0\ninjected', 'v01.2.3', '$(command)', 'release-1']:
            with patch.dict(os.environ, {'MESHIT_VERSION': version}):
                with self.assertRaises(ValueError):
                    build_info(ROOT)

    def test_every_pio_target_is_cataloged(self):
        esp, nrf = check(ROOT)
        self.assertEqual({t["environment"] for t in esp+nrf}, {"heltec-v3", "heltec-v4", "heltec-v4-3", "rak4631", "rak4631-s140-6-1-1", "heltec-t114", "t-deck", "t-deck-plus", "t-deck-pro", "t-deck-pro-v1-1", "t-deck-pro-max"})

    def test_releases_reject_mixed_builds_before_replacing_previous_bundle(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root/'package.json').write_text('{"version":"0.1.0"}')
            fw = root/'dist/firmware'
            fw.mkdir(parents=True)
            (fw/'build-info.json').write_text('{"version":"0.2.0","commit":"unknown"}')
            old = root/'dist/meshit-release.zip'
            old.write_bytes(b'previous bundle')
            with patch.dict(os.environ, {'MESHIT_VERSION': '0.1.0'}):
                with self.assertRaisesRegex(ValueError, 'versions/commits'):
                    package(root)
            self.assertEqual(old.read_bytes(), b'previous bundle')

    def test_actual_bundle_is_complete_and_checksums_match(self):
        import hashlib
        fw = ROOT/'dist/firmware'
        self.assertTrue((fw/'nrf52-targets.json').is_file(), 'Package firmware before running release tests')
        package(ROOT)
        archive = ROOT/'dist/meshit-release.zip'
        with ZipFile(archive) as bundle:
            self.assertIsNone(bundle.testzip())
            names = set(bundle.namelist())
            for name in ['esp32/meshit-heltec-v3.bin', 'esp32/meshit-heltec-v3-factory.bin',
                         'nrf52/meshit-rak4631.uf2', 'nrf52/meshit-rak4631-s140-6-1-1.uf2',
                         'esp32/meshit-heltec-v4-factory.bin', 'esp32/meshit-heltec-v4-3.bin',
                         'esp32/meshit-t-deck.bin', 'esp32/meshit-t-deck-plus-factory.bin',
                         'esp32/meshit-t-deck-pro.bin', 'esp32/meshit-t-deck-pro-v1-1.bin',
                         'esp32/meshit-t-deck-pro-max-factory.bin', 'nrf52/meshit-heltec-t114.uf2', 'nrf52/nrf52-targets.json', 'webflasher/api.php', 'webflasher/index.html']:
                self.assertIn(name, names)
            self.assertFalse(any(name.endswith('.apk') for name in names))
            for line in bundle.read('SHA256SUMS').decode().splitlines():
                digest, name = line.split('  ', 1)
                self.assertEqual(hashlib.sha256(bundle.read(name)).hexdigest(), digest, name)


if __name__ == '__main__':
    unittest.main()
