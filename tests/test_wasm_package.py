import hashlib
import importlib.util
from pathlib import Path
import tempfile
import unittest

path = Path(__file__).resolve().parents[1] / 'tools' / 'package_wasm.py'
spec = importlib.util.spec_from_file_location('package_wasm', path)
package_wasm = importlib.util.module_from_spec(spec)
spec.loader.exec_module(package_wasm)


class PackageWasm(unittest.TestCase):
    def files(self, root, payload=b'asset'):
        for name in package_wasm.REQUIRED:
            (root / name).write_bytes(b'\0asm' if name == 'rocket.wasm' else payload)

    def test_package_inventory_size_and_hashes(self):
        with tempfile.TemporaryDirectory() as tmp:
            source, destination = Path(tmp) / 'source', Path(tmp) / 'package'
            source.mkdir(); self.files(source)
            result = package_wasm.package(source, destination)
            self.assertEqual(set(result['files']), set(package_wasm.REQUIRED))
            self.assertTrue((destination / 'SHA256SUMS').is_file())
            self.assertIn('uncompressed_bytes=', (destination / 'SIZE.txt').read_text())
            for name in package_wasm.REQUIRED:
                self.assertEqual(result['files'][name]['sha256'],
                                 hashlib.sha256((destination / name).read_bytes()).hexdigest())
            package_wasm.package(source, destination)  # repeat is clean/idempotent
            self.assertFalse((destination / 'unexpected').exists())

    def test_missing_and_oversized(self):
        with tempfile.TemporaryDirectory() as tmp:
            source, destination = Path(tmp) / 'source', Path(tmp) / 'package'
            source.mkdir(); self.files(source)
            (source / 'rocket.wasm').unlink()
            with self.assertRaisesRegex(ValueError, 'missing'):
                package_wasm.package(source, destination)
            self.files(source, b'x' * (package_wasm.MAX_BYTES + 1))
            with self.assertRaisesRegex(ValueError, 'above'):
                package_wasm.package(source, destination)


if __name__ == '__main__':
    unittest.main()
