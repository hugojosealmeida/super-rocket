#include <stdio.h>
#include "storage.h"
void rocket_init(unsigned int seed);
void rocket_tick(void);
int rocket_key(int code, int down);
const char *rocket_snapshot(void);

int main(int argc, char **argv)
{
    int i;
    if (argc != 2) return 2;
    storage_set_directory(argv[1]);
    rocket_init(123);
    rocket_key(13, 1); rocket_tick(); rocket_key(13, 0);
    for (i = 0; i < 120; i++) {
        if (i == 0) rocket_key(87, 1);
        if (i == 30) rocket_key(87, 0);
        if (i == 40) rocket_key(32, 1);
        if (i == 60) rocket_key(32, 0);
        rocket_tick();
    }
    puts(rocket_snapshot());
    return 0;
}
