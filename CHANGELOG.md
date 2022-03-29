# Changelog

All notable changes to super-rocket are documented here.

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
