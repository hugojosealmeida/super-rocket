#include <string.h>

#include "test.h"
#include "util.h"

void test_util(void)
{
    char buf[16];

    CHECK(util_clamp_int(5, 0, 10) == 5);
    CHECK(util_clamp_int(-3, 0, 10) == 0);
    CHECK(util_clamp_int(42, 0, 10) == 10);

    CHECK(util_clamp(0.5, 0.0, 1.0) == 0.5);
    CHECK(util_clamp(-1.0, 0.0, 1.0) == 0.0);
    CHECK(util_clamp(2.0, 0.0, 1.0) == 1.0);

    CHECK(util_distance(0.0, 0.0, 3.0, 4.0) == 5.0);
    CHECK(util_distance(1.0, 1.0, 1.0, 1.0) == 0.0);

    CHECK(util_approach(0.0, 10.0, 3.0) == 3.0);
    CHECK(util_approach(10.0, 0.0, 3.0) == 7.0);
    CHECK(util_approach(5.0, 5.0, 3.0) == 5.0);

    util_format_time(buf, sizeof(buf), 65.0);
    CHECK(strcmp(buf, "01:05") == 0);
    util_format_time(buf, sizeof(buf), 0.0);
    CHECK(strcmp(buf, "00:00") == 0);
}
