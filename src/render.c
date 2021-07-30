#include "render.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "term.h"

typedef struct {
    char *cells;
    int width;
    int height;
} Canvas;

static void put_cell(Canvas *c, int x, int y, char ch)
{
    if (x >= 0 && x < c->width && y >= 0 && y < c->height)
        c->cells[y * c->width + x] = ch;
}

static void draw_text(Canvas *c, int x, int y, const char *text)
{
    size_t i;

    for (i = 0; text[i] != '\0'; i++)
        put_cell(c, x + (int)i, y, text[i]);
}

static void draw_text_center(Canvas *c, int y, const char *text)
{
    draw_text(c, (c->width - (int)strlen(text)) / 2, y, text);
}

static void draw_box(Canvas *c, int bx, int by, int bw, int bh)
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

            put_cell(c, bx + x, by + y, ch);
        }
    }
}

static void draw_stars(const Game *g, Canvas *c)
{
    int i;

    for (i = 0; i < g->star_count; i++) {
        double sx = fmod(g->stars[i].x - g->camera_x * g->stars[i].factor,
                         GAME_STAR_SPAN);
        int x;
        int y = g->stars[i].y;

        if (sx < 0.0)
            sx += GAME_STAR_SPAN;

        x = (int)sx;
        if (x >= 0 && x < c->width && y >= 0 && y < g->play_height)
            c->cells[y * c->width + x] = g->stars[i].ch;
    }
}

static void draw_disk(Canvas *c, double center_x, double center_y, double radius,
                      char body, char detail, int seed)
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

            if (dist > radius)
                continue;
            if (x < 0 || x >= c->width || y < 0 || y >= c->height)
                continue;

            if (dist > radius - 0.9)
                ch = '~';
            else if ((x * 31 + y * 17 + seed) % 11 == 0)
                ch = detail;
            else
                ch = body;

            c->cells[y * c->width + x] = ch;
        }
    }
}

static void draw_label(Canvas *c, double center_x, int y, const char *text)
{
    int x = (int)(center_x - (double)strlen(text) / 2.0);

    draw_text(c, x, y, text);
}

static void draw_rocket(Canvas *c, int cx, int cy, double ax, double ay)
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
                put_cell(c, cx - 4 + col, cy - 2 + row, ch);
        }
    }

    if (ax > 0.1)
        put_cell(c, cx - 5, cy, '*');
    if (ax < -0.1)
        put_cell(c, cx + 5, cy, '*');
    if (ay < -0.1)
        put_cell(c, cx, cy + 2, '*');
    if (ay > 0.1)
        put_cell(c, cx, cy - 3, '*');
}

static void draw_menu(const Game *g, Canvas *c)
{
    int mid = c->height / 2;
    int i;

    draw_text_center(c, mid - 10, "S U P E R   R O C K E T");
    draw_text_center(c, mid - 9, "-------------------------");
    draw_text_center(c, mid - 7, "E A R T H   ->   M A R S");

    for (i = 0; i < DIFFICULTY_COUNT; i++) {
        char label[80];

        snprintf(label, sizeof(label), "%s [%d] %-6s   best %d",
                 (i == (int)g->difficulty) ? ">" : " ", i + 1,
                 game_difficulty_name((Difficulty)i), g->best[i]);
        draw_text_center(c, mid - 4 + i, label);
    }

    draw_text_center(c, mid, "[ ENTER ]  lift off        [ Q ]  quit");
    draw_text_center(c, mid + 3, "Fly with ARROWS or WASD.");
    draw_text_center(c, mid + 4, "Thrust burns fuel. Dodge the asteroids.");
    draw_text_center(c, mid + 5, "Reach Mars orbit to complete the mission.");
}

static void format_time(char *buf, size_t size, double seconds)
{
    int total = (int)seconds;

    snprintf(buf, size, "%02d:%02d", total / 60, total % 60);
}

