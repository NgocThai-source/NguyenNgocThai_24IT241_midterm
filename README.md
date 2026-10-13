# Midterm Project – Implement `ls(1)`

A simplified re-implementation of the UNIX `ls(1)` command written in C from
scratch, following the provided NetBSD manual page (`ls.pdf`).

- **Student:** Nguyễn Ngọc Thái – 24IT241
- **Repository:** https://github.com/NgocThai-source/NguyenNgocThai_24IT241_Midterm_DHMT

---

## 1. Build and run

Requirements: a C99 compiler (`cc`, `gcc` or `clang`) and `make`, on any
POSIX system (tested on macOS; also builds on Linux and NetBSD).

```sh
make            # builds the ./ls executable
./ls [-AacdFfhiklnqRrSstuw] [file ...]
make debug      # rebuild with -g and AddressSanitizer/UBSan
make clean      # remove obj/ and the binary
```

Examples:

```sh
./ls                 # list the current directory
./ls -la /etc        # long listing including hidden files
./ls -lhS ~          # long listing, human readable sizes, largest first
./ls -R src          # recursive listing
./ls -d */           # directories themselves, not their contents
./ls -Fi a.txt dir   # inode numbers and type indicators
```

The program exits with `0` on success and `1` if any error occurred (e.g. a
missing file or an unreadable directory). Errors are printed to `stderr` as
`ls: <path>: <reason>` and do not stop the listing of the other operands.

---

## 2. Implemented features

### Options

| Option | Behaviour |
|--------|-----------|
| `-A` | List all entries except `.` and `..`. Always set for the super-user (uid 0). |
| `-a` | Include entries whose names begin with a dot. |
| `-c` | Use the status-change time (`st_ctime`) for sorting (`-t`) and printing (`-l`). |
| `-d` | List directories as plain files; symbolic links in the argument list are not followed. |
| `-F` | Append `/` directory, `*` executable, `@` symlink, `%` whiteout, `=` socket, `\|` FIFO. |
| `-f` | Output is not sorted (directory order / argument order). |
| `-h` | Human readable sizes (`512B`, `4.9K`, `23M`) for `-s` and `-l`. Overrides `-k`. |
| `-i` | Print the inode number of each file. |
| `-k` | Block counts (`-s`, `total`) in kilobytes. The rightmost of `-k` / `-h` wins. |
| `-l` | Long format (see below). |
| `-n` | Like `-l` but owner and group are shown as numeric IDs. |
| `-q` | Print non-printable characters as `?` (default when stdout is a terminal). |
| `-R` | Recursively list subdirectories. |
| `-r` | Reverse the sort order. |
| `-S` | Sort by size, largest first. |
| `-s` | Print the number of blocks used (512-byte units or `$BLOCKSIZE`). |
| `-t` | Sort by time, most recent first. |
| `-u` | Use the last access time (`st_atime`) for sorting and printing. |
| `-w` | Print non-printable characters raw (default when stdout is not a terminal). |

Options that override each other – the **last one given wins**, as stated in
the manual: `-l`/`-n`, `-c`/`-u`, `-q`/`-w`, `-R`/`-d`, `-h`/`-k`. Also `-t`/`-S`
(last wins), and `-f` always disables sorting.

### Operands

- With no operand, the current directory (`.`) is listed.
- Non-directory operands are displayed first, then each directory; the two
  groups are sorted separately (lexicographically, or according to
  `-t`/`-S`/`-r`).
- When more than one operand is given, each directory listing is preceded
  by a `name:` header; listings are separated by a blank line.
- Symbolic links given as operands are followed (so `ls link-to-dir` shows
  the directory contents) except with `-d`, `-l` or `-F`; dangling links are
  still listed.

### Long format (`-l` / `-n`)

```
total 224
-rwsr-xr-x 1 alice  staff  101136 Oct 13 22:33 exe
lrwxr-xr-x 1 alice  staff       5 Oct 13 22:33 link -> a.txt
crw-rw-rw- 1 root   wheel    3,   2 Oct 13 22:33 /dev/null
-rw-r--r-- 1 alice  staff       0 Jan  1  2020 old
```

- `total` line with the sum of allocated blocks before each directory listing.
- File mode: type (`- d l s p c b w`) and permissions including setuid /
  setgid (`s`/`S`) and sticky (`t`/`T`) bits.
- Number of links, owner name, group name (numeric if unknown or with `-n`).
- Size in bytes, or `major, minor` for character/block devices.
- Date: `Mon dd HH:MM` for files within six months, `Mon dd  YYYY`
  otherwise; the time zone comes from `$TZ`.
- Symbolic links are shown as `name -> target`.
- All columns are aligned on the widest value of the listing.

### Environment

- `BLOCKSIZE` – block unit for `-s` and `total` when `-h`/`-k` are not given
  (e.g. `BLOCKSIZE=1k`; accepts suffixes `k`, `m`, `g`; minimum 512).
- `TZ` – time zone used to display dates.

### Robustness / edge cases

- Every allocation is checked; no fixed-size limit on the number of entries.
- Missing files, unreadable directories and entries that cannot be `lstat`ed
  are reported and skipped, the exit status becomes `1`.
- `-R` uses `lstat(2)`, so symbolic links to directories are never followed
  and cannot cause infinite loops; `.` and `..` are never descended into.
- Symlink targets of any length are read with a growing buffer.
- `-q` decodes names in the current locale, so UTF‑8 names (e.g.
  `tiếng việt`) are printed correctly while control characters become `?`.
- Tested with AddressSanitizer / UBSan (`make debug`) on large trees
  (`/etc`, `/dev`, `/usr/share/zoneinfo`): no memory errors.

---

## 3. Project structure

```
.
├── Makefile
├── README.md
├── .gitignore
└── src/
    ├── main.c / options.h   option parsing, defaults, program entry
    ├── list.c / list.h      operands, directory reading, -R recursion
    ├── entry.c / entry.h    struct entry and a dynamic array of entries
    ├── sort.c / sort.h      name / time / size ordering, -r, -f
    ├── print.c / print.h    one-per-line and long format output, -q/-w
    ├── mode.c / mode.h      "drwxr-xr-x" mode string and -F indicators
    └── util.c / util.h      checked malloc, error messages, path join,
                             human readable sizes
```

Flow: `main()` parses the options into the global `g_opts` →
`list_operands()` stats each operand and splits files from directories →
`sort_entries()` orders each group → `print_entries()` formats and aligns
the output → `list_directory()` reads directories and recurses for `-R`.

---

## 4. Notes / differences from the system `ls`

- Output is always one entry per line (the manual's default); multi-column
  output is not part of the given subset.
- With `-s` the `total` line is printed only when the output is a terminal,
  as described in the manual; with `-l` it is always printed.
- Archive states `a`/`A` are NetBSD-specific and not available through
  portable POSIX interfaces, so they are not shown.
