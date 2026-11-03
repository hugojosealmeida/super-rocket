#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "app.h"
#include "game.h"
#include "rng.h"
#include "term.h"

#define FRAME_TIME (1.0 / 30.0)

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

    /*
     * The size check runs before term_init() so a too-small terminal is
     * refused with no alt-screen or raw-mode side effects to unwind.
     */
    if (term_size(&width, &height) == TERM_SIZE_TOO_SMALL) {
        fprintf(stderr, "super-rocket: terminal too small (need at least %dx%d, got %dx%d)\n",
                TERM_MIN_COLS, TERM_MIN_ROWS, width, height);
        return 1;
    }

    term_init();
    game_init(&game, width, height);

    game.fps = 0.0;

    while (running && !term_should_stop()) {
        static double last_frame = 0.0;
        double frame_start = term_time();
        double elapsed;
        TermInput input;

        term_read_input(&input);
        app_handle_input(&game, &input, &running);
        app_step(&game);

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
