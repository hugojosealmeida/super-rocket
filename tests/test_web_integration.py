"""Real-socket tests; no third-party Python packages required."""
import os
from pathlib import Path
import re
import signal
import socket
import subprocess
import sys
import tempfile
import time
import unittest
from html.parser import HTMLParser

BINARY = str(Path(sys.argv.pop(1)).resolve())


class RetroMarkup(HTMLParser):
    def __init__(self):
        super().__init__()
        self.tags = set()
        self.attributes = set()

    def handle_starttag(self, tag, attrs):
        self.tags.add(tag)
        self.attributes.update(key for key, _ in attrs)


class WebIntegration(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.home = tempfile.TemporaryDirectory(prefix="rocket-web-tests-")
        cls.start_server()

    @classmethod
    def start_server(cls):
        with socket.socket() as probe:
            probe.bind(("127.0.0.1", 0))
            cls.port = probe.getsockname()[1]
        cls.process = subprocess.Popen(
            [BINARY, str(cls.port)],
            env=dict(os.environ, HOME=cls.home.name),
            cwd=cls.home.name,  # executable must not depend on source-tree assets
            stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        deadline = time.monotonic() + 5
        while time.monotonic() < deadline:
            if cls.process.poll() is not None:
                raise RuntimeError(cls.process.communicate())
            try:
                with socket.create_connection(("127.0.0.1", cls.port), 0.2):
                    break
            except OSError:
                time.sleep(0.02)
        else:
            cls.process.kill()
            raise RuntimeError("server startup timed out")
        cls.token = re.search(rb'var token = "([0-9a-f]{32})"', cls.get("/play")).group(1).decode()

    @classmethod
    def tearDownClass(cls):
        cls.stop_server()
        cls.home.cleanup()

    @classmethod
    def stop_server(cls):
        cls.process.send_signal(signal.SIGTERM)
        out, err = cls.process.communicate(timeout=5)
        if cls.process.returncode != 0 or b"ERROR:" in err or b"runtime error:" in err:
            raise AssertionError((cls.process.returncode, out, err))

    @classmethod
    def raw(cls, data):
        with socket.create_connection(("127.0.0.1", cls.port), 2) as sock:
            sock.settimeout(3)
            sock.sendall(data)
            response = bytearray()
            while True:
                try:
                    part = sock.recv(65536)
                except ConnectionResetError:
                    if response:
                        return bytes(response)
                    raise
                if not part:
                    return bytes(response)
                response.extend(part)

    @classmethod
    def get(cls, target):
        return cls.raw(("GET " + target + " HTTP/1.0\r\n\r\n").encode())

    def test_index_and_page(self):
        page = self.get("/")
        self.assertIn(b"200 OK", page)
        self.assertIn(b'/play', page)
        self.assertIn(b'/frame', page)
        page = self.get("/play")
        self.assertIn(b"SUPER ROCKET", page)
        self.assertIn(b"COLS=80 ROWS=23", page)
        header, body = page.split(b"\r\n\r\n", 1)
        self.assertEqual(int(re.search(rb"Content-Length: (\d+)", header).group(1)), len(body))

    def test_retro_page_markup_and_launch_guide(self):
        for target in ("/", "/play", "/frame"):
            body = self.get(target).split(b"\r\n\r\n", 1)[1].decode("ascii")
            parsed = RetroMarkup()
            parsed.feed(body)
            self.assertTrue(parsed.tags <= {
                "html", "head", "title", "body", "frameset", "frame", "noframes",
                "script", "center", "h1", "p", "form", "input", "textarea", "table",
                "caption", "tr", "th", "td"})
            self.assertNotIn("style", parsed.attributes)
            self.assertNotIn("class", parsed.attributes)
            for modern in ("fetch(", "XMLHttpRequest", "WebSocket", "WebAssembly", "<CANVAS", "=>"):
                self.assertNotIn(modern, body)
        guide = Path(__file__).resolve().parents[1] / "web" / "index.html"
        self.assertIn("HTML 3.2 Final", guide.read_text())
        self.assertIn("./super-rocket-web", guide.read_text())
        self.assertIn(b"200 OK", self.get("/index.html"))

    def test_bad_routes_and_methods(self):
        for target in ("/absent", "/../etc/passwd", "/%2e%2e/etc/passwd", "/a\\b"):
            with self.subTest(target=target):
                self.assertIn(b"404 Not Found", self.get(target))
        self.assertIn(b"405 Method Not Allowed", self.raw(b"POST / HTTP/1.0\r\n\r\n"))
        self.assertIn(b"400 Bad Request", self.raw(b"GET / HTTP/2.0\r\n\r\n"))
        self.assertIn(b"400 Bad Request", self.raw(b"GET /\x00evil HTTP/1.0\r\n\r\n"))
        self.assertIn(b"413 Request Entity Too Large", self.raw(b"G" * 8192))
        self.assertIn(b"200 OK", self.get("/"))  # survived every malformed request

    def test_fragmented_request(self):
        with socket.create_connection(("127.0.0.1", self.port), 2) as sock:
            sock.settimeout(3)
            sock.sendall(b"GET / HTTP/1.0\r\n")
            time.sleep(0.05)
            sock.sendall(b"Host: localhost\r\n\r\n")
            self.assertIn(b"200 OK", sock.recv(32768))

    def test_slow_client_does_not_block(self):
        with socket.create_connection(("127.0.0.1", self.port), 2) as slow:
            slow.settimeout(7)
            slow.sendall(b"GET / HTTP/1.0\r\nUnfinished: ")
            start = time.monotonic()
            self.assertIn(b"200 OK", self.get("/"))
            self.assertLess(time.monotonic() - start, 1)
            self.assertEqual(slow.recv(1024), b"")  # absolute read deadline

    def test_keys_and_frames(self):
        self.assertIn(b"403 Forbidden", self.get("/key?d=13"))
        self.assertIn(b"403 Forbidden", self.get("/key?t=wrong&d=13"))
        for action in ("clear=1", "d=13", "u=13", "d=87", "u=87"):
            reply = self.get("/key?t=" + self.token + "&" + action)
            self.assertIn(b"image/gif", reply)
            self.assertIn(b"GIF89a", reply)
        for action in ("d=-1", "d=256", "d=%00", "d=%xx", "clear=0", "d=13&u=13"):
            with self.subTest(action=action):
                self.assertIn(b"400 Bad Request", self.get("/key?t=" + self.token + "&" + action))
        time.sleep(0.1)
        frame = self.get("/frame")
        self.assertIn(b"screen.value", frame)
        self.assertIn(b"setTimeout", frame)
        self.assertNotIn(b"WebAssembly", frame)

    def test_connection_limit_and_recovery(self):
        peers = []
        try:
            for _ in range(16):
                peers.append(socket.create_connection(("127.0.0.1", self.port), 2))
                peers[-1].sendall(b"GET / HTTP/1.0\r\n")
            time.sleep(0.1)
            self.assertIn(b"503 Service Unavailable", self.get("/"))
        finally:
            for peer in peers:
                peer.close()
        time.sleep(0.1)
        self.assertIn(b"200 OK", self.get("/"))

    def test_cli_errors(self):
        for args in (["0"], ["65536"], ["-1"], ["abc"], ["--bind"], ["--bind", "invalid"],
                     ["8080", "8081"], [str(self.port)]):
            with self.subTest(args=args):
                result = subprocess.run([BINARY] + args, capture_output=True, timeout=3,
                                        env=dict(os.environ, HOME=self.home.name))
                self.assertNotEqual(result.returncode, 0)
        result = subprocess.run([BINARY, "--version"], capture_output=True, timeout=3)
        self.assertEqual(result.returncode, 0)
        self.assertIn(b"1.0.0", result.stdout)

    def test_documented_cli_and_readme_links(self):
        root = Path(__file__).resolve().parents[1]
        readme = (root / "README.md").read_text()
        help_text = subprocess.check_output([BINARY, "--help"]).decode()
        for option in ("--bind", "--data-dir"):
            self.assertIn(option, readme)
            self.assertIn(option, help_text)
        for target in re.findall(r"\]\(([^)]+)\)", readme):
            if "://" not in target and not target.startswith("#"):
                self.assertTrue((root / target.split("#", 1)[0]).exists(), target)
        self.assertIn("make web", readme)
        self.assertIn("./super-rocket-web 8080", readme)
        self.assertIn("make test-sanitize", readme)

    def test_persistent_shared_saves_and_terminal_isolation(self):
        cls = type(self)
        cls.stop_server()
        native = Path(self.home.name) / ".super_rocket_scores"
        native.write_text("999\n888\n777\n")
        data = Path(self.home.name) / ".super_rocket_web"
        config = data / ".super_rocket_config"
        config.write_text("1\n1\n1\n")
        scores = data / ".super_rocket_scores"
        scores.write_text("123\n456\n789\n")
        leaderboard = data / ".super_rocket_leaderboard"
        leaderboard.write_text("ALX 12345 1\n")
        cls.start_server()
        # Both a fresh HTTP client and a reload see the same saved game/theme.
        self.assertIn(b"#FFFF55", self.get("/frame"))
        self.assertIn(b"#FFFF55", self.get("/frame"))
        self.assertIn(b"sound:off", self.get("/frame"))
        self.get("/key?t=" + cls.token + "&d=84")  # T -> mono
        self.get("/key?t=" + cls.token + "&u=84")
        deadline = time.monotonic() + 2
        while config.read_text().splitlines()[0] != "2" and time.monotonic() < deadline:
            time.sleep(0.02)
        self.assertEqual(config.read_text().splitlines()[0], "2")
        cls.stop_server()
        cls.start_server()
        self.assertIn(b"#FFFFFF", self.get("/frame"))
        self.assertEqual(scores.read_text(), "123\n456\n789\n")
        self.assertEqual(leaderboard.read_text(), "ALX 12345 1\n")
        self.assertEqual(native.read_text(), "999\n888\n777\n")

    def test_save_directory_errors(self):
        for directory in ("", "/nonexistent/parent/rocket-saves", "x" * 1000):
            result = subprocess.run([BINARY, "--data-dir", directory], capture_output=True, timeout=3)
            self.assertNotEqual(result.returncode, 0)
        env = dict(os.environ)
        env.pop("HOME", None)
        result = subprocess.run([BINARY], env=env, capture_output=True, timeout=3)
        self.assertNotEqual(result.returncode, 0)


if __name__ == "__main__":
    unittest.main()
