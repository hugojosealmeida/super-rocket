#include "web_page.h"

#include <stdio.h>
#include <string.h>
#include "game.h"

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

/*
 * Forwards key presses to the server with image requests, so the page never
 * navigates away. Legacy ES3 syntax; keyboard APIs varied across historical
 * browsers (see docs/BROWSER_COMPATIBILITY.md). Arrows and Space return false
 * so the page does not scroll.
 */
static const char key_script[] =
    "<SCRIPT LANGUAGE=\"JavaScript\">\n"
    "<!--\n"
    "function keyCodeOf(e) {\n"
    "  if (e && e.which) return e.which;\n"
    "  if (window.event && window.event.keyCode) return window.event.keyCode;\n"
    "  return 0;\n"
    "}\n"
    "var held = new Array(), queue = new Array(), busy = false, image = null;\n"
    "var lastFrame = (new Date()).getTime();\n"
    "function gameScreen() {\n"
    "  if (document.getElementById) return document.getElementById('screen');\n"
    "  if (document.all) return document.all.screen;\n"
    "  return null;\n"
    "}\n"
    "function pump() {\n"
    "  if (busy || queue.length == 0) return;\n"
    "  busy = true; image = new Image();\n"
    "  image.onload = image.onerror = sent;\n"
    "  image.src = queue[0];\n"
    "}\n"
    "function sent() {\n"
    "  var i; for (i = 1; i < queue.length; i++) queue[i-1] = queue[i];\n"
    "  queue.length--; busy = false; pump();\n"
    "}\n"
    "function keySend(kind, code) {\n"
    "  if (queue.length > 64) return;\n"
    "  queue[queue.length] = \"/key?t=\" + token + \"&\" + kind + \"=\" + code + \"&r=\" + Math.random();\n"
    "  pump();\n"
    "}\n"
    "function keyDown(e) {\n"
    "  var code = keyCodeOf(e);\n"
    "  if (!code) return true;\n"
    "  if (!((code >= 65 && code <= 90) || (code >= 37 && code <= 40) ||\n"
    "      (code >= 49 && code <= 51) || code == 8 || code == 13 || code == 27 || code == 32)) return true;\n"
    "  if (!held[code]) { held[code] = true; keySend(\"d\", code); }\n"
    "  return false;\n"
    "}\n"
    "function keyUp(e) {\n"
    "  var code = keyCodeOf(e);\n"
    "  if (held[code]) { held[code] = false; keySend(\"u\", code); return false; }\n"
    "  return true;\n"
    "}\n"
    "function keyBlur() {\n"
    "  held = new Array();\n"
    "  keySend(\"clear\", 1);\n"
    "  return true;\n"
    "}\n"
    "document.onkeydown = keyDown;\n"
    "document.onkeyup = keyUp;\n"
    "window.onblur = keyBlur;\n"
    "if (document.captureEvents && window.Event) document.captureEvents(Event.KEYDOWN | Event.KEYUP);\n"
    "function heartbeat() {\n"
    "  var i; for (i = 0; i < 256; i++) if (held[i]) keySend(\"d\", i);\n"
    "  var width = window.innerWidth || (document.body ? document.body.clientWidth : 0);\n"
    "  if (document.forms[0]) {\n"
    "    if ((new Date()).getTime() - lastFrame > 3000)\n"
    "      document.forms[0].connection.value = 'No recent frames. Check the C server, then reload this page.';\n"
    "    else if (width && width < 700)\n"
    "      document.forms[0].connection.value = 'Window too narrow: widen it or scroll to see all 80 columns.';\n"
    "    else document.forms[0].connection.value = 'Connected. Click the screen and press Enter to launch.';\n"
    "  }\n"
    "  setTimeout(\"heartbeat()\", 500);\n"
    "}\n"
    "setTimeout(\"heartbeat()\", 500);\n"
    "// -->\n"
    "</SCRIPT>\n";

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

