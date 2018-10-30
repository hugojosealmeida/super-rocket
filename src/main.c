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

#define THRUST_MEMORY 0.12

static int W = 80;
static int H = 24;
static struct termios orig_termios;
static int term_active = 0;
static volatile sig_atomic_t stop_requested = 0;

typedef struct {
    int x;
    int y;
    char ch;
} Star;

#define MAX_STARS 240
static Star stars[MAX_STARS];
static int star_count = 0;

static double player_x, player_y;
static double player_vx, player_vy;
static double thrust_t[4];

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

    star_count = W * 3;
    if (star_count > MAX_STARS)
        star_count = MAX_STARS;

    for (i = 0; i < star_count; i++) {
        stars[i].x = rand() % W;
        stars[i].y = rand() % (H - 2) + 1;
        stars[i].ch = (rand() % 5 == 0) ? '+' : '.';
    }
}

static void draw_stars(char *frame)
{
    int i;

    for (i = 0; i < star_count; i++) {
        int x = stars[i].x;
        int y = stars[i].y;

        if (x >= 0 && x < W && y >= 0 && y < H - 1)
            frame[y * W + x] = stars[i].ch;
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

            if (ch != ' ' && x >= 0 && x < W && y >= 0 && y < H - 1)
                frame[y * W + x] = ch;
        }
    }

    if (ax < -0.1 && cx - 5 >= 0 && cy >= 0 && cy < H - 1)
        frame[cy * W + cx - 5] = '*';
    if (ax > 0.1 && cx + 5 < W && cy >= 0 && cy < H - 1)
        frame[cy * W + cx + 5] = '*';
    if (ay < -0.1 && cx >= 0 && cx < W && cy - 3 >= 0)
        frame[(cy - 3) * W + cx] = '*';
    if (ay > 0.1 && cx >= 0 && cx < W && cy + 2 < H - 1)
        frame[(cy + 2) * W + cx] = '*';
}

static void update_player(void)
{
    double ax = 0.0;
    double ay = 0.0;
    double len;
    double speed;
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

    player_vx += ax * PLAYER_ACCEL * DT;
    player_vy += ay * PLAYER_ACCEL * DT;

    player_vx -= player_vx * PLAYER_DRAG * DT;
    player_vy -= player_vy * PLAYER_DRAG * DT;

    speed = sqrt(player_vx * player_vx + player_vy * player_vy);
    if (speed > PLAYER_MAX_SPEED) {
        player_vx = player_vx / speed * PLAYER_MAX_SPEED;
        player_vy = player_vy / speed * PLAYER_MAX_SPEED;
    }

    player_x += player_vx * DT;
    player_y += player_vy * DT;

    if (player_x < 4.0) {
        player_x = 4.0;
        player_vx = 0.0;
    }
    if (player_x > W - 5.0) {
        player_x = W - 5.0;
        player_vx = 0.0;
    }
    if (player_y < 2.0) {
        player_y = 2.0;
        player_vy = 0.0;
    }
    if (player_y > H - 3.0) {
        player_y = H - 3.0;
        player_vy = 0.0;
    }

    for (i = 0; i < 4; i++) {
        if (thrust_t[i] > 0.0)
            thrust_t[i] -= DT;
    }
}

static void draw_frame(void)
{
    char *frame;
    int x, y;
    double last_ax = 0.0;
    double last_ay = 0.0;

    if (thrust_t[0] > 0.0)
        last_ay -= 1.0;
    if (thrust_t[1] > 0.0)
        last_ay += 1.0;
    if (thrust_t[2] > 0.0)
        last_ax -= 1.0;
    if (thrust_t[3] > 0.0)
        last_ax += 1.0;

    frame = malloc((size_t)W * (size_t)H);

    memset(frame, ' ', (size_t)W * (size_t)H);
    draw_stars(frame);
    draw_rocket(frame, (int)(player_x + 0.5), (int)(player_y + 0.5), last_ax, last_ay);

    printf("\033[H");
    for (y = 0; y < H - 1; y++) {
        for (x = 0; x < W; x++)
            putchar(frame[y * W + x]);
    }

    printf("\033[%d;1H", H);
    printf("ARROWS / WASD  thrust     Q  quit     VEL %5.1f",
           sqrt(player_vx * player_vx + player_vy * player_vy));
    fflush(stdout);

    free(frame);
}

int main(void)
{
    int running = 1;

    srand((unsigned)time(NULL) ^ (unsigned)getpid());

    term_size();
    term_init();
    init_stars();

    player_x = W / 2.0;
    player_y = (H - 1) / 2.0;
    player_vx = 0.0;
    player_vy = 0.0;

    while (running && !stop_requested) {
        long frame_start = now_ms();
        int key;
        long elapsed;

        while ((key = read_key()) != 0) {
            if (key == 'q' || key == 'Q' || key == KEY_ESC)
                running = 0;

            if (key == KEY_UP || key == 'w' || key == 'W')
                thrust_t[0] = THRUST_MEMORY;
            if (key == KEY_DOWN || key == 's' || key == 'S')
                thrust_t[1] = THRUST_MEMORY;
            if (key == KEY_LEFT || key == 'a' || key == 'A')
                thrust_t[2] = THRUST_MEMORY;
            if (key == KEY_RIGHT || key == 'd' || key == 'D')
                thrust_t[3] = THRUST_MEMORY;
        }

        update_player();
        draw_frame();

        elapsed = now_ms() - frame_start;
        if (elapsed < FRAME_MS)
            sleep_ms((int)(FRAME_MS - elapsed));
    }

    term_restore();

    return 0;
}
