#ifndef SUPER_ROCKET_THEME_H
#define SUPER_ROCKET_THEME_H

typedef enum {
    THEME_STAR_FAR,
    THEME_STAR_MID,
    THEME_STAR_BRIGHT,
    THEME_STAR_ACCENT,
    THEME_EARTH,
    THEME_EARTH_DETAIL,
    THEME_EARTH_EDGE,
    THEME_MARS,
    THEME_MARS_DETAIL,
    THEME_MARS_EDGE,
    THEME_ROCKET,
    THEME_COCKPIT,
    THEME_FLAME,
    THEME_LABEL,
    THEME_TEXT,
    THEME_DIM,
    THEME_GOOD,
    THEME_WARN,
    THEME_BAD,
    THEME_BOX,
    THEME_TITLE,
    THEME_COUNT
} ThemeRole;

void theme_set(int index);
int theme_count(void);
unsigned char theme_color(ThemeRole role);
const char *theme_name(int index);

#endif
