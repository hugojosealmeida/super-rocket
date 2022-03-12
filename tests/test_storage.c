#include <stdio.h>
#include <string.h>

#include "storage.h"
#include "test.h"

void test_storage(void)
{
    const char *name = ".super_rocket_test_ints";
    const int written[3] = { 11, -4, 9001 };
    int read_back[3] = { 0, 0, 0 };

    CHECK(storage_write_ints(name, written, 3) == 0);
    CHECK(storage_read_ints(name, read_back, 3) == 3);
    CHECK(read_back[0] == 11);
    CHECK(read_back[1] == -4);
    CHECK(read_back[2] == 9001);

    remove(storage_path(name));

    CHECK(storage_read_ints(name, read_back, 3) < 0);
}