int web_index_document(char *out, size_t size)
{
    static const char index[] =
        "<HTML>\n<HEAD>\n<TITLE>super-rocket</TITLE>\n</HEAD>\n"
        "<FRAMESET ROWS=\"*,1\">\n"
        "<FRAME SRC=\"/play\" NAME=\"play\">\n"
        "<FRAME SRC=\"/frame\" NAME=\"transport\">\n"
        "<NOFRAMES><BODY>This game needs frames and JavaScript. "
        "You can also run ./super-rocket in a terminal.</BODY></NOFRAMES>\n"
        "</FRAMESET>\n"
        "</HTML>\n";

    if (size < sizeof(index))
        return -1;
    memcpy(out, index, sizeof(index));
    return (int)sizeof(index) - 1;
}

static int frame_markup(char *out, size_t size, const char *chars,
                         const unsigned char *colors, int width, int height)
{
    Writer w;
    int x, y;
    const char *current = NULL;
    if (width <= 0 || height <= 0 || width > 80 || height > 23 || size == 0 ||
        chars == NULL || colors == NULL)
        return -1;
    w.pos = out; w.left = size; w.ok = 1; out[0] = '\0';
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            int i = y * width + x;
            const char *color = web_html_color(colors[i]);
            if (current == NULL || strcmp(current, color) != 0) {
                if (current != NULL)
                    put(&w, "</FONT>");
                put(&w, "<FONT COLOR=\"");
                put(&w, color);
                put(&w, "\">");
                current = color;
            }
            put_escaped(&w, chars[i]);
        }
        if (y + 1 < height)
            put_char(&w, '\n');
    }
    put(&w, "</FONT>");
    return w.ok ? (int)(w.pos - out) : -1;
}

int web_frame_document(char *out, size_t size, const char *chars,
                       const unsigned char *colors, int width, int height)
{
    Writer w;
    char markup[65536];
    if (size == 0 || frame_markup(markup, sizeof(markup), chars, colors, width, height) < 0)
        return -1;
    w.pos = out; w.left = size; w.ok = 1; out[0] = '\0';
    put(&w, "<HTML><HEAD><TITLE>super-rocket</TITLE></HEAD>"
        "<BODY BGCOLOR=\"#FFFFFF\" TEXT=\"#000000\"><CENTER>"
        "<TABLE BGCOLOR=\"#000000\" BORDER=0 CELLPADDING=8 CELLSPACING=0>"
        "<TR><TD NOWRAP><PRE><FONT FACE=\"Courier New, Courier, monospace\" SIZE=3>");
    put(&w, markup);
    put(&w, "</FONT></PRE></TD></TR></TABLE></CENTER></BODY></HTML>\n");
    return w.ok ? (int)(w.pos - out) : -1;
}