static void draw_overlay(const Game *g, Canvas *c)
{
    if (g->state == GAME_PAUSED) {
        int w = 30;
        int h = 5;
        int x = (c->width - w) / 2;
        int y = g->play_height / 2 - h / 2;

        draw_box(c, x, y, w, h);
        draw_text_center(c, y + 1, "PAUSED");
        draw_text_center(c, y + 3, "P resume    Q menu");
    } else if (g->state == GAME_WON) {
        char time_text[16];
        char score_text[32];
        int w = 38;
        int h = 10;
        int x = (c->width - w) / 2;
        int y = g->play_height / 2 - h / 2;

        format_time(time_text, sizeof(time_text), g->mission_time);
        snprintf(score_text, sizeof(score_text), "SCORE  %d", g->score);

        draw_box(c, x, y, w, h);
        draw_text_center(c, y + 1, "MARS ORBIT ACHIEVED");
        draw_text_center(c, y + 3, time_text);
        draw_text_center(c, y + 4, score_text);
        if (g->new_best)
            draw_text_center(c, y + 5, "NEW BEST!");
        draw_text_center(c, y + 8, "R fly again    Q menu");
    } else if (g->state == GAME_DEAD) {
        int w = 34;
        int h = 7;
        int x = (c->width - w) / 2;
        int y = g->play_height / 2 - h / 2;

        draw_box(c, x, y, w, h);
        draw_text_center(c, y + 1, "MISSION FAILED");
        draw_text_center(c, y + 3, "the rocket broke apart");
        draw_text_center(c, y + 5, "R retry    Q menu");
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

static void draw_hud(const Game *g, Canvas *c)
{
    char hull_bar[16];
    char fuel_bar[16];
    char time_text[16];
    char line[160];
    double speed;

    if (g->state == GAME_MENU)
        return;

    make_bar(hull_bar, g->hull, GAME_HULL_MAX, 10);
    make_bar(fuel_bar, g->fuel, g->fuel_max, 10);
    speed = sqrt(g->player_vx * g->player_vx + g->player_vy * g->player_vy);

    snprintf(line, sizeof(line),
             "HULL [%s] %3.0f%%   FUEL [%s] %3.0f%%   VEL %4.1f",
             hull_bar, g->hull, fuel_bar,
             g->fuel / g->fuel_max * 100.0, speed);
    draw_text(c, 0, g->height - 2, line);

    format_time(time_text, sizeof(time_text), g->mission_time);
    snprintf(line, sizeof(line),
             "DIST %4.0f / %.0f   TIME %s   %s   P pause   Q menu",
             g->player_x, GAME_WORLD_LENGTH, time_text,
             game_difficulty_name(g->difficulty));
    draw_text(c, 0, g->height - 1, line);
}

void render_frame(const Game *g)
{
    Canvas c;
    int shx = 0;
    int shy = 0;

    c.width = g->width;
    c.height = g->height;
    c.cells = malloc((size_t)c.width * (size_t)c.height);
    if (c.cells == NULL)
        return;

    memset(c.cells, ' ', (size_t)c.width * (size_t)c.height);
    draw_stars(g, &c);

    if (g->state != GAME_MENU) {
        if (g->shake > 0.0) {
            shx = (int)(sin(g->shake * 47.0) * 2.0);
            shy = (int)(cos(g->shake * 61.0) * 1.5);
        }

        draw_disk(&c, 0.0 - g->camera_x + shx, g->world_mid + shy,
                  GAME_EARTH_RADIUS, '#', 'o', 3);
        draw_label(&c, 0.0 - g->camera_x + shx,
                   (int)g->world_mid - (int)GAME_EARTH_RADIUS - 2 + shy, "EARTH");

        draw_disk(&c, GAME_WORLD_LENGTH - g->camera_x + shx, g->world_mid + shy,
                  GAME_MARS_RADIUS, '#', '.', 7);
        draw_label(&c, GAME_WORLD_LENGTH - g->camera_x + shx,
                   (int)g->world_mid - (int)GAME_MARS_RADIUS - 2 + shy, "MARS");

        {
            int i;

            for (i = 0; i < GAME_MAX_ASTEROIDS; i++) {
                if (!g->asteroids[i].alive)
                    continue;
                draw_disk(&c, g->asteroids[i].x - g->camera_x + shx,
                          g->asteroids[i].y + shy,
                          (double)g->asteroids[i].radius * 1.2, 'O', 'o', i * 13 + 5);
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
                        (int)(g->player_y + 0.5) + shy, last_ax, last_ay);
        }

        {
            int i;

            for (i = 0; i < GAME_MAX_PARTICLES; i++) {
                if (g->particles[i].life <= 0.0)
                    continue;
                put_cell(&c, (int)(g->particles[i].x - g->camera_x + 0.5) + shx,
                         (int)(g->particles[i].y + 0.5) + shy, g->particles[i].ch);
            }
        }
    } else {
        draw_menu(g, &c);
    }

    draw_overlay(g, &c);
    draw_hud(g, &c);

    term_draw(c.cells, c.width, c.height);

    free(c.cells);
}
