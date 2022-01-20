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

void term_init(void);
void term_shutdown(void);
void term_size(int *width, int *height);
void term_read_input(TermInput *in);
void term_draw(const char *chars, const unsigned char *colors, int width, int height);
double term_time(void);
void term_sleep(double seconds);
int term_should_stop(void);

#endif
