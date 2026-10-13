/*
 * print.c - Output formatting.
 *
 * Every entry is printed on its own line. Optional columns (inode, block
 * count, long format fields) are formatted first into a per-entry record
 * so that the width of each column can be computed and the output aligned.
 */
#include <sys/types.h>
#include <sys/stat.h>
#ifdef __linux__
#include <sys/sysmacros.h>  /* major(), minor() on Linux */
#endif

#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <wchar.h>
#include <wctype.h>

#include "mode.h"
#include "options.h"
#include "print.h"
#include "sort.h"
#include "util.h"

/* Half a year in seconds: older (or future) files show the year. */
#define SIX_MONTHS ((time_t)(365 * 24 * 60 * 60 / 2))

/* Pre-formatted columns of one output line. */
struct line {
    char inode[32];
    char blocks[32];
    char mode[MODE_STR_LEN];
    char nlink[32];
    char owner[64];
    char group[64];
    char size[48];
    char date[64];
};

/* Maximum width of every column of a listing. */
struct widths {
    int inode, blocks, nlink, owner, group, size;
};

/* ------------------------------------------------------------------ */
/* Column formatting                                                   */
/* ------------------------------------------------------------------ */

/*
 * Format a number of 512-byte blocks using the unit selected by -h, -k
 * or $BLOCKSIZE. Partial units are rounded up.
 */
static void
format_blocks(char *buf, size_t len, unsigned long long blocks512)
{
    unsigned long long bytes = blocks512 * 512ULL;
    unsigned long long unit;

    if (g_opts.unit == BLOCKS_HUMAN) {
        humanize(buf, len, bytes);
        return;
    }
    unit = (g_opts.unit == BLOCKS_KILO) ? 1024ULL
                                        : (unsigned long long)g_opts.blocksize;
    snprintf(buf, len, "%llu", (bytes + unit - 1) / unit);
}

/* Owner name, or the numeric uid with -n or when the uid is unknown. */
static void
format_owner(char *buf, size_t len, uid_t uid)
{
    struct passwd *pw = g_opts.numeric_ids ? NULL : getpwuid(uid);

    if (pw != NULL)
        snprintf(buf, len, "%s", pw->pw_name);
    else
        snprintf(buf, len, "%lu", (unsigned long)uid);
}

/* Group name, or the numeric gid with -n or when the gid is unknown. */
static void
format_group(char *buf, size_t len, gid_t gid)
{
    struct group *gr = g_opts.numeric_ids ? NULL : getgrgid(gid);

    if (gr != NULL)
        snprintf(buf, len, "%s", gr->gr_name);
    else
        snprintf(buf, len, "%lu", (unsigned long)gid);
}

/*
 * Size field: major/minor numbers for device files, otherwise the size in
 * bytes (human readable with -h).
 */
static void
format_size(char *buf, size_t len, const struct stat *st)
{
    if (S_ISCHR(st->st_mode) || S_ISBLK(st->st_mode))
        snprintf(buf, len, "%3lu, %3lu",
                 (unsigned long)major(st->st_rdev),
                 (unsigned long)minor(st->st_rdev));
    else if (g_opts.unit == BLOCKS_HUMAN)
        humanize(buf, len, (unsigned long long)st->st_size);
    else
        snprintf(buf, len, "%lld", (long long)st->st_size);
}

/*
 * Date field: "Mon dd HH:MM" for files within six months of now,
 * "Mon dd  YYYY" otherwise. Uses the timezone from $TZ via localtime(3).
 */
static void
format_date(char *buf, size_t len, time_t t, time_t now)
{
    struct tm *tm = localtime(&t);
    const char *fmt;

    if (tm == NULL) {
        snprintf(buf, len, "%-12s", "?");
        return;
    }
    if (t > now - SIX_MONTHS && t < now + SIX_MONTHS)
        fmt = "%b %e %H:%M";
    else
        fmt = "%b %e  %Y";
    if (strftime(buf, len, fmt, tm) == 0)
        snprintf(buf, len, "%-12s", "?");
}

/* Fill the pre-formatted columns of one entry. */
static void
format_line(struct line *ln, const struct entry *e, time_t now)
{
    const struct stat *st = &e->st;

    snprintf(ln->inode, sizeof(ln->inode), "%llu",
             (unsigned long long)st->st_ino);
    format_blocks(ln->blocks, sizeof(ln->blocks),
                  (unsigned long long)st->st_blocks);

    if (!g_opts.long_format)
        return;

    mode_string(st->st_mode, ln->mode);
    snprintf(ln->nlink, sizeof(ln->nlink), "%lu", (unsigned long)st->st_nlink);
    format_owner(ln->owner, sizeof(ln->owner), st->st_uid);
    format_group(ln->group, sizeof(ln->group), st->st_gid);
    format_size(ln->size, sizeof(ln->size), st);
    format_date(ln->date, sizeof(ln->date), entry_time(e), now);
}

