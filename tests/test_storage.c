#include <stdio.h>
#include <string.h>

#include "storage.h"
#include "test.h"
#include "game.h"

static int writes;
static void wrote(void) { writes++; }

void test_storage(void)
{
    const char *name = ".super_rocket_test_ints";
    const int written[3] = { 11, -4, 9001 };
    int read_back[3] = { 0, 0, 0 };

    CHECK(storage_write_ints(name, written, 3) == 0);
    CHECK(storage_read_ints(name, read_back, 3) == 3);
    CHECK(read_back[0] == 11);
    CHECK(read_back[1] == -4);
    CHECK(read_back[2] == 9001);

    remove(storage_path(name));

    CHECK(storage_read_ints(name, read_back, 3) < 0);
    {
        Game game;
        int i;
        memset(&game, 0, sizeof(game));
        for (i = 0; i < GAME_LEADERBOARD_SIZE; i++)
            strcpy(game.leaderboard[i].name, "AAA");
        writes = 0;
        storage_set_modified_hook(wrote);
        CHECK(storage_write_ints(name, written, 3) == 0);
        CHECK(writes == 1);
        game_save_leaderboard(&game);
        CHECK(writes == 2); /* direct stdio writes also notify the frontend */
        storage_set_directory("/nonexistent/rocket-save-test");
        CHECK(storage_write_ints(name, written, 3) == -1);
        CHECK(writes == 2);
        storage_set_directory(NULL);
        storage_set_modified_hook(NULL);
        remove(storage_path(name));
        remove(storage_path(".super_rocket_leaderboard"));
    }
    {
        char original[1024];
        char too_long[1000];
        snprintf(original, sizeof(original), "%s", storage_path(name));
        CHECK(storage_set_directory("/an/alternate/save/directory") == 0);
        CHECK(strcmp(storage_path(name), "/an/alternate/save/directory/.super_rocket_test_ints") == 0);
        CHECK(storage_set_directory("") == -1);
        memset(too_long, 'a', sizeof(too_long) - 1);
        too_long[sizeof(too_long) - 1] = '\0';
        CHECK(storage_set_directory(too_long) == -1);
        CHECK(storage_set_directory(NULL) == 0);
        CHECK(strcmp(storage_path(name), original) == 0);
    }
}
