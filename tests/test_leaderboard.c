#include <string.h>

#include "game.h"
#include "test.h"

void test_leaderboard(void)
{
    Game g;

    game_init(&g, 100, 30);
    CHECK(game_leaderboard_position(&g, 0) == -1);
    CHECK(game_leaderboard_position(&g, 5000) == 0);

    g.score = 4321;
    g.leaderboard_rank = 0;
    strcpy(g.initials, "HJA");
    g.state = GAME_INITIALS;

    game_commit_initials(&g);
    CHECK(g.state == GAME_WON);
    CHECK(g.leaderboard[0].score == 4321);
    CHECK(strcmp(g.leaderboard[0].name, "HJA") == 0);

    game_load_leaderboard(&g);
    CHECK(g.leaderboard[0].score == 4321);
    CHECK(strcmp(g.leaderboard[0].name, "HJA") == 0);

    g.initials[0] = '\0';
    game_initial_push(&g, 'A');
    game_initial_push(&g, 'B');
    game_initial_push(&g, 'C');
    game_initial_push(&g, 'D');
    CHECK(strcmp(g.initials, "ABC") == 0);
    game_initial_pop(&g);
    CHECK(strcmp(g.initials, "AB") == 0);
}
