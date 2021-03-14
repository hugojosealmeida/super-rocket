#include <stdio.h>

#include "game.h"
#include "render.h"
#include "term.h"

#define FRAME_TIME (1.0 / 30.0)

static void handle_input(Game *g, const TermInput *in, int *running)
{
    if (in->quit) {
        *running = 0;
        return;
    }

    switch (g->state) {
    case GAME_MENU:
        if (in->back) {
            *running = 0;
        } else if (in->enter) {
            game_start(g);
        }
        break;

    case GAME_PLAYING:
        if (in->back) {
            game_to_menu(g);
        } else if (in->pause) {
            game_pause(g);
        } else if (in->restart) {
            game_start(g);
        } else {
            game_thrust(g, in->up, in->down, in->left, in->right);
        }
        break;

    case GAME_PAUSED:
        if (in->back) {
            game_to_menu(g);
        } else if (in->pause || in->enter) {
            game_resume(g);
        } else if (in->restart) {
            game_start(g);
        }
        break;

    case GAME_WON:
    case GAME_DEAD:
        if (in->restart) {
            game_start(g);
        } else if (in->back || in->enter) {
            game_to_menu(g);
        }
        break;
    }
}

int main(void)
{
    Game game;
    int width;
    int height;
    int running = 1;

    term_init();
    term_size(&width, &height);
    game_init(&game, width, height);

    while (running && !term_should_stop()) {
        double frame_start = term_time();
        double elapsed;
        TermInput input;

        term_read_input(&input);
        handle_input(&game, &input, &running);
        game_update(&game);
        render_frame(&game);

        elapsed = term_time() - frame_start;
        if (elapsed < FRAME_TIME)
            term_sleep(FRAME_TIME - elapsed);
    }

    term_shutdown();

    return 0;
}
