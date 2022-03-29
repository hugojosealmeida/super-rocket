#include <string.h>

#include "game.h"
#include "rng.h"
#include "test.h"

void test_campaign(void)
{
    Game g;
    int i;
    int distinct;

    CHECK(strcmp(game_sector_name(0), "EARTH DEPARTURE") == 0);
    CHECK(strcmp(game_sector_name(1), "DEEP SPACE") == 0);
    CHECK(strcmp(game_sector_name(2), "MARS APPROACH") == 0);
    CHECK(strcmp(game_sector_name(7), "EARTH DEPARTURE") == 0);

    game_init(&g, 100, 30);
    CHECK(g.sector == 0);

    game_roll_upgrades(&g);
    distinct = 1;
    if (g.upgrades[0] == g.upgrades[1] || g.upgrades[0] == g.upgrades[2] ||
        g.upgrades[1] == g.upgrades[2])
        distinct = 0;
    CHECK(distinct);
    for (i = 0; i < 3; i++)
        CHECK(g.upgrades[i] >= 0 && g.upgrades[i] < UPGRADE_COUNT);

    g.state = GAME_UPGRADE;
    g.fuel_max = 100.0;
    g.fuel = 20.0;
    g.upgrades[0] = UPGRADE_FUEL;
    game_choose_upgrade(&g, 0);
    CHECK(g.fuel_max == 125.0);
    CHECK(g.fuel == 45.0);

    g.state = GAME_UPGRADE;
    g.fire_delay = 0.18;
    g.upgrades[0] = UPGRADE_RATE;
    game_choose_upgrade(&g, 0);
    CHECK(g.fire_delay < 0.18);

    g.state = GAME_UPGRADE;
    g.upgrades[0] = UPGRADE_WEAPON;
    g.weapon_level = 3;
    game_choose_upgrade(&g, 0);
    CHECK(g.weapon_level == 3);

    g.state = GAME_UPGRADE;
    g.upgrades[0] = UPGRADE_SHIELD;
    g.shield = 0.0;
    game_choose_upgrade(&g, 0);
    CHECK(g.shield == 100.0);

    CHECK(strstr(game_upgrade_name(UPGRADE_HULL), "HULL") != NULL);

    CHECK(strcmp(game_level_name(0), "EARTH TO MARS") == 0);
    CHECK(strcmp(game_level_name(1), "MORE ENEMIES") == 0);
    CHECK(strcmp(game_level_name(7), "EARTH TO MARS") == 0);

    game_init(&g, 100, 30);
    game_start(&g);
    CHECK(g.level == 0);
    CHECK(g.level_banner > 0.0);

    /* reaching Mars orbit on level 1 starts level 2 instead of winning */
    g.weapon_level = 3;
    g.score = 0;
    g.mission_time = 100.0;
    g.sector = 2;
    g.invuln = 99.0;
    g.player_x = GAME_WORLD_LENGTH - 2.0;
    g.player_y = g.world_mid;
    g.player_vx = 0.0;
    g.player_vy = 0.0;
    game_update(&g);
    CHECK(g.state == GAME_PLAYING);
    CHECK(g.level == 1);
    CHECK(g.sector == 0);
    CHECK(g.player_x < 100.0);
    CHECK(g.score > 0);
    CHECK(g.weapon_level == 3);
    CHECK(g.hull == GAME_HULL_MAX);
    CHECK(g.fuel == g.fuel_max);
    CHECK(g.mission_time > 100.0);
    CHECK(g.level_banner > 0.0);

    /* level 2 respawns asteroids and UFOs faster than level 1 */
    {
        double asteroids_l1;
        double asteroids_l2;
        double ufos_l1;
        double ufos_l2;

        game_init(&g, 100, 30);
        game_start(&g);

        rng_seed(20220328);
        g.spawn_timer = 0.0;
        g.ufo_timer = 0.0;
        game_update(&g);
        asteroids_l1 = g.spawn_timer;
        ufos_l1 = g.ufo_timer;

        rng_seed(20220328);
        g.spawn_timer = 0.0;
        g.ufo_timer = 0.0;
        g.level = 1;
        game_update(&g);
        asteroids_l2 = g.spawn_timer;
        ufos_l2 = g.ufo_timer;

        CHECK(asteroids_l2 < asteroids_l1);
        CHECK(ufos_l2 < ufos_l1);
    }

    /* reaching Mars orbit on level 2 finishes the campaign */
    g.sector = 2;
    g.invuln = 99.0;
    g.player_x = GAME_WORLD_LENGTH - 2.0;
    g.player_y = g.world_mid;
    g.player_vx = 0.0;
    g.player_vy = 0.0;
    game_update(&g);
    CHECK(g.level == 1);
    CHECK(g.state == GAME_WON || g.state == GAME_INITIALS);
}
