/*
 * options.h - Command line options of ls(1).
 *
 * All flags parsed from the command line are stored in a single global
 * structure so that every module (sorting, printing, traversal) reads the
 * same configuration.
 */
#ifndef LS_OPTIONS_H
#define LS_OPTIONS_H

/* Which timestamp is used for sorting (-t) and printing (-l). */
enum time_field {
    TIME_MODIFY,  /* st_mtime (default)   */
    TIME_CHANGE,  /* st_ctime (-c)        */
    TIME_ACCESS   /* st_atime (-u)        */
};

/* How entries are ordered. */
enum sort_key {
    SORT_NAME,    /* lexicographical (default) */
    SORT_TIME,    /* -t */
    SORT_SIZE,    /* -S */
    SORT_NONE     /* -f */
};

/* How non-printable characters in file names are shown. */
enum print_mode {
    PRINT_QUOTE,  /* -q: replace with '?' (default on a terminal) */
    PRINT_RAW     /* -w: print as-is (default otherwise)          */
};

/* Units used to display block counts (-s and the "total" line). */
enum block_unit {
    BLOCKS_DEFAULT,  /* 512 bytes or $BLOCKSIZE */
    BLOCKS_KILO,     /* -k */
    BLOCKS_HUMAN     /* -h */
};

struct options {
    int all;              /* -a: show every entry, including . and ..   */
    int almost_all;       /* -A: show dot files except . and ..         */
    int dir_as_file;      /* -d: do not descend into directories        */
    int classify;         /* -F: append a type indicator to names       */
    int show_inode;       /* -i: print inode numbers                    */
    int long_format;      /* -l / -n: long listing                      */
    int numeric_ids;      /* -n: print uid/gid as numbers               */
    int recursive;        /* -R: descend into subdirectories            */
    int reverse;          /* -r: reverse the sort order                 */
    int show_blocks;      /* -s: print allocated blocks                 */
    enum time_field time; /* -c / -u                                    */
    enum sort_key sort;   /* -t / -S / -f                               */
    enum print_mode pmode;/* -q / -w                                    */
    enum block_unit unit; /* -k / -h                                    */
    long blocksize;       /* bytes per block when unit == BLOCKS_DEFAULT*/
    int stdout_is_tty;    /* cached isatty(STDOUT_FILENO)               */
};

/* The single, program-wide set of options (defined in main.c). */
extern struct options g_opts;

/* Exit status of the program: set to 1 as soon as any error occurs. */
extern int g_exit_status;

#endif /* LS_OPTIONS_H */
