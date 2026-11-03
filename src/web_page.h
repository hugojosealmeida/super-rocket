#ifndef SUPER_ROCKET_WEB_PAGE_H
#define SUPER_ROCKET_WEB_PAGE_H

#include <stddef.h>

/*
 * Converts an ANSI color code from the theme (30-37, 90-97) into an HTML
 * color such as "#FFFFFF".  Unknown codes map to the default text color.
 */
const char *web_html_color(unsigned char ansi);

/*
 * Writes the page the browser opens first.  It frames the live screen at
 * /screen.  Returns the length written, or -1 when out is too small.
 */
int web_index_document(char *out, size_t size);

int web_play_document(char *out, size_t size, const char *token);
int web_update_document(char *out, size_t size, const char *chars, int width,
                        int height, unsigned long version, int changed,
                        unsigned char text_color, int beep);

/*
 * Writes one HTML 3.2 document that shows the frame (width x height
 * characters) in a monospace textarea.  Characters that are special in HTML
 * are escaped.  text_color is the ANSI color used for the whole frame,
 * because a textarea has a single text color.  Returns the length written,
 * or -1 when the size is invalid or out is too small.
 */
int web_frame_document(char *out, size_t size, const char *chars, int width, int height,
                       unsigned char text_color);

#endif
