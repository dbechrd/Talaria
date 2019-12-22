#include "ta_log.h"
#include "ta_timer.h"
#include "dlb/dlb_vector.h"
#include "SDL/SDL_Timer.h"
#include "SDL/SDL_Thread.h"
#include <stdarg.h>
#include <stdio.h>
#include <time.h>

ta_log tg_debug_log;

const char *ta_log_source_str(ta_log_source src) {
    switch(src) {
        case SRC_ASSERT:     return "ASSERT";
        case SRC_AUDIO:      return "AUDIO";
        case SRC_EDITOR:     return "EDITOR";
        case SRC_EVENT:      return "EVENT";
        case SRC_FILE:       return "FILE";
        case SRC_FONT:       return "FONT";
        case SRC_GAME:       return "GAME";
        case SRC_GLTF:       return "GLTF";
        case SRC_JSON:       return "JSON";
        case SRC_MATH:       return "MATH";
        case SRC_OPENGL:     return "OPENGL";
        case SRC_PRIMITIVE:  return "PRIMITIVE";
        case SRC_RENDER:     return "RENDER";
        case SRC_RIGID_BODY: return "RIGID_BODY";
        case SRC_SCENE:      return "SCENE";
        case SRC_SHADER:     return "SHADER";
        case SRC_SYSTEM:     return "SYSTEM";
        case SRC_TEXTURE:    return "TEXTURE";
        case SRC_WINDOW:     return "WINDOW";
        default:
            DLB_ASSERT(!"<UNKNOWN_SRC_TYPE>");
            return 0;
    }
}

void ta_log_init(ta_log *log, FILE *stream, bool flush, bool echo,
    u32 src_include, u32 src_exclude)
{
    DLB_ASSERT(log);
    DLB_ASSERT(stream);
    log->stream = stream;
    log->flush = flush;
    log->echo = echo;
    log->src_include = src_include;
    log->src_exclude = src_exclude;
    log->last_write_ms = ta_timer_elapsed_ms();
    fprintf(log->stream,
        "[     Timestamp     ][Thread][  Elapsed  ][  Delta   ][ Source  ][       Message       ]\n"
        "--------------------------------------------------------------------------------\n");
}

void ta_log_init_file(ta_log *log, const char *filename, bool flush, bool echo,
    u32 src_include, u32 src_exclude)
{
    FILE *stream = fopen(filename, "wb");
    ta_log_init(log, stream, flush, echo, src_include, src_exclude);
    log->filename = filename;
}

void ta_log_flush(ta_log *log)
{
    fflush(log->stream);
    if (log->echo) {
        fflush(stdout);
    }
}

static void ta_log_timestamp(char *buf, int len)
{
    time_t ts = time(0);
    struct tm *date = localtime(&ts);
    strftime(buf, len, "%F %T", date);
}

static void ta_log_write_timestamp(ta_log *log, u32 src)
{
    char timestamp[] = "1970-01-01 00:00:00";
    ta_log_timestamp(timestamp, sizeof(timestamp));
    double elapsed_sec = ta_timer_elapsed_sec();
    double elapsed_ms = ta_timer_elapsed_ms();
    double ms_since_last_write = elapsed_ms - log->last_write_ms;
    log->last_write_ms = elapsed_ms;

    SDL_threadID thread_id = SDL_ThreadID();

    fprintf(log->stream, "[%s][%6u][%10.3fs][%7.3fms][ %10s] ", timestamp,
        thread_id, elapsed_sec, ms_since_last_write, ta_log_source_str(src));
    if (log->echo) {
        fprintf(stdout, "[%s][%6u][%10.3fs][%9.3fms][ %10s] ", timestamp,
            thread_id, elapsed_sec, ms_since_last_write, ta_log_source_str(src));
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
        ta_log_flush(log);
    }
}

void ta_log_write(ta_log *log, u32 src, const char *fmt, ...)
{
    if (log->src_include & src && !(log->src_exclude & src)) {
        ta_log_write_timestamp(log, src);
        va_list args;
        va_start(args, fmt);
        vfprintf(log->stream, fmt, args);
        if (log->echo) {
            vfprintf(stdout, fmt, args);
        }
        va_end(args);
        if (log->flush) {
            ta_log_flush(log);
        }
    }
}

void ta_log_free(ta_log *log)
{
    if (log->filename) {
        fclose(log->stream);
    }
}