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

## Controls

| Key | Action |
| --- | --- |
| Arrow keys / `WASD` | Thrust in that direction |
| `P` | Pause / resume |
| `R` | Restart the mission |
| `Q` | Back to the menu (quit from the menu) |
| `Esc` | Quit |
| `1` `2` `3` | Select difficulty in the menu |

## How to play

- Pick a difficulty and press `Enter` to launch.
- Thrust burns fuel. Gravity from Earth and Mars pulls you around, so plan your burns.
- Dodge asteroids: every hit damages the hull. At zero hull the rocket breaks apart.
- Reach the dashed orbit around Mars to complete the mission.
- Score is based on remaining hull, remaining fuel and mission time. Your best score
  for each difficulty is kept in `~/.super_rocket_scores`.

Difficulty changes fuel capacity, gravity strength and asteroid frequency.

## Project layout

```
src/main.c    input dispatch and the game loop
src/game.c    physics, gravity, fuel, asteroids, scoring
src/game.h    shared game state and constants
src/render.c  frame buffer, ASCII art and colors
src/term.c    raw terminal mode, input parsing, frame output
```

The game started as a single-file experiment in 2018 and grew into this
three-module version over the following years.

## License

MIT
