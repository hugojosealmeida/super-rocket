const assert = require('node:assert/strict');
const saves = require('../wasm/storage.js');
const files = {
  '.super_rocket_scores':'123\n456\n789\n',
  '.super_rocket_stats':'1\n0\n1\n2\n3\n4\n5\n6\n7\n8\n9\n',
  '.super_rocket_config':'1\n0\n1\n',
  '.super_rocket_leaderboard':'ALX 12345 1\n--- 0 1\n'
};
const raw = JSON.stringify({schema:1, files});
assert.deepEqual(saves.decode(raw), files);
for (const bad of ['{', 'x'.repeat(32769), JSON.stringify({schema:2, files}),
  JSON.stringify({schema:1, files:{'other':'x'}}),
  JSON.stringify({schema:1, files:{'.super_rocket_scores':'-1 0 0'}}),
  JSON.stringify({schema:1, files:{'.super_rocket_scores':'9999999999999 0 0'}}),
  JSON.stringify({schema:1, files:{'.super_rocket_config':'3 0 1'}}),
  JSON.stringify({schema:1, files:{'.super_rocket_leaderboard':'<x> 1 1'}}),
  JSON.stringify({schema:1, files:{'.super_rocket_leaderboard':'AAA 1 9'}}),
  JSON.stringify({schema:1, files:{'.super_rocket_stats':'0'}})]) assert.throws(() => saves.decode(bad));
function runtime() {
  const map = new Map();
  return {map, FS:{
    analyzePath(name) { return {exists:map.has(name)}; }, mkdir(name) { map.set(name, null); },
    readFile(name) { return map.get(name); }, writeFile(name, value) { map.set(name, value); },
    unlink(name) { map.delete(name); }
  }};
}
const data = new Map([[saves.key, raw], ['unrelated', 'keep']]);
const store = {getItem:k=>data.get(k) || null, setItem:(k,v)=>data.set(k,v), removeItem:k=>data.delete(k)};
const first = runtime();
const adapter = saves.open(first, () => store);
for (const name of saves.names) assert.equal(first.map.get('/saves/' + name), files[name]);
first.FS.writeFile('/saves/.super_rocket_config','2\n1\n0\n');
assert.equal(adapter.flush(), true);
const second = runtime();
saves.open(second, () => store);
assert.equal(second.map.get('/saves/.super_rocket_config'),'2\n1\n0\n');
adapter.reset();
assert.equal(data.has(saves.key), false);
assert.equal(data.get('unrelated'), 'keep');
assert.ok(saves.names.every(name => !first.map.has('/saves/' + name)));
let message = '';
const denied = saves.open(runtime(), () => {throw new Error('denied');}, text=>message=text);
assert.equal(denied.flush(), false); assert.match(message, /Session only/);
const future = JSON.stringify({schema:999, files}); data.set(saves.key, future);
const readonly = saves.open(runtime(), () => store);
assert.equal(readonly.flush(), false); assert.equal(data.get(saves.key), future);
data.delete(saves.key);
const quota = saves.open(runtime(), () => ({...store, setItem(){throw new Error('full');}}), text=>message=text);
assert.equal(quota.flush(), false); assert.match(message, /Session only/);
console.log('WASM save hydration, durability, isolation, corruption, reset and quota cases PASS');
