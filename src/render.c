#include "render.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "term.h"
#include "theme.h"
#include "util.h"

#define C_DEFAULT 0

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
            color = theme_color(THEME_STAR_FAR);
        else if (g->stars[i].factor < 0.6)
            color = theme_color(THEME_STAR_MID);
        else
            color = theme_color(THEME_STAR_BRIGHT);

        if ((g->frame / 7 + (unsigned long)i) % 29 == 0)
            color = theme_color(THEME_STAR_ACCENT);

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

static void draw_pickup(Canvas *c, const Pickup *p, double camera_x, int shx, int shy)
{
    char ch;
    unsigned char color;
    int x = (int)(p->x - camera_x + 0.5) + shx;
    int y = (int)(p->y + sin(p->bob) * 0.6 + 0.5) + shy;

    switch (p->type) {
    case PICKUP_FUEL:
        ch = 'F';
        color = theme_color(THEME_LABEL);
        break;
    case PICKUP_REPAIR:
        ch = '+';
        color = theme_color(THEME_GOOD);
        break;
    default:
        ch = 'O';
        color = theme_color(THEME_TITLE);
        break;
    }

    put_cell(c, x, y, ch, color);
    put_cell(c, x - 1, y, '[', theme_color(THEME_DIM));
    put_cell(c, x + 1, y, ']', theme_color(THEME_DIM));
}

static void draw_ufo(Canvas *c, const Ufo *u, double camera_x, int shx, int shy)
{
    static const char *sprite[] = {
        "  ___  ",
        " /o o\\ ",
        " \\___/ "
    };
    static const unsigned char colors[] = { 96, 94, 96 };
    int cx = (int)(u->x - camera_x + 0.5) + shx;
    int cy = (int)(u->y + 0.5) + shy;
    int row, col;

    for (row = 0; row < 3; row++) {
        for (col = 0; col < 7; col++) {
            char ch = sprite[row][col];

            if (ch != ' ')
                put_cell(c, cx - 3 + col, cy - 1 + row, ch, colors[row]);
        }
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
                         ch == '>' ? theme_color(THEME_COCKPIT) : theme_color(THEME_ROCKET));
        }
    }

    if (ax > 0.1)
        put_cell(c, cx - 5, cy, flame, theme_color(THEME_FLAME));
    if (ax < -0.1)
        put_cell(c, cx + 5, cy, flame, theme_color(THEME_FLAME));
    if (ay < -0.1)
        put_cell(c, cx, cy + 2, flame, theme_color(THEME_FLAME));
    if (ay > 0.1)
        put_cell(c, cx, cy - 3, flame, theme_color(THEME_FLAME));
}

static void draw_menu(const Game *g, Canvas *c)
{
    int mid = c->height / 2;
    int i;

    draw_text_center(c, mid - 10, "S U P E R   R O C K E T", theme_color(THEME_TITLE));
    draw_text_center(c, mid - 9, "-------------------------", theme_color(THEME_BOX));
    draw_text_center(c, mid - 7, "E A R T H   ->   M A R S", theme_color(THEME_TEXT));

    for (i = 0; i < DIFFICULTY_COUNT; i++) {
        char label[80];
        int selected = (i == (int)g->difficulty);

        snprintf(label, sizeof(label), "%s [%d] %-6s   best %d",
                 selected ? ">" : " ", i + 1,
                 game_difficulty_name((Difficulty)i), g->best[i]);
        draw_text_center(c, mid - 4 + i, label,
                         selected ? theme_color(THEME_WARN) : theme_color(THEME_DIM));
    }

    draw_text_center(c, mid, "[ ENTER ] lift off   [ H ] help   [ I ] scores   [ Q ] quit",
                     theme_color(THEME_TEXT));
    draw_text_center(c, mid + 3, "Fly with ARROWS or WASD.", theme_color(THEME_DIM));
    draw_text_center(c, mid + 4, "Thrust burns fuel. Dodge the asteroids.", theme_color(THEME_DIM));
    draw_text_center(c, mid + 5, "Reach Mars orbit to complete the mission.", theme_color(THEME_DIM));

    {
        char settings_line[96];

        snprintf(settings_line, sizeof(settings_line),
                 "T theme:%-8s  M sound:%-3s  F fps:%-3s",
                 theme_name(g->settings.theme),
                 g->settings.sound ? "on" : "off",
                 g->settings.show_fps ? "on" : "off");
        draw_text_center(c, mid + 6, settings_line, theme_color(THEME_DIM));
    }

    if (g->stats.runs > 0) {
        char stats_line[96];
        char best_text[16];

        if (g->stats.best_time > 0)
            util_format_time(best_text, sizeof(best_text), (double)g->stats.best_time);
        else
            snprintf(best_text, sizeof(best_text), "--:--");

        snprintf(stats_line, sizeof(stats_line),
                 "RUNS %d   WINS %d   BEST %s   DOWNED %d",
                 g->stats.runs, g->stats.wins, best_text,
                 g->stats.asteroids_destroyed);
        draw_text_center(c, mid + 8, stats_line, theme_color(THEME_DIM));
    }

    draw_text_center(c, mid + 10, "super-rocket " GAME_VERSION, theme_color(THEME_DIM));
}

