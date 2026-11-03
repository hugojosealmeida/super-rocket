#ifndef SUPER_ROCKET_WEB_PAGE_H
#define SUPER_ROCKET_WEB_PAGE_H

#include <stddef.h>

/* Includes worst-case per-cell color changes and JavaScript string escaping. */
#define WEB_PAGE_CAPACITY 131072

/*
 * Converts an ANSI color code from the theme (30-37, 90-97) into an HTML
 * color such as "#FFFFFF".  Unknown codes map to the default text color.
 */
const char *web_html_color(unsigned char ansi);

/*
 * Writes the page the browser opens first.  It frames the live screen at
 * /play and /frame. Returns the length written, or -1 when out is too small.
 */
int web_index_document(char *out, size_t size);

int web_play_document(char *out, size_t size, const char *token);
int web_update_document(char *out, size_t size, const char *chars,
                        const unsigned char *colors, int width,
                        int height, unsigned long version, int changed,
                        int beep);

/*
 * Writes one HTML 3.2 document that shows the frame (width x height
 * characters) in a black monospace PRE panel on a white page. Each cell uses
 * the C renderer's ANSI color, converted to legacy FONT markup. HTML special
 * characters are escaped. Returns the length written,
 * or -1 when the size is invalid or out is too small.
 */
int web_frame_document(char *out, size_t size, const char *chars,
                       const unsigned char *colors, int width, int height);

#endif
