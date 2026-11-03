import hashlib
import json
from pathlib import Path
import sys

REQUIRED = {'index.html', 'client.js', 'platform.js', 'storage.js', 'rocket.js', 'rocket.wasm'}
MAX_PACKAGE = 5 * 1024 * 1024
root = Path(sys.argv[1] if len(sys.argv) > 1 else 'dist/wasm')
files = {path.name: path for path in root.iterdir() if path.is_file() and path.name != 'manifest.json'}
missing = REQUIRED - files.keys()
extra = set(files) - REQUIRED
if missing or extra:
    raise SystemExit(f'artifact inventory mismatch; missing={sorted(missing)} extra={sorted(extra)}')
total = sum(path.stat().st_size for path in files.values())
if total > MAX_PACKAGE:
    raise SystemExit(f'package is {total} bytes, above {MAX_PACKAGE} byte budget')
if files['rocket.wasm'].read_bytes()[:4] != b'\0asm':
    raise SystemExit('rocket.wasm is not a WebAssembly binary')
manifest = {'schema': 1, 'total_bytes': total, 'files': {
    name: {'bytes': path.stat().st_size, 'sha256': hashlib.sha256(path.read_bytes()).hexdigest()}
    for name, path in sorted(files.items())}}
manifest_path = root / 'manifest.json'
if manifest_path.exists() and json.loads(manifest_path.read_text()) != manifest:
    raise SystemExit('manifest does not match generated files')
if not manifest_path.exists():
    manifest_path.write_text(json.dumps(manifest, indent=2) + '\n')
print(f'WASM artifact inventory PASS: {total:,} bytes, {len(files)} files')
