#include <string.h>

#include "game.h"
#include "test.h"

void test_game(void)
{
    Game g;
    int i;

    game_init(&g, 100, 30);
    CHECK(g.width == 100);
    CHECK(g.height == 30);
    CHECK(g.play_height == 28);
    CHECK(g.state == GAME_MENU);

    game_set_difficulty(&g, DIFFICULTY_HARD);
    CHECK(strcmp(game_difficulty_name(g.difficulty), "HARD") == 0);

    game_start(&g);
    CHECK(g.state == GAME_PLAYING);
    CHECK(g.fuel == g.fuel_max);
    CHECK(g.hull == GAME_HULL_MAX);
    CHECK(g.sector == 0);

    game_fire(&g);
    CHECK(g.stats.lasers_fired > 0);
    game_fire(&g);
    CHECK(g.stats.lasers_fired == 1);

    for (i = 0; i < 600; i++)
        game_update(&g);

    CHECK(g.mission_time > 0.0);
    CHECK(g.player_x > 0.0);

    g.state = GAME_UPGRADE;
    g.hull = 40.0;
    g.upgrades[0] = UPGRADE_HULL;
    game_choose_upgrade(&g, 0);
    CHECK(g.hull == 70.0);
    CHECK(g.state == GAME_PLAYING);

    game_set_difficulty(&g, DIFFICULTY_EASY);
    game_start(&g);
    CHECK(g.fuel_max == 140.0);

    game_set_difficulty(&g, DIFFICULTY_HARD);
    game_start(&g);
    CHECK(g.fuel_max == 90.0);
}
