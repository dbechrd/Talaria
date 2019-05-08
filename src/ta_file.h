#pragma once
#include "ta_buffer.h"
#include "dlb_types.h"
#include <stdio.h>

typedef enum ta_file_mode {
    FILE_READ,
    FILE_WRITE,
} ta_file_mode;

typedef struct ta_file_pos {
    size_t line;
    size_t column;
} ta_file_pos;

typedef struct ta_file {
    const char *filename;
    ta_file_mode mode;
    FILE *hnd;
    char prev;
    bool replay;
    bool eof;

    // Debug info
    ta_file_pos pos;
    char context_buf[80];  // Line buffer for debug context
    int context_len;
} ta_file;

#define PANIC(format, ...) { \
    fprintf(stderr, "\n---[PANIC]----------------------------------------------------------------------\n" \
        "Source file: %s:%d\n\n", __FILE__, __LINE__); \
    fprintf(stderr, (format), __VA_ARGS__); \
    fprintf(stderr, "--------------------------------------------------------------------------------\n"); \
    UNUSED(getchar()); \
    exit(1); }

#define PANIC_FILE(f, format, ...) { \
    fprintf(stderr, "\n---[PANIC_FILE]-----------------------------------------------------------------\n" \
        "Source file: %s:%d\n", __FILE__, __LINE__); \
    ta_file_debug_context(f); \
    fprintf(stderr, (format), __VA_ARGS__); \
    fprintf(stderr, "--------------------------------------------------------------------------------\n"); \
    UNUSED(getchar()); \
    exit(1); }

ta_file *ta_file_open(const char *filename, ta_file_mode mode);
void ta_file_close(ta_file *f);
void ta_file_debug_context(ta_file *f);
char ta_file_char(ta_file *f);
char ta_file_char_escaped(ta_file *f);
char ta_file_peek(ta_file *f);
char ta_file_read(ta_file *f, char *buf, size_t count, const char *valid_chars,
    const char *delims, int *len);
int ta_file_expect_char(ta_file *f, const char *chars, int times);
int ta_file_allow_char(ta_file *f, const char *chars, int times);
ta_buffer *ta_file_read_all(const char *filename);