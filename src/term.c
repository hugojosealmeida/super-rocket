#define _POSIX_C_SOURCE 199309L

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
static int saved_flags = -1;
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
    int flags;

    tcgetattr(STDIN_FILENO, &saved_termios);

    raw = saved_termios;
    raw.c_lflag &= (tcflag_t)~(ICANON | ECHO);
    raw.c_iflag &= (tcflag_t)~(IXON | ICRNL);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

    /*
     * O_NONBLOCK is a status flag of the open file description behind stdin,
     * not of our descriptor, so the shell that launched the game keeps it after
     * we exit and every blocking read there starts failing with EAGAIN.  Save
     * what we inherited (any kind of stdin needs the non-blocking read that
     * term_read_input() relies on, tty or not) and hand it back in
     * term_shutdown().  If the original flags cannot be read there is nothing
     * to restore, so the flag is left alone instead of being leaked.
     */
    flags = fcntl(STDIN_FILENO, F_GETFL);
    if (flags != -1) {
        saved_flags = flags;
        fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
    }

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

    if (saved_flags != -1) {
        /* the shell gets its blocking stdin back */
        fcntl(STDIN_FILENO, F_SETFL, saved_flags);
        saved_flags = -1;
    }

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

void term_beep(void)
{
    putchar('\a');
    fflush(stdout);
}

void term_draw(const char *chars, const unsigned char *colors, int width, int height)
{
    int x, y;

    for (y = 0; y < height; y++) {
        int current = -1;

        printf("\033[%d;1H", y + 1);
        for (x = 0; x < width; x++) {
            int color = colors[y * width + x];

            if (color != current) {
                printf("\033[%dm", color != 0 ? color : 0);
                current = color;
            }
            putchar(chars[y * width + x]);
        }
        printf("\033[0m");
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
    in->letter = -1;

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

        if (c >= 'a' && c <= 'z')
            in->letter = c - 'a' + 'A';
        else if (c >= 'A' && c <= 'Z')
            in->letter = c;

        switch (c) {
        case 'w': case 'W': in->up = 1; break;
        case 's': case 'S': in->down = 1; break;
        case 'a': case 'A': in->left = 1; break;
        case 'd': case 'D': in->right = 1; break;
        case 'p': case 'P': in->pause = 1; break;
        case 'r': case 'R': in->restart = 1; break;
        case 'q': case 'Q': in->back = 1; break;
        case 't': case 'T': in->theme = 1; break;
        case 'm': case 'M': in->sound = 1; break;
        case 'f': case 'F': in->fps = 1; break;
        case 'h': case 'H': in->help = 1; break;
        case 'i': case 'I': in->info = 1; break;
        case ' ': in->fire = 1; break;
        case 127: case 8: in->backspace = 1; break;
        case '\r': case '\n': in->enter = 1; break;
        default:
            if (c >= '0' && c <= '9')
                in->digit = c - '0';
            break;
        }
    }
}
