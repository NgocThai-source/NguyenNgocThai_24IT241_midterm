/*
 * mode.h - Textual representation of a file's type and permissions.
 */
#ifndef LS_MODE_H
#define LS_MODE_H

#include <sys/types.h>

/* Size of the buffer needed by mode_string(): 10 characters + NUL. */
#define MODE_STR_LEN 11

/*
 * Fill buf with the "drwxr-xr-x" style string described in the
 * "Long Format" section of the manual (like strmode(3)).
 */
void mode_string(mode_t mode, char buf[MODE_STR_LEN]);

/*
 * Return the -F indicator for a file: '/', '*', '@', '%', '=', '|' or
 * '\0' when no indicator applies.
 */
char classify_char(mode_t mode);

#endif /* LS_MODE_H */
