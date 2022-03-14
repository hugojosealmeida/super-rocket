#include <string.h>

#include "game.h"
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
}
