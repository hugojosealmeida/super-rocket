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
      return { played, paused, stillPaused, independent, invalid, resumed, reset: snapshot() };
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
