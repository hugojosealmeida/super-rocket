/* Platform bridge only; the game is compiled C, not JavaScript. */
(async function () {
    const status = document.getElementById('status');
    try {
        if (typeof WebAssembly !== 'object') throw new Error('WebAssembly is unavailable.');
        const module = await createRocketModule();
        module.FS.mkdir('/saves');
        /* In-memory storage until the durable browser adapter is installed. */
        module.FS.chdir('/saves');
        const seed = new Uint32Array(1);
        if (window.crypto && window.crypto.getRandomValues) window.crypto.getRandomValues(seed);
        else seed[0] = Date.now() >>> 0;
        module._rocket_init(seed[0]);
        const clock = RocketPlatform.clock();
        function frame(now) {
            if (!document.hidden) {
                const count = clock.advance(now / 1000);
                for (let i = 0; i < count; i++) module._rocket_tick();
            }
            requestAnimationFrame(frame);
        }
        function blur() { module._rocket_blur(); clock.reset(); }
        window.addEventListener('blur', blur);
        document.addEventListener('visibilitychange', () => { if (document.hidden) blur(); });
        requestAnimationFrame(frame);
        status.textContent = 'C module ready: super-rocket ' + module.UTF8ToString(module._rocket_version());
    } catch (error) {
        status.textContent = 'Unable to load the C game. Use HTTP(S) and check the static files. ' + error.message;
    }
}());
