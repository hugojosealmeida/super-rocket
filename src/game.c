#include "game.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "rng.h"
#include "storage.h"
#include "util.h"

#define DT 0.033

#define PLAYER_ACCEL     26.0
#define PLAYER_MAX_SPEED 45.0
#define PLAYER_DRAG      0.55
#define THRUST_MEMORY    0.12

#define FUEL_BURN        1.5
#define GRAVITY_MU       1500.0

typedef struct {
    double fuel;
    double gravity;
    double spawn_interval;
} DifficultyConfig;

static const DifficultyConfig DIFFICULTIES[DIFFICULTY_COUNT] = {
    { 140.0, 0.80, 2.8 },
    { 115.0, 1.00, 2.2 },
    {  90.0, 1.20, 1.6 }
};

static const char *DIFFICULTY_NAMES[DIFFICULTY_COUNT] = {
    "EASY", "NORMAL", "HARD"
};

const char *game_difficulty_name(Difficulty difficulty)
{
    if (difficulty < 0 || difficulty >= DIFFICULTY_COUNT)
        return "NORMAL";
    return DIFFICULTY_NAMES[difficulty];
}

void game_load_scores(Game *g)
{
    int i;

    for (i = 0; i < DIFFICULTY_COUNT; i++)
        g->best[i] = 0;

    storage_read_ints(".super_rocket_scores", g->best, DIFFICULTY_COUNT);
}

void game_save_scores(const Game *g)
{
    storage_write_ints(".super_rocket_scores", g->best, DIFFICULTY_COUNT);
}

void game_load_stats(Game *g)
{
    int values[GAME_STAT_COUNT];
    int i;

    g->stats.runs = 0;
    g->stats.wins = 0;
    g->stats.deaths = 0;
    g->stats.asteroids_destroyed = 0;
    g->stats.pickups_collected = 0;
    g->stats.lasers_fired = 0;
    g->stats.best_time = 0;
    g->stats.total_distance = 0;
    g->stats.total_fuel_used = 0;

    if (storage_read_ints(".super_rocket_stats", values, GAME_STAT_COUNT) != 0)
        return;

    g->stats.runs = values[0] < 0 ? 0 : values[0];
    g->stats.wins = values[1] < 0 ? 0 : values[1];
    g->stats.deaths = values[2] < 0 ? 0 : values[2];
    g->stats.asteroids_destroyed = values[3] < 0 ? 0 : values[3];
    g->stats.pickups_collected = values[4] < 0 ? 0 : values[4];
    g->stats.lasers_fired = values[5] < 0 ? 0 : values[5];
    g->stats.best_time = values[6] < 0 ? 0 : values[6];
    g->stats.total_distance = values[7] < 0 ? 0 : values[7];
    g->stats.total_fuel_used = values[8] < 0 ? 0 : values[8];

    (void)i;
}

void game_save_stats(const Game *g)
{
    int values[GAME_STAT_COUNT];

    values[0] = g->stats.runs;
    values[1] = g->stats.wins;
    values[2] = g->stats.deaths;
    values[3] = g->stats.asteroids_destroyed;
    values[4] = g->stats.pickups_collected;
    values[5] = g->stats.lasers_fired;
    values[6] = g->stats.best_time;
    values[7] = g->stats.total_distance;
    values[8] = g->stats.total_fuel_used;

    storage_write_ints(".super_rocket_stats", values, GAME_STAT_COUNT);
}

void game_reset_stats(Game *g)
{
    int values[GAME_STAT_COUNT] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };

    storage_write_ints(".super_rocket_stats", values, GAME_STAT_COUNT);
    game_load_stats(g);
}

void game_load_settings(Game *g)
{
    int values[3];

    g->settings.theme = 0;
    g->settings.sound = 1;
    g->settings.show_fps = 0;

    if (storage_read_ints(".super_rocket_config", values, 3) == 0) {
        g->settings.theme = util_clamp_int(values[0], 0, GAME_THEME_COUNT - 1);
        g->settings.sound = values[1] != 0;
        g->settings.show_fps = values[2] != 0;
    }
}

void game_save_settings(const Game *g)
{
    int values[3];

    values[0] = g->settings.theme;
    values[1] = g->settings.sound;
    values[2] = g->settings.show_fps;

    storage_write_ints(".super_rocket_config", values, 3);
}

void game_toggle_help(Game *g)
{
    g->show_help = !g->show_help;
}

