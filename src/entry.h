/*
 * entry.h - A file to be listed and a growable array of such files.
 */
#ifndef LS_ENTRY_H
#define LS_ENTRY_H

#include <stddef.h>
#include <sys/stat.h>

/* One file shown by ls. */
struct entry {
    char *name;       /* name as it is displayed (operand or d_name) */
    char *path;       /* path used for system calls (stat, readlink) */
    struct stat st;   /* file status (from stat or lstat)            */
};

/* A dynamic array of entries. */
struct entry_list {
    struct entry *items;
    size_t len;
    size_t cap;
};

/* Initialise an empty list. */
void entry_list_init(struct entry_list *list);

/*
 * Append a new entry. The strings are copied; st is copied by value.
 */
void entry_list_add(struct entry_list *list, const char *name,
                    const char *path, const struct stat *st);

/* Release every entry and the array itself. */
void entry_list_free(struct entry_list *list);

#endif /* LS_ENTRY_H */
