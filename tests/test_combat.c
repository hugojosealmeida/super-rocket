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

void test_combat(void)
{
    Game g;
    Asteroid *rock;
    Ufo *ufo;
    Laser *laser;
    EnemyShot *shot;
    Pickup *pickup;

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
}