void game_cycle_theme(Game *g)
{
    g->settings.theme = (g->settings.theme + 1) % GAME_THEME_COUNT;
    game_save_settings(g);
}

void game_toggle_sound(Game *g)
{
    g->settings.sound = !g->settings.sound;
    game_save_settings(g);
}

void game_toggle_fps(Game *g)
{
    g->settings.show_fps = !g->settings.show_fps;
    game_save_settings(g);
}

static double play_mid(const Game *g)
{
    return g->world_mid;
}

static void init_stars(Game *g)
{
    int i;

    g->star_count = g->width * 4;
    if (g->star_count > GAME_MAX_STARS)
        g->star_count = GAME_MAX_STARS;

    for (i = 0; i < g->star_count; i++) {
        g->stars[i].x = (double)rng_range(0, (int)GAME_STAR_SPAN);
        g->stars[i].y = rng_range(1, g->play_height - 1);
        g->stars[i].ch = (rng_range(0, 6) == 0) ? '+' : '.';
        g->stars[i].factor = (i % 3 == 0) ? 0.20 : (i % 3 == 1) ? 0.45 : 0.75;
    }
}

void game_init(Game *g, int width, int height)
{
    g->width = width;
    g->height = height;
    g->play_height = height - 2;
    g->world_mid = (double)g->play_height / 2.0;
    g->difficulty = DIFFICULTY_NORMAL;

    init_stars(g);
    game_load_settings(g);
    game_load_scores(g);
    game_load_stats(g);

    game_reset(g);
    g->state = GAME_MENU;
}

void game_reset(Game *g)
{
    const DifficultyConfig *config = &DIFFICULTIES[g->difficulty];
    int i;

    g->player_x = GAME_EARTH_RADIUS + 18.0;
    g->player_y = g->world_mid;
    g->player_vx = 0.0;
    g->player_vy = 0.0;
    g->camera_x = g->player_x - g->width * 0.35;
    g->fuel_max = config->fuel;
    g->fuel = config->fuel;
    g->hull = GAME_HULL_MAX;
    g->mission_time = 0.0;
    g->invuln = 0.0;
    g->shake = 0.0;
    g->spawn_timer = 3.0;
    g->pickup_timer = 5.0;
    g->shield = 0.0;
    g->fire_cooldown = 0.0;
    g->weapon_level = 1;
    g->ufo_timer = 9.0;
    g->sector = 0;
    g->gravity_scale = config->gravity;
    g->spawn_interval = config->spawn_interval;
    g->score = 0;
    g->new_best = 0;
    g->show_help = 0;

    for (i = 0; i < 4; i++)
        g->thrust[i] = 0.0;

    for (i = 0; i < GAME_MAX_ASTEROIDS; i++)
        g->asteroids[i].alive = 0;

    for (i = 0; i < GAME_MAX_PARTICLES; i++)
        g->particles[i].life = 0.0;

    for (i = 0; i < GAME_MAX_PICKUPS; i++)
        g->pickups[i].alive = 0;

    for (i = 0; i < GAME_MAX_LASERS; i++)
        g->lasers[i].alive = 0;

    for (i = 0; i < GAME_MAX_UFOS; i++)
        g->ufos[i].alive = 0;

    for (i = 0; i < GAME_MAX_SHOTS; i++)
        g->shots[i].alive = 0;
}

void game_set_difficulty(Game *g, Difficulty difficulty)
{
    if (difficulty >= 0 && difficulty < DIFFICULTY_COUNT)
        g->difficulty = difficulty;
}

static void record_run(Game *g, int won)
{
    if (won) {
        int elapsed = (int)g->mission_time;

        g->stats.wins++;
        if (g->stats.best_time == 0 || elapsed < g->stats.best_time)
            g->stats.best_time = elapsed;
    } else {
        g->stats.deaths++;
    }

    g->stats.total_distance += (int)g->player_x;
    g->stats.total_fuel_used += (int)(g->fuel_max - g->fuel);
    game_save_stats(g);
}

void game_start(Game *g)
{
    game_reset(g);
    g->stats.runs++;
    game_save_stats(g);
    g->state = GAME_PLAYING;
}

void game_pause(Game *g)
{
    if (g->state == GAME_PLAYING)
        g->state = GAME_PAUSED;
}

void game_resume(Game *g)
{
    if (g->state == GAME_PAUSED)
        g->state = GAME_PLAYING;
}

