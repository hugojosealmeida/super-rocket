/* Pure platform utilities, also exercised in Node. No game rules. */
(function (root) {
    function clock() {
        let previous = null, accumulated = 0;
        const step = 1 / 30;
        return {
            reset() { previous = null; accumulated = 0; },
            advance(now) {
                if (!Number.isFinite(now) || (previous !== null && now < previous)) {
                    previous = null; accumulated = 0; return 0;
                }
                if (previous === null) { previous = now; return 0; }
                accumulated = Math.min(accumulated + now - previous, step * 5);
                previous = now;
                const count = Math.min(5, Math.floor((accumulated + 1e-10) / step));
                accumulated -= count * step;
                return count;
            }
        };
    }
    const api = { clock };
    if (typeof module === 'object') module.exports = api;
    else root.RocketPlatform = api;
}(typeof window === 'object' ? window : globalThis));
