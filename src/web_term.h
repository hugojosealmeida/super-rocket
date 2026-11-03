#ifndef SUPER_ROCKET_WEB_TERM_H
#define SUPER_ROCKET_WEB_TERM_H

/* The web screen matches the default terminal canvas. */
#define WEB_TERM_WIDTH 80
#define WEB_TERM_HEIGHT 23
#define WEB_TERM_MAX_CELLS (WEB_TERM_WIDTH * WEB_TERM_HEIGHT)

/*
 * Returns the latest frame the game drew.  version increases whenever the
 * drawn content changes, so the server only sends new frames.
 */
void web_term_latest(const char **chars, const unsigned char **colors, int *width,
                     int *height, unsigned long *version);

/* Returns 1 once after the game asked for a sound, then 0. */
int web_term_take_beep(void);

#endif