void game_to_menu(Game *g)
{
    g->state = GAME_MENU;
}

void game_thrust(Game *g, int up, int down, int left, int right)
{
    if (g->state != GAME_PLAYING)
        return;

    if (up)
        g->thrust[0] = THRUST_MEMORY;
    if (down)
        g->thrust[1] = THRUST_MEMORY;
    if (left)
        g->thrust[2] = THRUST_MEMORY;
    if (right)
        g->thrust[3] = THRUST_MEMORY;
}

void game_fire(Game *g)
{
    int i;
    int slot = -1;

    if (g->state != GAME_PLAYING || g->show_help)
        return;
    if (g->fire_cooldown > 0.0)
        return;

    for (i = 0; i < GAME_MAX_LASERS; i++) {
        if (!g->lasers[i].alive) {
            slot = i;
            break;
        }
    }

    if (slot < 0)
        return;

    g->lasers[slot].x = g->player_x + 5.0;
    g->lasers[slot].y = g->player_y;
    g->lasers[slot].vx = 70.0;
    g->lasers[slot].vy = 0.0;
    g->lasers[slot].life = 1.4;
    g->lasers[slot].alive = 1;

    g->fire_cooldown = 0.18;
    g->stats.lasers_fired++;
}

static void spawn_particle(Game *g, double x, double y, double vx, double vy,
                           double life, char ch, unsigned char color)
{
    int i;

    for (i = 0; i < GAME_MAX_PARTICLES; i++) {
        if (g->particles[i].life <= 0.0) {
            g->particles[i].x = x;
            g->particles[i].y = y;
            g->particles[i].vx = vx;
            g->particles[i].vy = vy;
            g->particles[i].life = life;
            g->particles[i].ch = ch;
            g->particles[i].color = color;
            return;
        }
    }
}

static void update_particles(Game *g)
{
    int i;

    for (i = 0; i < GAME_MAX_PARTICLES; i++) {
        Particle *p = &g->particles[i];

        if (p->life <= 0.0)
            continue;

        p->life -= DT;
        p->x += p->vx * DT;
        p->y += p->vy * DT;
        p->vx *= 0.985;
        p->vy *= 0.985;
    }
}

static void explode_rocket(Game *g)
{
    static const char sparks[] = "*+.'";
    int i;

    for (i = 0; i < 44; i++) {
        double angle = rng_double() * 6.283185307179586;
        double speed = 4.0 + rng_double() * 28.0;

        spawn_particle(g, g->player_x, g->player_y,
                       cos(angle) * speed, sin(angle) * speed,
                       0.6 + rng_double() * 1.2,
                       sparks[rng_range(0, 4)],
                       (unsigned char)((i % 3 == 0) ? 91 : (i % 2 ? 93 : 33)));
    }

    g->shake = 0.8;
}

static void add_planet_gravity(const Game *g, double cx, double cy,
                               double radius, double *ax, double *ay)
{
    double dx = cx - g->player_x;
    double dy = cy - g->player_y;
    double dist2 = dx * dx + dy * dy;
    double min_dist = radius + 1.0;
    double dist;
    double pull;

    if (dist2 < min_dist * min_dist)
        dist2 = min_dist * min_dist;

    dist = sqrt(dist2);
    pull = GRAVITY_MU * g->gravity_scale / dist2;

    *ax += dx / dist * pull;
    *ay += dy / dist * pull;
}

static void bounce_off_earth(Game *g)
{
    double radius = GAME_EARTH_RADIUS + 1.5;
    double dx = g->player_x;
    double dy = g->player_y - play_mid(g);
    double dist = sqrt(dx * dx + dy * dy);
    double nx, ny, vn;

    if (dist >= radius || dist < 0.001)
        return;

    nx = dx / dist;
    ny = dy / dist;

    g->player_x = nx * radius;
    g->player_y = play_mid(g) + ny * radius;

    vn = g->player_vx * nx + g->player_vy * ny;
    if (vn < 0.0) {
        g->player_vx -= 2.0 * vn * nx;
        g->player_vy -= 2.0 * vn * ny;
    }

    g->player_vx *= 0.45;
    g->player_vy *= 0.45;
}

static void spawn_pickup(Game *g);

