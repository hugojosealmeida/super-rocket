/* Runs a real compiled C module using only a static file server. */
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const http = require('node:http');
const engines = require('playwright');
const os = require('node:os');
const { execFileSync } = require('node:child_process');

(async function () {
  const root = path.resolve(process.argv[2] || 'dist/wasm');
  const engine = process.argv[3] || 'chromium';
  const prefix = '/games/rocket/';
  const types = { '.html': 'text/html', '.js': 'text/javascript', '.wasm': 'application/wasm' };
  const allowed = new Set(['index.html', 'rocket.js', 'rocket.wasm', 'client.js', 'platform.js']);
  const server = http.createServer((req, res) => {
    const url = new URL(req.url, 'http://localhost');
    const file = url.pathname.startsWith(prefix) ? url.pathname.slice(prefix.length) || 'index.html' : '';
    if (!allowed.has(file)) { res.writeHead(404); res.end('not found'); return; }
    res.writeHead(200, { 'Content-Type': types[path.extname(file)], 'Cache-Control': 'no-store' });
    res.end(fs.readFileSync(path.join(root, file)));
  });
  await new Promise(resolve => server.listen(0, '127.0.0.1', resolve));
  const base = 'http://127.0.0.1:' + server.address().port + prefix;
  let browser;
  try {
    browser = await engines[engine].launch();
    const page = await browser.newPage();
    const errors = [], requests = [];
    page.on('pageerror', e => errors.push(e.message));
    page.on('request', req => requests.push(req.url()));
    await page.goto(base);
    await page.waitForFunction(() => document.getElementById('status').textContent.includes('C module ready'));
    assert.match(await page.locator('#status').textContent(), /1\.1\.0/);
    const screen = page.locator('#screen');
    const grid = (await screen.textContent()).split('\n');
    assert.equal(grid.length, 23);
    assert.ok(grid.every(line => line.length === 80));
    assert.equal(await screen.evaluate(el => getComputedStyle(el.closest('table')).backgroundColor), 'rgb(0, 0, 0)');
    assert.equal(await page.evaluate(() => getComputedStyle(document.body).backgroundColor), 'rgb(255, 255, 255)');
    assert.ok((await screen.locator('font').count()) > 1);
    assert.match(await screen.evaluate(el => getComputedStyle(el).fontFamily), /Courier/);
    const waitText = text => page.waitForFunction(text => document.getElementById('screen').textContent.includes(text), text);
    const waitAbsent = text => page.waitForFunction(text => !document.getElementById('screen').textContent.includes(text), text);
    await screen.click();
    for (const [key, text] of [['3','> [3]'],['1','> [1]'],['h','H O W   T O   F L Y']]) {
      await page.keyboard.press(key); await waitText(text);
    }
    await page.keyboard.press('h'); await waitAbsent('H O W   T O   F L Y');
    await page.keyboard.press('i'); await waitText('T O P   P I L O T S');
    await page.keyboard.press('i'); await waitAbsent('T O P   P I L O T S');
    await page.keyboard.press('t'); await waitText('theme:amber');
    await page.keyboard.press('f'); await waitText('fps:on');
    await page.keyboard.press('x'); await waitText('STATISTICS RESET');
    await page.keyboard.press('Enter'); await waitText('LEVEL 1');
    await page.keyboard.down('w'); await page.waitForTimeout(150); await page.keyboard.up('w');
    await page.keyboard.press('Space');
    await page.keyboard.down('p'); await waitText('PAUSED');
    await page.keyboard.down('p'); await page.waitForTimeout(100); await waitText('PAUSED');
    await page.keyboard.up('p'); await page.keyboard.press('p'); await waitAbsent('PAUSED');
    await page.evaluate(() => { const input = document.createElement('input'); input.id = 'notes'; document.body.appendChild(input); input.focus(); });
    await waitText('PAUSED');
    await page.keyboard.type('pwasd');
    assert.equal(await page.locator('#notes').inputValue(), 'pwasd', 'unrelated input keeps its keyboard');
    await screen.click(); await page.keyboard.press('p'); await waitAbsent('PAUSED');
    await page.keyboard.press('r'); await waitText('LEVEL 1');
    await page.keyboard.press('q'); await waitText('lift off');
    await page.keyboard.press('Escape'); await waitText('lift off');
    const state = await page.evaluate(async () => {
      const module = await createRocketModule();
      if (!module._rocket_snapshot) return null; // Inspection exists only in test builds.
      module.FS.mkdir('/saves'); module._rocket_init(123);
      const snapshot = () => JSON.parse(module.UTF8ToString(module._rocket_snapshot()));
      module._rocket_key(13, 1); module._rocket_tick(); module._rocket_key(13, 0);
      for (let i = 0; i < 120; i++) {
        if (i === 0) module._rocket_key(87, 1);
        if (i === 30) module._rocket_key(87, 0);
        if (i === 40) module._rocket_key(32, 1);
        if (i === 60) module._rocket_key(32, 0);
        module._rocket_tick();
      }
      const played = snapshot();
      module._rocket_blur(); module._rocket_tick();
      const paused = snapshot();
      module._rocket_tick();
      const stillPaused = snapshot();
      const invalid = module._rocket_key(-1, 1);
      const second = await createRocketModule();
      second.FS.mkdir('/saves'); second._rocket_init(123);
      const independent = JSON.parse(second.UTF8ToString(second._rocket_snapshot()));
      module._rocket_key(80, 1); module._rocket_tick();
      const resumed = snapshot();
      module._rocket_init(123);
      const screens = [];
      const testScreen = document.createElement('pre');
      document.body.appendChild(testScreen);
      for (let state = 0; state <= 6; state++) {
        module._rocket_test_state(state);
        RocketPlatform.draw(module, testScreen, -1);
        screens.push(testScreen.textContent);
      }
      module._rocket_init(123);
      RocketPlatform.draw(module, testScreen, -1);
      const classic = [...testScreen.querySelectorAll('font')].map(font => font.color);
      module._rocket_key(84, 1); module._rocket_tick(); module._rocket_key(84, 0);
      RocketPlatform.draw(module, testScreen, -1);
      const amber = [...testScreen.querySelectorAll('font')].map(font => font.color);
      testScreen.remove();
      return { played, paused, stillPaused, independent, invalid, resumed, reset: snapshot(), screens, classic, amber };
    });
    if (state) {
      const temp = fs.mkdtempSync(path.join(os.tmpdir(), 'rocket-parity-'));
      try {
        const native = JSON.parse(execFileSync(path.resolve('tests/wasm_parity'), [temp], { encoding: 'utf8' }));
        for (const key of Object.keys(native))
          assert.ok(Math.abs(native[key] - state.played[key]) <= 1e-8 * Math.max(1, Math.abs(native[key])), key + ' native/WASM parity');
      } finally { fs.rmSync(temp, { recursive: true, force: true }); }
      assert.equal(state.paused.state, 2);
      assert.equal(state.stillPaused.time, state.paused.time);
      assert.equal(state.resumed.state, 1);
      assert.equal(state.independent.state, 0);
      assert.equal(state.reset.time, 0);
      assert.equal(state.invalid, 0);
      assert.notDeepEqual(state.amber, state.classic);
      for (const [i, expected] of ['lift off', 'EARTH', 'PAUSED', 'MARS ORBIT ACHIEVED', 'MISSION FAILED', 'SECTOR CLEARED', 'INITIALS'].entries())
        assert.ok(state.screens[i].includes(expected), 'C game screen ' + i);
    }
    const wasm = await page.request.get(base + 'rocket.wasm');
    assert.equal(wasm.headers()['content-type'], 'application/wasm');
    assert.deepEqual([...((await wasm.body()).subarray(0, 4))], [0, 97, 115, 109]);
    assert.ok(requests.every(url => url.startsWith(base)), 'only same-directory static requests');
    assert.deepEqual(errors, []);
    console.log(engine + ': actual WASM bootstrap and nested static hosting PASS');
  } finally {
    if (browser) await browser.close();
    server.closeAllConnections();
    await new Promise(resolve => server.close(resolve));
  }
}()).catch(error => { console.error(error); process.exitCode = 1; });
