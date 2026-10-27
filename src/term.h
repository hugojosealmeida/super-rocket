#ifndef SUPER_ROCKET_TERM_H
#define SUPER_ROCKET_TERM_H

typedef struct {
    int up;
    int down;
    int left;
    int right;
    int enter;
    int pause;
    int restart;
    int back;
    int quit;
    int digit;
    int fire;
    int help;
    int info;
    int theme;
    int sound;
    int fps;
    int backspace;
    int letter;
} TermInput;

/* Below this the frame would wrap and become unreadable garbage. */
#define TERM_MIN_COLS 40
#define TERM_MIN_ROWS 12

#define TERM_SIZE_OK 0
#define TERM_SIZE_TOO_SMALL 1

void term_init(void);
void term_shutdown(void);

/*
 * Picks the canvas the game draws into.  Returns TERM_SIZE_OK with *width
 * and *height filled: the terminal's own size when ioctl(TIOCGWINSZ) works,
 * or the 80x23 default when the size cannot be queried (stdout is no tty,
 * e.g. a pipe in CI).  Returns TERM_SIZE_TOO_SMALL when the terminal reports
 * fewer than TERM_MIN_COLS columns or TERM_MIN_ROWS rows; then *width and
 * *height hold the actual size so the caller can say what was found.  A tty
 * that reports 0x0 (an unresized container terminal) counts as too small:
 * only a failing ioctl keeps the fallback.
 */
int term_size(int *width, int *height);
void term_read_input(TermInput *in);
void term_beep(void);
void term_draw(const char *chars, const unsigned char *colors, int width, int height);
double term_time(void);
void term_sleep(double seconds);
int term_should_stop(void);

#endif
