/*
 * util.c - Small helpers shared by every module.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "options.h"
#include "util.h"

void *
xmalloc(size_t size)
{
    void *p = malloc(size ? size : 1);

    if (p == NULL) {
        fprintf(stderr, "ls: out of memory\n");
        exit(EXIT_FAILURE);
    }
    return p;
}

void *
xrealloc(void *ptr, size_t size)
{
    void *p = realloc(ptr, size ? size : 1);

    if (p == NULL) {
        fprintf(stderr, "ls: out of memory\n");
        exit(EXIT_FAILURE);
    }
    return p;
}

char *
xstrdup(const char *s)
{
    size_t len = strlen(s) + 1;
    char *copy = xmalloc(len);

    memcpy(copy, s, len);
    return copy;
}

void
warn_path(const char *path, int errnum)
{
    /* Flush stdout first so that messages appear in a sensible order. */
    fflush(stdout);
    fprintf(stderr, "ls: %s: %s\n", path, strerror(errnum));
    g_exit_status = 1;
}

char *
join_path(const char *dir, const char *name)
{
    size_t dlen = strlen(dir);
    size_t nlen = strlen(name);
    int need_slash = (dlen > 0 && dir[dlen - 1] != '/');
    char *path = xmalloc(dlen + need_slash + nlen + 1);

    memcpy(path, dir, dlen);
    if (need_slash)
        path[dlen] = '/';
    memcpy(path + dlen + need_slash, name, nlen + 1);
    return path;
}

void
humanize(char *buf, size_t len, unsigned long long bytes)
{
    static const char units[] = "BKMGTPE";
    double value = (double)bytes;
    int unit = 0;

    if (bytes < 1024) {
        snprintf(buf, len, "%lluB", bytes);
        return;
    }

    /* Scale down until the value fits in 4 characters (e.g. "1023K"). */
    while (value >= 1023.5 && unit < (int)sizeof(units) - 2) {
        value /= 1024.0;
        unit++;
    }

    /* One decimal place for small values, like humanize_number(3). */
    if (value < 9.95)
        snprintf(buf, len, "%.1f%c", value, units[unit]);
    else
        snprintf(buf, len, "%.0f%c", value, units[unit]);
}

int
num_width(unsigned long long n)
{
    int width = 1;

    while (n >= 10) {
        n /= 10;
        width++;
    }
    return width;
}
