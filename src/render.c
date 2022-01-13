#include "render.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "term.h"
#include "util.h"

#define C_DEFAULT  0
#define C_DIM      90
#define C_GRAY     37
#define C_WHITE    97
#define C_CYAN     96
#define C_BLUE     94
#define C_GREEN    92
#define C_RED      91
#define C_DARK_RED 31
#define C_YELLOW   93
#define C_ORANGE   33
#define C_MAGENTA  95

typedef struct {
    char *cells;
    unsigned char *colors;
    int width;
    int height;
} Canvas;

typedef struct {
    char body;
    char detail;
    char edge;
    unsigned char body_color;
    unsigned char detail_color;
    unsigned char edge_color;
    int seed;
} DiskStyle;

static void put_cell(Canvas *c, int x, int y, char ch, unsigned char color)
{
    if (x >= 0 && x < c->width && y >= 0 && y < c->height) {
        c->cells[y * c->width + x] = ch;
        c->colors[y * c->width + x] = color;
    }
}

static void draw_text(Canvas *c, int x, int y, const char *text, unsigned char color)
{
    size_t i;

    for (i = 0; text[i] != '\0'; i++)
        put_cell(c, x + (int)i, y, text[i], color);
}

static void draw_text_center(Canvas *c, int y, const char *text, unsigned char color)
{
    draw_text(c, (c->width - (int)strlen(text)) / 2, y, text, color);
}

static void draw_box(Canvas *c, int bx, int by, int bw, int bh, unsigned char color)
{
    int x, y;

    for (y = 0; y < bh; y++) {
        for (x = 0; x < bw; x++) {
            char ch = ' ';

            if (x == 0 || x == bw - 1 || y == 0 || y == bh - 1) {
                if ((x == 0 || x == bw - 1) && (y == 0 || y == bh - 1))
                    ch = '+';
                else if (y == 0 || y == bh - 1)
                    ch = '-';
                else
                    ch = '|';
            }

            put_cell(c, bx + x, by + y, ch, color);
        }
    }
}

static void draw_stars(const Game *g, Canvas *c)
{
    int i;

    for (i = 0; i < g->star_count; i++) {
        double sx = fmod(g->stars[i].x - g->camera_x * g->stars[i].factor,
                         GAME_STAR_SPAN);
        unsigned char color;
        int x;
        int y = g->stars[i].y;

        if (sx < 0.0)
            sx += GAME_STAR_SPAN;

        x = (int)sx;
        if (x < 0 || x >= c->width || y < 0 || y >= g->play_height)
            continue;

        if (g->stars[i].factor < 0.3)
            color = C_DIM;
        else if (g->stars[i].factor < 0.6)
            color = C_GRAY;
        else
            color = C_WHITE;

        if ((g->frame / 7 + (unsigned long)i) % 29 == 0)
            color = C_CYAN;

        put_cell(c, x, y, g->stars[i].ch, color);
    }
}

static void draw_disk(Canvas *c, double center_x, double center_y, double radius,
                      const DiskStyle *style)
{
    int x0 = (int)(center_x - radius - 1.0);
    int x1 = (int)(center_x + radius + 1.0);
    int y0 = (int)(center_y - radius - 1.0);
    int y1 = (int)(center_y + radius + 1.0);
    int x, y;

    for (y = y0; y <= y1; y++) {
        for (x = x0; x <= x1; x++) {
            double dx = (x + 0.5) - center_x;
            double dy = ((y + 0.5) - center_y) * 2.0;
            double dist = sqrt(dx * dx + dy * dy);
            char ch;
            unsigned char color;

            if (dist > radius)
                continue;
            if (x < 0 || x >= c->width || y < 0 || y >= c->height)
                continue;

            if (dist > radius - 0.9) {
                ch = style->edge;
                color = style->edge_color;
            } else if ((x * 31 + y * 17 + style->seed) % 11 == 0) {
                ch = style->detail;
                color = style->detail_color;
            } else {
                ch = style->body;
                color = style->body_color;
            }

            put_cell(c, x, y, ch, color);
        }
    }
}