static void spawn_ufo(Game *g)
{
    int i;
    int slot = -1;

    if (g->player_x < 300.0)
        return;

    for (i = 0; i < GAME_MAX_UFOS; i++) {
        if (!g->ufos[i].alive) {
            slot = i;
            break;
        }
    }

    if (slot < 0)
        return;

    g->ufos[slot].x = g->camera_x + g->width + 15.0;
    g->ufos[slot].y = 4.0 + rng_double() * (double)(g->play_height - 9);
    g->ufos[slot].vx = -5.0 - rng_double() * 3.0;
    g->ufos[slot].vy = 0.0;
    g->ufos[slot].fire_timer = 1.6 + rng_double() * 1.5;
    g->ufos[slot].wobble = rng_double() * 6.283185307179586;
    g->ufos[slot].hp = 2 + g->sector / 2;
    g->ufos[slot].alive = 1;
}

static void spawn_enemy_shot(Game *g, double x, double y, double tx, double ty)
{
    int i;
    double dx;
    double dy;
    double len;
    double speed = 16.0 + (double)g->sector * 2.0;

    for (i = 0; i < GAME_MAX_SHOTS; i++) {
        if (!g->shots[i].alive) {
            dx = tx - x;
            dy = ty - y;
            len = sqrt(dx * dx + dy * dy);
            if (len < 0.001)
                return;

            g->shots[i].x = x;
            g->shots[i].y = y;
            g->shots[i].vx = dx / len * speed;
            g->shots[i].vy = dy / len * speed;
            g->shots[i].life = 4.0;
            g->shots[i].alive = 1;
            return;
        }
    }
}

static void update_ufos(Game *g)
{
    int i;

    g->ufo_timer -= DT;
    if (g->ufo_timer <= 0.0) {
        spawn_ufo(g);
        g->ufo_timer = 11.0 + rng_double() * 7.0 - (double)g->sector * 1.5;
        if (g->ufo_timer < 5.0)
            g->ufo_timer = 5.0;
    }

    for (i = 0; i < GAME_MAX_UFOS; i++) {
        Ufo *u = &g->ufos[i];

        if (!u->alive)
            continue;

        u->x += u->vx * DT;
        u->wobble += DT * 2.4;
        u->y += sin(u->wobble) * 5.0 * DT;

        if (u->y < 2.0)
            u->y = 2.0;
        if (u->y > g->play_height - 3.0)
            u->y = g->play_height - 3.0;

        u->fire_timer -= DT;
        if (u->fire_timer <= 0.0) {
            spawn_enemy_shot(g, u->x - 3.0, u->y,
                             g->player_x, g->player_y);
            u->fire_timer = 2.4 - (double)g->sector * 0.2 +
                            rng_double() * 0.8;
            if (u->fire_timer < 0.9)
                u->fire_timer = 0.9;
        }

        if (u->x < g->camera_x - 20.0) {
            u->alive = 0;
            continue;
        }
    }
}

static void update_shots(Game *g)
{
    int i;

    for (i = 0; i < GAME_MAX_SHOTS; i++) {
        EnemyShot *shot = &g->shots[i];

        if (!shot->alive)
            continue;

        shot->x += shot->vx * DT;
        shot->y += shot->vy * DT;
        shot->life -= DT;

        if (shot->life <= 0.0 || shot->y < -2.0 ||
            shot->y > g->play_height + 2.0 ||
            shot->x < g->camera_x - 40.0) {
            shot->alive = 0;
            continue;
        }

        if (g->invuln <= 0.0 &&
            util_distance(shot->x, shot->y, g->player_x, g->player_y) < 1.7) {
            double damage = 14.0;
            int j;

            shot->alive = 0;
            g->invuln = 0.8;
            g->shake = 0.45;

            if (g->shield > 0.0) {
                double absorbed = g->shield < damage ? g->shield : damage;

                g->shield -= absorbed;
                damage -= absorbed;
            }

            g->hull -= damage;

            for (j = 0; j < 8; j++) {
                double angle = rng_double() * 6.283185307179586;
                double speed = 2.0 + rng_double() * 7.0;

                spawn_particle(g, g->player_x, g->player_y,
                               cos(angle) * speed, sin(angle) * speed,
                               0.25 + rng_double() * 0.35, '*', 95);
            }

            if (g->hull <= 0.0) {
                g->hull = 0.0;
                g->shield = 0.0;
                g->score = 0;
                explode_rocket(g);
                record_run(g, 0);
                g->state = GAME_DEAD;
            }
        }
    }
}

