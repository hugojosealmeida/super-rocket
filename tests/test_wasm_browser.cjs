/* Runs a real compiled C module using only a static file server. */
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const http = require('node:http');
const engines = require('playwright');

(async function () {
  const root = path.resolve(process.argv[2] || 'dist/wasm');
  const engine = process.argv[3] || 'chromium';
  const prefix = '/games/rocket/';
  const types = { '.html': 'text/html', '.js': 'text/javascript', '.wasm': 'application/wasm' };
  const allowed = new Set(['index.html', 'rocket.js', 'rocket.wasm', 'client.js']);
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
