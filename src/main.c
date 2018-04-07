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
#include <sys/ioctl.h>

#define FRAME_MS 33

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
    unsigned char buf[32];
    ssize_t n;
    ssize_t i;

    n = read(STDIN_FILENO, buf, sizeof(buf));
    if (n <= 0)
        return 0;

    for (i = 0; i < n; i++) {
        unsigned char c = buf[i];

        if (c == 0x1b) {
            if (i + 2 < n && buf[i + 1] == '[') {
                i += 2;
                continue;
            }
            return 27;
        }

        return c;
    }

    return 0;
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

static void draw_rocket(char *frame, int cx, int cy)
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
}

static void draw_frame(void)
{
    char *frame;
    int x, y;

    frame = malloc((size_t)W * (size_t)H);

    memset(frame, ' ', (size_t)W * (size_t)H);
    draw_stars(frame);
    draw_rocket(frame, W / 2, (H - 1) / 2);

    printf("\033[H");
    for (y = 0; y < H - 1; y++) {
        for (x = 0; x < W; x++)
            putchar(frame[y * W + x]);
    }
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

    while (running && !stop_requested) {
        long frame_start = now_ms();
        int key;
        long elapsed;

        key = read_key();

        if (key == 'q' || key == 'Q' || key == 27)
            running = 0;

        draw_frame();

        elapsed = now_ms() - frame_start;
        if (elapsed < FRAME_MS)
            sleep_ms((int)(FRAME_MS - elapsed));
    }

    term_restore();

    return 0;
}
