#include "game.h"
#include "app.h"
#include "keys.h"
#include "render.h"
#include "rng.h"
#include "storage.h"
#include <stdio.h>
#include <string.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#else
#include <time.h>
#endif

static Game game;
static int initialized;
static char cells[80 * 23];
static unsigned char palette[80 * 23];
static unsigned int version;
static int beep;
#ifdef __EMSCRIPTEN__
EM_JS(void, browser_save, (), {
    if (Module['onSave']) Module['onSave']();
});
#endif

const char *rocket_version(void)
{
    return GAME_VERSION;
}

void rocket_init(unsigned int seed)
{
    memset(&game, 0, sizeof(game));
    keys_reset();
    version = 0;
    beep = 0;
#ifdef __EMSCRIPTEN__
    storage_set_directory("/saves");
    storage_set_modified_hook(browser_save);
#endif
    rng_seed(seed);
    game_init(&game, 80, 23);
    initialized = 1;
    render_frame(&game);
}

void rocket_tick(void)
{
    TermInput input;
    int running = 1;
    if (!initialized)
        return;
    keys_take(&input);
    app_handle_input(&game, &input, &running);
    if (!running)
        game_to_menu(&game);
    game.fps = 30.0;
    app_step(&game);
}

int rocket_key(int code, int down)
{
    if (!initialized || code < 0 || code > 255 || (down != 0 && down != 1))
        return 0;
    if (down)
        return keys_down(code);
    keys_up(code);
    return 1;
}

void rocket_blur(void)
{
    keys_reset();
    if (initialized && game.state == GAME_PLAYING)
        game_pause(&game);
}

const char *rocket_chars(void) { return cells; }
const unsigned char *rocket_colors(void) { return palette; }
unsigned int rocket_frame(void) { return version; }
int rocket_beep(void) { int result = beep; beep = 0; return result; }

void term_init(void) { keys_reset(); }
void term_shutdown(void) { }
int term_size(int *width, int *height) { *width = 80; *height = 23; return TERM_SIZE_OK; }
void term_read_input(TermInput *input) { keys_take(input); }
void term_beep(void) { beep = 1; }
int term_should_stop(void) { return 0; }
void term_sleep(double seconds) { (void)seconds; } /* Browser loop never blocks. */
double term_time(void)
{
#ifdef __EMSCRIPTEN__
    return emscripten_get_now() / 1000.0;
#else
    return (double)clock() / CLOCKS_PER_SEC; /* Native parity tests only. */
#endif
}

void term_draw(const char *chars, const unsigned char *colors, int width, int height)
{
    if (width != 80 || height != 23)
        return;
    if (version == 0 || memcmp(cells, chars, sizeof(cells)) ||
        memcmp(palette, colors, sizeof(palette))) {
        memcpy(cells, chars, sizeof(cells));
        memcpy(palette, colors, sizeof(palette));
        version++;
    }
}

#ifdef ROCKET_TEST
void rocket_test_state(int state)
{
    if (state >= GAME_MENU && state <= GAME_INITIALS) {
        game.state = (GameState)state;
        if (state == GAME_INITIALS) { game.leaderboard_rank = 0; game.score = 12345; }
        render_frame(&game);
    }
}

const char *rocket_snapshot(void)
{
    static char result[512];
    snprintf(result, sizeof(result),
        "{\"state\":%d,\"frame\":%lu,\"x\":%.17g,\"y\":%.17g,"
        "\"vx\":%.17g,\"vy\":%.17g,\"fuel\":%.17g,\"hull\":%.17g,\"time\":%.17g}",
        game.state, game.frame, game.player_x, game.player_y, game.player_vx,
        game.player_vy, game.fuel, game.hull, game.mission_time);
    return result;
}
#endif
