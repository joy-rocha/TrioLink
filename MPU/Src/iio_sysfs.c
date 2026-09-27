#include "iio_sysfs.h"
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>

int iio_find_device(const char *name_match, char *devpath_out, size_t out_len) {
    DIR *d = opendir(IIO_SYSFS_BASE);
    if (!d) return -1;

    struct dirent *entry;
    while ((entry = readdir(d)) != NULL) {
        if (strncmp(entry->d_name, "iio:device", 10) != 0)
            continue;

        char namefile[IIO_MAX_PATH];
        snprintf(namefile, sizeof(namefile), "%s/%s/name", IIO_SYSFS_BASE, entry->d_name);

        FILE *f = fopen(namefile, "r");
        if (!f) continue;

        // Lê o nome do sensor
        char buf[64] = {0};
        if (fgets(buf, sizeof(buf), f) != NULL) {
            buf[strcspn(buf, "\n")] = 0; // remove '\n' 

            if (strstr(buf, name_match) != NULL) {
                fclose(f);
                snprintf(devpath_out, out_len, "%s/%s", IIO_SYSFS_BASE, entry->d_name);
                closedir(d);
                return 0;
            }
        }
        fclose(f);
    }
    closedir(d);

    errno = ENODEV;
    return -1;
}

int iio_read_attr_double(const char *devpath, const char *attr, double *out) {
    char path[IIO_MAX_PATH];
    snprintf(path, sizeof(path), "%s/%s", devpath, attr);

    FILE *f = fopen(path, "r");
    if (!f) return -1;

    int n = fscanf(f, "%lf", out);
    fclose(f);
    return (n == 1) ? 0 : -1;
}

int iio_read_attr_long(const char *devpath, const char *attr, long *out) {
    char path[IIO_MAX_PATH];
    snprintf(path, sizeof(path), "%s/%s", devpath, attr);

    FILE *f = fopen(path, "r");
    if (!f) return -1;

    int n = fscanf(f, "%ld", out);
    fclose(f);
    return (n == 1) ? 0 : -1;
}