static void draw_label(Canvas *c, double center_x, int y, const char *text,
                       unsigned char color)
{
    int x = (int)(center_x - (double)strlen(text) / 2.0);

    draw_text(c, x, y, text, color);
}

static void draw_orbit(Canvas *c, double center_x, double center_y, double radius,
                       char ch, unsigned char color)
{
    const double pi = 3.14159265358979;
    int steps = (int)(radius * 8.0);
    int i;

    if (steps < 32)
        steps = 32;

    for (i = 0; i < steps; i++) {
        double angle = 2.0 * pi * (double)i / (double)steps;

        if (i % 2 != 0)
            continue;

        put_cell(c,
                 (int)(center_x + cos(angle) * radius + 0.5),
                 (int)(center_y + sin(angle) * radius / 2.0 + 0.5),
                 ch, color);
    }
}

static void draw_rocket(Canvas *c, int cx, int cy, double ax, double ay, char flame)
{
    static const char *sprite[] = {
        "   __   ",
        "  /  \\_ ",
        " |     >",
        "  \\__/  "
    };
    int row, col;

    for (row = 0; row < 4; row++) {
        for (col = 0; col < 8; col++) {
            char ch = sprite[row][col];

            if (ch != ' ')
                put_cell(c, cx - 4 + col, cy - 2 + row, ch,
                         ch == '>' ? C_CYAN : C_WHITE);
        }
    }

    if (ax > 0.1)
        put_cell(c, cx - 5, cy, flame, C_ORANGE);
    if (ax < -0.1)
        put_cell(c, cx + 5, cy, flame, C_ORANGE);
    if (ay < -0.1)
        put_cell(c, cx, cy + 2, flame, C_ORANGE);
    if (ay > 0.1)
        put_cell(c, cx, cy - 3, flame, C_ORANGE);
}

static void draw_menu(const Game *g, Canvas *c)
{
    int mid = c->height / 2;
    int i;

    draw_text_center(c, mid - 10, "S U P E R   R O C K E T", C_CYAN);
    draw_text_center(c, mid - 9, "-------------------------", C_BLUE);
    draw_text_center(c, mid - 7, "E A R T H   ->   M A R S", C_WHITE);

    for (i = 0; i < DIFFICULTY_COUNT; i++) {
        char label[80];
        int selected = (i == (int)g->difficulty);

        snprintf(label, sizeof(label), "%s [%d] %-6s   best %d",
                 selected ? ">" : " ", i + 1,
                 game_difficulty_name((Difficulty)i), g->best[i]);
        draw_text_center(c, mid - 4 + i, label, selected ? C_YELLOW : C_GRAY);
    }

    draw_text_center(c, mid, "[ ENTER ]  lift off        [ Q ]  quit", C_WHITE);
    draw_text_center(c, mid + 3, "Fly with ARROWS or WASD.", C_GRAY);
    draw_text_center(c, mid + 4, "Thrust burns fuel. Dodge the asteroids.", C_GRAY);
    draw_text_center(c, mid + 5, "Reach Mars orbit to complete the mission.", C_GRAY);
    draw_text_center(c, mid + 7, "super-rocket " GAME_VERSION, C_DIM);
}

static void draw_overlay(const Game *g, Canvas *c)
{
    if (g->state == GAME_PAUSED) {
        int w = 34;
        int h = 5;
        int x = (c->width - w) / 2;
        int y = g->play_height / 2 - h / 2;

        draw_box(c, x, y, w, h, C_CYAN);
        draw_text_center(c, y + 1, "PAUSED", C_YELLOW);
        draw_text_center(c, y + 3, "P resume   R restart   Q menu", C_WHITE);
    } else if (g->state == GAME_WON) {
        char time_text[16];
        char score_text[32];
        int w = 38;
        int h = 10;
        int x = (c->width - w) / 2;
        int y = g->play_height / 2 - h / 2;

        util_format_time(time_text, sizeof(time_text), g->mission_time);
        snprintf(score_text, sizeof(score_text), "SCORE  %d", g->score);

        draw_box(c, x, y, w, h, C_GREEN);
        draw_text_center(c, y + 1, "MARS ORBIT ACHIEVED", C_GREEN);
        draw_text_center(c, y + 3, time_text, C_WHITE);
        draw_text_center(c, y + 4, score_text, C_WHITE);
        if (g->new_best)
            draw_text_center(c, y + 5, "NEW BEST!", C_YELLOW);
        draw_text_center(c, y + 8, "R fly again    Q menu", C_CYAN);
    } else if (g->state == GAME_DEAD) {
        int w = 34;
        int h = 7;
        int x = (c->width - w) / 2;
        int y = g->play_height / 2 - h / 2;

        draw_box(c, x, y, w, h, C_RED);
        draw_text_center(c, y + 1, "MISSION FAILED", C_RED);
        draw_text_center(c, y + 3, "the rocket broke apart", C_GRAY);
        draw_text_center(c, y + 5, "R retry    Q menu", C_CYAN);
    }
}

