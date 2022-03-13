#include <string.h>

#include "test.h"
#include "theme.h"

void test_theme(void)
{
    int i;

    CHECK(theme_count() == 3);
    CHECK(strcmp(theme_name(0), "classic") == 0);
    CHECK(strcmp(theme_name(1), "amber") == 0);
    CHECK(strcmp(theme_name(2), "mono") == 0);
    CHECK(strcmp(theme_name(99), "classic") == 0);

    for (i = 0; i < theme_count(); i++) {
        theme_set(i);
        CHECK(theme_color(THEME_ROCKET) != 0);
        CHECK(theme_color(THEME_TEXT) != 0);
        CHECK(theme_color(THEME_BAD) != 0);
    }

    theme_set(99);
    CHECK(theme_color(THEME_ROCKET) != 0);
    theme_set(0);
    CHECK(theme_color((ThemeRole)-1) == 0);
}
