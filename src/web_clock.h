#ifndef SUPER_ROCKET_WEB_CLOCK_H
#define SUPER_ROCKET_WEB_CLOCK_H

#define WEB_FRAME_TIME (1.0 / 30.0)

/* Advance a fixed simulation deadline; discard excessive suspend backlog. */
double web_tick_after(double previous, double now);

#endif
