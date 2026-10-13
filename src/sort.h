/*
 * sort.h - Ordering of entries according to -t, -S, -r and -f.
 */
#ifndef LS_SORT_H
#define LS_SORT_H

#include <time.h>

#include "entry.h"

/*
 * Sort the list in place using the order selected on the command line.
 * Does nothing when -f is given.
 */
void sort_entries(struct entry_list *list);

/* The timestamp selected by -c / -u (modification time by default). */
time_t entry_time(const struct entry *e);

#endif /* LS_SORT_H */