static void draw_help(const Game *g, Canvas *c)
{
    int w = 54;
    int h = 17;
    int x = (c->width - w) / 2;
    int y = g->play_height / 2 - h / 2;

    if (x < 0)
        x = 0;
    if (y < 0)
        y = 0;

    draw_box(c, x, y, w, h, theme_color(THEME_BOX));
    draw_text_center(c, y + 1, "H O W   T O   F L Y", theme_color(THEME_TITLE));
    draw_text_center(c, y + 3, "ARROWS / WASD   thrust", theme_color(THEME_TEXT));
    draw_text_center(c, y + 4, "SPACE           fire laser", theme_color(THEME_TEXT));
    draw_text_center(c, y + 5, "P               pause", theme_color(THEME_TEXT));
    draw_text_center(c, y + 6, "R               restart mission", theme_color(THEME_TEXT));
    draw_text_center(c, y + 7, "T / M / F       theme / sound / fps", theme_color(THEME_TEXT));
    draw_text_center(c, y + 8, "Q               menu or quit", theme_color(THEME_TEXT));
    draw_text_center(c, y + 9, "I               leaderboard", theme_color(THEME_TEXT));
    draw_text_center(c, y + 11, "Collect fuel, shields and repairs.", theme_color(THEME_DIM));
    draw_text_center(c, y + 12, "Shoot asteroids and avoid the UFOs.", theme_color(THEME_DIM));
    draw_text_center(c, y + 13, "Reach the Mars orbit to win.", theme_color(THEME_DIM));
    draw_text_center(c, y + 15, "H close help", theme_color(THEME_WARN));
}

static void draw_leaderboard(const Game *g, Canvas *c)
{
    int w = 44;
    int h = 11;
    int x = (c->width - w) / 2;
    int y = g->play_height / 2 - h / 2;
    int i;

    if (x < 0)
        x = 0;
    if (y < 0)
        y = 0;

    draw_box(c, x, y, w, h, theme_color(THEME_BOX));
    draw_text_center(c, y + 1, "T O P   P I L O T S", theme_color(THEME_TITLE));

    for (i = 0; i < GAME_LEADERBOARD_SIZE; i++) {
        char line[64];

        snprintf(line, sizeof(line), "%d. %-3s  %6d  %s", i + 1,
                 g->leaderboard[i].name, g->leaderboard[i].score,
                 game_difficulty_name((Difficulty)g->leaderboard[i].difficulty));
        draw_text_center(c, y + 3 + i, line,
                         i == g->leaderboard_rank ? theme_color(THEME_WARN)
                                                  : theme_color(THEME_TEXT));
    }

    draw_text_center(c, y + 9, "I close", theme_color(THEME_DIM));
}

