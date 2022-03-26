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

Run the test suite with:

```sh
make test
```

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
- Reach the dashed orbit around Mars to complete the mission.
- Chain kills without taking damage to build a combo multiplier.
- Score is based on remaining hull, fuel, mission time and combat. Your best score
  for each difficulty is kept in `~/.super_rocket_scores`, and the top five pilots
  are stored in `~/.super_rocket_leaderboard`.

Difficulty changes fuel capacity, gravity strength, asteroid frequency and UFO pressure.

## Project layout

```
src/main.c     input dispatch and the game loop
src/game.c     physics, entities, sectors, upgrades, scoring
src/game.h     shared game state and constants
src/render.c   frame buffer, ASCII art, themes and colors
src/term.c     raw terminal mode, input parsing, frame output, sound
src/rng.c      deterministic xorshift random numbers
src/util.c     shared math and formatting helpers
src/storage.c  HOME-relative persistence for scores, stats and config
src/theme.c    color palettes (classic, amber, mono)
tests/         unit tests (make test)
```

The game started as a single-file experiment in 2018 and grew into this
modular version over the following years.

## License

MIT
