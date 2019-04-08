#pragma once
#include "dlb_types.h"
#include <stdio.h>

typedef struct {
	const char *filename;
    FILE *stream;
    bool flush;
} ta_log;

extern ta_log *tg_debug_log;

void ta_log_init(ta_log *log, const char *filename, bool flush);
void ta_log_write(ta_log *log, const char *fmt, ...);
void ta_log_append(ta_log *log, const char* fmt, ...);
//void ta_log_free(ta_log *log);