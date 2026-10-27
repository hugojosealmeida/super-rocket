#include "game.h"
#include "test.h"

static void clear_enemies(Game *g)
{
    int i;

    for (i = 0; i < GAME_MAX_ASTEROIDS; i++)
        g->asteroids[i].alive = 0;
    for (i = 0; i < GAME_MAX_UFOS; i++)
        g->ufos[i].alive = 0;
    for (i = 0; i < GAME_MAX_LASERS; i++)
        g->lasers[i].alive = 0;
    for (i = 0; i < GAME_MAX_SHOTS; i++)
        g->shots[i].alive = 0;
    for (i = 0; i < GAME_MAX_PICKUPS; i++)
        g->pickups[i].alive = 0;
}

/*
 * Issue #2: the streak timer is game time, not wall-clock time. While the
 * simulation is frozen (paused, upgrade screen, help or leaderboard overlay)
 * a 200 frame stall must leave the combo and its timer untouched.
 */
static void combo_survives_frozen_simulation(Game *g)
{
    int i;

    clear_enemies(g);
    g->combo = 9;
    g->combo_timer = 4.0;
    g->mission_time = 0.0;

    for (i = 0; i < 200; i++)
        game_update(g);

    CHECK(g->combo == 9);
    CHECK(g->combo_timer == 4.0);
    CHECK(g->mission_time == 0.0);
}

void test_combat(void)
{
    Game g;
    Asteroid *rock;
    Ufo *ufo;
    Laser *laser;
    EnemyShot *shot;
    Pickup *pickup;
    int i;
    double timer;

    game_init(&g, 100, 30);
    game_start(&g);

    clear_enemies(&g);
    g.invuln = 0.0;
    g.shield = 50.0;
    g.hull = 100.0;
    rock = &g.asteroids[0];
    rock->x = g.player_x;
    rock->y = g.player_y;
    rock->vx = 0.0;
    rock->vy = 0.0;
    rock->radius = 1;
    rock->hp = 1;
    rock->alive = 1;
    game_update(&g);
    CHECK(!rock->alive);
    CHECK(g.shield == 26.0);
    CHECK(g.hull == 100.0);

    clear_enemies(&g);
    g.invuln = 0.0;
    g.score = 0;
    ufo = &g.ufos[0];
    ufo->x = g.player_x + 12.0;
    ufo->y = g.player_y;
    ufo->vx = 0.0;
    ufo->fire_timer = 5.0;
    ufo->hp = 1;
    ufo->alive = 1;
    laser = &g.lasers[0];
    laser->x = g.player_x + 12.0;
    laser->y = g.player_y;
    laser->vx = 0.0;
    laser->vy = 0.0;
    laser->life = 1.0;
    laser->alive = 1;
    game_update(&g);
    CHECK(!ufo->alive);
    CHECK(g.score == 200);

    clear_enemies(&g);
    g.fuel = 10.0;
    pickup = &g.pickups[0];
    pickup->x = g.player_x;
    pickup->y = g.player_y;
    pickup->type = PICKUP_FUEL;
    pickup->alive = 1;
    game_update(&g);
    CHECK(!pickup->alive);
    CHECK(g.fuel == 40.0);

    clear_enemies(&g);
    g.invuln = 0.0;
    g.shield = 0.0;
    g.hull = 100.0;
    shot = &g.shots[0];
    shot->x = g.player_x;
    shot->y = g.player_y;
    shot->vx = 0.0;
    shot->vy = 0.0;
    shot->life = 1.0;
    shot->alive = 1;
    game_update(&g);
    CHECK(g.hull == 86.0);

    /* --- combo streak only ticks in game time (issue #2) --- */

    game_init(&g, 100, 30);
    game_start(&g);
    game_pause(&g);
    CHECK(g.state == GAME_PAUSED);
    combo_survives_frozen_simulation(&g);

    game_resume(&g);
    CHECK(g.state == GAME_PLAYING);
    g.state = GAME_UPGRADE;
    combo_survives_frozen_simulation(&g);

    game_init(&g, 100, 30);
    game_start(&g);
    g.show_help = 1;
    combo_survives_frozen_simulation(&g);
    g.show_help = 0;
    g.show_scores = 1;
    combo_survives_frozen_simulation(&g);
    g.show_scores = 0;

    /*
     * A streak earned just before a sector boundary survives the whole
     * upgrade screen, and keeps its remaining time when flight resumes.
     */
    game_init(&g, 100, 30);
    game_start(&g);
    clear_enemies(&g);
    g.combo = 6;
    g.combo_timer = 4.0;
    g.state = GAME_UPGRADE;
    for (i = 0; i < 120; i++)
        game_update(&g);
    CHECK(g.combo == 6);
    CHECK(g.combo_timer == 4.0);
    game_choose_upgrade(&g, 0);
    CHECK(g.state == GAME_PLAYING);
    for (i = 0; i < 30; i++) {
        clear_enemies(&g);
        game_update(&g);
    }
    CHECK(g.combo == 6);
    timer = g.combo_timer;
    CHECK(timer < 4.0 && timer > 2.5);

    /*
     * While the world really is flying, decay behaves exactly as before:
     * 4.0s of game time burns the streak down to zero.
     */
    game_init(&g, 100, 30);
    game_start(&g);
    clear_enemies(&g);
    g.combo = 9;
    g.combo_timer = 4.0;
    for (i = 0; i < 60; i++) {
        clear_enemies(&g);
        game_update(&g);
    }
    CHECK(g.state == GAME_PLAYING);
    CHECK(g.combo == 9);
    timer = g.combo_timer;
    CHECK(timer > 1.9 && timer < 2.1);
    for (i = 0; i < 70; i++) {
        clear_enemies(&g);
        game_update(&g);
    }
    CHECK(g.state == GAME_PLAYING);
    CHECK(g.combo == 0);
    CHECK(g.combo_timer <= 0.0);

    /* Toasts still fade in every state, including the frozen ones. */
    game_init(&g, 100, 30);
    CHECK(g.state == GAME_MENU);
    g.toast_timer = 2.5;
    for (i = 0; i < 100; i++)
        game_update(&g);
    CHECK(g.toast_timer <= 0.0);

    game_init(&g, 100, 30);
    game_start(&g);
    g.show_help = 1;
    g.combo = 9;
    g.combo_timer = 4.0;
    g.toast_timer = 2.5;
    for (i = 0; i < 10; i++)
        game_update(&g);
    CHECK(g.toast_timer < 2.5);
    CHECK(g.toast_timer > 2.0);
    CHECK(g.combo == 9);
    CHECK(g.combo_timer == 4.0);
}
