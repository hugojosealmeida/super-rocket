# Architecture

super-rocket is a small real-time terminal game. The code is split into modules
so that the simulation, the rendering and the platform layer stay independent.

```
main.c / web_main.c ──> app.c ──> game.c / render.c
                                  │
                                  └─ term.h interface
                                     term.c (native) / web_term.c (browser)
```

## Modules

### main.c
Owns the terminal frame loop, CLI and RNG seeding. Reads input through
`term_read_input` and delegates actions/frame stepping to `app.c`.

### app.c / app.h
Shared input dispatch (`app_handle_input`) and simulation/render step
(`app_step`). Both frontends use the same game actions. Initials entry consumes
letters before global shortcuts so names can contain H, I, T, P and so on.

### game.c / game.h
All simulation state lives in the `Game` struct: player, camera, fuel, hull,
shield, asteroids, pickups, lasers, UFOs, enemy shots, particles, stars,
sectors, levels, upgrades, statistics and the leaderboard. The module is pure logic -
it never writes to the screen except for the optional terminal bell. Fixed
timestep of `DT = 0.033` seconds (~30 fps).

Key entry points:
- `game_init` / `game_reset` / `game_start`
- `game_update` - advances one frame
- `game_thrust`, `game_fire`, `game_choose_upgrade`
- persistence: `game_load/save_scores`, `_settings`, `_stats`, `_leaderboard`

### render.c / render.h
Builds a character + color frame buffer (`Canvas`) each frame and hands it to
`term_draw`. All drawing is ASCII: planets are disks, the rocket is a small
sprite, pickups and lasers are single glyphs. Colors come from the active
theme via `theme_color(role)`.

### term.c / term.h
Platform layer. Puts the terminal in raw mode and stdin in non-blocking mode,
reads and decodes keys (including escape sequences for arrows), queries the
window size, emits colored frames and provides `term_beep`.  `term_size()`
returns `TERM_SIZE_TOO_SMALL` (with the actual size) when the terminal is
below 40x12 — including a tty that reports 0x0, such as an unresized container
terminal — so `main()` can refuse to start before `term_init()` has any side
effects; only when there is no tty to query does it keep the 80x23 fallback
used by pipes and CI.  `term_shutdown()` puts the terminal back the way the
shell handed it over: the saved termios settings and the saved stdin file
status flags (`O_NONBLOCK` belongs to the open file description, so the shell
would otherwise keep it).

### rng.c / rng.h
Deterministic xorshift64* RNG. Seeded from the clock or from `--seed`, so runs
can be reproduced exactly.

### util.c / util.h
Small math and formatting helpers shared by game and render: clamp, distance,
approach, time formatting.

### storage.c / storage.h
Resolves file names relative to `$HOME` (or a process-local directory override
for web saves) and reads/writes simple integer lists.
Used for scores, settings and statistics. The leaderboard uses its own text
format.

### theme.c / theme.h
Three palettes (classic, amber, mono) indexed by a `ThemeRole` enum. The active
theme is applied at the start of every `render_frame`.

## Data persistence

| File | Contents |
| --- | --- |
| `~/.super_rocket_scores` | best score per difficulty (3 ints) |
| `~/.super_rocket_config` | theme, sound, fps (3 ints) |
| `~/.super_rocket_stats` | lifetime statistics (11 ints) |
| `~/.super_rocket_leaderboard` | top five `NAME score difficulty` lines |

Integer files are read defensively: a file with fewer values than expected is
accepted and the missing fields keep their defaults, so the format can grow
without wiping player data.

## Game flow

```
MENU ──enter──> PLAYING ──sector cleared──> UPGRADE ──choose──> PLAYING
                  │                                             │
                  ├──hull 0──> DEAD                             │
                  ├──Mars orbit, level 1──> PLAYING (level 2) <─┘
                  └──Mars orbit, level 2──> INITIALS? ──> WON
```

`PLAYING` runs the simulation. `PAUSED`, `UPGRADE`, `INITIALS`, `WON` and
`DEAD` freeze it and show an overlay. The help (H) and leaderboard (I) panels
can be opened over any state and pause the simulation while open.

