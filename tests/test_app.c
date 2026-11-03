#include <string.h>
#include "test.h"
#include "app.h"

void test_app(void)
{
    Game g;
    TermInput in;
    int running = 1;
    game_init(&g, 80, 23);
    memset(&in, 0, sizeof(in));
    in.digit = in.letter = -1;
    in.enter = 1;
    app_handle_input(&g, &in, &running);
    CHECK(g.state == GAME_PLAYING);
    in.enter = 0;
    in.pause = 1;
    app_handle_input(&g, &in, &running);
    CHECK(g.state == GAME_PAUSED);
    app_handle_input(&g, &in, &running);
    CHECK(g.state == GAME_PLAYING);
    in.pause = 0;
    in.back = 1;
    app_handle_input(&g, &in, &running);
    CHECK(g.state == GAME_MENU);
    app_handle_input(&g, &in, &running);
    CHECK(running == 0);

    running = 1;
    memset(&in, 0, sizeof(in));
    in.digit = in.letter = -1;
    g.state = GAME_INITIALS;
    g.initials[0] = '\0';
    g.show_help = g.show_scores = 0;
    in.letter = 'H'; in.help = 1;
    app_handle_input(&g, &in, &running);
    CHECK(strcmp(g.initials, "H") == 0 && !g.show_help);
    in.letter = 'I'; in.help = 0; in.info = 1;
    app_handle_input(&g, &in, &running);
    CHECK(strcmp(g.initials, "HI") == 0 && !g.show_scores);
    in.letter = 'P'; in.info = 0; in.pause = 1;
    app_handle_input(&g, &in, &running);
    CHECK(strcmp(g.initials, "HIP") == 0);
    in.letter = 'X'; in.pause = 0;
    app_handle_input(&g, &in, &running);
    CHECK(strcmp(g.initials, "HIP") == 0); /* bounded to three letters */
    in.letter = -1; in.backspace = 1;
    app_handle_input(&g, &in, &running);
    CHECK(strcmp(g.initials, "HI") == 0);
}
