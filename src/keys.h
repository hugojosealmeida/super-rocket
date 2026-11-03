#ifndef SUPER_ROCKET_KEYS_H
#define SUPER_ROCKET_KEYS_H

#include "term.h"

/*
 * Keyboard state for the web frontend.  Browsers report key down and key up
 * events with a key code (the same codes as the JavaScript keyCode/which
 * values: 37-40 for arrows, 'A'-'Z' for letters, 32 for Space).  This module
 * turns them into the TermInput the terminal frontend produces.
 */

#define KEYS_CODE_COUNT 256

void keys_reset(void);

/* Returns 1 when the code is a key the game uses, 0 otherwise. */
int keys_down(int code);

void keys_up(int code);

/* Releases every held key, for example when the browser window loses focus. */
void keys_release_all(void);

/* Fills in the input for this frame and clears the one-shot events. */
void keys_take(TermInput *in);

#endif
