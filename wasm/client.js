/* Platform bridge only; the game is compiled C, not JavaScript. */
(async function () {
    const status = document.getElementById('status');
    try {
        if (typeof WebAssembly !== 'object') throw new Error('WebAssembly is unavailable.');
        const module = await createRocketModule();
        status.textContent = 'C module ready: super-rocket ' + module.UTF8ToString(module._rocket_version());
    } catch (error) {
        status.textContent = 'Unable to load the C game. Use HTTP(S) and check the static files. ' + error.message;
    }
}());
