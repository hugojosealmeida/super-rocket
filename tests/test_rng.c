#include "test.h"
#include "rng.h"

void test_rng(void)
{
    uint64_t first;
    uint64_t second;
    int i;

    rng_seed(12345);
    first = rng_next();
    rng_seed(12345);
    second = rng_next();
    CHECK(first == second);

    rng_seed(1);
    for (i = 0; i < 1000; i++) {
        int value = rng_range(5, 9);
        CHECK(value >= 5 && value < 9);
    }

    rng_seed(7);
    for (i = 0; i < 100; i++) {
        double value = rng_double();
        CHECK(value >= 0.0 && value < 1.0);
    }
}