static void make_bar(char *buf, double value, double max, int width)
{
    int filled = (int)(value / max * (double)width + 0.5);
    int i;

    for (i = 0; i < width; i++)
        buf[i] = (i < filled) ? '#' : '-';
    buf[width] = '\0';
}

static unsigned char gauge_color(double ratio)
{
    if (ratio > 0.6)
        return C_GREEN;
    if (ratio > 0.3)
        return C_YELLOW;
    return C_RED;
}

static void draw_hud(const Game *g, Canvas *c)
{
    char hull_bar[16];
    char fuel_bar[16];
    char time_text[16];
    char line[160];
    char *fuel_pos;
    double speed;
    double hull_ratio = g->hull / GAME_HULL_MAX;
    double fuel_ratio = g->fuel / g->fuel_max;
    int row = g->height - 2;
    int x;

    if (g->state == GAME_MENU)
        return;

    make_bar(hull_bar, g->hull, GAME_HULL_MAX, 10);
    make_bar(fuel_bar, g->fuel, g->fuel_max, 10);
    speed = sqrt(g->player_vx * g->player_vx + g->player_vy * g->player_vy);

    snprintf(line, sizeof(line),
             "HULL [%s] %3.0f%%   FUEL [%s] %3.0f%%   VEL %4.1f",
             hull_bar, hull_ratio * 100.0, fuel_bar, fuel_ratio * 100.0, speed);
    draw_text(c, 0, row, line, C_WHITE);

    draw_text(c, 0, row, "HULL", C_CYAN);

    for (x = 0; x < 10; x++)
        put_cell(c, 6 + x, row, hull_bar[x],
                 hull_bar[x] == '#' ? gauge_color(hull_ratio) : C_DIM);

    fuel_pos = strstr(line, "FUEL [");
    if (fuel_pos != NULL) {
        int fx = (int)(fuel_pos - line);

        draw_text(c, fx, row, "FUEL", C_CYAN);
        for (x = 0; x < 10; x++)
            put_cell(c, fx + 6 + x, row, fuel_bar[x],
                     fuel_bar[x] == '#' ? C_CYAN : C_DIM);
    }

    util_format_time(time_text, sizeof(time_text), g->mission_time);
    {
        int progress = (int)(g->player_x / GAME_WORLD_LENGTH * 100.0);

        if (progress < 0)
            progress = 0;
        if (progress > 100)
            progress = 100;

        snprintf(line, sizeof(line),
                 "DIST %4.0f / %.0f   %3d%%   TIME %s   %s   P pause   Q menu",
                 g->player_x, GAME_WORLD_LENGTH, progress, time_text,
                 game_difficulty_name(g->difficulty));
    }
    draw_text(c, 0, g->height - 1, line, C_GRAY);
    draw_text(c, 0, g->height - 1, "DIST", C_BLUE);
}

