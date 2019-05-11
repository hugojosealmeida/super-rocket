/*
 * super-rocket
 *
 * A small terminal game about flying a rocket from Earth to Mars.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <math.h>
#include <sys/ioctl.h>

#define FRAME_MS 33
#define DT       0.033

#define KEY_UP    1001
#define KEY_DOWN  1002
#define KEY_LEFT  1003
#define KEY_RIGHT 1004
#define KEY_ESC   27

#define PLAYER_ACCEL     26.0
#define PLAYER_MAX_SPEED 45.0
#define PLAYER_DRAG      0.9
#define THRUST_MEMORY    0.12

#define WORLD_LENGTH 1800.0
#define EARTH_RADIUS 7.0
#define MARS_RADIUS  4.5
#define MARS_ORBIT   16.5
#define STAR_SPAN    2400.0
#define GRAVITY_MU   1500.0

static int W = 80;
static int H = 24;
static struct termios orig_termios;
static int term_active = 0;
static volatile sig_atomic_t stop_requested = 0;

typedef struct {
    double x;
    int y;
    char ch;
    double factor;
} Star;

#define MAX_STARS 400
static Star stars[MAX_STARS];
static int star_count = 0;

static double player_x, player_y;
static double player_vx, player_vy;
static double thrust_t[4];
static double camera_x;
static int mission_won = 0;

static int play_height(void)
{
    return H - 1;
}

static double world_mid(void)
{
    return (H - 1) / 2.0;
}

static void on_signal(int sig)
{
    (void)sig;
    stop_requested = 1;
}

static void term_size(void)
{
    struct winsize ws;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 20 && ws.ws_row > 10) {
        W = ws.ws_col;
        H = ws.ws_row;
    }
}

static void term_init(void)
{
    struct termios raw;

    tcgetattr(STDIN_FILENO, &orig_termios);

    raw = orig_termios;
    raw.c_lflag &= (tcflag_t)~(ICANON | ECHO);
    raw.c_iflag &= (tcflag_t)~(IXON | ICRNL);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);

    printf("\033[?1049h\033[?25l\033[2J");
    fflush(stdout);

    term_active = 1;

    signal(SIGINT, on_signal);
    signal(SIGTERM, on_signal);
}

static void term_restore(void)
{
    if (!term_active)
        return;

    printf("\033[0m\033[?25h\033[?1049l");
    fflush(stdout);

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
    term_active = 0;
}

static void sleep_ms(int ms)
{
    struct timespec ts;

    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (long)(ms % 1000) * 1000000L;
    nanosleep(&ts, NULL);
}

static long now_ms(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000L + ts.tv_nsec / 1000000L;
}

static int read_key(void)
{
    static unsigned char buf[64];
    static int len = 0;
    ssize_t n;
    int c;

    if (len == 0) {
        n = read(STDIN_FILENO, buf, sizeof(buf));
        if (n <= 0)
            return 0;
        len = (int)n;
    }

    c = buf[0];

    if (c == 0x1b) {
        if (len >= 3 && buf[1] == '[') {
            int code = buf[2];

            memmove(buf, buf + 3, (size_t)(len - 3));
            len -= 3;

            if (code == 'A')
                return KEY_UP;
            if (code == 'B')
                return KEY_DOWN;
            if (code == 'C')
                return KEY_RIGHT;
            if (code == 'D')
                return KEY_LEFT;
            return 0;
        }

        memmove(buf, buf + 1, (size_t)(len - 1));
        len--;
        return KEY_ESC;
    }

    memmove(buf, buf + 1, (size_t)(len - 1));
    len--;
    return c;
}

static void init_stars(void)
{
    int i;

    star_count = W * 4;
    if (star_count > MAX_STARS)
        star_count = MAX_STARS;

    for (i = 0; i < star_count; i++) {
        stars[i].x = (double)(rand() % (int)STAR_SPAN);
        stars[i].y = rand() % (play_height() - 2) + 1;
        stars[i].ch = (rand() % 6 == 0) ? '+' : '.';
        stars[i].factor = (i % 3 == 0) ? 0.20 : (i % 3 == 1) ? 0.45 : 0.75;
    }
}

static void draw_stars(char *frame)
{
    int i;

    for (i = 0; i < star_count; i++) {
        double sx = fmod(stars[i].x - camera_x * stars[i].factor, STAR_SPAN);
        int x;
        int y = stars[i].y;

        if (sx < 0.0)
            sx += STAR_SPAN;

        x = (int)sx;
        if (x >= 0 && x < W && y >= 0 && y < play_height())
            frame[y * W + x] = stars[i].ch;
    }
}

static void draw_disk(char *frame, double center_x, double center_y, double radius,
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
            if (x < 0 || x >= W || y < 0 || y >= play_height())
                continue;

            if (dist > radius - 0.9)
                ch = '~';
            else if ((x * 31 + y * 17 + seed) % 11 == 0)
                ch = detail;
            else
                ch = body;

            frame[y * W + x] = ch;
        }
    }
}

static void draw_label(char *frame, double center_x, int y, const char *text)
{
    int x = (int)(center_x - (double)strlen(text) / 2.0);
    size_t i;

    for (i = 0; text[i] != '\0'; i++) {
        int sx = x + (int)i;

        if (sx >= 0 && sx < W && y >= 0 && y < play_height())
            frame[y * W + sx] = text[i];
    }
}

static void draw_rocket(char *frame, int cx, int cy, double ax, double ay)
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
            int x = cx - 4 + col;
            int y = cy - 2 + row;

            if (ch != ' ' && x >= 0 && x < W && y >= 0 && y < play_height())
                frame[y * W + x] = ch;
        }
    }

    if (ax > 0.1 && cx - 5 >= 0 && cy >= 0 && cy < play_height())
        frame[cy * W + cx - 5] = '*';
    if (ax < -0.1 && cx + 5 < W && cy >= 0 && cy < play_height())
        frame[cy * W + cx + 5] = '*';
    if (ay < -0.1 && cx >= 0 && cx < W && cy + 2 < play_height())
        frame[(cy + 2) * W + cx] = '*';
    if (ay > 0.1 && cx >= 0 && cx < W && cy - 3 >= 0)
        frame[(cy - 3) * W + cx] = '*';
}

static void add_planet_gravity(double cx, double cy, double radius, double *ax, double *ay)
{
    double dx = cx - player_x;
    double dy = cy - player_y;
    double dist2 = dx * dx + dy * dy;
    double min_dist = radius + 1.0;
    double dist;
    double pull;

    if (dist2 < min_dist * min_dist)
        dist2 = min_dist * min_dist;

    dist = sqrt(dist2);
    pull = GRAVITY_MU / dist2;

    *ax += dx / dist * pull;
    *ay += dy / dist * pull;
}

static void bounce_off_planet(double cx, double cy, double radius)
{
    double dx = player_x - cx;
    double dy = player_y - cy;
    double dist = sqrt(dx * dx + dy * dy);
    double nx, ny, vn;

    if (dist >= radius || dist < 0.001)
        return;

    nx = dx / dist;
    ny = dy / dist;

    player_x = cx + nx * radius;
    player_y = cy + ny * radius;

    vn = player_vx * nx + player_vy * ny;
    if (vn < 0.0) {
        player_vx -= 2.0 * vn * nx;
        player_vy -= 2.0 * vn * ny;
    }

    player_vx *= 0.45;
    player_vy *= 0.45;
}

static void update_player(void)
{
    double ax = 0.0;
    double ay = 0.0;
    double gx = 0.0;
    double gy = 0.0;
    double len;
    double speed;
    double target;
    int i;

    if (thrust_t[0] > 0.0)
        ay -= 1.0;
    if (thrust_t[1] > 0.0)
        ay += 1.0;
    if (thrust_t[2] > 0.0)
        ax -= 1.0;
    if (thrust_t[3] > 0.0)
        ax += 1.0;

    len = sqrt(ax * ax + ay * ay);
    if (len > 0.0) {
        ax /= len;
        ay /= len;
    }

    add_planet_gravity(0.0, world_mid(), EARTH_RADIUS, &gx, &gy);
    add_planet_gravity(WORLD_LENGTH, world_mid(), MARS_RADIUS, &gx, &gy);

    player_vx += (ax * PLAYER_ACCEL + gx) * DT;
    player_vy += (ay * PLAYER_ACCEL + gy) * DT;

    player_vx -= player_vx * PLAYER_DRAG * DT;
    player_vy -= player_vy * PLAYER_DRAG * DT;

    speed = sqrt(player_vx * player_vx + player_vy * player_vy);
    if (speed > PLAYER_MAX_SPEED) {
        player_vx = player_vx / speed * PLAYER_MAX_SPEED;
        player_vy = player_vy / speed * PLAYER_MAX_SPEED;
    }

    player_x += player_vx * DT;
    player_y += player_vy * DT;

    bounce_off_planet(0.0, world_mid(), EARTH_RADIUS + 1.5);

    if (player_x > WORLD_LENGTH + 40.0) {
        player_x = WORLD_LENGTH + 40.0;
        if (player_vx > 0.0)
            player_vx = -player_vx * 0.4;
    }
    if (player_y < 2.0) {
        player_y = 2.0;
        if (player_vy < 0.0)
            player_vy = -player_vy * 0.5;
    }
    if (player_y > play_height() - 2.0) {
        player_y = play_height() - 2.0;
        if (player_vy > 0.0)
            player_vy = -player_vy * 0.5;
    }

    for (i = 0; i < 4; i++) {
        if (thrust_t[i] > 0.0)
            thrust_t[i] -= DT;
    }

    target = player_x - W * 0.35;
    if (target < -20.0)
        target = -20.0;
    if (target > WORLD_LENGTH - W + 20.0)
        target = WORLD_LENGTH - W + 20.0;

    camera_x += (target - camera_x) * 0.15;

    if (!mission_won) {
        double dx = player_x - WORLD_LENGTH;
        double dy = player_y - world_mid();

        if (sqrt(dx * dx + dy * dy) < MARS_ORBIT)
            mission_won = 1;
    }
}

static void draw_frame(void)
{
    char *frame;
    int x, y;
    double last_ax = 0.0;
    double last_ay = 0.0;
    double earth_screen = 0.0 - camera_x;
    double mars_screen = WORLD_LENGTH - camera_x;

    if (thrust_t[0] > 0.0)
        last_ay -= 1.0;
    if (thrust_t[1] > 0.0)
        last_ay += 1.0;
    if (thrust_t[2] > 0.0)
        last_ax -= 1.0;
    if (thrust_t[3] > 0.0)
        last_ax += 1.0;

    frame = malloc((size_t)W * (size_t)play_height());

    memset(frame, ' ', (size_t)W * (size_t)play_height());
    draw_stars(frame);

    draw_disk(frame, earth_screen, world_mid(), EARTH_RADIUS, '#', 'o', 3);
    draw_label(frame, earth_screen, (int)world_mid() - (int)EARTH_RADIUS - 2, "EARTH");

    draw_disk(frame, mars_screen, world_mid(), MARS_RADIUS, '#', '.', 7);
    draw_label(frame, mars_screen, (int)world_mid() - (int)MARS_RADIUS - 2, "MARS");

    draw_rocket(frame, (int)(player_x - camera_x + 0.5), (int)(player_y + 0.5),
                last_ax, last_ay);

    printf("\033[H");
    for (y = 0; y < play_height(); y++) {
        for (x = 0; x < W; x++)
            putchar(frame[y * W + x]);
    }

    printf("\033[%d;1H", H);
    if (mission_won) {
        printf("MARS REACHED!  Press R to fly again, Q to quit.                          ");
    } else {
        printf("DIST %4.0f / %.0f     VEL %4.1f     R restart     Q quit            ",
               player_x, WORLD_LENGTH,
               sqrt(player_vx * player_vx + player_vy * player_vy));
    }
    fflush(stdout);

    free(frame);
}

static void reset_game(void)
{
    player_x = EARTH_RADIUS + 11.0;
    player_y = world_mid();
    player_vx = 0.0;
    player_vy = 0.0;
    camera_x = player_x - W * 0.35;
    mission_won = 0;
}

int main(void)
{
    int running = 1;

    srand((unsigned)time(NULL) ^ (unsigned)getpid());

    term_size();
    term_init();
    init_stars();

    reset_game();

    while (running && !stop_requested) {
        long frame_start = now_ms();
        int key;
        long elapsed;

        while ((key = read_key()) != 0) {
            if (key == 'q' || key == 'Q' || key == KEY_ESC)
                running = 0;
            if ((key == 'r' || key == 'R') && mission_won)
                reset_game();

            if (!mission_won) {
                if (key == KEY_UP || key == 'w' || key == 'W')
                    thrust_t[0] = THRUST_MEMORY;
                if (key == KEY_DOWN || key == 's' || key == 'S')
                    thrust_t[1] = THRUST_MEMORY;
                if (key == KEY_LEFT || key == 'a' || key == 'A')
                    thrust_t[2] = THRUST_MEMORY;
                if (key == KEY_RIGHT || key == 'd' || key == 'D')
                    thrust_t[3] = THRUST_MEMORY;
            }
        }

        if (!mission_won)
            update_player();

        draw_frame();

        elapsed = now_ms() - frame_start;
        if (elapsed < FRAME_MS)
            sleep_ms((int)(FRAME_MS - elapsed));
    }

    term_restore();

    return 0;
}
