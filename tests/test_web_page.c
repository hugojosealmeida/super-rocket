#include <string.h>

#include "test.h"
#include "theme.h"
#include "web_page.h"

void test_web_page(void)
{
    char out[4096];
    char small[64];
    const char frame[] = "AB CD";
    int n;

    /* happy path: a 2x2 frame in a textarea with the requested size */
    n = web_frame_document(out, sizeof(out), "ABCD", 2, 2, 97);
    CHECK(n > 0);
    CHECK(strstr(out, "COLS=2 ROWS=2") != NULL);
    CHECK(strstr(out, "AB\nCD</TEXTAREA>") != NULL);
    CHECK(strstr(out, "TEXT=\"#FFFFFF\"") != NULL);
    CHECK(strstr(out, "BGCOLOR=\"#000000\"") != NULL);
    CHECK(strstr(out, "<FORM>") != NULL);
    CHECK((int)strlen(out) == n);

    /* the frame may contain spaces and be a single row */
    n = web_frame_document(out, sizeof(out), frame, 5, 1, 37);
    CHECK(n > 0);
    CHECK(strstr(out, "COLS=5 ROWS=1") != NULL);
    CHECK(strstr(out, "AB CD</TEXTAREA>") != NULL);
    CHECK(strstr(out, "TEXT=\"#C0C0C0\"") != NULL);

    /* edge: HTML special characters are escaped, never emitted raw */
    n = web_frame_document(out, sizeof(out), "<&>\"", 4, 1, 97);
    CHECK(n > 0);
    CHECK(strstr(out, "&lt;&amp;&gt;&quot;</TEXTAREA>") != NULL);
    CHECK(strstr(out, "<&") == NULL);

    /* edge: invalid sizes are refused */
    CHECK(web_frame_document(out, sizeof(out), "A", 0, 1, 97) == -1);
    CHECK(web_frame_document(out, sizeof(out), "A", 1, 0, 97) == -1);
    CHECK(web_frame_document(out, sizeof(out), "A", -3, 1, 97) == -1);
    CHECK(web_frame_document(out, 0, "A", 1, 1, 97) == -1);

    /* edge: output buffer too small is refused, not truncated */
    CHECK(web_frame_document(small, sizeof(small), "ABCDEFGH", 8, 1, 97) == -1);

    /* edge: a full game-sized frame fits in the server's buffer */
    {
        char chars[80 * 23];
        char big[16384];

        memset(chars, '<', sizeof(chars));
        n = web_frame_document(big, sizeof(big), chars, 80, 23, 97);
        CHECK(n > 0);
        CHECK(strstr(big, "COLS=80 ROWS=23") != NULL);
    }

    /* colors: every theme color maps to a 7-character HTML color */
    {
        int t;
        int role;

        for (t = 0; t < theme_count(); t++) {
            theme_set(t);
            for (role = 0; role < THEME_COUNT; role++) {
                const char *html = web_html_color(theme_color((ThemeRole)role));

                CHECK(strlen(html) == 7);
                CHECK(html[0] == '#');
            }
        }
        theme_set(0);
    }

    /* edge: unknown color codes fall back to the default text color */
    CHECK(strcmp(web_html_color(0), "#FFFFFF") == 0);
    CHECK(strcmp(web_html_color(255), "#FFFFFF") == 0);
    CHECK(strcmp(web_html_color(97), "#FFFFFF") == 0);
    CHECK(strcmp(web_html_color(31), "#CC0000") == 0);
}
