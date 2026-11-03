"""Optional pinned Emscripten build. Never used by make / make web."""
import argparse
from pathlib import Path
import re
import shlex
import shutil
import subprocess
import sys
import hashlib
import json

SDK = "4.0.21"
ROOT = Path(__file__).resolve().parents[1]
SOURCES = ["src/wasm_main.c", "src/app.c", "src/game.c", "src/render.c", "src/keys.c",
           "src/rng.c", "src/util.c", "src/theme.c", "src/storage.c"]
EXPORTS = ["_rocket_" + name for name in
           ("version", "init", "tick", "key", "blur", "visibility", "visible", "chars", "colors", "frame", "beep")]


def build(emcc, out, testing=False):
    compiler = shlex.split(emcc)
    if not compiler:
        raise ValueError("Empty EMCC command")
    try:
        version = subprocess.check_output(compiler + ["--version"], text=True)
    except (OSError, subprocess.CalledProcessError) as exc:
        raise ValueError("Emscripten is missing; install/activate emsdk " + SDK +
                         " or set EMCC. See docs/WASM_ARCHITECTURE.md") from exc
    if not re.search(r"\b" + re.escape(SDK) + r"\b", version):
        raise ValueError("Use the pinned Emscripten SDK " + SDK)
    out.mkdir(parents=True, exist_ok=True)
    output_js = out / "rocket.js"
    if output_js.is_relative_to(ROOT):
        output_js = output_js.relative_to(ROOT)
    exports = EXPORTS + (["_rocket_snapshot", "_rocket_test_state"] if testing else [])
    subprocess.run(compiler + SOURCES + (["-DROCKET_TEST"] if testing else []) + [
        "-std=c99", "-O2", "-Wall", "-Wextra", "-Werror", "--no-entry",
        "-sMODULARIZE=1", "-sEXPORT_NAME=createRocketModule", "-sENVIRONMENT=web",
        "-sALLOW_MEMORY_GROWTH=0", "-sINITIAL_MEMORY=16777216", "-sSTACK_SIZE=1048576",
        "-sFORCE_FILESYSTEM=1", "-sEXPORTED_RUNTIME_METHODS=FS,UTF8ToString,HEAPU8",
        "-sEXPORTED_FUNCTIONS=" + ",".join(exports), "-lm", "-o", str(output_js)
    ], cwd=ROOT, check=True)
    for name in ("index.html", "client.js", "platform.js", "storage.js"):
        shutil.copyfile(ROOT / "wasm" / name, out / name)
    total = sum(p.stat().st_size for p in out.iterdir() if p.is_file())
    manifest = {'schema': 1, 'total_bytes': total, 'files': {p.name: {
        'bytes': p.stat().st_size,
        'sha256': hashlib.sha256(p.read_bytes()).hexdigest()
    } for p in sorted(out.iterdir()) if p.is_file() and p.name != 'manifest.json'}}
    (out / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    print(f"Static WASM package: {total:,} bytes in {out}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--emcc", default="emcc")
    parser.add_argument("--out", type=Path, default=ROOT / "dist" / "wasm")
    parser.add_argument("--test", action="store_true", help="test-only state inspection exports")
    args = parser.parse_args()
    try:
        build(args.emcc, args.out.resolve(), args.test)
    except (ValueError, subprocess.CalledProcessError) as exc:
        print("WASM build failed: " + str(exc), file=sys.stderr)
        sys.exit(1)
