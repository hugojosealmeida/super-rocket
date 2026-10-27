#include <stdio.h>
#include <string.h>

#include "game.h"
#include "test.h"

/*
 * GAME_VERSION is what `--version` (src/main.c) prints and what the menu
 * footer (src/render.c) renders.  CHANGELOG.md is the documented release
 * history, so the version defined in code must always equal the newest
 * released `## <version>` heading there.  If either side changes without the
 * other, this test fails.
 */

/*
 * Copy the newest released version heading (the first `## ` line starting
 * with a digit, skipping `## Unreleased`) into `out`.
 * Returns the number of characters copied, 0 when no released heading
 * exists, or -1 when CHANGELOG.md could not be opened.
 */
static int changelog_latest_version(char *out, size_t size)
{
    static const char *paths[] = { "CHANGELOG.md", "../CHANGELOG.md" };
    char line[256];
    FILE *f = NULL;
    size_t i;

    /* `make test` runs from the repo root; the fallback covers running
       tests/run_tests directly from the tests directory. */
    for (i = 0; i < sizeof(paths) / sizeof(paths[0]) && f == NULL; i++)
        f = fopen(paths[i], "r");

    if (f == NULL)
        return -1;

    while (fgets(line, sizeof(line), f) != NULL) {
        const char *p = line;
        size_t n = 0;

        if (strncmp(p, "## ", 3) != 0)
            continue;
        p += 3;
        if (*p < '0' || *p > '9')
            continue; /* "Unreleased" and other non-version headings */

        while (*p != '\0' && *p != ' ' && *p != '\t' &&
               *p != '\n' && *p != '\r' && n + 1 < size)
            out[n++] = *p++;
        out[n] = '\0';
        fclose(f);
        return (int)n;
    }

    fclose(f);
    return 0;
}

void test_version(void)
{
    char documented[64];
    int n;

    /* The version printed by `super-rocket --version` and the menu footer. */
    CHECK(strcmp(GAME_VERSION, "1.0.0") == 0);

    documented[0] = '\0';
    n = changelog_latest_version(documented, sizeof(documented));
    if (n < 0)
        fprintf(stderr, "  CHANGELOG.md not found (run `make test` from the repo root)\n");
    CHECK(n > 0);
    CHECK(n > 0 && strcmp(GAME_VERSION, documented) == 0);
}
