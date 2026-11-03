# Browser frontends

The project has two browser options. The **old-school C-server page comes
first** because it preserves the 1990s browser-page experience. The
WebAssembly page is a separate modern static-hosting option.

## Play in an old-school browser page

The game still runs in **C**. The C executable serves a 1990s-style page;
small scripts only forward keys and copy screen updates. There is no JavaScript
port of the physics or game rules, no WebAssembly, canvas, CSS or plugin.
HTML 1.0 alone cannot capture keys; the live UI uses HTML 3.2-era markup with
period frame and keyboard-event extensions, not strict HTML 3.2-only markup.

Build with the same C99/POSIX toolchain as the terminal game:

```sh
make web
./super-rocket-web 8080
```

Open **http://127.0.0.1:8080/**, click the game screen, and press **Enter**.
Keep the server running; **Ctrl-C** stops it. Use a different port if 8080 is
busy. `web/index.html` is a launch guide, not a standalone game; opening an
HTML file or using a generic static file server cannot replace the C server.
No Node, Emscripten or frontend build tools are needed to play.

The terminal controls also apply in the browser:

| Key | Action |
| --- | --- |
| Arrow keys / `WASD` | Thrust |
| `Space` | Fire lasers |
| `P` | Pause / resume |
| `R` | Restart |
| `T` | Cycle theme |
| `M` | Toggle sound preference |
| `F` | Toggle FPS |
| `H` | Help |
| `I` | Leaderboard |
| `Q` / `Esc` | Menu / quit |
| `1` `2` `3` | Difficulty or upgrade |
| `X` | Reset statistics |
| Letters / `Backspace` | Leaderboard initials |

Browser-specific behavior:

- `Esc`, or `Q` from the menu, returns to the menu rather than stopping the
  shared server. Restart the mission with `R`.
- Sound starts **off** every time the server starts. `M` enables optional ASCII
  BEL notifications; most browsers remain silent, which is supported.
- The page is white with a black, fixed 80×23 monospace game panel. Character
  positions and per-cell colors come from the same C renderer as the terminal.
- Screen updates target about 30 Hz, matching the C simulation and terminal
  refresh cadence. Network latency and browser scheduling still affect the
  C-server page, so use local or trusted-LAN play.
- Enable frames and JavaScript. If the screen stays at “Connecting”, check the
  server and browser settings. Widen a narrow window to see all 80 columns.

**One server is one shared game.** All connected browsers control the same
rocket and share its scores/settings. Web saves are in `~/.super_rocket_web/`;
terminal `~/.super_rocket_*` files are untouched. Choose another save directory:

```sh
./super-rocket-web 8081 --data-dir ./web-saves
```

The directory is created if its parent exists. Use distinct ports and save
directories for independent servers; do not run multiple writers on one
directory. If `HOME` is unset, `--data-dir` is required.

The server binds to **127.0.0.1 only** by default. To play from another machine
on a trusted private LAN, explicitly bind the host's private IPv4 address:

```sh
./super-rocket-web 8080 --bind 192.168.1.10
```

Replace the example address with the host's address. **Do not expose this
server to the Internet:** it has no accounts, TLS or private player sessions,
and any visitor can control/reset the shared game. Do not run it as root.

Chromium, Firefox and WebKit are verified in CI. Historical Netscape/IE
releases are **unverified**, not supported promises. See the
[compatibility matrix](BROWSER_COMPATIBILITY.md).

## Play the browser-local WebAssembly version

This is a third frontend for static hosting. The visitor's browser runs the
compiled C game locally, so every page has its own rocket and key presses do not
go to a server. The terminal and C-server versions remain available at
[GitHub](https://github.com/hugojosealmeida/super-rocket).

```sh
make wasm EMCC="docker run --rm -v $PWD:/src -w /src emscripten/emsdk:4.0.21 emcc"
make package-wasm
python3 -m http.server 8000 --directory dist/wasm-package
```

Open **http://127.0.0.1:8000/** using HTTP(S), not `file://`. The package is
currently about 169 KB uncompressed and contains the generated WebAssembly,
loader, page, browser bridge, checksum and size files. Upload only the contents
of `dist/wasm-package/`.

The page keeps the retro appearance: white page, black fixed 80×23 monospace
panel, C-rendered character colors and all themes. It requires JavaScript and
WebAssembly, so historical browsers are not supported.

Scores, settings, statistics and the local leaderboard are stored in that
browser origin. A nickname is only a local label; there are no accounts,
trusted global scores, shared spectators or cross-device saves. Storage errors
fall back to a session-only game with a visible warning. See the
[static deployment guide](WASM_DEPLOYMENT.md) for package, MIME, cache and
hosting checks.

## Browser tests

```sh
make test-web
make test-wasm-build
make test-sanitize
```

CI runs real WebAssembly browser tests on Chromium, Firefox and WebKit. It
checks controls, focus, persistence, failure messages, deterministic parity,
static-only network behavior and artifact integrity.
