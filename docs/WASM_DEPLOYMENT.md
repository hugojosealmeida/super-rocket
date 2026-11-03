# Static WebAssembly deployment

This is an optional third frontend. It does not replace the terminal executable
or the C server.

| Frontend | Runs where | Command | Persistence | Hosting |
| --- | --- | --- | --- | --- |
| Terminal | native C process | `make && ./super-rocket` | native HOME files | none |
| C server | C process | `make web && ./super-rocket-web 8080` | server directory | running process |
| WebAssembly | visitor's browser | `make wasm` | browser origin | static files only |

## Build and preview

The reproducible build uses Emscripten SDK **4.0.21**. A local `emcc` may be
used, or Docker can provide the pinned SDK:

```sh
make wasm EMCC="docker run --rm -v $PWD:/src -w /src emscripten/emsdk:4.0.21 emcc"
make package-wasm
python3 -m http.server 8000 --directory dist/wasm-package
```

Open `http://127.0.0.1:8000/`. Do not open the files with `file://`; WebAssembly
fetching and browser origin storage require HTTP(S). `make` and `make web` do not
require Emscripten and remain independent.

`make package-wasm` creates a directory containing only the runtime files plus
`manifest.json`, `SHA256SUMS` and `SIZE.txt`. The local package is currently
about **169 KB uncompressed**, below the 5 MB engineering ceiling. Rebuilds must
use the generated manifest and size report; do not assume a hosting account's
entire quota is available. Do not upload test artifacts, native binaries, SDK
files, `node_modules` or private save data.

## Static-host checks

Before upload, verify the exact directory at a nested path such as
`/games/super-rocket/`:

- All six runtime files return 200 and are not HTML error documents.
- `rocket.wasm` has `Content-Type: application/wasm`.
- Relative asset paths work below the site's root; URL and filename case match.
- Compressed responses have the correct `Content-Encoding`.
- JavaScript and WebAssembly are from the same package and cache invalidation
  does not create a mixed-version pair.
- HTTPS pages do not load HTTP assets. Missing/corrupt assets show the visible
  loader error rather than a blank page.
- Existing pages and terminal/C-server installations are unchanged.

The repository verifies these conditions against a local static server. Actual
external verification requires the real public URL and access to the selected
static hosting account; until then, mark that check unverified. No hosting
configuration, upload or credentials are part of the source build.

## Browser behavior and limitations

The WASM version runs an independent C `Game` per page. There is no `/key`,
`/frame`, `/play`, leaderboard API or game-state request. Keyboard events and
the 30 Hz fixed-step loop stay in the visitor's page. The visual design remains
retro: white outer page, black fixed 80x23 monospace panel and C renderer colors.

Scores, settings, statistics and leaderboard entries are stored in a versioned,
validated `localStorage` snapshot for that browser origin. Tabs run separate
rockets but share the save snapshot; last successful writer wins. If storage is
blocked/full/ephemeral, the game remains playable for the session and shows a
warning. Reset removes only this game's key. Local nicknames and scores are
editable records, not accounts or a trusted global ranking.

JavaScript and WebAssembly are required. Historical 1990s browsers cannot run
this frontend, even though the page keeps a retro appearance. Audio is optional,
user-gesture/browser-policy dependent and nonfatal when unavailable.

## Tests

```sh
make test                 # terminal/common C tests
make test-web             # C-server unit and socket tests
make test-sanitize        # native/server ASan and UBSan tests
make test-wasm-build      # SDK-independent WASM contracts and JS tests
make wasm-test EMCC="docker run --rm -v $PWD:/src -w /src emscripten/emsdk:4.0.21 emcc"
make package-wasm
python3 tests/test_wasm_artifacts.py dist/wasm-package
python3 tests/test_wasm_static.py
```

Optional Playwright browser tests run in CI for Chromium, Firefox and WebKit.
They test real compiled C/WASM, controls, focus, persistence, failure messages,
deterministic parity and static-only network behavior. Browser tests are tools,
not requirements for playing the packaged game.
