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

int storage_read_ints(const char *filename, int *values, int count)
{
    FILE *file;
    int i;

    file = fopen(storage_path(filename), "r");
    if (file == NULL)
        return -1;

    for (i = 0; i < count; i++) {
        if (fscanf(file, "%d", &values[i]) != 1) {
            fclose(file);
            return -1;
        }
    }

    fclose(file);
    return 0;
}

int storage_write_ints(const char *filename, const int *values, int count)
{
    FILE *file;
    int i;

    file = fopen(storage_path(filename), "w");
    if (file == NULL)
        return -1;

    for (i = 0; i < count; i++)
        fprintf(file, "%d\n", values[i]);

    fclose(file);
    return 0;
}
