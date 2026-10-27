#include <string.h>

#include "game.h"
#include "test.h"

/*
 * Issue #5: game_resize() must re-measure geometry only.  A resize that
 * clobbered the flight state would drop the player mid-mission, so every
 * other field has to survive it untouched.
 */
static void resize_updates_geometry_and_keeps_flight(void)
{
    Game g;
    double player_x;
    double player_y;
    int i;

    game_init(&g, 80, 23);
    game_start(&g);

    /* fly a little so the camera and entities are not in their init state */
    for (i = 0; i < 60; i++)
        game_update(&g);

    g.score = 4321;
    g.hull = 55.0;
    g.fuel = 33.0;
    player_x = g.player_x;
    player_y = g.player_y;

    game_resize(&g, 120, 40);

    CHECK(g.width == 120);
    CHECK(g.height == 40);
    CHECK(g.play_height == 38);
    CHECK(g.world_mid == 19.0);
    CHECK(g.star_count <= GAME_MAX_STARS);
    CHECK(g.star_count == g.width * 4 || g.star_count == GAME_MAX_STARS);

    /* flight state untouched */
    CHECK(g.state == GAME_PLAYING);
    CHECK(g.score == 4321);
    CHECK(g.hull == 55.0);
    CHECK(g.fuel == 33.0);
    CHECK(g.player_x == player_x);
    CHECK(g.player_y == player_y);

    /* camera still inside the bounds game_update() steers towards */
    CHECK(g.camera_x >= -20.0);
    CHECK(g.camera_x <= GAME_WORLD_LENGTH - g.width + 20.0);

    /* stars were rebuilt for the new play height */
    for (i = 0; i < g.star_count; i++) {
        CHECK(g.stars[i].y >= 1.0);
        CHECK(g.stars[i].y < (double)g.play_height);
    }
}

/* Growing the window shrinks the far camera bound: the camera must follow. */
static void resize_clamps_camera_to_the_new_world_bounds(void)
{
    Game g;

    game_init(&g, 40, 12);
    game_start(&g);

    /* camera riding the far edge the old (narrower) width allowed */
    g.camera_x = GAME_WORLD_LENGTH - 40 + 20;

    game_resize(&g, 200, 30);
    CHECK(g.camera_x == GAME_WORLD_LENGTH - 200 + 20);

    /* shrinking again leaves an in-bounds camera exactly where it was */
    game_resize(&g, 40, 12);
    CHECK(g.camera_x == GAME_WORLD_LENGTH - 200 + 20);

    /* and the near edge is clamped the same way */
    g.camera_x = -100.0;
    game_resize(&g, 80, 24);
    CHECK(g.camera_x == -20.0);
}

/* Resizing in the menu must keep the starfield drifting at the new width. */
static void resize_in_menu_keeps_starfield_scrolling(void)
{
    Game g;
    double camera_before;
    int i;

    game_init(&g, 80, 23);
    CHECK(g.state == GAME_MENU);

    game_resize(&g, 120, 30);
    CHECK(g.state == GAME_MENU);
    CHECK(g.star_count == GAME_MAX_STARS); /* 120 * 4 = 480, capped */

    /* the menu camera belongs to the drift, not to the play clamp */
    camera_before = g.camera_x;
    for (i = 0; i < 30; i++)
        game_update(&g);

    CHECK(g.camera_x > camera_before);
    CHECK(g.state == GAME_MENU);
}

/* Sizes that cannot host the HUD plus one play row are refused, not drawn. */
static void resize_ignores_unusable_sizes(void)
{
    Game g;

    game_init(&g, 80, 23);

    game_resize(&g, 0, 0);
    CHECK(g.width == 80);
    CHECK(g.height == 23);

    game_resize(&g, 60, 2); /* play_height would fall to 0 */
    CHECK(g.width == 80);
    CHECK(g.height == 23);
}

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

    resize_updates_geometry_and_keeps_flight();
    resize_clamps_camera_to_the_new_world_bounds();
    resize_in_menu_keeps_starfield_scrolling();
    resize_ignores_unusable_sizes();
}