static void draw_overlay(const Game *g, Canvas *c)
{
    if (g->state == GAME_PAUSED) {
        int w = 34;
        int h = 5;
        int x = (c->width - w) / 2;
        int y = g->play_height / 2 - h / 2;

        draw_box(c, x, y, w, h, theme_color(THEME_BOX));
        draw_text_center(c, y + 1, "PAUSED", theme_color(THEME_WARN));
        draw_text_center(c, y + 3, "P resume   R restart   Q menu", theme_color(THEME_TEXT));
    } else if (g->state == GAME_INITIALS) {
        char line[64];
        char prompt[16];
        int w = 40;
        int h = 9;
        int x = (c->width - w) / 2;
        int y = g->play_height / 2 - h / 2;
        size_t i;

        draw_box(c, x, y, w, h, theme_color(THEME_GOOD));
        draw_text_center(c, y + 1, "NEW HIGH SCORE", theme_color(THEME_WARN));

        snprintf(line, sizeof(line), "SCORE %d", g->score);
        draw_text_center(c, y + 3, line, theme_color(THEME_TEXT));

        for (i = 0; i < 3; i++)
            prompt[i] = g->initials[i] != '\0' ? g->initials[i] : '_';
        prompt[3] = '\0';
        snprintf(line, sizeof(line), "INITIALS   %c %c %c",
                 prompt[0], prompt[1], prompt[2]);
        draw_text_center(c, y + 5, line, theme_color(THEME_TITLE));
        draw_text_center(c, y + 7, "ENTER confirm   BACKSPACE erase",
                         theme_color(THEME_DIM));
    } else if (g->state == GAME_WON) {
        char time_text[16];
        char score_text[32];
        int w = 38;
        int h = 10;
        int x = (c->width - w) / 2;
        int y = g->play_height / 2 - h / 2;

        util_format_time(time_text, sizeof(time_text), g->mission_time);
        snprintf(score_text, sizeof(score_text), "SCORE  %d", g->score);

        draw_box(c, x, y, w, h, theme_color(THEME_GOOD));
        draw_text_center(c, y + 1, "MARS ORBIT ACHIEVED", theme_color(THEME_GOOD));
        draw_text_center(c, y + 3, time_text, theme_color(THEME_TEXT));
        draw_text_center(c, y + 4, score_text, theme_color(THEME_TEXT));
        if (g->new_best)
            draw_text_center(c, y + 5, "NEW BEST!", theme_color(THEME_WARN));
        draw_text_center(c, y + 8, "R fly again    Q menu", theme_color(THEME_LABEL));
    } else if (g->state == GAME_UPGRADE) {
        int w = 48;
        int h = 12;
        int x = (c->width - w) / 2;
        int y = g->play_height / 2 - h / 2;
        int i;

        if (x < 0)
            x = 0;
        if (y < 0)
            y = 0;

        draw_box(c, x, y, w, h, theme_color(THEME_BOX));
        draw_text_center(c, y + 1, "SECTOR CLEARED", theme_color(THEME_GOOD));
        draw_text_center(c, y + 2, "Choose one upgrade", theme_color(THEME_TEXT));

        for (i = 0; i < 3; i++) {
            char line[72];

            snprintf(line, sizeof(line), "[%d]  %s", i + 1,
                     game_upgrade_name(g->upgrades[i]));
            draw_text_center(c, y + 4 + i * 2, line,
                             i == 0 ? theme_color(THEME_WARN)
                                    : theme_color(THEME_TEXT));
        }

        draw_text_center(c, y + 10, "1-3 select    ENTER takes the first",
                         theme_color(THEME_DIM));
    } else if (g->state == GAME_DEAD) {
        int w = 34;
        int h = 7;
        int x = (c->width - w) / 2;
        int y = g->play_height / 2 - h / 2;

        draw_box(c, x, y, w, h, theme_color(THEME_BAD));
        draw_text_center(c, y + 1, "MISSION FAILED", theme_color(THEME_BAD));
        draw_text_center(c, y + 3, "the rocket broke apart", theme_color(THEME_DIM));
        draw_text_center(c, y + 5, "R retry    Q menu", theme_color(THEME_LABEL));
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
        return theme_color(THEME_GOOD);
    if (ratio > 0.3)
        return theme_color(THEME_WARN);
    return theme_color(THEME_BAD);
}

static void draw_hud(const Game *g, Canvas *c)
{
    char hull_bar[16];
    char fuel_bar[16];
    char shield_bar[16];
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
    make_bar(shield_bar, g->shield, 100.0, 5);
    speed = sqrt(g->player_vx * g->player_vx + g->player_vy * g->player_vy);

    snprintf(line, sizeof(line),
             "HULL [%s] %3.0f%%  FUEL [%s] %3.0f%%  SHD [%s] %3.0f%%  VEL %4.1f",
             hull_bar, hull_ratio * 100.0, fuel_bar, fuel_ratio * 100.0,
             shield_bar, g->shield, speed);
    draw_text(c, 0, row, line, theme_color(THEME_TEXT));

    draw_text(c, 0, row, "HULL", theme_color(THEME_LABEL));

    for (x = 0; x < 10; x++)
        put_cell(c, 6 + x, row, hull_bar[x],
                 hull_bar[x] == '#' ? gauge_color(hull_ratio) : theme_color(THEME_DIM));

    fuel_pos = strstr(line, "FUEL [");
    if (fuel_pos != NULL) {
        int fx = (int)(fuel_pos - line);

        draw_text(c, fx, row, "FUEL", theme_color(THEME_LABEL));
        for (x = 0; x < 10; x++)
            put_cell(c, fx + 6 + x, row, fuel_bar[x],
                     fuel_bar[x] == '#' ? theme_color(THEME_LABEL) : theme_color(THEME_DIM));
    }

    fuel_pos = strstr(line, "SHD [");
    if (fuel_pos != NULL) {
        int fx = (int)(fuel_pos - line);

        draw_text(c, fx, row, "SHD", theme_color(THEME_TITLE));
        for (x = 0; x < 5; x++)
            put_cell(c, fx + 5 + x, row, shield_bar[x],
                     shield_bar[x] == '#' ? theme_color(THEME_TITLE) : theme_color(THEME_DIM));
    }

    util_format_time(time_text, sizeof(time_text), g->mission_time);
    {
        int progress = (int)(g->player_x / GAME_WORLD_LENGTH * 100.0);

        if (progress < 0)
            progress = 0;
        if (progress > 100)
            progress = 100;

        snprintf(line, sizeof(line),
                 "DIST %4.0f/%.0f %3d%%  S%d/3  TIME %s  %s  P pause  Q menu",
                 g->player_x, GAME_WORLD_LENGTH, progress, g->sector + 1,
                 time_text, game_difficulty_name(g->difficulty));
    }
    draw_text(c, 0, g->height - 1, line, theme_color(THEME_DIM));
    draw_text(c, 0, g->height - 1, "DIST", theme_color(THEME_LABEL));
}

void render_frame(const Game *g)
{
    Canvas c;
    int shx = 0;
    int shy = 0;

    theme_set(g->settings.theme);

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
        DiskStyle earth = { '#', 'o', '~',
                            theme_color(THEME_EARTH),
                            theme_color(THEME_EARTH_DETAIL),
                            theme_color(THEME_EARTH_EDGE), 3 };
        DiskStyle mars = { '#', '.', '~',
                           theme_color(THEME_MARS),
                           theme_color(THEME_MARS_DETAIL),
                           theme_color(THEME_MARS_EDGE), 7 };

        if (g->shake > 0.0) {
            shx = (int)(sin(g->shake * 47.0) * 2.0);
            shy = (int)(cos(g->shake * 61.0) * 1.5);
        }

        draw_disk(&c, 0.0 - g->camera_x + shx, g->world_mid + shy,
                  GAME_EARTH_RADIUS, &earth);
        draw_label(&c, 0.0 - g->camera_x + shx,
                   (int)g->world_mid - (int)GAME_EARTH_RADIUS - 2 + shy,
                   "EARTH", theme_color(THEME_LABEL));

        draw_disk(&c, GAME_WORLD_LENGTH - g->camera_x + shx, g->world_mid + shy,
                  GAME_MARS_RADIUS, &mars);
    draw_orbit(&c, GAME_WORLD_LENGTH - g->camera_x + shx, g->world_mid + shy,
               GAME_MARS_ORBIT, '.', theme_color(THEME_MARS_DETAIL));
        draw_label(&c, GAME_WORLD_LENGTH - g->camera_x + shx,
                   (int)g->world_mid - (int)GAME_MARS_RADIUS - 2 + shy,
                   "MARS", theme_color(THEME_MARS));

        {
            int i;

            for (i = 0; i < GAME_MAX_ASTEROIDS; i++) {
                DiskStyle asteroid;

                if (!g->asteroids[i].alive)
                    continue;

                asteroid.body = 'O';
                asteroid.detail = 'o';
                asteroid.edge = '.';
                asteroid.body_color = (g->asteroids[i].radius == 2)
                                          ? theme_color(THEME_WARN)
                                          : theme_color(THEME_STAR_BRIGHT);
                asteroid.detail_color = theme_color(THEME_DIM);
                asteroid.edge_color = theme_color(THEME_STAR_FAR);
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

            for (i = 0; i < GAME_MAX_UFOS; i++) {
                if (g->ufos[i].alive)
                    draw_ufo(&c, &g->ufos[i], g->camera_x, shx, shy);
            }

            for (i = 0; i < GAME_MAX_LASERS; i++) {
                if (!g->lasers[i].alive)
                    continue;

                if (fabs(g->lasers[i].vy) > fabs(g->lasers[i].vx))
                    put_cell(&c, (int)(g->lasers[i].x - g->camera_x + 0.5) + shx,
                             (int)(g->lasers[i].y + 0.5) + shy, '|',
                             theme_color(THEME_STAR_ACCENT));
                else
                    put_cell(&c, (int)(g->lasers[i].x - g->camera_x + 0.5) + shx,
                             (int)(g->lasers[i].y + 0.5) + shy, '-',
                             theme_color(THEME_STAR_ACCENT));
            }

            for (i = 0; i < GAME_MAX_PICKUPS; i++) {
                if (g->pickups[i].alive)
                    draw_pickup(&c, &g->pickups[i], g->camera_x, shx, shy);
            }

            for (i = 0; i < GAME_MAX_SHOTS; i++) {
                if (!g->shots[i].alive)
                    continue;

                put_cell(&c, (int)(g->shots[i].x - g->camera_x + 0.5) + shx,
                         (int)(g->shots[i].y + 0.5) + shy, 'o',
                         theme_color(THEME_BAD));
            }

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

    if (g->state == GAME_PLAYING && g->sector_banner > 0.0) {
        char banner[64];

        snprintf(banner, sizeof(banner), "SECTOR %d  -  %s",
                 g->sector + 1, game_sector_name(g->sector));
        draw_text_center(&c, 4, banner, theme_color(THEME_TITLE));
    }

    if (g->state == GAME_PLAYING) {
        if (g->fuel / g->fuel_max < 0.25 && (g->frame / 15) % 2 == 0)
            draw_text_center(&c, 1, "LOW FUEL", theme_color(THEME_BAD));
        if (g->hull / GAME_HULL_MAX < 0.30 && (g->frame / 15) % 2 == 0)
            draw_text_center(&c, 2, "HULL CRITICAL", theme_color(THEME_BAD));
    }

    if (g->toast_timer > 0.0 && g->toast[0] != '\0') {
        int w = (int)strlen(g->toast) + 4;
        int x = (c.width - w) / 2;

        if (x < 0)
            x = 0;
        draw_text(&c, x, 2, g->toast, theme_color(THEME_WARN));
    }

    if (g->settings.show_fps && g->state != GAME_MENU) {
        char fps_text[24];

        snprintf(fps_text, sizeof(fps_text), "FPS %3.0f", g->fps);
        draw_text(&c, c.width - (int)strlen(fps_text), 0, fps_text,
                  theme_color(THEME_DIM));
    }

    draw_overlay(g, &c);
    if (g->show_help)
        draw_help(g, &c);
    if (g->show_scores)
        draw_leaderboard(g, &c);
    draw_hud(g, &c);

    term_draw(c.cells, c.colors, c.width, c.height);

    free(c.cells);
    free(c.colors);
}
