#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "test.h"

int test_failures = 0;
int test_checks = 0;

void test_report(const char *name, int failures_before)
{
    if (test_failures == failures_before)
        printf("ok   %s\n", name);
    else
        printf("FAIL %s\n", name);
}

static int make_temp_home(char *path, size_t size)
{
    const char *base = getenv("TMPDIR");

    if (base == NULL)
        base = "/tmp";

    snprintf(path, size, "%s/super-rocket-tests-XXXXXX", base);
    if (mkdtemp(path) == NULL)
        return -1;

    setenv("HOME", path, 1);
    return 0;
}

int main(void)
{
    char home[512];
    int failures;
    int i;

    if (make_temp_home(home, sizeof(home)) != 0) {
        fprintf(stderr, "could not create a temporary HOME\n");
        return 2;
    }

    failures = test_failures;
    test_util();
    test_report("util", failures);

    failures = test_failures;
    test_rng();
    test_report("rng", failures);

    failures = test_failures;
    test_storage();
    test_report("storage", failures);

    failures = test_failures;
    test_game();
    test_report("game", failures);

    failures = test_failures;
    test_leaderboard();
    test_report("leaderboard", failures);

    printf("\n%d checks, %d failures\n", test_checks, test_failures);

    /* best effort cleanup of the throwaway home */
    for (i = 0; i < 1; i++) {
        char cmd[640];

        snprintf(cmd, sizeof(cmd), "rm -rf '%s'", home);
        if (system(cmd) != 0) {
            /* ignore */
        }
    }

    return test_failures == 0 ? 0 : 1;
}
