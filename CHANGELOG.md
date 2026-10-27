# Changelog

All notable changes to super-rocket are documented here.

## Unreleased

### Added
- `make test-san` builds and runs the unit suite as `tests/run_tests_san` with
  AddressSanitizer and UndefinedBehaviorSanitizer
  (`-std=c99 -g -O1 -fsanitize=address,undefined -fno-sanitize-recover=all
  -fno-omit-frame-pointer`, see the `Makefile`), and a new `test-san` CI job
  runs `make test-san CC=gcc` on ubuntu-latest for every push and pull
  request, so off-by-one indexing in the `GAME_MAX_*` entity pools or the
  hand-rolled file parsing fails the pipeline instead of shipping silently.
  The plain `make test` stays as the fast local run.

### Changed
- `GAME_VERSION` is now `1.0.0`, matching the released `1.0.0 - 2022-03-31`
  section below; `--version` and the menu footer had kept printing `0.9.0`
  after that release was documented.  `make check-version` (and the CI
  `Version` step) asserts the printed version equals the newest
  `## <version>` heading in this changelog so the two cannot drift again,
  and the unit suite (`make test`) carries the same assertion.
- `super-rocket` now exits with a clear error when the terminal is too small
  to play in.  Below 40 columns or 12 rows the game prints
  `super-rocket: terminal too small (need at least 40x12, got WxH)` on stderr
  and stops before entering the alternate screen or raw mode; previously it
  silently drew an 80-column frame that wrapped into unreadable output.  When
  there is no tty to query (pipes, CI) the old 80x23 fallback is kept, so
  `--version`/`--help` and the pipeline are unaffected.  A tty that answers the
  size query with 0x0 - an unresized container terminal, e.g. `docker exec -t`
  - is treated as too small rather than as "no tty", so containerised runs may
  need `stty rows 24 cols 80` or a resized window.

### Fixed
- Saved theme, sound and FPS settings are restored on startup again
  (`game_load_settings` misread the return value of `storage_read_ints`, so the
  values in `~/.super_rocket_config` were discarded, and an empty or corrupt
  file read uninitialized memory).
- The combo streak no longer decays while the simulation is frozen.  Pausing,
  choosing an upgrade between sectors or opening the help/leaderboard overlay
  keeps the streak and its timer intact; it only ticks during flight, like
  `mission_time` does.  Toast timers still run in every state.
- Quitting no longer leaves the terminal in non-blocking mode.  `term_init()`
  set `O_NONBLOCK` on stdin, a status flag of the open file description shared
  with the shell, and `term_shutdown()` only restored the termios settings — so
  after the game, `cat` and every other blocking read in that tab failed with
  `EAGAIN` until the tab was reopened.  The original flags are now saved and
  put back on shutdown.

## 1.0.0 - 2022-03-31

### Added
- Campaign of three sectors with an upgrade choice between them.
- Two-level campaign: level 2 repeats the flight with more enemies.
- Lasers, asteroid hit points and destructible rocks.
- UFO enemies that fire aimed plasma bolts.
- Pickups for fuel, repair and shield.
- Shield that absorbs damage before the hull.
- Combo multiplier for chained kills.
- Player statistics and six achievements with toasts.
- Top-five leaderboard with three-letter initials.
- Three color themes (classic, amber, mono).
- Settings for theme, sound and FPS, persisted to `~/.super_rocket_config`.
- Help overlay (H) and leaderboard panel (I).
- Sound effects via the terminal bell, mutable with M.
- Command line options: `--seed`, `--version`, `--help`.
- Deterministic xorshift RNG.
- Unit test suite (`make test`) and GitHub Actions CI.

### Changed
- Split the monolithic `main.c` into game, render, term, rng, util, storage and
  theme modules.
- HUD shows hull, fuel, shield, weapon recharge, sector and progress.

## 0.9.0 - 2022-01-07

### Added
- Difficulty levels (easy, normal, hard) with persistent high scores.
- Colors, gauge HUD, particle effects and screen shake.
- Mars orbit marker, low-fuel and hull-critical warnings.
- Menu, pause, victory and failure screens.

## 0.1.0 - 2018-2018

### Added
- Initial terminal game: starfield, keyboard control, inertial physics.
- Earth-to-Mars world with scrolling camera and planetary gravity.
- Fuel, hull damage, asteroids and game over.
