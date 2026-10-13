/*
 * list.c - Operand handling and directory traversal.
 */
#include <sys/types.h>
#include <sys/stat.h>

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "entry.h"
#include "list.h"
#include "options.h"
#include "print.h"
#include "sort.h"
#include "util.h"

/*
 * Non-zero once something has been written to stdout; used to separate
 * successive directory listings with a blank line.
 */
static int g_output_started;

/* "." and ".." are never descended into with -R. */
static int
is_dot_or_dotdot(const char *name)
{
    return strcmp(name, ".") == 0 || strcmp(name, "..") == 0;
}

/* Decide, from -a and -A, whether a directory entry is shown. */
static int
is_visible(const char *name)
{
    if (name[0] != '.' || g_opts.all)
        return 1;
    return g_opts.almost_all && !is_dot_or_dotdot(name);
}

/*
 * Get the status of a command line operand.
 *
 * Symbolic links given as operands are followed, so that "ls link-to-dir"
 * lists the directory, except with -d (links are not indirected through),
 * -l and -F (the link itself is described). A dangling link falls back to
 * lstat(2) so that the link is still listed.
 */
static int
stat_operand(const char *path, struct stat *st)
{
    if (g_opts.dir_as_file || g_opts.long_format || g_opts.classify)
        return lstat(path, st);
    if (stat(path, st) == 0)
        return 0;
    return lstat(path, st);
}

/*
 * Read every visible entry of a directory into list. Entries whose status
 * cannot be read are reported and skipped. Returns -1 if the directory
 * itself cannot be opened.
 */
static int
read_directory(const char *path, const char *display, struct entry_list *list)
{
    DIR *dir;
    struct dirent *de;

    dir = opendir(path);
    if (dir == NULL) {
        warn_path(display, errno);
        return -1;
    }

    for (;;) {
        struct stat st;
        char *child;

        errno = 0;
        de = readdir(dir);
        if (de == NULL) {
            if (errno != 0)
                warn_path(display, errno);
            break;
        }
        if (!is_visible(de->d_name))
            continue;

        child = join_path(path, de->d_name);
        if (lstat(child, &st) < 0)
            warn_path(child, errno);
        else
            entry_list_add(list, de->d_name, child, &st);
        free(child);
    }

    closedir(dir);
    return 0;
}

/*
 * List the contents of one directory and, with -R, of all its
 * subdirectories. show_header prints a "path:" line first.
 */
static void
list_directory(const char *path, const char *display, int show_header)
{
    struct entry_list list;
    size_t i;

    if (show_header) {
        if (g_output_started)
            putchar('\n');
        print_name(display);
        fputs(":\n", stdout);
    }
    g_output_started = 1;

    entry_list_init(&list);
    if (read_directory(path, display, &list) < 0)
        return;

    sort_entries(&list);
    print_entries(&list, 1);

    /*
     * -R: descend into subdirectories in the order they were printed.
     * Entries were obtained with lstat(2), so symbolic links to
     * directories are not followed and cannot create loops.
     */
    if (g_opts.recursive) {
        for (i = 0; i < list.len; i++) {
            const struct entry *e = &list.items[i];

            if (S_ISDIR(e->st.st_mode) && !is_dot_or_dotdot(e->name))
                list_directory(e->path, e->path, 1);
        }
    }

    entry_list_free(&list);
}

void
list_operands(char **operands, int count)
{
    struct entry_list files, dirs;
    int i;
    size_t j;

    entry_list_init(&files);
    entry_list_init(&dirs);

    /* Split operands into files and directories; report missing ones. */
    for (i = 0; i < count; i++) {
        struct stat st;

        if (stat_operand(operands[i], &st) < 0) {
            warn_path(operands[i], errno);
            continue;
        }
        if (S_ISDIR(st.st_mode) && !g_opts.dir_as_file)
            entry_list_add(&dirs, operands[i], operands[i], &st);
        else
            entry_list_add(&files, operands[i], operands[i], &st);
    }

    /* Both groups are sorted separately; files are displayed first. */
    sort_entries(&files);
    sort_entries(&dirs);

    if (files.len > 0) {
        print_entries(&files, 0);
        g_output_started = 1;
    }

    /* Headers are needed as soon as more than one operand was given. */
    for (j = 0; j < dirs.len; j++)
        list_directory(dirs.items[j].path, dirs.items[j].name, count > 1);

    entry_list_free(&files);
    entry_list_free(&dirs);
}
