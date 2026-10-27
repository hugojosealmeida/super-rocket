#define _POSIX_C_SOURCE 199309L

#include "rng.h"

#include <time.h>
#include <unistd.h>

static uint64_t rng_state = 0x9e3779b97f4a7c15ULL;

void rng_seed(uint64_t seed)
{
    if (seed == 0)
        seed = 0x9e3779b97f4a7c15ULL;
    rng_state = seed;
}

uint64_t rng_next(void)
{
    uint64_t x = rng_state;

    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    rng_state = x;

    return x * 2685821657736338717ULL;
}

uint32_t rng_u32(void)
{
    return (uint32_t)(rng_next() >> 32);
}

int rng_range(int low, int high)
{
    if (high <= low)
        return low;
    return low + (int)(rng_u32() % (uint32_t)(high - low));
}

double rng_double(void)
{
    return (double)(rng_next() >> 11) * (1.0 / 9007199254740992.0);
}

uint64_t rng_seed_from_clock(void)
{
    struct timespec ts;
    uint64_t seed;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    seed = (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
    seed ^= (uint64_t)getpid() << 32;
    rng_seed(seed);

    return seed;
}
