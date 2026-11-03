from pathlib import Path
import unittest


class WasmDocs(unittest.TestCase):
    def test_documented_variants_and_deployment(self):
        root = Path(__file__).resolve().parents[1]
        readme = (root / 'README.md').read_text()
        guide = (root / 'docs' / 'WEB_FRONTENDS.md').read_text()
        text = readme + guide + (root / 'docs' / 'WASM_DEPLOYMENT.md').read_text()
        for expected in ('make wasm', 'make package-wasm', 'dist/wasm-package',
                         'application/wasm', 'localStorage', 'make web', 'file://'):
            self.assertIn(expected, text)
        self.assertIn('[browser frontends guide](docs/WEB_FRONTENDS.md)', readme)
        self.assertLess(guide.index('## Play in an old-school browser page'),
                        guide.index('## Play the browser-local WebAssembly version'))
        self.assertTrue((root / 'docs' / 'WASM_ARCHITECTURE.md').is_file())
        self.assertTrue((root / 'docs' / 'WASM_DEPLOYMENT.md').is_file())


if __name__ == '__main__':
    unittest.main()
