#include "ta_log.h"
#include "ta_timer.h"
#include "dlb/dlb_vector.h"
#include "SDL/SDL_Timer.h"
#include <stdarg.h>
#include <stdio.h>
#include <time.h>

ta_log *tg_debug_log;

void ta_log_init(ta_log *log, const char *filename, bool flush)
{
	FILE *stream = fopen(filename, "wb");
	if (!stream) {
		DLB_ASSERT(!"ta_log_init: Failed to initialize log");
	}
	log->filename = filename;
    log->stream = stream;
    log->flush = flush;
}

static void ta_log_timestamp(char *buf, int len)
{
    time_t ts = time(0);
    struct tm *date = localtime(&ts);
    strftime(buf, len, "%F %T", date);
}

static void ta_log_write_timestamp(ta_log *log)
{
	char timestamp[] = "1970-01-01 00:00:00";
	ta_log_timestamp(timestamp, sizeof(timestamp));
	double elapsed_sec = ta_timer_elapsed_sec();
	fprintf(log->stream, "[%s][%.3fs] ", timestamp, elapsed_sec);
#if _DEBUG
	fprintf(stdout, "[%s][%.3fs] ", timestamp, elapsed_sec);
	fflush(stdout);
#endif
}

void ta_log_append(ta_log *log, const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	vfprintf(log->stream, fmt, args);
#if _DEBUG
	vfprintf(stdout, fmt, args);
	fflush(stdout);
#endif
	va_end(args);
	if (log->flush) {
		fflush(log->stream);
	}
}

void ta_log_write(ta_log *log, const char *fmt, ...)
{
	ta_log_write_timestamp(log);
	va_list args;
	va_start(args, fmt);
	vfprintf(log->stream, fmt, args);
#if _DEBUG
	vfprintf(stdout, fmt, args);
	fflush(stdout);
#endif
	va_end(args);
	if (log->flush) {
		fflush(log->stream);
	}
}

void ta_log_free(ta_log *log)
{
	fclose(log->stream);
}