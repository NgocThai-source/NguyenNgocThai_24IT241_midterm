/*
 * util.h - Small helpers: checked memory allocation, error reporting,
 * path joining and number formatting.
 */
#ifndef LS_UTIL_H
#define LS_UTIL_H

#include <stddef.h>
#include <sys/types.h>

/* malloc/realloc/strdup that terminate the program when out of memory. */
void *xmalloc(size_t size);
void *xrealloc(void *ptr, size_t size);
char *xstrdup(const char *s);

/*
 * Print "ls: <path>: <strerror(errnum)>" on stderr and set the exit
 * status to failure.
 */
void warn_path(const char *path, int errnum);

/* Return a newly allocated "dir/name" (no doubled slash). */
char *join_path(const char *dir, const char *name);

/*
 * Format a byte count in a human readable way ("512B", "1.5K", "23M")
 * into buf, as done by humanize_number(3).
 */
void humanize(char *buf, size_t len, unsigned long long bytes);

/* Number of decimal digits needed to print n. */
int num_width(unsigned long long n);

#endif /* LS_UTIL_H */
