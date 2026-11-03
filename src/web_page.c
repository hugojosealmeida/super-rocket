#include "web_page.h"

#include <stdio.h>
#include <string.h>

typedef struct {
    unsigned char ansi;
    const char *html;
} ColorEntry;

static const ColorEntry colors[] = {
    { 30, "#000000" },
    { 31, "#CC0000" },
    { 32, "#00CC00" },
    { 33, "#CCCC00" },
    { 34, "#0000CC" },
    { 35, "#CC00CC" },
    { 36, "#00CCCC" },
    { 37, "#C0C0C0" },
    { 90, "#808080" },
    { 91, "#FF5555" },
    { 92, "#55FF55" },
    { 93, "#FFFF55" },
    { 94, "#5555FF" },
    { 95, "#FF55FF" },
    { 96, "#55FFFF" },
    { 97, "#FFFFFF" }
};

#define DEFAULT_TEXT "#FFFFFF"

const char *web_html_color(unsigned char ansi)
{
    size_t i;

    for (i = 0; i < sizeof(colors) / sizeof(colors[0]); i++) {
        if (colors[i].ansi == ansi)
            return colors[i].html;
    }
    return DEFAULT_TEXT;
}

typedef struct {
    char *pos;
    size_t left;
    int ok;
} Writer;

static void put(Writer *w, const char *s)
{
    size_t n = strlen(s);

    if (!w->ok)
        return;
    if (n >= w->left) {
        w->ok = 0;
        return;
    }
    memcpy(w->pos, s, n);
    w->pos += n;
    w->left -= n;
    *w->pos = '\0';
}

static void put_char(Writer *w, char c)
{
    char buf[2];

    buf[0] = c;
    buf[1] = '\0';
    put(w, buf);
}

static void put_escaped(Writer *w, char c)
{
    switch (c) {
    case '<': put(w, "&lt;"); break;
    case '>': put(w, "&gt;"); break;
    case '&': put(w, "&amp;"); break;
    case '"': put(w, "&quot;"); break;
    default: put_char(w, c); break;
    }
}

int web_frame_document(char *out, size_t size, const char *chars, int width, int height,
                       unsigned char text_color)
{
    Writer w;
    char head[256];
    int x;
    int y;

    if (width <= 0 || height <= 0 || size == 0)
        return -1;

    snprintf(head, sizeof(head),
             "<HTML>\n<HEAD>\n<TITLE>super-rocket</TITLE>\n</HEAD>\n"
             "<BODY BGCOLOR=\"#000000\" TEXT=\"%s\">\n"
             "<FORM>\n<TEXTAREA NAME=\"screen\" COLS=%d ROWS=%d WRAP=OFF>",
             web_html_color(text_color), width, height);

    w.pos = out;
    w.left = size;
    w.ok = 1;
    out[0] = '\0';

    put(&w, head);
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++)
            put_escaped(&w, chars[y * width + x]);
        if (y + 1 < height)
            put_char(&w, '\n');
    }
    put(&w, "</TEXTAREA>\n</FORM>\n</BODY>\n</HTML>\n");

    if (!w.ok)
        return -1;
    return (int)(w.pos - out);
}
