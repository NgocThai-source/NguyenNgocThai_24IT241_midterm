/*
 * sort.c - Ordering of entries.
 *
 * Default order is lexicographical by name. -t sorts newest first and -S
 * largest first; ties are broken by name. -r reverses whatever order is
 * in effect and -f disables sorting altogether.
 */
#include <stdlib.h>
#include <string.h>

#include "options.h"
#include "sort.h"

time_t
entry_time(const struct entry *e)
{
    switch (g_opts.time) {
    case TIME_CHANGE:
        return e->st.st_ctime;
    case TIME_ACCESS:
        return e->st.st_atime;
    case TIME_MODIFY:
    default:
        return e->st.st_mtime;
    }
}

/*
 * Nanosecond part of the selected timestamp, so that files changed within
 * the same second are still ordered correctly. macOS names the timespec
 * fields st_*timespec, other POSIX.1-2008 systems name them st_*tim.
 */
static long
entry_time_nsec(const struct entry *e)
{
#if defined(__APPLE__)
    switch (g_opts.time) {
    case TIME_CHANGE:
        return e->st.st_ctimespec.tv_nsec;
    case TIME_ACCESS:
        return e->st.st_atimespec.tv_nsec;
    case TIME_MODIFY:
    default:
        return e->st.st_mtimespec.tv_nsec;
    }
#else
    switch (g_opts.time) {
    case TIME_CHANGE:
        return e->st.st_ctim.tv_nsec;
    case TIME_ACCESS:
        return e->st.st_atim.tv_nsec;
    case TIME_MODIFY:
    default:
        return e->st.st_mtim.tv_nsec;
    }
#endif
}

static int
cmp_name(const struct entry *a, const struct entry *b)
{
    return strcmp(a->name, b->name);
}

static int
cmp_time(const struct entry *a, const struct entry *b)
{
    time_t ta = entry_time(a);
    time_t tb = entry_time(b);
    long na, nb;

    if (ta != tb)
        return ta > tb ? -1 : 1;    /* most recent first */
    na = entry_time_nsec(a);
    nb = entry_time_nsec(b);
    if (na != nb)
        return na > nb ? -1 : 1;
    return cmp_name(a, b);
}

static int
cmp_size(const struct entry *a, const struct entry *b)
{
    if (a->st.st_size != b->st.st_size)
        return a->st.st_size > b->st.st_size ? -1 : 1; /* largest first */
    return cmp_name(a, b);
}

/* qsort(3) callback: dispatch on the sort key and apply -r. */
static int
compare(const void *pa, const void *pb)
{
    const struct entry *a = pa;
    const struct entry *b = pb;
    int result;

    switch (g_opts.sort) {
    case SORT_TIME:
        result = cmp_time(a, b);
        break;
    case SORT_SIZE:
        result = cmp_size(a, b);
        break;
    case SORT_NAME:
    default:
        result = cmp_name(a, b);
        break;
    }
    return g_opts.reverse ? -result : result;
}

void
sort_entries(struct entry_list *list)
{
    if (g_opts.sort == SORT_NONE || list->len < 2)
        return;
    qsort(list->items, list->len, sizeof(*list->items), compare);
}
