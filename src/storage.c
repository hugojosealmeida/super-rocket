#include "storage.h"

#include <stdio.h>
#include <stdlib.h>

static char path_buffer[1024];

const char *storage_path(const char *filename)
{
    const char *home = getenv("HOME");

    if (home != NULL)
        snprintf(path_buffer, sizeof(path_buffer), "%s/%s", home, filename);
    else
        snprintf(path_buffer, sizeof(path_buffer), "%s", filename);

    return path_buffer;
}
