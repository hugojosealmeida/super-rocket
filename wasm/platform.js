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
    const palette = {
        30: '#000000', 31: '#CC0000', 32: '#00CC00', 33: '#CCCC00',
        34: '#0000CC', 35: '#CC00CC', 36: '#00CCCC', 37: '#C0C0C0',
        90: '#808080', 91: '#FF5555', 92: '#55FF55', 93: '#FFFF55',
        94: '#5555FF', 95: '#FF55FF', 96: '#55FFFF', 97: '#FFFFFF'
    };
    function runs(chars, colors, width = 80, height = 23) {
        if (!Number.isInteger(width) || !Number.isInteger(height) || width < 1 || height < 1 ||
            width > 80 || height > 23 || chars.length < width * height || colors.length < width * height)
            throw new Error('Invalid C frame dimensions/buffers');
        const result = [];
        for (let i = 0; i < width * height; i++) {
            const color = palette[colors[i]] || '#FFFFFF';
            if (!result.length || result[result.length - 1].color !== color) result.push({ color, text: '' });
            result[result.length - 1].text += chars[i] >= 32 && chars[i] <= 126 ? String.fromCharCode(chars[i]) : ' ';
            if ((i + 1) % width === 0 && i + 1 < width * height) result[result.length - 1].text += '\n';
        }
        return result;
    }
    function draw(module, screen, previous) {
        const version = module._rocket_frame();
        if (version === previous) return previous;
        const heap = module.HEAPU8; // Fresh memory view for each frame.
        const chars = module._rocket_chars(), colors = module._rocket_colors(), count = 80 * 23;
        if (!heap || chars < 0 || colors < 0 || chars + count > heap.length || colors + count > heap.length)
            throw new Error('Invalid C frame memory');
        const fragment = screen.ownerDocument.createDocumentFragment();
        for (const run of runs(heap.subarray(chars, chars + count), heap.subarray(colors, colors + count))) {
            const font = screen.ownerDocument.createElement('font');
            font.color = run.color;
            font.textContent = run.text; // Never interpret C data as HTML.
            fragment.appendChild(font);
        }
        screen.replaceChildren(fragment);
        return version;
    }
    const api = { clock, runs, draw, palette };
    if (typeof module === 'object') module.exports = api;
    else root.RocketPlatform = api;
}(typeof window === 'object' ? window : globalThis));
