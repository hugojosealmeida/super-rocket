#ifndef SUPER_ROCKET_UTIL_H
#define SUPER_ROCKET_UTIL_H

#include <stddef.h>

int util_clamp_int(int value, int low, int high);
double util_clamp(double value, double low, double high);
double util_distance(double x1, double y1, double x2, double y2);
double util_approach(double value, double target, double delta);
void util_format_time(char *buf, size_t size, double seconds);

#endif