void render_frame(const Game *g)
{
    Canvas c;
    int shx = 0;
    int shy = 0;

    c.width = g->width;
    c.height = g->height;
    c.cells = malloc((size_t)c.width * (size_t)c.height);
    c.colors = malloc((size_t)c.width * (size_t)c.height);
    if (c.cells == NULL || c.colors == NULL) {
        free(c.cells);
        free(c.colors);
        return;
    }

    memset(c.cells, ' ', (size_t)c.width * (size_t)c.height);
    memset(c.colors, C_DEFAULT, (size_t)c.width * (size_t)c.height);
    draw_stars(g, &c);

    if (g->state != GAME_MENU) {
        DiskStyle earth = { '#', 'o', '~', C_BLUE, C_GREEN, C_CYAN, 3 };
        DiskStyle mars = { '#', '.', '~', C_RED, C_DARK_RED, C_ORANGE, 7 };

        if (g->shake > 0.0) {
            shx = (int)(sin(g->shake * 47.0) * 2.0);
            shy = (int)(cos(g->shake * 61.0) * 1.5);
        }

        draw_disk(&c, 0.0 - g->camera_x + shx, g->world_mid + shy,
                  GAME_EARTH_RADIUS, &earth);
        draw_label(&c, 0.0 - g->camera_x + shx,
                   (int)g->world_mid - (int)GAME_EARTH_RADIUS - 2 + shy,
                   "EARTH", C_CYAN);

        draw_disk(&c, GAME_WORLD_LENGTH - g->camera_x + shx, g->world_mid + shy,
                  GAME_MARS_RADIUS, &mars);
        draw_orbit(&c, GAME_WORLD_LENGTH - g->camera_x + shx, g->world_mid + shy,
                   GAME_MARS_ORBIT, '.', C_DARK_RED);
        draw_label(&c, GAME_WORLD_LENGTH - g->camera_x + shx,
                   (int)g->world_mid - (int)GAME_MARS_RADIUS - 2 + shy,
                   "MARS", C_RED);

        {
            int i;

            for (i = 0; i < GAME_MAX_ASTEROIDS; i++) {
                DiskStyle asteroid;

                if (!g->asteroids[i].alive)
                    continue;

                asteroid.body = 'O';
                asteroid.detail = 'o';
                asteroid.edge = '.';
                asteroid.body_color = (g->asteroids[i].radius == 2) ? C_ORANGE : C_YELLOW;
                asteroid.detail_color = C_GRAY;
                asteroid.edge_color = C_DIM;
                asteroid.seed = i * 13 + 5;

                draw_disk(&c, g->asteroids[i].x - g->camera_x + shx,
                          g->asteroids[i].y + shy,
                          (double)g->asteroids[i].radius * 1.2, &asteroid);
            }
        }

        if (!(g->invuln > 0.0 && ((int)(g->invuln * 12.0) % 2 == 0))) {
            double last_ax = 0.0;
            double last_ay = 0.0;

            if (g->thrust[0] > 0.0)
                last_ay -= 1.0;
            if (g->thrust[1] > 0.0)
                last_ay += 1.0;
            if (g->thrust[2] > 0.0)
                last_ax -= 1.0;
            if (g->thrust[3] > 0.0)
                last_ax += 1.0;

            draw_rocket(&c, (int)(g->player_x - g->camera_x + 0.5) + shx,
                        (int)(g->player_y + 0.5) + shy, last_ax, last_ay,
                        ((g->frame / 2) % 2 == 0) ? '*' : '~');
        }

        {
            int i;

            for (i = 0; i < GAME_MAX_PARTICLES; i++) {
                if (g->particles[i].life <= 0.0)
                    continue;
                put_cell(&c, (int)(g->particles[i].x - g->camera_x + 0.5) + shx,
                         (int)(g->particles[i].y + 0.5) + shy,
                         g->particles[i].ch, g->particles[i].color);
            }
        }
    } else {
        draw_menu(g, &c);
    }

    if (g->state == GAME_PLAYING) {
        if (g->fuel / g->fuel_max < 0.25 && (g->frame / 15) % 2 == 0)
            draw_text_center(&c, 1, "LOW FUEL", C_RED);
        if (g->hull / GAME_HULL_MAX < 0.30 && (g->frame / 15) % 2 == 0)
            draw_text_center(&c, 2, "HULL CRITICAL", C_RED);
    }

    draw_overlay(g, &c);
    draw_hud(g, &c);

    term_draw(c.cells, c.colors, c.width, c.height);

    free(c.cells);
    free(c.colors);
}
