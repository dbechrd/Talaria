#include "ta_log.h"
#include "ta_timer.h"
#include "dlb/dlb_vector.h"
#include "SDL/SDL_Timer.h"
#include <stdarg.h>
#include <stdio.h>
#include <time.h>

ta_log tg_debug_log;

void ta_log_init(ta_log *log, FILE *stream, bool flush, bool echo)
{
    DLB_ASSERT(log);
    DLB_ASSERT(stream);
    log->stream = stream;
    log->flush = flush;
    log->echo = echo;
    log->last_write_ms = ta_timer_elapsed_ms();
    fprintf(log->stream,
        "[     Timestamp     ][  Elapsed  ][   Delta   ][ Source  ][       Message       ]\n"
        "---------------------------------------------------------------------------------\n");
}

void ta_log_init_file(ta_log *log, const char *filename, bool flush, bool echo)
{
    FILE *stream = fopen(filename, "wb");
    ta_log_init(log, stream, flush, echo);
    log->filename = filename;
}

static void ta_log_timestamp(char *buf, int len)
{
    time_t ts = time(0);
    struct tm *date = localtime(&ts);
    strftime(buf, len, "%F %T", date);
}

static void ta_log_write_timestamp(ta_log *log, const char *src)
{
    char timestamp[] = "1970-01-01 00:00:00";
    ta_log_timestamp(timestamp, sizeof(timestamp));
    double elapsed_sec = ta_timer_elapsed_sec();
    double elapsed_ms = ta_timer_elapsed_ms();
    double ms_since_last_write = elapsed_ms - log->last_write_ms;
    log->last_write_ms = elapsed_ms;

    fprintf(log->stream, "[%s][%10.3fs][%7.3fms][ %8s] ", timestamp, elapsed_sec,
        ms_since_last_write, src);
    if (log->echo) {
        fprintf(stdout, "[%s][%10.3fs][%9.3fms][ %8s] ", timestamp, elapsed_sec,
            ms_since_last_write, src);
    }
}

void ta_log_append(ta_log *log, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vfprintf(log->stream, fmt, args);
    if (log->echo) {
        vfprintf(stdout, fmt, args);
    }
    va_end(args);
    if (log->flush) {
        fflush(log->stream);
        if (log->echo) {
            fflush(stdout);
        }
    }
}

void ta_log_write(ta_log *log, const char *src, const char *fmt, ...)
{
    ta_log_write_timestamp(log, src);
    va_list args;
    va_start(args, fmt);
    vfprintf(log->stream, fmt, args);
    if (log->echo) {
        vfprintf(stdout, fmt, args);
    }
    va_end(args);
    if (log->flush) {
        fflush(log->stream);
        if (log->echo) {
            fflush(stdout);
        }
    }
}

void ta_log_free(ta_log *log)
{
    if (log->filename) {
        fclose(log->stream);
    }
}