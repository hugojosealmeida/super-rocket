/* Platform bridge only; the game is compiled C, not JavaScript. */
(async function () {
    const status = document.getElementById('status');
    try {
        const capabilities = RocketPlatform.capabilities();
        if (!capabilities.wasm) throw new Error('WebAssembly is unavailable. Use the terminal or C-server version.');
        if (!capabilities.raf) throw new Error('This browser has no animation scheduler.');
        let saves;
        const module = await createRocketModule({onSave() { if (saves) saves.flush(); }});
        saves = RocketStorage.open(module, () => window.localStorage,
            text => { document.getElementById('storage-status').textContent = text; });
        const seed = new Uint32Array(1);
        if (window.crypto && window.crypto.getRandomValues) window.crypto.getRandomValues(seed);
        else seed[0] = Date.now() >>> 0;
        module._rocket_init(seed[0]);
        document.getElementById('reset-saves').addEventListener('click', () => {
            if (!window.confirm('Delete only this game’s browser saves and restart?')) return;
            saves.reset(); module._rocket_init(seed[0]); clock.reset();
        });
        const screen = document.getElementById('screen');
        const input = RocketPlatform.keyboard(module, screen);
        let drawn = RocketPlatform.draw(module, screen, -1);
        const clock = RocketPlatform.clock();
        let audio = null;
        function sound() {
            if (audio || !module._rocket_beep()) return;
            try {
                const Context = window.AudioContext || window.webkitAudioContext;
                if (!Context) return;
                audio = new Context();
                const oscillator = audio.createOscillator(), gain = audio.createGain();
                oscillator.frequency.value = 440; gain.gain.value = 0.04;
                oscillator.connect(gain); gain.connect(audio.destination); oscillator.start(); oscillator.stop(audio.currentTime + 0.06);
            } catch (_) { audio = null; }
        }
        function frame(now) {
            if (!document.hidden && module._rocket_visible()) {
                const count = clock.advance(now / 1000);
                for (let i = 0; i < count; i++) { module._rocket_tick(); input.afterTick(); }
                drawn = RocketPlatform.draw(module, screen, drawn);
                sound();
            }
            requestAnimationFrame(frame);
        }
        function blur() { input.clear(); clock.reset(); }
        window.addEventListener('blur', blur);
        document.addEventListener('visibilitychange', () => {
            if (document.hidden) { module._rocket_visibility(0); blur(); }
            else { module._rocket_visibility(1); clock.reset(); }
        });
        requestAnimationFrame(frame);
        status.textContent = 'C module ready: super-rocket ' + module.UTF8ToString(module._rocket_version());
    } catch (error) {
        status.textContent = 'Unable to load the C game. Use HTTP(S) and check the static files. ' + error.message;
    }
}());
