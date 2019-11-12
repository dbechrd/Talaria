#pragma once
#include "dlb/dlb_types.h"

typedef struct _iobuf FILE;

typedef struct ta_log {
    const char *filename;
    FILE *stream;
    bool flush;
    bool echo;
    double last_write_ms;
} ta_log;

extern ta_log tg_debug_log;

void ta_log_init(ta_log *log, FILE *stream, bool flush, bool echo);
void ta_log_init_file(ta_log *log, const char *filename, bool flush, bool echo);
void ta_log_write(ta_log *log, const char *src, const char *fmt, ...);
void ta_log_append(ta_log *log, const char* fmt, ...);
//void ta_log_free(ta_log *log);