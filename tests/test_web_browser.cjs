/* Development-only browser tests. The shipped page uses legacy JavaScript. */
const assert = require('node:assert/strict');
const { spawn } = require('node:child_process');
const fs = require('node:fs');
const net = require('node:net');
const os = require('node:os');
const path = require('node:path');
const engines = require('playwright');

async function main() {
  const binary = path.resolve(process.argv[2] || './super-rocket-web');
  const engine = process.argv[3] || 'chromium';
  const home = fs.mkdtempSync(path.join(os.tmpdir(), 'rocket-browser-'));
  const listener = net.createServer();
  await new Promise(resolve => listener.listen(0, '127.0.0.1', resolve));
  const port = listener.address().port;
  await new Promise(resolve => listener.close(resolve));
  const server = spawn(binary, [String(port)], { env: { ...process.env, HOME: home } });
  const exited = new Promise(resolve => server.once('exit', resolve));
  let browser;
  const errors = [];
  try {
    await new Promise((resolve, reject) => {
      const timeout = setTimeout(() => reject(new Error('startup timed out')), 5000);
      server.stdout.once('data', () => { clearTimeout(timeout); resolve(); });
      server.once('error', reject);
      server.once('exit', code => { clearTimeout(timeout); reject(new Error('server exit ' + code)); });
    });
    browser = await engines[engine].launch();
    const page = await browser.newPage();
    page.on('pageerror', e => errors.push(e.message));
    await page.goto('http://127.0.0.1:' + port);
    const frame = page.frame({ name: 'play' });
    assert.ok(frame, 'play frame attached');
    const screen = frame.locator('textarea');
    const waitText = text => page.waitForFunction(
      text => frames[0].document.forms[0].screen.value.includes(text), text);
    const waitNoText = text => page.waitForFunction(
      text => !frames[0].document.forms[0].screen.value.includes(text), text);
    await waitText('lift off');
    await screen.click();
    await page.keyboard.press('3');
    await waitText('> [3]');
    await page.keyboard.press('1');
    await waitText('> [1]');
    await page.keyboard.press('h');
    await waitText('H O W   T O   F L Y');
    await page.keyboard.press('h');
    await waitNoText('H O W   T O   F L Y');
    await page.keyboard.press('i');
    await waitText('T O P   P I L O T S');
    await page.keyboard.press('i');
    await waitNoText('T O P   P I L O T S');
    await page.keyboard.press('t');
    await waitText('theme:amber');
    await page.keyboard.press('f');
    await waitText('fps:on');
    await page.keyboard.press('m');
    await waitText('sound:on');
    await page.keyboard.press('x');
    await waitText('STATISTICS RESET');
    await page.keyboard.press('Enter');
    await waitText('LEVEL 1');
    await page.keyboard.down('p');
    await waitText('PAUSED');
    await page.keyboard.down('p'); // autorepeat must not resume
    await page.waitForTimeout(200);
    assert.ok((await screen.inputValue()).includes('PAUSED'));
    await page.keyboard.up('p');
    await page.keyboard.press('p');
    await waitNoText('PAUSED');
    await page.keyboard.down('w');
    assert.equal(await frame.evaluate(() => !!held[87]), true);
    await page.waitForTimeout(600); // crosses heartbeat boundary
    await page.keyboard.up('w');
    assert.equal(await frame.evaluate(() => !!held[87]), false);
    await page.keyboard.down('ArrowRight');
    await frame.evaluate(() => keyBlur());
    assert.equal(await frame.evaluate(() => !!held[39]), false);
    await page.keyboard.up('ArrowRight');
    const before = await frame.evaluate(() => ({ x: window.pageXOffset, y: window.pageYOffset }));
    await page.keyboard.press('Space');
    const after = await frame.evaluate(() => ({ x: window.pageXOffset, y: window.pageYOffset }));
    assert.deepEqual(after, before, 'Space does not scroll');
    await page.keyboard.press('r');
    await waitText('LEVEL 1');
    await page.keyboard.press('q');
    await waitText('lift off');
    await page.keyboard.press('Escape');
    await waitText('lift off');
    await page.reload();
    await waitText('theme:amber');
    assert.deepEqual(errors, [], 'no browser JavaScript errors');
    console.log(engine + ': launch, controls, held keys, autorepeat, blur, scroll and reload PASS');
  } finally {
    if (browser) await browser.close();
    server.kill('SIGTERM');
    await exited;
    fs.rmSync(home, { recursive: true, force: true });
  }
}

main().catch(error => { console.error(error); process.exitCode = 1; });
