#include "theme.h"

#include <stddef.h>

typedef struct {
    const char *name;
    unsigned char colors[THEME_COUNT];
} Theme;

static const Theme themes[] = {
    {
        "classic",
        {
            90, 37, 97, 96,
            94, 92, 96,
            91, 31, 33,
            97, 96, 33,
            96, 97, 90,
            92, 93, 91,
            96, 96
        }
    },
    {
        "amber",
        {
            90, 33, 93, 91,
            33, 93, 31,
            91, 31, 33,
            97, 93, 33,
            93, 97, 90,
            93, 93, 91,
            33, 93
        }
    },
    {
        "mono",
        {
            90, 37, 97, 97,
            37, 90, 37,
            97, 90, 37,
            97, 97, 37,
            97, 97, 90,
            97, 97, 97,
            37, 97
        }
    }
};

static int active_theme = 0;

void theme_set(int index)
{
    active_theme = index;
    if (active_theme < 0)
        active_theme = 0;
    if (active_theme >= theme_count())
        active_theme = theme_count() - 1;
}

int theme_count(void)
{
    return (int)(sizeof(themes) / sizeof(themes[0]));
}

unsigned char theme_color(ThemeRole role)
{
    if (role < 0 || role >= THEME_COUNT)
        return 0;
    return themes[active_theme].colors[role];
}

const char *theme_name(int index)
{
    if (index < 0 || index >= theme_count())
        return themes[0].name;
    return themes[index].name;
}
