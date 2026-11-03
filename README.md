# super-rocket

The super rocket — a small terminal game in C about flying a rocket from Earth to Mars.

```
        S U P E R   R O C K E T
        -------------------------
         E A R T H  ->  M A R S
```

## Build and run

```sh
make
./super-rocket
```

Requires a POSIX terminal with ANSI colors (macOS or Linux) and a C99 compiler.
There are no external libraries beyond libc and libm.

The optional web frontend uses 1990s-style HTML and small keyboard/transport
scripts; the game remains C. Chromium, Firefox and WebKit are verified by CI.
Historical Netscape/IE releases are unverified, not promised as supported.
See [browser compatibility and the test checklist](docs/BROWSER_COMPATIBILITY.md).

## Play the browser-local WebAssembly version

There is also a third frontend for static hosting. The visitor's browser runs
the compiled C game locally, so every page has its own rocket and key presses do
not go to a server. The terminal and C-server versions remain available.

```sh
make wasm EMCC="docker run --rm -v $PWD:/src -w /src emscripten/emsdk:4.0.21 emcc"
make package-wasm
python3 -m http.server 8000 --directory dist/wasm-package
```

Open `http://127.0.0.1:8000/`, using HTTP(S) rather than `file://`. The package
is currently about 169 KB uncompressed and contains the generated WebAssembly,
loader, page, browser bridge, checksum and size files. Upload only the contents
of `dist/wasm-package/`.

The WebAssembly page keeps the retro appearance: a white page, black fixed
80x23 monospace game panel, C-rendered character colors and all themes. It
requires JavaScript and WebAssembly, so it is not for historical browsers.
Scores, settings, statistics and the local leaderboard are stored in that
browser origin. A nickname is only a local label; there are no accounts,
trusted global scores, shared spectators or cross-device saves. Storage errors
fall back to a session-only game with a visible warning.

See [the static deployment guide](docs/WASM_DEPLOYMENT.md) and [the WebAssembly
architecture](docs/WASM_ARCHITECTURE.md) for artifact checks, cache/MIME
requirements, browser limitations and test commands. External hosting
verification requires the actual public URL; the repository verifies a local
static server only.

The terminal must be at least 40 columns by 12 rows. Smaller windows are
refused with `terminal too small` on stderr before the game touches the
terminal, instead of drawing a frame that wraps into unreadable output. This
includes a terminal that reports a 0x0 size, which is what an unresized
container terminal (`docker exec` without a configured window) does; redirecting
the output to a pipe or file is unaffected and keeps the old 80x23 fallback.

Run the test suite with:

```sh
make test
```

Check that the reported version still matches the changelog with:

```sh
make check-version
```

## Play in an old-school browser page

The game still runs in **C**. A second C executable serves a 1990s-style page;
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

