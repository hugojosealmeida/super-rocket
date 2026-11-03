import importlib.util
from pathlib import Path
import tempfile
import unittest

spec = importlib.util.spec_from_file_location("build_wasm", Path(__file__).resolve().parents[1] / "tools/build_wasm.py")
builder = importlib.util.module_from_spec(spec)
spec.loader.exec_module(builder)


class WasmBuild(unittest.TestCase):
    def test_missing_sdk(self):
        with tempfile.TemporaryDirectory() as directory:
            with self.assertRaisesRegex(ValueError, "missing"):
                builder.build("/nonexistent/rocket-emcc", Path(directory))

    def test_empty_command(self):
        with self.assertRaisesRegex(ValueError, "Empty"):
            builder.build("", Path("unused"))

    def test_wrong_version(self):
        with tempfile.TemporaryDirectory() as directory:
            fake = Path(directory) / "emcc"
            fake.write_text("#!/bin/sh\necho 'emcc 0.0.1'\n")
            fake.chmod(0o700)
            with self.assertRaisesRegex(ValueError, "pinned"):
                builder.build(str(fake), Path(directory))

    def test_source_isolation(self):
        self.assertNotIn("src/main.c", builder.SOURCES)
        self.assertNotIn("src/term.c", builder.SOURCES)
        self.assertNotIn("src/web_server.c", builder.SOURCES)
        self.assertNotIn("src/web_main.c", builder.SOURCES)


if __name__ == "__main__":
    unittest.main()