int web_play_document(char *out, size_t size, const char *token)
{
    Writer w;
    const char *p;
    if (size == 0)
        return -1;
    /* Token is inserted into JavaScript, so only accept hexadecimal capabilities. */
    if (strlen(token) != 32)
        return -1;
    for (p = token; *p; p++)
        if (!((*p >= '0' && *p <= '9') || (*p >= 'a' && *p <= 'f')))
            return -1;
    w.pos = out; w.left = size; w.ok = 1; out[0] = '\0';
    put(&w, "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 3.2 Final//EN\">\n"
        "<HTML><HEAD><TITLE>super-rocket</TITLE>\n<SCRIPT LANGUAGE=\"JavaScript\">"
        "var token = \"");
    put(&w, token);
    put(&w, "\";</SCRIPT>\n");
    put(&w, key_script);
    put(&w, "</HEAD><BODY BGCOLOR=\"#FFFFFF\" TEXT=\"#000000\">"
        "<CENTER><H1>SUPER ROCKET</H1><P>EARTH -&gt; MARS</P>"
        "<P>Click this page, then press Enter to launch. JavaScript must be enabled.</P>"
        "<FORM ACTION=\"/play\"><P><INPUT TYPE=TEXT NAME=\"connection\" SIZE=74 "
        "VALUE=\"Waiting for frames. Enable JavaScript; keep the C server running.\"></P></FORM>"
        "<TABLE BGCOLOR=\"#000000\" BORDER=0 CELLPADDING=8 CELLSPACING=0>"
        "<TR><TD NOWRAP><PRE><FONT FACE=\"Courier New, Courier, monospace\" SIZE=3 COLOR=\"#FFFFFF\">"
        "<SPAN ID=\"screen\">"
        "Connecting to the C game server... If this stays visible, frames or JavaScript may be disabled."
        "</SPAN></FONT></PRE></TD></TR></TABLE>"
        "<TABLE BORDER=1><CAPTION>Keyboard controls</CAPTION>"
        "<TR><TH>Key</TH><TH>Action</TH></TR>"
        "<TR><TD>Arrows / WASD</TD><TD>Thrust</TD></TR>"
        "<TR><TD>Space / Enter</TD><TD>Fire / Launch</TD></TR>"
        "<TR><TD>P / R / Q / Esc</TD><TD>Pause / Restart / Back / Menu</TD></TR>"
        "<TR><TD>T / M / F</TD><TD>Theme / Sound preference / FPS</TD></TR>"
        "<TR><TD>H / I</TD><TD>Help / Leaderboard</TD></TR>"
        "<TR><TD>1 2 3 / X</TD><TD>Difficulty or upgrade / Reset statistics</TD></TR>"
        "<TR><TD>Letters / Backspace</TD><TD>Initials / Delete</TD></TR>"
        "</TABLE><P>One shared game. C server, no plugins, no canvas.</P>"
        "<P>super-rocket " GAME_VERSION " - MIT</P></CENTER></BODY></HTML>\n");
    return w.ok ? (int)(w.pos - out) : -1;
}

int web_update_document(char *out, size_t size, const char *chars,
                        const unsigned char *colors, int width,
                        int height, unsigned long version, int changed,
                        int beep)
{
    Writer w;
    char tail[1024];
    char markup[65536];
    int i, n = 0;
    if (size == 0 || width < 0 || height < 0 || width > 80 || height > 23)
        return -1;
    w.pos = out; w.left = size; w.ok = 1; out[0] = '\0';
    put(&w, "<HTML><HEAD><TITLE>Frame transport</TITLE></HEAD><BODY>"
        "<SCRIPT LANGUAGE=\"JavaScript\">\n<!--\n"
        "function update() {\n"
        "  var p = parent.frames[0];\n"
        "  if (!p || !p.gameScreen || !p.gameScreen()) { setTimeout(\"location.replace('/frame')\", 100); return; }\n");
    put(&w, "  p.lastFrame = (new Date()).getTime();\n");
    if (changed) {
        n = frame_markup(markup, sizeof(markup), chars, colors, width, height);
        if (n < 0)
            return -1;
        put(&w, "  p.gameScreen().innerHTML = \"");
        for (i = 0; i < n; i++) {
            unsigned char c = (unsigned char)markup[i];
            if (c == '"' || c == '\\') {
                put_char(&w, '\\'); put_char(&w, (char)c);
            } else if (c == '\n') {
                put(&w, "\\n");
            } else if (c == '<' || c == '>' || c == '&' || c < 32 || c > 126) {
                char escaped[8];
                snprintf(escaped, sizeof(escaped), "\\x%02x", c);
                put(&w, escaped);
            } else {
                put_char(&w, (char)c);
            }
        }
        put(&w, "\";\n");
    }
    if (beep)
        put(&w, "  p.gameScreen().innerHTML += '\\x07';\n");
    snprintf(tail, sizeof(tail),
        "  setTimeout(\"location.replace('/frame?v=%lu&r=\" + Math.random() + \"')\", 100);\n"
        "}\nupdate();\n// -->\n</SCRIPT></BODY></HTML>\n", version);
    put(&w, tail);
    return w.ok ? (int)(w.pos - out) : -1;
}
