const assert = require('node:assert/strict');
const platform = require('../wasm/platform.js');
for (const hz of [20, 30, 60, 120, 144, 1000]) {
  const clock = platform.clock();
  let ticks = 0;
  for (let i = 0; i <= hz * 10; i++) ticks += clock.advance(i / hz);
  assert.equal(ticks, 300, 'same simulation ticks at ' + hz + ' Hz');
}
const clock = platform.clock();
assert.equal(clock.advance(0), 0);
assert.equal(clock.advance(1000), 5);
clock.reset();
assert.equal(clock.advance(1001), 0);
assert.equal(clock.advance(NaN), 0);
assert.equal(clock.advance(-1), 0);
assert.equal(clock.advance(-2), 0);
console.log('WASM fixed-step scheduling and delay/reset edges PASS');
const chars = new Uint8Array([60, 38, 62, 34]);
const colors = new Uint8Array([91, 92, 94, 97]);
assert.deepEqual(platform.runs(chars, colors, 2, 2).map(run => run.text), ['<', '&\n', '>', '"']);
assert.equal(platform.runs(chars, colors, 2, 2)[0].color, '#FF5555');
assert.throws(() => platform.runs(chars, colors, 0, 1));
assert.throws(() => platform.runs(chars, colors, 80, 23));
assert.throws(() => platform.runs(chars, colors, 81, 1));
const worst = platform.runs(new Uint8Array(1840).fill(60), Uint8Array.from({length:1840}, (_, i) => i % 2 ? 91 : 92));
assert.equal(worst.length, 1840);
assert.ok(worst.every(run => run.text.startsWith('<')));
assert.equal(platform.runs(new Uint8Array([0]), new Uint8Array([255]), 1, 1)[0].text, ' ');
console.log('WASM safe color-run rendering and frame bounds PASS');
