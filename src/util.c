#include "util.h"

#include <math.h>
#include <stdio.h>

int util_clamp_int(int value, int low, int high)
{
    if (value < low)
        return low;
    if (value > high)
        return high;
    return value;
}

double util_clamp(double value, double low, double high)
{
    if (value < low)
        return low;
    if (value > high)
        return high;
    return value;
}

double util_distance(double x1, double y1, double x2, double y2)
{
    double dx = x2 - x1;
    double dy = y2 - y1;

    return sqrt(dx * dx + dy * dy);
}

double util_approach(double value, double target, double delta)
{
    if (value < target)
        return value + delta < target ? value + delta : target;
    if (value > target)
        return value - delta > target ? value - delta : target;
    return value;
}

void util_format_time(char *buf, size_t size, double seconds)
{
    int total = (int)seconds;

    if (total < 0)
        total = 0;

    snprintf(buf, size, "%02d:%02d", total / 60, total % 60);
}
