#include "web_clock.h"

double web_tick_after(double previous, double now)
{
    double next = previous + WEB_FRAME_TIME;
    return next < now - WEB_FRAME_TIME * 4 ? now + WEB_FRAME_TIME : next;
}