## Web version

The separate static WebAssembly frontend is specified in
[its architecture decision](WASM_ARCHITECTURE.md). The C-server design below
remains available and is not replaced by that frontend.

The web version is the same game in a 1990s-style browser page. It targets
late-1990s frame-capable browsers (historical compatibility is not yet verified), and it is served by
the C program `super-rocket-web` (`make web`). The terminal build is unchanged.

```
browser (HTML 3.2 page)                     super-rocket-web (C)
-----------------------                     -----------------------------
GET /                  ----------------->   page (src/web_page.c)
GET /play              ----------------->   stable keyboard/colored PRE page
GET /frame?v=N         ----------------->   hidden-frame snapshot update,
                                            only changed screen content
JS onkeydown / onkeyup
GET /key?d=CODE        ----------------->   held-key state (src/keys.c)
GET /key?u=CODE        ----------------->   released key
GET /key?clear=1       ----------------->   window lost focus, release all
```

### Decisions

- **Language and runtime.** The game logic stays in C. The server is plain C99
  with POSIX sockets and no external libraries. Browsers only run HTML and a
  small ES3-syntax snippet, which forwards key presses. Browsers cannot
  read the keyboard without scripting, so this snippet is required.
- **Screen refresh.** A hidden frame polls every 100 ms and copies changed
  colored HTML into the visible page's fixed monospace panel. Physics still runs at 30 ticks per
  second. This replaces the initial server-push proposal: reloading the keyboard
  document would discard held-key state and focus. No fetch, XHR, canvas or
  WebAssembly is used. Frames are a period browser extension, not HTML 3.2
  proper. We do not claim strict HTML 3.2 validation for the frameset wrapper.
  The live page is embedded in the C binary, avoiding runtime asset paths and
  arbitrary file serving. `web/index.html` is a standalone HTML 3.2 launch guide,
  not a client-only game. The server works from any working directory.
  The page background is white, and a black table cell frames the game. PRE
  preserves the exact 80×23 character grid; a fixed Courier font and per-run
  FONT colors preserve terminal layout and the C renderer's color buffer.
  Only adjacent cells with the same color are grouped. The C server HTML-escapes
  characters, then JavaScript-string-escapes the markup for transport. The
  stable SPAN's innerHTML is updated without replacing the keyboard document.
  Buffer limits cover the worst case of a color change on every cell.
- **Keyboard.** Key presses are sent as image requests (`new Image().src`), so
  the page never navigates away from the game. The server answers with a tiny
  GIF. Image requests are retained and serialized so key-up cannot overtake
  key-down. A per-server random token prevents drive-by cross-site control.
  Arrows and WASD hold thrust and fire until the key is released.
  One-shot keys (pause, theme, help and so on) are queued once.
  Held keys are renewed every 500 ms; a two-second lease releases them after
  a lost connection. Window blur also releases held keys.
- **Timing and sound.** A monotonic server clock schedules 30 simulation ticks
  per second, independently of browser polling or monitor refresh. Short stalls
  catch up; a long suspend discards excess backlog. Sound starts off on every
  server launch. M toggles the game preference; sound events deliver ASCII BEL
  to the game panel, a best-effort legacy notification. Most browsers remain
  silent. No plugin or audio API is required and silence is a supported outcome.
- **One game per server.** The server runs one shared game. Every connected
  browser sees the same screen, and key presses from any browser control it.
  Scores, statistics and settings use the existing file formats under
  `~/.super_rocket_web/` (override with `--data-dir DIR`). Terminal files are
  untouched. All browsers visiting one server share that server's record;
  per-player saves and cookies are not part of this version. Run separate
  servers with separate ports/directories for independent games. Do not run
  two servers against the same save directory concurrently.
- **Network safety.** The server binds to `127.0.0.1` by default. Requests are
  limited to `HTTP_MAX_REQUEST` bytes, only `GET` is accepted, and only the
  paths the server serves are answered (see `src/http.c`).
