# Architecture

super-rocket is a small real-time terminal game. The code is split into modules
so that the simulation, the rendering and the platform layer stay independent.

```
main.c  ──>  game.c  ──>  rng.c, util.c, storage.c, term.c (beep)
   │
   └──────>  render.c  ──>  theme.c, util.c, term.c (draw)
```

## Modules

### main.c
Owns the frame loop. Reads input through `term_read_input`, dispatches it with
`handle_input`, advances the simulation with `game_update` and draws with
`render_frame`. Also parses command line options and seeds the RNG.

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
below 40x12, so `main()` can refuse to start before `term_init()` has any
side effects; when there is no tty to query it keeps the 80x23 fallback used
by pipes and CI.  `term_shutdown()` puts the terminal back the way the shell
handed it over: the saved termios settings and the saved stdin file status
flags (`O_NONBLOCK` belongs to the open file description, so the shell would
otherwise keep it).

### rng.c / rng.h
Deterministic xorshift64* RNG. Seeded from the clock or from `--seed`, so runs
can be reproduced exactly.

### util.c / util.h
Small math and formatting helpers shared by game and render: clamp, distance,
approach, time formatting.

### storage.c / storage.h
Resolves file names relative to `$HOME` and reads/writes simple integer lists.
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
