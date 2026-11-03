"""Prepare a minimal static WASM upload directory and size/checksum report."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import sys

REQUIRED = ('index.html', 'client.js', 'platform.js', 'storage.js', 'rocket.js', 'rocket.wasm')
MAX_BYTES = 5 * 1024 * 1024


def package(source, destination):
    source = Path(source)
    destination = Path(destination)
    if destination.exists():
        for path in destination.iterdir():
            if path.is_dir(): shutil.rmtree(path)
            else: path.unlink()
    destination.mkdir(parents=True, exist_ok=True)
    missing = [name for name in REQUIRED if not (source / name).is_file()]
    if missing:
        raise ValueError('missing generated files: ' + ', '.join(missing))
    files = {}
    for name in REQUIRED:
        target = destination / name
        shutil.copyfile(source / name, target)
        files[name] = {'bytes': target.stat().st_size,
                       'sha256': hashlib.sha256(target.read_bytes()).hexdigest()}
    total = sum(item['bytes'] for item in files.values())
    if total > MAX_BYTES:
        raise ValueError(f'package is {total} bytes, above {MAX_BYTES} byte ceiling')
    manifest = {'schema': 1, 'total_bytes': total, 'files': files}
    (destination / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    sums = ''.join(f"{item['sha256']}  {name}\n" for name, item in files.items())
    (destination / 'SHA256SUMS').write_text(sums)
    (destination / 'SIZE.txt').write_text(
        f"uncompressed_bytes={total}\nfile_count={len(files)}\nceiling_bytes={MAX_BYTES}\n")
    return manifest


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', nargs='?', default='dist/wasm')
    parser.add_argument('destination', nargs='?', default='dist/wasm-package')
    args = parser.parse_args()
    try:
        result = package(args.source, args.destination)
        print(f"Packaged {result['total_bytes']:,} bytes into {args.destination}")
    except (OSError, ValueError) as exc:
        print('WASM packaging failed: ' + str(exc), file=sys.stderr)
        sys.exit(1)
