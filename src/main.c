/*
 * main.c - Entry point of ls(1): parse options and start the listing.
 *
 * Usage: ls [-AacdFfhiklnqRrSstuw] [file ...]
 */
#include <ctype.h>
#include <errno.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "list.h"
#include "options.h"

#define DEFAULT_BLOCKSIZE 512L
#define MAX_BLOCKSIZE     (1024L * 1024L * 1024L)  /* 1 GiB */

struct options g_opts;
int g_exit_status = 0;

static void
usage(void)
{
    fprintf(stderr, "usage: ls [-AacdFfhiklnqRrSstuw] [file ...]\n");
    exit(EXIT_FAILURE);
}

/*
 * Parse $BLOCKSIZE like getbsize(3): a number optionally followed by
 * 'k', 'm' or 'g'. Invalid or out of range values fall back to 512.
 */
static long
parse_blocksize(const char *value)
{
    char *end;
    long n;
    long mult = 1;

    if (value == NULL || *value == '\0')
        return DEFAULT_BLOCKSIZE;

    errno = 0;
    n = strtol(value, &end, 10);
    if (errno != 0 || end == value || n <= 0) {
        fprintf(stderr, "ls: %s: unknown blocksize\n", value);
        return DEFAULT_BLOCKSIZE;
    }

    switch (tolower((unsigned char)*end)) {
    case '\0':
        break;
    case 'k':
        mult = 1024L;
        break;
    case 'm':
        mult = 1024L * 1024L;
        break;
    case 'g':
        mult = 1024L * 1024L * 1024L;
        break;
    default:
        fprintf(stderr, "ls: %s: unknown blocksize\n", value);
        return DEFAULT_BLOCKSIZE;
    }
    if (*end != '\0' && end[1] != '\0') {
        fprintf(stderr, "ls: %s: unknown blocksize\n", value);
        return DEFAULT_BLOCKSIZE;
    }

    if (n > MAX_BLOCKSIZE / mult) {
        fprintf(stderr, "ls: maximum blocksize is 1G\n");
        return MAX_BLOCKSIZE;
    }
    n *= mult;
    if (n < DEFAULT_BLOCKSIZE) {
        fprintf(stderr, "ls: minimum blocksize is 512\n");
        return DEFAULT_BLOCKSIZE;
    }
    return n;
}

/* Default settings, before any command line flag is applied. */
static void
init_options(void)
{
    memset(&g_opts, 0, sizeof(g_opts));
    g_opts.time = TIME_MODIFY;
    g_opts.sort = SORT_NAME;
    g_opts.unit = BLOCKS_DEFAULT;
    g_opts.stdout_is_tty = isatty(STDOUT_FILENO);

    /* -q is the default on a terminal, -w otherwise. */
    g_opts.pmode = g_opts.stdout_is_tty ? PRINT_QUOTE : PRINT_RAW;

    /* -A is always set for the super-user. */
    if (getuid() == 0)
        g_opts.almost_all = 1;
}

/*
 * Apply the command line flags. Pairs of mutually exclusive options
 * (-l/-n, -c/-u, -q/-w, -R/-d, -h/-k, -t/-S) override each other: the
 * last one specified wins. -f disables sorting regardless of -t/-S.
 * Returns the index of the first operand.
 */
static int
parse_options(int argc, char **argv)
{
    int ch;
    int no_sort = 0;

    while ((ch = getopt(argc, argv, "AacdFfhiklnqRrSstuw")) != -1) {
        switch (ch) {
        case 'A':
            g_opts.almost_all = 1;
            break;
        case 'a':
            g_opts.all = 1;
            break;
        case 'c':
            g_opts.time = TIME_CHANGE;
            break;
        case 'u':
            g_opts.time = TIME_ACCESS;
            break;
        case 'd':
            g_opts.dir_as_file = 1;
            g_opts.recursive = 0;
            break;
        case 'R':
            g_opts.recursive = 1;
            g_opts.dir_as_file = 0;
            break;
        case 'F':
            g_opts.classify = 1;
            break;
        case 'f':
            no_sort = 1;
            break;
        case 'h':
            g_opts.unit = BLOCKS_HUMAN;
            break;
        case 'k':
            g_opts.unit = BLOCKS_KILO;
            break;
        case 'i':
            g_opts.show_inode = 1;
            break;
        case 'l':
            g_opts.long_format = 1;
            g_opts.numeric_ids = 0;
            break;
        case 'n':
            g_opts.long_format = 1;
            g_opts.numeric_ids = 1;
            break;
        case 'q':
            g_opts.pmode = PRINT_QUOTE;
            break;
        case 'w':
            g_opts.pmode = PRINT_RAW;
            break;
        case 'r':
            g_opts.reverse = 1;
            break;
        case 'S':
            g_opts.sort = SORT_SIZE;
            break;
        case 't':
            g_opts.sort = SORT_TIME;
            break;
        case 's':
            g_opts.show_blocks = 1;
            break;
        default:
            usage();
        }
    }

    if (no_sort)
        g_opts.sort = SORT_NONE;

    /* $BLOCKSIZE is only used when neither -h nor -k was given. */
    g_opts.blocksize = DEFAULT_BLOCKSIZE;
    if (g_opts.unit == BLOCKS_DEFAULT && (g_opts.show_blocks ||
                                          g_opts.long_format))
        g_opts.blocksize = parse_blocksize(getenv("BLOCKSIZE"));

    return optind;
}

int
main(int argc, char **argv)
{
    int first;

    /* Needed to recognise printable multibyte characters in names. */
    setlocale(LC_ALL, "");

    init_options();
    first = parse_options(argc, argv);

    if (first >= argc) {
        /* No operand: list the current directory. */
        char *dot[] = { "." };

        list_operands(dot, 1);
    } else {
        list_operands(argv + first, argc - first);
    }

    if (fflush(stdout) != 0 || ferror(stdout)) {
        perror("ls: stdout");
        g_exit_status = 1;
    }
    return g_exit_status;
}
