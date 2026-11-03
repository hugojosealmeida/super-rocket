#ifndef SUPER_ROCKET_APP_H
#define SUPER_ROCKET_APP_H

#include "game.h"
#include "term.h"

/* Applies one frame of input to the game state, shared by every frontend. */
void app_handle_input(Game *g, const TermInput *in, int *running);

/* Advances the simulation by one frame and draws it. */
void app_step(Game *g);

#endif
