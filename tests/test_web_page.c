#include <string.h>

#include "test.h"
#include "theme.h"
#include "web_page.h"

void test_web_page(void)
{
    char out[4096], small[64];
    unsigned char colors[80 * 23];
    int n;
    memset(colors, 97, sizeof(colors));

    n = web_frame_document(out, sizeof(out), "ABCD", colors, 2, 2);
    CHECK(n > 0);
    CHECK(strstr(out, "AB\nCD</FONT>") != NULL);
    CHECK(strstr(out, "BODY BGCOLOR=\"#FFFFFF\" TEXT=\"#000000\"") != NULL);
    CHECK(strstr(out, "TABLE BGCOLOR=\"#000000\"") != NULL);
    CHECK(strstr(out, "<FONT COLOR=\"#FFFFFF\">") != NULL);
    CHECK(strstr(out, "FACE=\"Courier New, Courier, monospace\"") != NULL);
    CHECK((int)strlen(out) == n);

    /* Each cell's terminal color survives, without changing character spacing. */
    colors[0] = 91; colors[1] = 92; colors[2] = 94; colors[3] = 97;
    n = web_frame_document(out, sizeof(out), "<&>\"", colors, 4, 1);
    CHECK(n > 0);
    CHECK(strstr(out, "<FONT COLOR=\"#FF5555\">&lt;</FONT>") != NULL);
    CHECK(strstr(out, "<FONT COLOR=\"#55FF55\">&amp;</FONT>") != NULL);
    CHECK(strstr(out, "<FONT COLOR=\"#5555FF\">&gt;</FONT>") != NULL);
    CHECK(strstr(out, "<FONT COLOR=\"#FFFFFF\">&quot;</FONT>") != NULL);
    CHECK(strstr(out, "<&") == NULL);

    CHECK(web_frame_document(out, sizeof(out), "A", colors, 0, 1) == -1);
    CHECK(web_frame_document(out, sizeof(out), "A", colors, 1, 0) == -1);
    CHECK(web_frame_document(out, sizeof(out), "A", colors, -3, 1) == -1);
    CHECK(web_frame_document(out, sizeof(out), "A", colors, 2147483647, 2147483647) == -1);
    CHECK(web_frame_document(out, 0, "A", colors, 1, 1) == -1);
    CHECK(web_frame_document(out, sizeof(out), NULL, colors, 1, 1) == -1);
    CHECK(web_frame_document(out, sizeof(out), "A", NULL, 1, 1) == -1);
    CHECK(web_frame_document(small, sizeof(small), "ABCDEFGH", colors, 8, 1) == -1);
    n = web_frame_document(out, sizeof(out), "A", colors, 1, 1);
    CHECK(n > 0);
    CHECK(web_frame_document(out, (size_t)n + 1, "A", colors, 1, 1) == n);
    CHECK(web_frame_document(out, (size_t)n, "A", colors, 1, 1) == -1);

    /* Worst case: every cell switches color and requires HTML/JS escaping. */
    {
        char chars[80 * 23];
        char big[WEB_PAGE_CAPACITY];
        size_t i;
        memset(chars, '<', sizeof(chars));
        for (i = 0; i < sizeof(colors); i++)
            colors[i] = i % 2 ? 91 : 92;
        CHECK(web_frame_document(big, sizeof(big), chars, colors, 80, 23) > 0);
        CHECK(web_update_document(big, sizeof(big), chars, colors, 80, 23, 1, 1, 0) > 0);
        CHECK(strstr(big, "gameScreen().innerHTML") != NULL);
    }

    /* Every native theme role is represented by a valid HTML color. */
    {
        int t, role;
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
    CHECK(strcmp(web_html_color(0), "#FFFFFF") == 0);
    CHECK(strcmp(web_html_color(255), "#FFFFFF") == 0);
    CHECK(strcmp(web_html_color(97), "#FFFFFF") == 0);
    CHECK(strcmp(web_html_color(31), "#CC0000") == 0);
}
