#include "storage.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char path_buffer[1024];
static char save_directory[900];
static void (*modified_hook)(void);

void storage_set_modified_hook(void (*hook)(void)) { modified_hook = hook; }
void storage_notify_modified(void) { if (modified_hook) modified_hook(); }

int storage_set_directory(const char *directory)
{
    if (directory == NULL) {
        save_directory[0] = '\0';
        return 0;
    }
    if (!directory[0] || strlen(directory) >= sizeof(save_directory))
        return -1;
    strcpy(save_directory, directory);
    return 0;
}

const char *storage_path(const char *filename)
{
    const char *home = save_directory[0] ? save_directory : getenv("HOME");

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
        if (fscanf(file, "%d", &values[i]) != 1)
            break;
    }

    fclose(file);
    return i;
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

    {
        int failed = ferror(file);
        if (fclose(file) != 0 || failed)
            return -1;
    }
    storage_notify_modified();
    return 0;
}
