# ADR: independent static WebAssembly frontend

Status: accepted for implementation. Baseline: v1.1.0, which remains unchanged.

## Three frontends

| Target | Execution | Deployment | Saves |
| --- | --- | --- | --- |
| `make` | native C terminal | local executable | native HOME files |
| `make web` | C HTTP server | running local/trusted-LAN process | server save directory |
| `make wasm` | C compiled to WebAssembly in each page | static HTTP(S) files | browser origin storage |

The new target must not link main.c, term.c, web_main.c or web_server.c. It shares
app/game/render/rng/util/theme and the stdio storage implementation. Separate
wasm entrypoint and term adapter expose a narrow C function API: initialize,
fixed-step tick, key down/up/clear, visibility pause, frame buffers and version.
JavaScript owns scheduling, DOM rendering, persistence transport and optional
audio, **not physics, enemies, scoring, upgrades or game rules**.

## Toolchain and deployment

Pin Emscripten **4.0.21** (native SDK or `emscripten/emsdk:4.0.21`). `make wasm`
emits self-contained `dist/wasm/` assets with relative URLs. No runtime CDN,
Node, custom server, threads, SharedArrayBuffer, COOP/COEP, fetch-to-game API,
or authentication service is required. JavaScript/WASM and modern DOM APIs are
required; ES3 checks apply only to the existing legacy C-server page.

Use a modularized loader, single-threaded fixed memory, explicit C exports,
and a browser main loop rather than blocking sleeps or the native main loop.
Audit clock/getpid seeding; retain explicit deterministic seeds in test builds.
Tests use test-only inspection exports, never a production God-mode API.

Start with a 5 MB raw static-package ceiling, well below the stated 100 MB
hosting allowance. Measure actual raw/compressed sizes and check remaining site
quota before deployment. Correct `.wasm` MIME is `application/wasm`. Test nested
paths, missing/corrupt assets and cache coherence. Actual host validation needs
the public deployment URL/access; a local test is not external evidence.

`make package-wasm` creates `dist/wasm-package/` containing only the six runtime
files plus `manifest.json`, `SHA256SUMS` and `SIZE.txt`. The generated package
is currently about 170 KB uncompressed (the manifest records the exact value),
well below the 5 MB engineering ceiling and the stated hosting allowance. This
is a measured local artifact, not a claim about remaining account quota.

## Runtime and rendering

Wait for the module and storage hydration before game_init. Use a monotonic
requestAnimationFrame accumulator at 30 C steps/second (the game retains its
existing DT=0.033); cap catch-up and drop long-suspend backlog. Clear keys and
pause active flight on blur/hidden tab; returning does not undo manual pause or
auto-resume a mission. Esc/Q from the menu keep the runtime alive.

Each module/page has its own Game and RNG. Reuse C key semantics. Export only
fixed character/color buffers, dimensions, a frame version and small lifecycle
operations, not JavaScript copies of the Game struct. Obtain fresh typed-array
views when reading memory. Render an 80×23 monospace black panel on a white page,
using the same ANSI-to-RGB palette, text nodes/escaped strings and color runs.
Never replace the focused keyboard document or poll `/key`, `/frame`, `/play`.

## Persistence decision: MEMFS + versioned localStorage snapshot

Keep C stdio for **all four files**, including the leaderboard, which directly
uses fopen in game.c. Before game_init, create `/saves` in Emscripten MEMFS and
hydrate a bounded, validated JSON snapshot from an origin-scoped localStorage
key. Use storage_set_directory to isolate these files. After successful C file
writes, a generic storage notification hook snapshots the allowlisted files.
The default hook is a no-op for existing frontends. Audit the direct leaderboard
write as well as storage_write_ints; relying on integer helpers alone loses it.

Why not IDBFS: this game's four tiny files fit a bounded synchronous snapshot;
localStorage avoids asynchronous hydration/flush races and unload-only writes.
IDBFS is preferable if saves grow substantially, but adds complexity here.
Use a schema version, size limits, numeric/name/difficulty validation and a
game-specific key. No terminal/server paths or unrelated origin keys are touched.
Blocked/full/ephemeral storage leaves a playable in-memory session with a visible
warning. Future schemas are not overwritten. Reset removes only game-owned data.

Each tab runs an independent rocket, but tabs of the same origin/profile share
the snapshot: **last successful writer wins**, no implicit live merging. Separate
profiles/origins are isolated. Saving is synchronous after writes, not deferred
until unload. Nicknames are local labels, not accounts; local scores are editable
and untrusted. No global rankings, spectators or cross-device synchronization.

## Tests and compatibility

Preserve native/server unit, socket, sanitizer and browser suites. Add pure
scheduler/storage/renderer tests, real WASM browser tests on Chromium, Firefox,
WebKit, independent modules/profiles, reload/reset and failure-injection cases.
Compare seeded native/WASM selected-state snapshots with float tolerances.
Do not claim historical-browser support. Optional audio is user-gesture gated;
silence/missing APIs must never prevent play.