/* Update column widths with the values of one line. */
static void
update_widths(struct widths *w, const struct line *ln)
{
#define WIDEN(field) do {                                   \
        int len_ = (int)strlen(ln->field);                  \
        if (len_ > w->field)                                \
            w->field = len_;                                \
    } while (0)

    WIDEN(inode);
    WIDEN(blocks);
    if (g_opts.long_format) {
        WIDEN(nlink);
        WIDEN(owner);
        WIDEN(group);
        WIDEN(size);
    }
#undef WIDEN
}

/* ------------------------------------------------------------------ */
/* Names                                                               */
/* ------------------------------------------------------------------ */

void
print_name(const char *name)
{
    mbstate_t state;
    size_t remaining = strlen(name);
    const char *p = name;

    if (g_opts.pmode == PRINT_RAW) {
        fputs(name, stdout);
        return;
    }

    /*
     * -q: decode the name character by character in the current locale so
     * that printable multibyte characters (e.g. UTF-8) are kept while
     * control characters and invalid bytes are replaced by '?'.
     */
    memset(&state, 0, sizeof(state));
    while (remaining > 0) {
        wchar_t wc;
        size_t n = mbrtowc(&wc, p, remaining, &state);

        if (n == (size_t)-1 || n == (size_t)-2) {
            /* Invalid or truncated sequence: replace one byte. */
            putchar('?');
            memset(&state, 0, sizeof(state));
            p++;
            remaining--;
            continue;
        }
        if (n == 0)
            break;
        if (iswprint((wint_t)wc))
            fwrite(p, 1, n, stdout);
        else
            putchar('?');
        p += n;
        remaining -= n;
    }
}

/*
 * Print the target of a symbolic link as " -> target". The buffer grows
 * until the whole target fits, since st_size may be 0 on some systems.
 */
static void
print_link_target(const struct entry *e)
{
    size_t size = e->st.st_size > 0 ? (size_t)e->st.st_size + 1 : 256;

    for (;;) {
        char *buf = xmalloc(size);
        ssize_t n = readlink(e->path, buf, size);

        if (n < 0) {
            free(buf);
            return;     /* target cannot be read: show the link only */
        }
        if ((size_t)n < size) {
            buf[n] = '\0';
            fputs(" -> ", stdout);
            print_name(buf);
            free(buf);
            return;
        }
        free(buf);
        size *= 2;
    }
}

/* ------------------------------------------------------------------ */
/* Listing                                                             */
/* ------------------------------------------------------------------ */

/* Print one complete line for an entry. */
static void
print_line(const struct entry *e, const struct line *ln,
           const struct widths *w)
{
    char indicator;

    if (g_opts.show_inode)
        printf("%*s ", w->inode, ln->inode);
    if (g_opts.show_blocks)
        printf("%*s ", w->blocks, ln->blocks);
    if (g_opts.long_format)
        printf("%s %*s %-*s  %-*s  %*s %s ",
               ln->mode, w->nlink, ln->nlink, w->owner, ln->owner,
               w->group, ln->group, w->size, ln->size, ln->date);

    print_name(e->name);

    if (g_opts.classify && (indicator = classify_char(e->st.st_mode)) != '\0')
        putchar(indicator);

    if (g_opts.long_format && S_ISLNK(e->st.st_mode))
        print_link_target(e);

    putchar('\n');
}

void
print_entries(const struct entry_list *list, int is_dir_contents)
{
    struct line *lines;
    struct widths w;
    unsigned long long total = 0;
    time_t now = time(NULL);
    size_t i;

    memset(&w, 0, sizeof(w));
    lines = xmalloc((list->len ? list->len : 1) * sizeof(*lines));

    /* First pass: format every column and find the widest value. */
    for (i = 0; i < list->len; i++) {
        format_line(&lines[i], &list->items[i], now);
        update_widths(&w, &lines[i]);
        total += (unsigned long long)list->items[i].st.st_blocks;
    }

    /*
     * The "total" line precedes the contents of a directory in long
     * format, and with -s when writing to a terminal.
     */
    if (is_dir_contents &&
        (g_opts.long_format || (g_opts.show_blocks && g_opts.stdout_is_tty))) {
        char buf[32];

        format_blocks(buf, sizeof(buf), total);
        printf("total %s\n", buf);
    }

    /* Second pass: print aligned lines. */
    for (i = 0; i < list->len; i++)
        print_line(&list->items[i], &lines[i], &w);

    free(lines);
}
