/*
 * entry.c - Growable array of file entries.
 */
#include <stdlib.h>

#include "entry.h"
#include "util.h"

void
entry_list_init(struct entry_list *list)
{
    list->items = NULL;
    list->len = 0;
    list->cap = 0;
}

void
entry_list_add(struct entry_list *list, const char *name, const char *path,
               const struct stat *st)
{
    struct entry *e;

    /* Grow geometrically so that appending is amortised O(1). */
    if (list->len == list->cap) {
        list->cap = list->cap ? list->cap * 2 : 16;
        list->items = xrealloc(list->items, list->cap * sizeof(*list->items));
    }

    e = &list->items[list->len++];
    e->name = xstrdup(name);
    e->path = xstrdup(path);
    e->st = *st;
}

void
entry_list_free(struct entry_list *list)
{
    size_t i;

    for (i = 0; i < list->len; i++) {
        free(list->items[i].name);
        free(list->items[i].path);
    }
    free(list->items);
    entry_list_init(list);
}
