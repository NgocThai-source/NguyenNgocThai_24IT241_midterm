/*
 * mode.c - File type and permission strings.
 */
#include <sys/stat.h>

#include "mode.h"

/* Entry type character (first column of the long format). */
static char
type_char(mode_t mode)
{
    if (S_ISDIR(mode))
        return 'd';
    if (S_ISLNK(mode))
        return 'l';
    if (S_ISSOCK(mode))
        return 's';
    if (S_ISFIFO(mode))
        return 'p';
    if (S_ISCHR(mode))
        return 'c';
    if (S_ISBLK(mode))
        return 'b';
#ifdef S_ISWHT
    if (S_ISWHT(mode))
        return 'w';
#endif
    return '-';
}

/*
 * Third character of a permission triplet.
 *   exec    - the execute bit of this triplet
 *   special - setuid / setgid / sticky bit belonging to this triplet
 *   on_x    - character used when both are set ('s' or 't')
 *   off_x   - character used when only special is set ('S' or 'T')
 */
static char
exec_char(int exec, int special, char on_x, char off_x)
{
    if (special)
        return exec ? on_x : off_x;
    return exec ? 'x' : '-';
}

void
mode_string(mode_t mode, char buf[MODE_STR_LEN])
{
    buf[0] = type_char(mode);

    /* Owner. */
    buf[1] = (mode & S_IRUSR) ? 'r' : '-';
    buf[2] = (mode & S_IWUSR) ? 'w' : '-';
    buf[3] = exec_char(mode & S_IXUSR, mode & S_ISUID, 's', 'S');

    /* Group. */
    buf[4] = (mode & S_IRGRP) ? 'r' : '-';
    buf[5] = (mode & S_IWGRP) ? 'w' : '-';
    buf[6] = exec_char(mode & S_IXGRP, mode & S_ISGID, 's', 'S');

    /* Other (the sticky bit lives here). */
    buf[7] = (mode & S_IROTH) ? 'r' : '-';
    buf[8] = (mode & S_IWOTH) ? 'w' : '-';
    buf[9] = exec_char(mode & S_IXOTH, mode & S_ISVTX, 't', 'T');

    buf[10] = '\0';
}

char
classify_char(mode_t mode)
{
    if (S_ISDIR(mode))
        return '/';
    if (S_ISLNK(mode))
        return '@';
    if (S_ISSOCK(mode))
        return '=';
    if (S_ISFIFO(mode))
        return '|';
#ifdef S_ISWHT
    if (S_ISWHT(mode))
        return '%';
#endif
    if (S_ISREG(mode) && (mode & (S_IXUSR | S_IXGRP | S_IXOTH)))
        return '*';
    return '\0';
}
