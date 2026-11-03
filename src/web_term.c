#if !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 200809L
#endif

#include "web_term.h"

#include <string.h>
#include <time.h>

#include "keys.h"
#include "term.h"

static char frame_chars[WEB_TERM_MAX_CELLS];
static unsigned char frame_colors[WEB_TERM_MAX_CELLS];
static int frame_width;
static int frame_height;
static unsigned long frame_version;
static int beep_pending;

void web_term_latest(const char **chars, const unsigned char **colors, int *width,
                     int *height, unsigned long *version)
{
    *chars = frame_chars;
    *colors = frame_colors;
    *width = frame_width;
    *height = frame_height;
    *version = frame_version;
}

int web_term_take_beep(void)
{
    int pending = beep_pending;

    beep_pending = 0;
    return pending;
}

void term_init(void)
{
    keys_reset();
    frame_width = frame_height = 0;
    frame_version = 0;
    beep_pending = 0;
}

void term_shutdown(void)
{
}

int term_size(int *width, int *height)
{
    *width = WEB_TERM_WIDTH;
    *height = WEB_TERM_HEIGHT;
    return TERM_SIZE_OK;
}

void term_read_input(TermInput *in)
{
    keys_take(in);
}

void term_beep(void)
{
    beep_pending = 1;
}

void term_draw(const char *chars, const unsigned char *colors, int width, int height)
{
    int cells;

    if (width <= 0 || height <= 0 || width > WEB_TERM_WIDTH || height > WEB_TERM_HEIGHT)
        return;

    cells = width * height;
    if (width == frame_width && height == frame_height &&
        memcmp(frame_chars, chars, (size_t)cells) == 0 &&
        memcmp(frame_colors, colors, (size_t)cells) == 0)
        return;

    memcpy(frame_chars, chars, (size_t)cells);
    memcpy(frame_colors, colors, (size_t)cells);
    frame_width = width;
    frame_height = height;
    frame_version++;
}

double term_time(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

void term_sleep(double seconds)
{
    struct timespec ts;

    if (seconds <= 0.0)
        return;

    ts.tv_sec = (time_t)seconds;
    ts.tv_nsec = (long)((seconds - (double)ts.tv_sec) * 1e9);
    nanosleep(&ts, NULL);
}

int term_should_stop(void)
{
    return 0;
}
