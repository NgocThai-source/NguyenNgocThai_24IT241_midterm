/*
 * print.h - Output formatting (one-per-line and long format).
 */
#ifndef LS_PRINT_H
#define LS_PRINT_H

#include "entry.h"

/*
 * Print every entry of the list, one per line, honouring -i, -s, -l, -n,
 * -F, -h, -k, -q and -w. When is_dir_contents is non-zero a "total" line
 * with the sum of allocated blocks is printed first (for -l, and for -s
 * when the output is a terminal).
 */
void print_entries(const struct entry_list *list, int is_dir_contents);

/* Print a file name, applying -q / -w to non-printable characters. */
void print_name(const char *name);

#endif /* LS_PRINT_H */
