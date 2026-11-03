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
