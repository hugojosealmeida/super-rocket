#include "keys.h"

#include <string.h>

static unsigned char held[KEYS_CODE_COUNT];
static TermInput pending;

static void clear_pending(void)
{
    memset(&pending, 0, sizeof(pending));
    pending.digit = -1;
    pending.letter = -1;
}

void keys_reset(void)
{
    memset(held, 0, sizeof(held));
    clear_pending();
}

/* Records a press of a one-shot key; held keys are handled in keys_take(). */
static int press_event(int code)
{
    if (code >= 'A' && code <= 'Z') {
        pending.letter = code;
        switch (code) {
        case 'P': pending.pause = 1; break;
        case 'R': pending.restart = 1; break;
        case 'Q': pending.back = 1; break;
        case 'T': pending.theme = 1; break;
        case 'M': pending.sound = 1; break;
        case 'F': pending.fps = 1; break;
        case 'H': pending.help = 1; break;
        case 'I': pending.info = 1; break;
        default: break;
        }
        return 1;
    }

    switch (code) {
    case 13: pending.enter = 1; return 1;
    case 27: pending.quit = 1; return 1;
    case 8: pending.backspace = 1; return 1;
    case 49: case 50: case 51:
        pending.digit = code - '0';
        return 1;
    default:
        return 0;
    }
}

static int is_known(int code)
{
    switch (code) {
    case 37: case 38: case 39: case 40:
    case 32: case 13: case 27: case 8:
    case 49: case 50: case 51:
        return 1;
    default:
        return code >= 'A' && code <= 'Z';
    }
}

int keys_down(int code)
{
    if (code < 0 || code >= KEYS_CODE_COUNT || !is_known(code))
        return 0;

    if (held[code])
        return 1;
    held[code] = 1;

    press_event(code);
    return 1;
}

void keys_up(int code)
{
    if (code >= 0 && code < KEYS_CODE_COUNT)
        held[code] = 0;
}

void keys_release_all(void)
{
    memset(held, 0, sizeof(held));
}

void keys_take(TermInput *in)
{
    memset(in, 0, sizeof(*in));
    in->digit = -1;
    in->letter = -1;

    in->up = held[38] || held[87];
    in->down = held[40] || held[83];
    in->left = held[37] || held[65];
    in->right = held[39] || held[68];
    in->fire = held[32];

    in->pause = pending.pause;
    in->restart = pending.restart;
    in->back = pending.back;
    in->theme = pending.theme;
    in->sound = pending.sound;
    in->fps = pending.fps;
    in->help = pending.help;
    in->info = pending.info;
    in->enter = pending.enter;
    in->quit = pending.quit;
    in->backspace = pending.backspace;
    if (pending.digit >= 0)
        in->digit = pending.digit;
    if (pending.letter >= 0)
        in->letter = pending.letter;

    clear_pending();
}
