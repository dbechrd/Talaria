#pragma once
#include "dlb/dlb_types.h"
#include <stdio.h>

typedef enum ta_file_mode {
    FILE_READ,
    FILE_WRITE,
} ta_file_mode;

typedef struct ta_file_pos {
    u64 line;   // line number where current token started (for better errors)
    u64 column; // column number where current token started (for better errors)
} ta_file_pos;

typedef struct ta_file {
    const char   *filename;  // relative path to file
    ta_file_mode mode;       // read/write mode
    FILE         *hnd;       // file handle
    char         prev;       // previous character read from file
    bool         replay;     // if true, the next character read will return `prev` and clear this flag
    bool         eof;        // if true, we've reached the end of the file

    // Debug info (for better error messages)
    ta_file_pos  pos;               // line/column where current token started
    char         context_buf[80];   // line buffer
    int          context_len;       // number of bytes in `context_buf` in use

    char         *contents;         // entire file contents (vector)
    size_t       contents_cursor;   // index of next character to read
} ta_file;

#define PANIC(format, ...) {                                            \
    fprintf(stderr, "\n---[PANIC]------------------------------\n"      \
                    "Source file: %s:%d\n\n", __FILE__, __LINE__);      \
    fprintf(stderr, (format), __VA_ARGS__);                             \
    fprintf(stderr, "\n----------------------------------------\n");    \
    UNUSED(getchar());                                                  \
    DLB_ASSERT(!"PANIC at the disco");                                  \
    exit(1); }

#define PANIC_FILE(f, format, ...) {                                    \
    fprintf(stderr, "\n---[PANIC_FILE]-------------------------\n"      \
                    "Source file: %s:%d\n", __FILE__, __LINE__);        \
    ta_file_debug_context(f);                                           \
    fprintf(stderr, (format), __VA_ARGS__);                             \
    fprintf(stderr, "\n----------------------------------------\n");    \
    UNUSED(getchar());                                                  \
    DLB_ASSERT(!"PANIC at the disco");                                  \
    exit(1); }

void ta_file_open           (ta_file *, const char *filename, ta_file_mode mode);
void ta_file_close          (ta_file *f);
void ta_file_debug_context  (ta_file *f);
char ta_file_char           (ta_file *f);
char ta_file_char_escaped   (ta_file *f);
char ta_file_peek           (ta_file *f);
char ta_file_read           (ta_file *f, char *buf, size_t count, const char *valid_chars, const char *delims, int *len);
int ta_file_expect_char     (ta_file *f, const char *chars, int times);
int ta_file_allow_char      (ta_file *f, const char *chars, int times);
char *ta_file_read_all      (const char *filename);