static void update_lasers(Game *g)
{
    int i;

    for (i = 0; i < GAME_MAX_LASERS; i++) {
        Laser *l = &g->lasers[i];

        if (!l->alive)
            continue;

        l->x += l->vx * DT;
        l->y += l->vy * DT;
        l->life -= DT;

        if (l->life <= 0.0 || l->x > g->camera_x + g->width + 20.0) {
            l->alive = 0;
            continue;
        }

        {
            int a;

            for (a = 0; a < GAME_MAX_ASTEROIDS; a++) {
                Asteroid *rock = &g->asteroids[a];
                double hit_radius;

                if (!rock->alive)
                    continue;

                hit_radius = (double)rock->radius * 1.2 + 0.8;
                if (util_distance(l->x, l->y, rock->x, rock->y) > hit_radius)
                    continue;

                l->alive = 0;
                rock->hp--;

                if (rock->hp <= 0) {
                    int j;

                    rock->alive = 0;
                    g->stats.asteroids_destroyed++;
                    g->score += rock->radius == 2 ? 100 : 50;
                    g->shake = 0.35;

                    for (j = 0; j < 14; j++) {
                        double angle = rng_double() * 6.283185307179586;
                        double speed = 4.0 + rng_double() * 10.0;

                        spawn_particle(g, rock->x, rock->y,
                                       cos(angle) * speed, sin(angle) * speed,
                                       0.3 + rng_double() * 0.5,
                                       (rng_range(0, 2) == 0) ? '*' : 'o',
                                       (unsigned char)(rng_range(0, 2) ? 33 : 93));
                    }
                } else {
                    spawn_particle(g, l->x, l->y, -2.0, 0.0, 0.2, '*', 97);
                }

                break;
            }
        }
    }
}

static void update_pickups(Game *g)
{
    int i;

    g->pickup_timer -= DT;
    if (g->pickup_timer <= 0.0) {
        spawn_pickup(g);
        g->pickup_timer = 6.0 + rng_double() * 5.0;
    }

    for (i = 0; i < GAME_MAX_PICKUPS; i++) {
        Pickup *p = &g->pickups[i];

        if (!p->alive)
            continue;

        p->x -= 2.0 * DT;
        p->bob += DT * 4.0;

        if (p->x < g->camera_x - 30.0) {
            p->alive = 0;
            continue;
        }

        if (util_distance(p->x, p->y, g->player_x, g->player_y) < 2.4) {
            unsigned char color;
            char ch;
            int j;

            switch (p->type) {
            case PICKUP_FUEL:
                g->fuel = util_clamp(g->fuel + 30.0, 0.0, g->fuel_max);
                color = 96;
                ch = 'F';
                break;
            case PICKUP_REPAIR:
                g->hull = util_clamp(g->hull + 25.0, 0.0, GAME_HULL_MAX);
                color = 92;
                ch = '+';
                break;
            default:
                g->shield = util_clamp(g->shield + 100.0, 0.0, 100.0);
                color = 94;
                ch = 'O';
                break;
            }

            for (j = 0; j < 8; j++) {
                double angle = rng_double() * 6.283185307179586;
                double speed = 2.0 + rng_double() * 6.0;

                spawn_particle(g, p->x, p->y,
                               cos(angle) * speed, sin(angle) * speed,
                               0.3 + rng_double() * 0.4, ch, color);
            }

            g->pickups[i].alive = 0;
            g->stats.pickups_collected++;
        }
    }
}

