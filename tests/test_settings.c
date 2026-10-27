#include <stdio.h>

#include "game.h"
#include "storage.h"
#include "test.h"

#define CONFIG_NAME ".super_rocket_config"

static void write_config(const char *text)
{
    FILE *file = fopen(storage_path(CONFIG_NAME), "w");

    if (file == NULL)
        return;

    if (text != NULL)
        fputs(text, file);

    fclose(file);
}

static void load_into(Game *g)
{
    g->settings.theme = -99;
    g->settings.sound = -99;
    g->settings.show_fps = -99;
    game_load_settings(g);
}

void test_settings(void)
{
    Game g;
    Game reloaded;

    remove(storage_path(CONFIG_NAME));

    /* A missing file keeps the documented defaults. */
    load_into(&g);
    CHECK(g.settings.theme == 0);
    CHECK(g.settings.sound == 1);
    CHECK(g.settings.show_fps == 0);

    /* Round trip: saved values come back on the next start. */
    g.settings.theme = 2;
    g.settings.sound = 0;
    g.settings.show_fps = 1;
    game_save_settings(&g);
    game_init(&reloaded, 100, 30);
    CHECK(reloaded.settings.theme == 2);
    CHECK(reloaded.settings.sound == 0);
    CHECK(reloaded.settings.show_fps == 1);
    load_into(&reloaded);
    CHECK(reloaded.settings.theme == 2);
    CHECK(reloaded.settings.sound == 0);
    CHECK(reloaded.settings.show_fps == 1);

    /* The helpers that write the file are covered too. */
    remove(storage_path(CONFIG_NAME));
    game_init(&g, 100, 30);
    game_cycle_theme(&g);
    game_toggle_sound(&g);
    game_toggle_fps(&g);
    load_into(&reloaded);
    CHECK(reloaded.settings.theme == 1);
    CHECK(reloaded.settings.sound == 0);
    CHECK(reloaded.settings.show_fps == 1);

    /* Theme is clamped to a valid palette index. */
    write_config("99\n1\n0\n");
    load_into(&reloaded);
    CHECK(reloaded.settings.theme == GAME_THEME_COUNT - 1);
    CHECK(reloaded.settings.sound == 1);
    CHECK(reloaded.settings.show_fps == 0);

    write_config("-5\n0\n1\n");
    load_into(&reloaded);
    CHECK(reloaded.settings.theme == 0);
    CHECK(reloaded.settings.sound == 0);
    CHECK(reloaded.settings.show_fps == 1);

    /* An empty file keeps all defaults. */
    write_config("");
    load_into(&reloaded);
    CHECK(reloaded.settings.theme == 0);
    CHECK(reloaded.settings.sound == 1);
    CHECK(reloaded.settings.show_fps == 0);

    /* Unparsable content keeps all defaults. */
    write_config("theme=mono\n");
    load_into(&reloaded);
    CHECK(reloaded.settings.theme == 0);
    CHECK(reloaded.settings.sound == 1);
    CHECK(reloaded.settings.show_fps == 0);

    /* A truncated file applies only the values actually read. */
    write_config("2\n");
    load_into(&reloaded);
    CHECK(reloaded.settings.theme == 2);
    CHECK(reloaded.settings.sound == 1);
    CHECK(reloaded.settings.show_fps == 0);

    write_config("1\n0\n");
    load_into(&reloaded);
    CHECK(reloaded.settings.theme == 1);
    CHECK(reloaded.settings.sound == 0);
    CHECK(reloaded.settings.show_fps == 0);

    remove(storage_path(CONFIG_NAME));
}
