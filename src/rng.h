#ifndef SUPER_ROCKET_RNG_H
#define SUPER_ROCKET_RNG_H

#include <stdint.h>

void rng_seed(uint64_t seed);
uint64_t rng_next(void);
uint32_t rng_u32(void);
int rng_range(int low, int high);
double rng_double(void);
uint64_t rng_seed_from_clock(void);

#endif
