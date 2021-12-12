#include "game.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

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

static const char *score_path(void)
{
    static char path[1024];
    const char *home = getenv("HOME");

    if (home != NULL)
        snprintf(path, sizeof(path), "%s/.super_rocket_scores", home);
    else
        snprintf(path, sizeof(path), ".super_rocket_scores");

    return path;
}

const char *game_difficulty_name(Difficulty difficulty)
{
    if (difficulty < 0 || difficulty >= DIFFICULTY_COUNT)
        return "NORMAL";
    return DIFFICULTY_NAMES[difficulty];
}

void game_load_scores(Game *g)
{
    FILE *file;
    int i;

    for (i = 0; i < DIFFICULTY_COUNT; i++)
        g->best[i] = 0;

    file = fopen(score_path(), "r");
    if (file == NULL)
        return;

    for (i = 0; i < DIFFICULTY_COUNT; i++) {
        if (fscanf(file, "%d", &g->best[i]) != 1)
            g->best[i] = 0;
    }

    fclose(file);
}

void game_save_scores(const Game *g)
{
    FILE *file;
    int i;

    file = fopen(score_path(), "w");
    if (file == NULL)
        return;

    for (i = 0; i < DIFFICULTY_COUNT; i++)
        fprintf(file, "%d\n", g->best[i]);

    fclose(file);
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
        g->stars[i].x = (double)(rand() % (int)GAME_STAR_SPAN);
        g->stars[i].y = rand() % (g->play_height - 2) + 1;
        g->stars[i].ch = (rand() % 6 == 0) ? '+' : '.';
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
    game_load_scores(g);

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
    g->gravity_scale = config->gravity;
    g->spawn_interval = config->spawn_interval;
    g->score = 0;
    g->new_best = 0;

    for (i = 0; i < 4; i++)
        g->thrust[i] = 0.0;

    for (i = 0; i < GAME_MAX_ASTEROIDS; i++)
        g->asteroids[i].alive = 0;

    for (i = 0; i < GAME_MAX_PARTICLES; i++)
        g->particles[i].life = 0.0;
}

void game_set_difficulty(Game *g, Difficulty difficulty)
{
    if (difficulty >= 0 && difficulty < DIFFICULTY_COUNT)
        g->difficulty = difficulty;
}

void game_start(Game *g)
{
    game_reset(g);
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
        double angle = (double)(rand() % 628) / 100.0;
        double speed = 4.0 + (double)(rand() % 140) / 5.0;

        spawn_particle(g, g->player_x, g->player_y,
                       cos(angle) * speed, sin(angle) * speed,
                       0.6 + (double)(rand() % 120) / 100.0,
                       sparks[rand() % 4],
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

        g->fuel -= FUEL_BURN * DT;
        if (g->fuel < 0.0)
            g->fuel = 0.0;

        spawn_particle(g, rx + (double)(rand() % 3 - 1) * 0.4,
                       ry + (double)(rand() % 3 - 1) * 0.4,
                       -ax * 14.0 + (double)(rand() % 9 - 4),
                       -ay * 14.0 + (double)(rand() % 9 - 4),
                       0.25 + (double)(rand() % 25) / 100.0,
                       (rand() % 4 == 0) ? '*' : '.',
                       (unsigned char)((rand() % 2) ? 33 : 93));
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
    if (g->shake > 0.0)
        g->shake -= DT * 2.0;

    target = g->player_x - g->width * 0.35;
    if (target < -20.0)
        target = -20.0;
    if (target > GAME_WORLD_LENGTH - g->width + 20.0)
        target = GAME_WORLD_LENGTH - g->width + 20.0;

    g->camera_x += (target - g->camera_x) * 0.15;

    {
        double dx = g->player_x - GAME_WORLD_LENGTH;
        double dy = g->player_y - play_mid(g);

        if (sqrt(dx * dx + dy * dy) < GAME_MARS_ORBIT) {
            double bonus = 240.0 - g->mission_time;

            if (bonus < 0.0)
                bonus = 0.0;

            g->score = (int)(g->hull * 8.0 + g->fuel * 6.0 + bonus * 3.0);
            if (g->score > g->best[g->difficulty]) {
                g->best[g->difficulty] = g->score;
                g->new_best = 1;
                game_save_scores(g);
            }
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

    x = g->camera_x + g->width + 12.0 + (double)(rand() % 180);
    y = 3.0 + (double)(rand() % (g->play_height - 6));
    radius = (rand() % 100 < 25) ? 2 : 1;

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
    g->asteroids[slot].vx = -(3.0 + (double)(rand() % 8));
    g->asteroids[slot].vy = ((double)(rand() % 100) - 50.0) / 25.0;
    g->asteroids[slot].radius = radius;
    g->asteroids[slot].alive = 1;
}

static void update_asteroids(Game *g)
{
    int i;

    g->spawn_timer -= DT;
    if (g->spawn_timer <= 0.0) {
        spawn_asteroid(g);
        g->spawn_timer = g->spawn_interval * (0.7 + (double)(rand() % 60) / 100.0);
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

            a->alive = 0;
            g->hull -= (a->radius == 2) ? 40.0 : 24.0;
            g->invuln = 1.2;
            g->shake = 0.5;

            for (j = 0; j < 10; j++) {
                double angle = (double)(rand() % 628) / 100.0;
                double speed = 3.0 + (double)(rand() % 80) / 10.0;

                spawn_particle(g, g->player_x, g->player_y,
                               cos(angle) * speed, sin(angle) * speed,
                               0.3 + (double)(rand() % 40) / 100.0, '*', 93);
            }

            if (g->hull <= 0.0) {
                g->hull = 0.0;
                g->score = 0;
                explode_rocket(g);
                g->state = GAME_DEAD;
            }
        }
    }
}

void game_update(Game *g)
{
    g->frame++;

    if (g->state == GAME_PLAYING) {
        g->mission_time += DT;
        update_player(g);
        if (g->state == GAME_PLAYING)
            update_asteroids(g);
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
