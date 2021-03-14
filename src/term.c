#include "term.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <sys/ioctl.h>

static struct termios saved_termios;
static int active = 0;
static volatile sig_atomic_t stop_requested = 0;

static void on_signal(int sig)
{
    (void)sig;
    stop_requested = 1;
}

void term_init(void)
{
    struct termios raw;

    tcgetattr(STDIN_FILENO, &saved_termios);

    raw = saved_termios;
    raw.c_lflag &= (tcflag_t)~(ICANON | ECHO);
    raw.c_iflag &= (tcflag_t)~(IXON | ICRNL);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);

    printf("\033[?1049h\033[?25l\033[2J");
    fflush(stdout);

    active = 1;

    signal(SIGINT, on_signal);
    signal(SIGTERM, on_signal);
}

void term_shutdown(void)
{
    if (!active)
        return;

    printf("\033[0m\033[?25h\033[?1049l");
    fflush(stdout);

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &saved_termios);
    active = 0;
}

void term_size(int *width, int *height)
{
    struct winsize ws;

    *width = 80;
    *height = 23;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col >= 40 && ws.ws_row >= 12) {
        *width = ws.ws_col;
        *height = ws.ws_row - 1;
    }
}

double term_time(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1000000000.0;
}

void term_sleep(double seconds)
{
    struct timespec ts;

    ts.tv_sec = (time_t)seconds;
    ts.tv_nsec = (long)((seconds - (double)ts.tv_sec) * 1000000000.0);
    nanosleep(&ts, NULL);
}

int term_should_stop(void)
{
    return stop_requested != 0;
}

void term_draw(const char *chars, int width, int height)
{
    int y;

    for (y = 0; y < height; y++) {
        printf("\033[%d;1H", y + 1);
        fwrite(chars + y * width, 1, (size_t)width, stdout);
    }
    fflush(stdout);
}

void term_read_input(TermInput *in)
{
    unsigned char buf[128];
    ssize_t n;
    ssize_t i;

    memset(in, 0, sizeof(*in));
    in->digit = -1;

    n = read(STDIN_FILENO, buf, sizeof(buf));
    if (n <= 0)
        return;

    for (i = 0; i < n; ) {
        unsigned char c = buf[i];

        if (c == 0x1b) {
            if (i + 2 < n && buf[i + 1] == '[') {
                switch (buf[i + 2]) {
                case 'A': in->up = 1; break;
                case 'B': in->down = 1; break;
                case 'C': in->right = 1; break;
                case 'D': in->left = 1; break;
                default: break;
                }
                i += 3;
                continue;
            }
            in->quit = 1;
            i++;
            continue;
        }

        i++;

        switch (c) {
        case 'w': case 'W': in->up = 1; break;
        case 's': case 'S': in->down = 1; break;
        case 'a': case 'A': in->left = 1; break;
        case 'd': case 'D': in->right = 1; break;
        case 'p': case 'P': in->pause = 1; break;
        case 'r': case 'R': in->restart = 1; break;
        case 'q': case 'Q': in->back = 1; break;
        case '\r': case '\n': in->enter = 1; break;
        default:
            if (c >= '0' && c <= '9')
                in->digit = c - '0';
            break;
        }
    }
}
