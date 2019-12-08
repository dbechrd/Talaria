#include "ta_file.h"
#include "ta_log.h"
#include "ta_buffer.h"
#include "dlb/dlb_memory.h"
#include <ctype.h>
#include <stdlib.h>

static const char *file_mode_str(ta_file_mode mode) {
    switch(mode) {
        case FILE_READ:  return "rb";
        case FILE_WRITE: return "wb";
        default: DLB_ASSERT(0); return 0;
    }
}

ta_file *ta_file_open(const char *filename, ta_file_mode mode) {
    FILE *hnd = fopen(filename, file_mode_str(mode));
    if (!hnd) {
        perror("fopen error");
        PANIC("Failed to open file: %s\n", filename);
    }
    ta_file *file = dlb_calloc(1, sizeof(*file));
    file->filename = filename;
    file->mode = mode;
    file->hnd = hnd;
    file->pos.line = 1;
    file->pos.column = 1;
    return file;
}

void ta_file_close(ta_file *f) {
    fclose(f->hnd);
    dlb_free(f);
}

void ta_file_debug_context(ta_file *f)
{
    fprintf(stderr, "File: %s:%llu:%llu\n\n", f->filename, f->pos.line, f->pos.column);
    fprintf(stderr, "%04llu:%.*s", f->pos.line, f->context_len, f->context_buf);
    u64 col = f->pos.column;

    char buf[1024] = { 0 };
    f->replay = false;
    ta_file_read(f, buf, sizeof(buf) - 1, 0, "\r\n", 0);
    fprintf(stderr, "%s\n", buf);

    fprintf(stderr, "     ");
    for (u64 i = 0; i < col - 1; i++) {
        fprintf(stderr, "-");
    }
    fprintf(stderr, "^\n\n");
}

static const char *char_printable(const char *c) {
    // HACK: Static buffer, don't hold pointers to this
    static char buf[2] = { 0 };
    if (isprint(*c)) {
        buf[0] = *c;
        return buf;
    }

    switch (*c) {
        case '\t': return "\\t";
        case '\r': return "\\r";
        case '\n': return "\\n";
        case '\0': return "\\0";
        default: return "?";
    }
}

char ta_file_char(ta_file *f) {
    if (f->replay) {
        f->replay = false;
        return f->prev;
    }
    if (f->prev == '\n') {
        f->context_len = 0;
        f->pos.line++;
        f->pos.column = 0;
    }
    f->pos.column++;
    int c = fgetc(f->hnd);
    if (c == '\r') {
        c = fgetc(f->hnd);
    }
    if (c == EOF) {
        f->eof = true;
    }
    if (f->context_len < sizeof(f->context_buf)) {
        f->context_buf[f->context_len] = (char)c;
    }
    f->context_len++;
    f->prev = (char)c;
    return f->prev;
}

char ta_file_char_escaped(ta_file *f)
{
    ta_file_expect_char(f, "\\", 1);
    char c = ta_file_char(f);
    switch (c) {
        case '\"':
            c = '\"';
            break;
        case '\\':
            c = '\\';
            break;
        case 't':
            c = '\t';
            break;
        case 'r':
            c = '\r';
            break;
        case 'n':
            c = '\n';
            break;
        case '0':
            c = '\0';
            break;
        case 'x':
            // TODO: Handle hex byte codes
            PANIC_FILE(f,
                "[PARSE_ERROR] Hex byte codes not yet supported in char "
                "literals.\n"
            );
            break;
        case 'u':
            // TODO: Handle short unicode code points
            PANIC_FILE(f,
                "[PARSE_ERROR] Short Unicode hex code points not yet supported "
                "in char literals.\n"
            );
            break;
        case 'U':
            // TODO: Handle long unicode code points
            PANIC_FILE(f,
                "[PARSE_ERROR] Long unicode hex code points not yet supported "
                "in char literals.\n"
            );
            break;
        case EOF:
            PANIC_FILE(f,
                "[PARSE_ERROR] Unexpected EOF while reading character.\n"
            );
        default:
            PANIC_FILE(f,
                "[PARSE_ERROR] Invalid escape sequence in char literal '%s'."
                "\n", char_printable(&c)
            );
    }
    return c;
}

char ta_file_peek(ta_file *f) {
    char c = ta_file_char(f);
    f->replay = true;
    return c;
}

static char str_contains_chr(const char *str, char c) {
    char found = 0;
    if (str) {
        const char *d = str;
        while (*d && *d != c) {
            d++;
        }
        found = *d;
    }
    return found;
}

char ta_file_read(ta_file *f, char *buf, size_t count, const char *valid_chars,
    const char *delims, int *len)
{
    DLB_ASSERT(!buf || count);
    DLB_ASSERT(valid_chars || delims || count);

    ta_file_pos pos_start = f->pos;
    pos_start.column += 1;

    char delim = 0;
    u32 i;
    for (i = 0; !count || i < count; i++) {
        char c = ta_file_char(f);
        if (f->eof) {
            break;
        }

        delim = str_contains_chr(delims, c);
        if (delim) {
            f->replay = true;
            break;
        }

        char valid = str_contains_chr(valid_chars, c);
        if (valid_chars && !valid) {
            if (delims) {
                PANIC_FILE(f,
                    "[PARSE_ERROR] Unexpected character '%s' in expression "
                    "starting at %d:%d. Expected [%s] or delimeter [%s].\n",
                    char_printable(&c), (int)pos_start.line,
                    (int)pos_start.column, valid_chars, delims
                );
            } else {
                f->replay = true;
                break;
            }
        }

        if (buf) buf[i] = c;
    }

    if (delims && !delim) {
        PANIC_FILE(f,
            "[PARSE_ERROR] Expected delim [%s] to end expression starting at "
            "%d:%d\n", delims, (int)pos_start.line, (int)pos_start.column
        );
    }

    if (len) *len = i;
    return delim;
}

int ta_file_expect_char(ta_file *f, const char *chars, int times) {
    int count;
    ta_file_read(f, 0, times, chars, 0, &count);
    if (count != times) {
        char next = ta_file_peek(f);
        PANIC_FILE(f,
            "[PARSE_ERROR] Missing expected character [%s]. Found '%s' instead."
            "\n", chars, char_printable(&next)
        );
    }
    return count;
}

int ta_file_allow_char(ta_file *f, const char *chars, int times) {
    int count;
    ta_file_read(f, 0, times, chars, 0, &count);
    return count;
}

ta_buffer ta_file_read_all(const char *filename)
{
    // Open file
    FILE *fs = fopen(filename, "rb");
    if (!fs) {
        ta_log_write(&tg_debug_log, SRC_FILE, "Unable to open %s for reading\n",
            filename);
        DLB_ASSERT(!"ta_file_read_all: failed to open file");
    }

    // Calculate length
    fseek(fs, 0, SEEK_END);
    long tell = ftell(fs);
    if (tell < 0) {
        ta_log_write(&tg_debug_log, SRC_FILE, "Unable to determine length of %s\n",
            filename);
        DLB_ASSERT(!"ta_file_read_all: failed to calculate file length");
    }
    rewind(fs);

    // Allocate buffer
    ta_buffer buffer = ta_buffer_init(tell + 1);

    // Read into buffer, null-terminate
    fread(buffer.data, 1, tell, fs);
    buffer.data[tell] = 0;

    // Close file
    fclose(fs);
    return buffer;
}