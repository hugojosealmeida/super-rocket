#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "app.h"
#include "game.h"
#include "rng.h"
#include "storage.h"
#include "term.h"
#include "web_server.h"
#include "web_term.h"

#define FRAME_TIME (1.0 / 30.0)
static volatile sig_atomic_t stopping;

static void stop_server(int sig)
{
    (void)sig;
    stopping = 1;
}

int main(int argc, char **argv)
{
    const char *bind_addr = "127.0.0.1";
    const char *data_dir = NULL;
    char default_dir[900];
    int port = 8080;
    int have_port = 0;
    int fd;
    int i;
    Game game;
    double next;

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0) {
            printf("usage: %s [PORT] [--bind IPv4] [--data-dir DIR]\n", argv[0]);
            puts("Defaults: 8080, 127.0.0.1. Use Ctrl-C to stop.");
            puts("One shared game; no TLS or accounts. Do not expose to the Internet.");
            return 0;
        }
        if (strcmp(argv[i], "--version") == 0) {
            printf("super-rocket-web %s\n", GAME_VERSION);
            return 0;
        }
        if (strcmp(argv[i], "--bind") == 0 && i + 1 < argc) {
            bind_addr = argv[++i];
        } else if (strcmp(argv[i], "--data-dir") == 0 && i + 1 < argc) {
            data_dir = argv[++i];
        } else {
            char *end;
            long value;
            errno = 0;
            value = strtol(argv[i], &end, 10);
            if (have_port || argv[i][0] < '0' || argv[i][0] > '9' ||
                *end != '\0' || errno != 0 || value < 1 || value > 65535) {
                fprintf(stderr, "super-rocket-web: invalid option or port '%s'\n", argv[i]);
                return 2;
            }
            port = (int)value;
            have_port = 1;
        }
    }

    if (data_dir == NULL) {
        const char *home = getenv("HOME");
        if (home == NULL || !home[0] ||
            snprintf(default_dir, sizeof(default_dir), "%s/.super_rocket_web", home) >= (int)sizeof(default_dir)) {
            fputs("super-rocket-web: set HOME or specify --data-dir DIR\n", stderr);
            return 2;
        }
        data_dir = default_dir;
    }
    {
        struct stat st;
        if (storage_set_directory(data_dir) != 0 ||
            (mkdir(data_dir, 0700) != 0 && errno != EEXIST) ||
            stat(data_dir, &st) != 0 || !S_ISDIR(st.st_mode)) {
            fprintf(stderr, "super-rocket-web: cannot use save directory '%s'\n", data_dir);
            return 1;
        }
    }

    fd = web_server_listen(bind_addr, port);
    if (fd < 0) {
        fprintf(stderr, "super-rocket-web: cannot listen on %s:%d\n", bind_addr, port);
        return 1;
    }
    signal(SIGINT, stop_server);
    signal(SIGTERM, stop_server);
    rng_seed_from_clock();
    term_init();
    game_init(&game, WEB_TERM_WIDTH, WEB_TERM_HEIGHT);
    game.settings.sound = 0;
    printf("super-rocket-web: http://%s:%d (one shared game; Ctrl-C stops server)\n", bind_addr, port);
    fflush(stdout);

    next = term_time();
    while (!stopping) {
        double now = term_time();
        if (now >= next) {
            TermInput input;
            int running = 1;
            term_read_input(&input);
            app_handle_input(&game, &input, &running);
            /* Esc / Q in a browser end the game, not the shared server. */
            if (!running)
                game_to_menu(&game);
            app_step(&game);
            game.fps = 30.0;
            next += FRAME_TIME;
            /* Bound catch-up after a long suspend; never simulate minutes at once. */
            if (next < now - FRAME_TIME * 4)
                next = now + FRAME_TIME;
        }
        now = term_time();
        web_server_service(fd, next > now ? next - now : 0.0);
    }
    web_server_shutdown(fd);
    term_shutdown();
    return 0;
}