static void update_player(Game *g)
{
    double ax = 0.0;
    double ay = 0.0;
    double gx = 0.0;
    double gy = 0.0;
    double len;
    double speed;
    double target;
    int i;

    if (g->thrust[0] > 0.0)
        ay -= 1.0;
    if (g->thrust[1] > 0.0)
        ay += 1.0;
    if (g->thrust[2] > 0.0)
        ax -= 1.0;
    if (g->thrust[3] > 0.0)
        ax += 1.0;

    len = sqrt(ax * ax + ay * ay);
    if (len > 0.0) {
        ax /= len;
        ay /= len;
    }

    add_planet_gravity(g, 0.0, play_mid(g), GAME_EARTH_RADIUS, &gx, &gy);
    add_planet_gravity(g, GAME_WORLD_LENGTH, play_mid(g), GAME_MARS_RADIUS, &gx, &gy);

    if (len > 0.0 && g->fuel <= 0.0) {
        ax = 0.0;
        ay = 0.0;
        len = 0.0;
    }

    if (len > 0.0) {
        double rx = g->player_x - ax * 2.0;
        double ry = g->player_y - ay * 2.0;

        g->fuel = util_clamp(g->fuel - FUEL_BURN * DT, 0.0, g->fuel_max);

        spawn_particle(g, rx + (rng_double() - 0.5) * 0.8,
                       ry + (rng_double() - 0.5) * 0.8,
                       -ax * 14.0 + (rng_double() - 0.5) * 9.0,
                       -ay * 14.0 + (rng_double() - 0.5) * 9.0,
                       0.25 + rng_double() * 0.25,
                       (rng_range(0, 4) == 0) ? '*' : '.',
                       (unsigned char)(rng_range(0, 2) ? 33 : 93));
    }

    g->player_vx += (ax * PLAYER_ACCEL + gx) * DT;
    g->player_vy += (ay * PLAYER_ACCEL + gy) * DT;

    g->player_vx -= g->player_vx * PLAYER_DRAG * DT;
    g->player_vy -= g->player_vy * PLAYER_DRAG * DT;

    speed = sqrt(g->player_vx * g->player_vx + g->player_vy * g->player_vy);
    if (speed > PLAYER_MAX_SPEED) {
        g->player_vx = g->player_vx / speed * PLAYER_MAX_SPEED;
        g->player_vy = g->player_vy / speed * PLAYER_MAX_SPEED;
    }

    g->player_x += g->player_vx * DT;
    g->player_y += g->player_vy * DT;

    bounce_off_earth(g);

    if (g->player_x > GAME_WORLD_LENGTH + 40.0) {
        g->player_x = GAME_WORLD_LENGTH + 40.0;
        if (g->player_vx > 0.0)
            g->player_vx = -g->player_vx * 0.4;
    }
    if (g->player_y < 2.0) {
        g->player_y = 2.0;
        if (g->player_vy < 0.0)
            g->player_vy = -g->player_vy * 0.5;
    }
    if (g->player_y > g->play_height - 2.0) {
        g->player_y = g->play_height - 2.0;
        if (g->player_vy > 0.0)
            g->player_vy = -g->player_vy * 0.5;
    }

    for (i = 0; i < 4; i++) {
        if (g->thrust[i] > 0.0)
            g->thrust[i] -= DT;
    }

    if (g->invuln > 0.0)
        g->invuln -= DT;
    if (g->fire_cooldown > 0.0)
        g->fire_cooldown -= DT;
    if (g->fire_cooldown < 0.0)
        g->fire_cooldown = 0.0;
    if (g->shake > 0.0)
        g->shake -= DT * 2.0;

    target = g->player_x - g->width * 0.35;
    if (target < -20.0)
        target = -20.0;
    if (target > GAME_WORLD_LENGTH - g->width + 20.0)
        target = GAME_WORLD_LENGTH - g->width + 20.0;

    g->camera_x += (target - g->camera_x) * 0.15;

    {
        if (util_distance(g->player_x, g->player_y, GAME_WORLD_LENGTH, play_mid(g)) <
            GAME_MARS_ORBIT) {
            double bonus = 240.0 - g->mission_time;

            if (bonus < 0.0)
                bonus = 0.0;

            g->score += (int)(g->hull * 8.0 + g->fuel * 6.0 + bonus * 3.0);
            if (g->score > g->best[g->difficulty]) {
                g->best[g->difficulty] = g->score;
                g->new_best = 1;
                game_save_scores(g);
            }
            record_run(g, 1);
            g->state = GAME_WON;
        }
    }
}

static void spawn_asteroid(Game *g)
{
    int i;
    int slot = -1;
    double x;
    double y;
    double dx;
    double dy;
    int radius;

    for (i = 0; i < GAME_MAX_ASTEROIDS; i++) {
        if (!g->asteroids[i].alive) {
            slot = i;
            break;
        }
    }

    if (slot < 0)
        return;

    x = g->camera_x + g->width + 40.0 + rng_double() * 160.0;
    y = 3.0 + rng_double() * (double)(g->play_height - 6);
    radius = (rng_range(0, 100) < 25) ? 2 : 1;

    dx = x - 0.0;
    dy = y - play_mid(g);
    if (sqrt(dx * dx + dy * dy) < GAME_EARTH_RADIUS + radius + 10.0)
        return;

    dx = x - GAME_WORLD_LENGTH;
    dy = y - play_mid(g);
    if (sqrt(dx * dx + dy * dy) < GAME_MARS_RADIUS + radius + 10.0)
        return;

    g->asteroids[slot].x = x;
    g->asteroids[slot].y = y;
    g->asteroids[slot].vx = -(3.0 + rng_double() * 8.0);
    g->asteroids[slot].vy = (rng_double() - 0.5) * 4.0;
    g->asteroids[slot].radius = radius;
    g->asteroids[slot].hp = (radius == 2) ? 3 : 1;
    g->asteroids[slot].alive = 1;
}

