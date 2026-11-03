import http.server
from pathlib import Path
import socketserver
import subprocess
import tempfile
import threading
import urllib.error
import urllib.request
import unittest


class StaticHost(unittest.TestCase):
    def test_nested_path_mime_and_no_arbitrary_files(self):
        root = Path(__file__).resolve().parents[1] / 'dist' / 'wasm-package'
        if not root.exists(): self.skipTest('run make package-wasm first')
        class Handler(http.server.SimpleHTTPRequestHandler):
            def __init__(self, *args, **kwargs): super().__init__(*args, directory=str(root), **kwargs)
            def log_message(self, *_): pass
        server = socketserver.TCPServer(('127.0.0.1', 0), Handler)
        thread = threading.Thread(target=server.serve_forever, daemon=True); thread.start()
        base = f'http://127.0.0.1:{server.server_address[1]}'
        try:
            with urllib.request.urlopen(base + '/rocket.wasm') as response:
                self.assertEqual(response.headers.get_content_type(), 'application/wasm')
                self.assertEqual(response.read(4), b'\0asm')
            with urllib.request.urlopen(base + '/index.html') as response:
                self.assertIn(b'WebAssembly', response.read())
            with self.assertRaises(urllib.error.HTTPError) as missing:
                urllib.request.urlopen(base + '/../../etc/passwd')
            self.assertEqual(missing.exception.code, 404)
        finally:
            server.shutdown(); server.server_close(); thread.join()


if __name__ == '__main__': unittest.main()