The [controls below](#controls) also apply in the browser, with these differences:

- `Esc`, or `Q` from the menu, returns to the menu rather than stopping the
  shared server. Restart the mission with `R`.
- Sound starts **off** every time the server starts. `M` enables optional ASCII
  BEL notifications; most browsers remain silent, which is supported.
- The white page contains a black, fixed 80×23 monospace game panel. Character
  positions and per-cell colors come directly from the same C renderer as the
  terminal. `T` changes the whole game palette (classic, amber, mono).
- Screen updates arrive at about 10 Hz while C simulation runs at 30 Hz.
  Use local or trusted-LAN play; Internet latency will affect controls.
- Enable frames and JavaScript. If the screen stays at "Connecting", check
  the server and browser settings. Widen a narrow window to see all 80 columns.
  Reload after a server restart (the control token changes).

**One server is one shared game.** All connected browsers control the same
rocket and share its scores/settings. Web saves are in `~/.super_rocket_web/`;
terminal `~/.super_rocket_*` files are untouched. Choose another save directory:

```sh
./super-rocket-web 8081 --data-dir ./web-saves
```

The directory is created if its parent exists. Use distinct ports **and** save
directories for independent servers; do not run multiple writers on one directory.
If `HOME` is unset, `--data-dir` is required.

The server binds to **127.0.0.1 only** by default. To play from another machine
on a trusted private LAN, explicitly bind the host's private IPv4 address:

```sh
./super-rocket-web 8080 --bind 192.168.1.10
```

Replace that example address with your host's address, and visit
`http://192.168.1.10:8080/` from the other machine. **Do not expose this server
to the Internet:** it has no accounts, TLS or private player sessions, and any
visitor can control/reset the shared game. Do not run it as root.

Chromium, Firefox and WebKit are verified in CI. Original Netscape/IE versions
are **unverified**, not supported promises. See the
[compatibility matrix and manual checklist](docs/BROWSER_COMPATIBILITY.md).

### Web tests

```sh
make test          # native/common unit tests
make test-web      # C frontend and real-socket happy-path/edge-case tests
make test-sanitize # native/web/socket tests under ASan and UBSan
```

Socket tests require Python 3; playing does not. Optional browser automation
instructions are in the compatibility document. CI runs Chromium, Firefox and
WebKit against the live C server and checks that shipped scripts parse as ES3.

## Command line options

```
--seed N     use a fixed random seed (reproducible runs)
--version    print the version and exit
--help       show usage
```

## Controls

| Key | Action |
| --- | --- |
| Arrow keys / `WASD` | Thrust in that direction |
| `Space` | Fire lasers |
| `P` | Pause / resume |
| `R` | Restart the mission |
| `T` | Cycle color theme |
| `M` | Mute / unmute sound |
| `F` | Toggle FPS counter |
| `H` | Help overlay |
| `I` | Leaderboard |
| `Q` | Back to the menu (quit from the menu) |
| `Esc` | Quit |
| `1` `2` `3` | Select difficulty in the menu |
| `X` | Reset statistics in the menu |

## How to play

- Pick a difficulty and press `Enter` to launch.
- Thrust burns fuel. Gravity from Earth and Mars pulls you around, so plan your burns.
- Dodge or shoot asteroids: every hit damages the hull. At zero hull the rocket breaks apart.
- Collect pickups: `F` fuel, `+` repair, `O` shield.
- Watch out for UFOs — they shoot back.
- The flight is split into three sectors. Clearing a sector grants an upgrade.
- Reach the dashed orbit around Mars to clear the level.
- The campaign has two levels: level 2 repeats the flight with more enemies.
  Clear it to complete the mission.
- Chain kills without taking damage to build a combo multiplier.
- Score is based on remaining hull, fuel, mission time and combat. Your best score
  for each difficulty is kept in `~/.super_rocket_scores`, and the top five pilots
  are stored in `~/.super_rocket_leaderboard`.

Difficulty changes fuel capacity, gravity strength, asteroid frequency and UFO pressure.

Score, mission time and the upgrades you chose carry over into level 2; hull,
fuel and shield are restored at the start of every level.

## Project layout

```
src/main.c     terminal CLI and game loop
src/app.c      shared input actions and frame stepping for both frontends
src/game.c     physics, entities, sectors, upgrades, scoring
src/game.h     shared game state and constants
src/render.c   frame buffer, ASCII art, themes and colors
src/term.c     raw terminal mode, input parsing, frame output, sound
src/rng.c      deterministic xorshift random numbers
src/util.c     shared math and formatting helpers
src/storage.c  HOME-relative persistence for scores, stats and config
src/theme.c    color palettes (classic, amber, mono)
src/web_main.c C web executable, CLI and fixed-timestep loop (make web)
src/web_server.c bounded nonblocking HTTP server, fixed routes and key endpoint
src/web_page.c legacy HTML and minimal keyboard/transport scripts
src/web_term.c browser platform adapter and changed-frame buffers
src/http.c     bounded request parser and query decoding
src/keys.c     held/released browser keys mapped to shared game input
tests/         unit tests (make test)
```

The game started as a single-file experiment in 2018 and grew into this
modular version over the following years.

## License

MIT
