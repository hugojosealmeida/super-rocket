/* Browser storage transport for existing C stdio files, not game rules. */
(function (root) {
    const key = 'super-rocket.wasm.saves';
    const names = ['.super_rocket_scores', '.super_rocket_stats', '.super_rocket_config', '.super_rocket_leaderboard'];
    const maxBytes = 32768;
    function integers(text, count) {
        const tokens = text.trim().split(/\s+/);
        if (tokens.length !== count || tokens.some(token => !/^\d+$/.test(token))) throw new Error('Invalid integer save');
        const values = tokens.map(Number);
        if (values.some(value => !Number.isSafeInteger(value) || value > 2147483647)) throw new Error('Integer out of bounds');
        return values;
    }
    function validate(files) {
        if (!files || typeof files !== 'object' || Array.isArray(files)) throw new Error('Invalid save map');
        for (const name of Object.keys(files)) {
            const text = files[name];
            if (!names.includes(name) || typeof text !== 'string' || text.length > 4096) throw new Error('Invalid file');
            if (name === names[0]) integers(text, 3);
            else if (name === names[1]) integers(text, 11);
            else if (name === names[2]) {
                const values = integers(text, 3);
                if (values[0] > 2 || values[1] > 1 || values[2] > 1) throw new Error('Invalid settings');
            } else {
                const lines = text.trim() ? text.trim().split('\n') : [];
                if (lines.length > 5) throw new Error('Too many scores');
                for (const line of lines) {
                    const match = /^([A-Z]{1,3}|---) (\d+) ([012])$/.exec(line);
                    if (!match || Number(match[2]) > 2147483647) throw new Error('Invalid leaderboard');
                }
            }
        }
        return files;
    }
    function decode(raw) {
        if (typeof raw !== 'string' || raw.length > maxBytes) throw new Error('Save is too large');
        const snapshot = JSON.parse(raw);
        if (!snapshot || snapshot.schema !== 1) throw new Error('Unsupported save schema');
        return validate(snapshot.files);
    }
    function open(runtime, getStorage, report = () => {}) {
        const fs = runtime.FS;
        if (!fs.analyzePath('/saves').exists) fs.mkdir('/saves');
        let storage, writable = true;
        const warning = () => report('Session only: browser saves unavailable, full or incompatible. Existing saved data is preserved.');
        try {
            storage = getStorage();
            const raw = storage.getItem(key);
            if (raw !== null) {
                const files = decode(raw); // Validate everything before mutating MEMFS.
                for (const name of Object.keys(files)) fs.writeFile('/saves/' + name, files[name]);
            }
            report('Local browser saves. Tabs share saves: last successful writer wins.');
        } catch (_) { writable = false; warning(); }
        return {
            flush() {
                if (!writable) return false;
                try {
                    const files = {};
                    for (const name of names) {
                        if (fs.analyzePath('/saves/' + name).exists)
                            files[name] = fs.readFile('/saves/' + name, {encoding:'utf8'});
                    }
                    validate(files);
                    const raw = JSON.stringify({schema:1, files});
                    if (raw.length > maxBytes) throw new Error('Save too large');
                    storage.setItem(key, raw); // Synchronous; not an unload-only write.
                    return true;
                } catch (_) { writable = false; warning(); return false; }
            },
            reset() {
                for (const name of names)
                    if (fs.analyzePath('/saves/' + name).exists) fs.unlink('/saves/' + name);
                try { storage = getStorage(); storage.removeItem(key); writable = true; report('Local saves reset.'); }
                catch (_) { writable = false; warning(); }
            }
        };
    }
    const api = { key, names, decode, validate, open };
    if (typeof module === 'object') module.exports = api;
    else root.RocketStorage = api;
}(typeof window === 'object' ? window : globalThis));
