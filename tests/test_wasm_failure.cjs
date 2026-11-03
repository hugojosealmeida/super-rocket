const assert = require('node:assert/strict');
const fs = require('node:fs');
const http = require('node:http');
const path = require('node:path');
const engines = require('playwright');
(async () => {
  const root = path.resolve(process.argv[2] || 'dist/wasm-test');
  const server = http.createServer((req,res) => {
    const file = path.basename(new URL(req.url, 'http://x').pathname);
    if (file === 'rocket.wasm') { res.writeHead(404); return res.end('<html>not wasm</html>'); }
    if (!['index.html','rocket.js','client.js','platform.js','storage.js'].includes(file)) { res.writeHead(404); return res.end(); }
    res.end(fs.readFileSync(path.join(root,file)));
  });
  await new Promise(resolve => server.listen(0, '127.0.0.1', resolve));
  const browser = await engines.chromium.launch(); const page = await browser.newPage();
  await page.goto('http://127.0.0.1:' + server.address().port + '/index.html');
  await page.waitForFunction(() => document.getElementById('status').textContent.includes('Unable to load'));
  assert.match(await page.locator('#status').textContent(), /static files|load/);
  await browser.close(); server.close();
  console.log('WASM missing/corrupt asset failure message PASS');
})().catch(error => { console.error(error); process.exitCode = 1; });
