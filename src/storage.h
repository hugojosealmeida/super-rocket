#ifndef SUPER_ROCKET_STORAGE_H
#define SUPER_ROCKET_STORAGE_H

const char *storage_path(const char *filename);
int storage_read_ints(const char *filename, int *values, int count);
int storage_write_ints(const char *filename, const int *values, int count);

#endif
