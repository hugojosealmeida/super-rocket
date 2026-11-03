# Retro frontend compatibility

The page uses **1990s web techniques**, not a modern JavaScript game engine:
HTML 3.2-era forms/tables/PRE/FONT, frame navigation, image requests and small scripts.
All simulation, scoring, physics and rendering logic runs in C on the server.

Frames, `onkeydown`/`onkeyup`, event capture, SPAN/ID and `innerHTML` updates are period browser extensions,
not part of the HTML 3.2 DTD. Pure HTML 1.0 cannot run this keyboard interface.
The standalone `web/index.html` launch guide uses HTML 3.2 markup. No CSS,
canvas, fetch, XHR, WebSockets, WebAssembly, audio API or plugin is required.

## Verified support

Results recorded 2026-11-03; tests use pinned Playwright 1.64.1 distributions.
Exact browser versions are printed in subsequent CI runs.

| Environment | Result |
| --- | --- |
| Playwright Chromium, Linux CI | Pass |
| Playwright Firefox, Linux CI | Pass |
| Playwright WebKit, Linux CI | Pass |
| Playwright Chromium, macOS arm64 | Pass |
| Playwright WebKit, macOS arm64 | Pass |
| Playwright Firefox, macOS arm64 | Not tested: browser exits at startup with a profile-folder error |
| Netscape Navigator 2 / Internet Explorer 3 | Not supported/verified; earlier minimum-version promises were incorrect |
| Netscape Navigator 4 / Internet Explorer 4–6 | Historical candidates only; **unverified**, not advertised as supported |

Evidence: [CI run for PR #37](https://github.com/hugojosealmeida/super-rocket/actions/runs/41712093592).
Playwright WebKit is not a claim that every Safari release was tested, nor is
Chromium coverage a guarantee about every Chrome or Edge version.

Browser scripts are additionally parsed by Acorn with `ecmaVersion: 3`. That
checks syntax, **not historical DOM/API support**. Historical support requires
the manual tests below on a real browser or an appropriately licensed VM.

## Automated tests

First run `make web test test-web test-sanitize`. C and socket tests need only
a C99/POSIX toolchain, make and Python 3 (Python is test-only).

Optional development-only browser tools:

```sh
npm install --prefix .browser-tools playwright@1.64.1 acorn@8.15.0
.browser-tools/node_modules/.bin/playwright install chromium firefox webkit
NODE_PATH="$PWD/.browser-tools/node_modules" node tests/test_web_browser.cjs ./super-rocket-web chromium
NODE_PATH="$PWD/.browser-tools/node_modules" node tests/test_web_browser.cjs ./super-rocket-web firefox
NODE_PATH="$PWD/.browser-tools/node_modules" node tests/test_web_browser.cjs ./super-rocket-web webkit
```

On Linux, `playwright install --with-deps` can install required browser system
libraries. These tools are not needed to build or play either C executable.

## Manual checklist, including historical browsers

Record browser/version, OS or VM, date, and pass/fail for **each** item before
adding a browser to the supported list. Keep an old browser inside an isolated
test network; do not use it to visit arbitrary Internet sites.

- Start `./super-rocket-web 8080`; load `http://127.0.0.1:8080/`.
  For a VM, bind explicitly to a private interface and use the host's address.
- See an 80×23 fixed character screen; click it and press Enter to launch.
- Select each difficulty, thrust with arrows and WASD, hold/release Space.
- Hold P: pause once, not repeatedly. Release/repress to resume.
- Test R, Q, Esc, T, M, F, H, I, X, digits, initials and Backspace.
  Esc/Q in the menu leave the shared server running. Audio may stay silent.
- Confirm arrows/Space do not scroll and typed letters do not edit the screen.
- Lose focus while thrusting; verify thrust stops. Close a held-key tab and
  verify the two-second lease releases keys.
- Resize narrowly: see a warning, not a changed/corrupt game grid.
- Reload and restart the server: saved scores/settings remain. Two browser
  windows share one game; terminal save files remain untouched.
- Disable JavaScript/frames: see explanatory text. Stop frame updates: see
  the stale-connection warning. Restart the server and reload to reconnect.
- Verify menu, flight, pause, help, leaderboard, upgrade, initials, win and
  failure screens. For hard-to-reach states, the C renderer tests cover them
  automatically, but visual validation still needs manual confirmation.

## Intentional limitations

- Screen snapshots target approximately 30 Hz, matching the C simulation and
  terminal refresh cadence. Network latency and browser scheduling remain
  visible in the C-server version; it is intended for local/trusted-LAN play.
- The game is a black 80×23 monospace panel on a white page. The C renderer's
  color buffer is converted to per-run FONT colors inside PRE, preserving spaces,
  line breaks and theme changes. ANSI codes map to a fixed RGB palette; an
  individually customized terminal profile or font can still look different.
- Sound uses optional ASCII BEL. Most browsers stay silent, even with M on.
- One server is one shared game/save record; there are no player accounts.
- A server restart generates a new control token. Reload existing pages after
  restarting the server.