static void spawn_pickup(Game *g)
{
    int i;
    int slot = -1;
    double x;
    double y;
    int roll;

    for (i = 0; i < GAME_MAX_PICKUPS; i++) {
        if (!g->pickups[i].alive) {
            slot = i;
            break;
        }
    }

    if (slot < 0)
        return;

    x = g->camera_x + g->width + 20.0 + rng_double() * 220.0;
    y = 3.0 + rng_double() * (double)(g->play_height - 6);

    if (util_distance(x, y, 0.0, play_mid(g)) < GAME_EARTH_RADIUS + 8.0)
        return;
    if (util_distance(x, y, GAME_WORLD_LENGTH, play_mid(g)) < GAME_MARS_RADIUS + 8.0)
        return;

    roll = rng_range(0, 100);
    g->pickups[slot].type = roll < 50 ? PICKUP_FUEL
                           : roll < 75 ? PICKUP_REPAIR
                                       : PICKUP_SHIELD;
    g->pickups[slot].x = x;
    g->pickups[slot].y = y;
    g->pickups[slot].bob = rng_double() * 6.28;
    g->pickups[slot].alive = 1;
}

static void update_asteroids(Game *g)
{
    int i;

    g->spawn_timer -= DT;
    if (g->spawn_timer <= 0.0) {
        spawn_asteroid(g);
        g->spawn_timer = g->spawn_interval * (0.7 + rng_double() * 0.6);
    }

    for (i = 0; i < GAME_MAX_ASTEROIDS; i++) {
        Asteroid *a = &g->asteroids[i];
        double dx;
        double dy;
        double hit_radius;

        if (!a->alive)
            continue;

        a->x += a->vx * DT;
        a->y += a->vy * DT;

        if (a->x < g->camera_x - 80.0 || a->y < -4.0 || a->y > g->play_height + 4.0) {
            a->alive = 0;
            continue;
        }

        if (g->invuln > 0.0)
            continue;

        dx = a->x - g->player_x;
        dy = a->y - g->player_y;
        hit_radius = a->radius + 1.2;

        if (dx * dx + dy * dy < hit_radius * hit_radius) {
            int j;
            double damage = (a->radius == 2) ? 40.0 : 24.0;

            a->alive = 0;
            g->invuln = 1.2;
            g->shake = 0.5;

            if (g->shield > 0.0) {
                double absorbed = g->shield < damage ? g->shield : damage;

                g->shield -= absorbed;
                damage -= absorbed;
            }

            g->hull -= damage;

            for (j = 0; j < 10; j++) {
                double angle = rng_double() * 6.283185307179586;
                double speed = 3.0 + rng_double() * 8.0;

                spawn_particle(g, g->player_x, g->player_y,
                               cos(angle) * speed, sin(angle) * speed,
                               0.3 + rng_double() * 0.4, '*', 93);
            }

            if (g->hull <= 0.0) {
                g->hull = 0.0;
                g->shield = 0.0;
                g->score = 0;
                explode_rocket(g);
                record_run(g, 0);
                g->state = GAME_DEAD;
            }
        }
    }
}

void game_update(Game *g)
{
    g->frame++;

    if (g->show_help)
        return;

    if (g->state == GAME_PLAYING) {
        g->mission_time += DT;
        update_player(g);
        if (g->state == GAME_PLAYING)
            update_asteroids(g);
        update_ufos(g);
        update_shots(g);
        update_lasers(g);
        update_pickups(g);
        update_particles(g);
    } else if (g->state == GAME_WON || g->state == GAME_DEAD) {
        update_particles(g);
        if (g->shake > 0.0)
            g->shake -= DT * 2.0;
    } else if (g->state == GAME_MENU) {
        g->camera_x += 14.0 * DT;
        if (g->camera_x > GAME_WORLD_LENGTH)
            g->camera_x -= GAME_WORLD_LENGTH;
    }
}
