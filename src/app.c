#include "app.h"

#include <stdio.h>
#include <string.h>

#include "render.h"

void app_handle_input(Game *g, const TermInput *in, int *running)
{
    if (in->quit) {
        *running = 0;
        return;
    }

    /* Initials may contain shortcut letters such as H, I, T and P. */
    if (g->state == GAME_INITIALS) {
        if (in->letter >= 'A' && in->letter <= 'Z')
            game_initial_push(g, (char)in->letter);
        else if (in->backspace)
            game_initial_pop(g);
        else if (in->enter)
            game_commit_initials(g);
        return;
    }

    if (in->help) {
        game_toggle_help(g);
        return;
    }

    if (in->info) {
        game_toggle_scores(g);
        return;
    }

    if (g->show_help || g->show_scores)
        return;

    if (in->theme)
        game_cycle_theme(g);
    if (in->sound)
        game_toggle_sound(g);
    if (in->fps)
        game_toggle_fps(g);

    switch (g->state) {
    case GAME_MENU:
        if (in->back) {
            *running = 0;
        } else if (in->enter) {
            game_start(g);
        } else if (in->digit >= 1 && in->digit <= DIFFICULTY_COUNT) {
            game_set_difficulty(g, (Difficulty)(in->digit - 1));
        } else if (in->letter == 'X') {
            game_reset_stats(g);
            snprintf(g->toast, sizeof(g->toast), "STATISTICS RESET");
            g->toast_timer = 2.5;
        }
        break;

    case GAME_PLAYING:
        if (in->back) {
            game_to_menu(g);
        } else if (in->pause) {
            game_pause(g);
        } else if (in->restart) {
            game_start(g);
        } else {
            game_thrust(g, in->up, in->down, in->left, in->right);
            if (in->fire)
                game_fire(g);
        }
        break;

    case GAME_PAUSED:
        if (in->back) {
            game_to_menu(g);
        } else if (in->pause || in->enter) {
            game_resume(g);
        } else if (in->restart) {
            game_start(g);
        }
        break;

    case GAME_INITIALS:
        break; /* handled before shortcuts */

    case GAME_UPGRADE:
        if (in->digit >= 1 && in->digit <= 3)
            game_choose_upgrade(g, in->digit - 1);
        else if (in->enter)
            game_choose_upgrade(g, 0);
        break;

    case GAME_WON:
    case GAME_DEAD:
        if (in->restart) {
            game_start(g);
        } else if (in->back || in->enter) {
            game_to_menu(g);
        }
        break;
    }
}

void app_step(Game *g)
{
    game_update(g);
    render_frame(g);
}
