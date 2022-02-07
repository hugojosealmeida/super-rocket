#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "game.h"
#include "render.h"
#include "rng.h"
#include "term.h"

#define FRAME_TIME (1.0 / 30.0)

static void handle_input(Game *g, const TermInput *in, int *running)
{
    if (in->quit) {
        *running = 0;
        return;
    }

    if (in->help) {
        game_toggle_help(g);
        return;
    }

    if (g->show_help)
        return;

    if (in->theme)
        game_cycle_theme(g);
    if (in->sound)
        game_toggle_sound(g);
    if (in->fps)
        game_toggle_fps(g);

    switch (g->state) {
    case GAME_MENU:
        if (in->back) {
            *running = 0;
        } else if (in->enter) {
            game_start(g);
        } else if (in->digit >= 1 && in->digit <= DIFFICULTY_COUNT) {
            game_set_difficulty(g, (Difficulty)(in->digit - 1));
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
            if (in->fire)
                game_fire(g);
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

int main(int argc, char **argv)
{
    Game game;
    int width;
    int height;
    int running = 1;
    int i;
    int have_seed = 0;
    uint64_t seed = 0;

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--seed") == 0 && i + 1 < argc) {
            seed = strtoull(argv[++i], NULL, 10);
            have_seed = 1;
        } else if (strncmp(argv[i], "--seed=", 7) == 0) {
            seed = strtoull(argv[i] + 7, NULL, 10);
            have_seed = 1;
        } else if (strcmp(argv[i], "--version") == 0) {
            printf("super-rocket %s\n", GAME_VERSION);
            return 0;
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printf("usage: %s [--seed N] [--version] [--help]\n", argv[0]);
            printf("\n");
            printf("  --seed N    use a fixed random seed\n");
            printf("  --version   print the version and exit\n");
            printf("  --help      show this help\n");
            return 0;
        } else {
            fprintf(stderr, "super-rocket: unknown option '%s'\n", argv[i]);
            fprintf(stderr, "usage: %s [--seed N]\n", argv[0]);
            return 2;
        }
    }

    if (have_seed)
        rng_seed(seed);
    else
        rng_seed_from_clock();

    term_init();
    term_size(&width, &height);
    game_init(&game, width, height);

    game.fps = 0.0;

    while (running && !term_should_stop()) {
        static double last_frame = 0.0;
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

        {
            double now = term_time();

            if (last_frame > 0.0 && now > last_frame) {
                double sample = 1.0 / (now - last_frame);

                game.fps = game.fps <= 0.0 ? sample : game.fps * 0.9 + sample * 0.1;
            }
            last_frame = now;
        }
    }

    term_shutdown();

    return 0;
}
