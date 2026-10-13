/*
 * list.h - Processing of operands and traversal of directories.
 */
#ifndef LS_LIST_H
#define LS_LIST_H

/*
 * List the given operands (files or directories). Non-directory operands
 * are printed first, then the contents of each directory operand, each
 * group being sorted separately. Errors are reported on stderr and set
 * g_exit_status, but never stop the listing of the remaining operands.
 */
void list_operands(char **operands, int count);

#endif /* LS_LIST_H */
