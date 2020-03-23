#include "ta_log.h"
#include "ta_timer.h"
#include "dlb/dlb_vector.h"
#include <stdarg.h>
#include <stdio.h>
#include <time.h>

// Note: Don't use DLB_ASSERT in this file because the assert handler writes
// to the log (will cause infinite recursion).
#undef DLB_ASSERT

// NOTE: I don't expect lock/unlock to ever error, but if it's a real thing that
// happens I'll refactor this to handle it better.
//#define TA_LOCK(mutex) assert(!SDL_LockMutex(mutex))
//#define TA_UNLOCK(mutex) assert(!SDL_UnlockMutex(mutex))
#define TA_LOCK(mutex)
#define TA_UNLOCK(mutex)
#define MAX_THREADS 8

ta_log tg_debug_log;

//typedef SDL_threadID ta_thread_id;
typedef unsigned int ta_thread_id;
static struct {
    ta_thread_id thread_id;
    double last_write_ms;
} thread_times[MAX_THREADS];

static void thread_set_last_write(ta_thread_id thread_id, double time_ms)
{
    //assert(thread_id);

    for (int i = 0; i < MAX_THREADS; ++i) {
        if (thread_times[i].thread_id == thread_id) {
            // Update time
            thread_times[i].last_write_ms = time_ms;
            return;
        } else if (!thread_times[i].thread_id) {
            // Add new thread to list
            thread_times[i].thread_id = thread_id;
            thread_times[i].last_write_ms = time_ms;
            return;
        }
    }

    assert(!"Thread table is full. Do clean-up or increase MAX_THREADS");
}

static double thread_get_last_write(ta_thread_id thread_id)
{
    //assert(thread_id);
    double time_ms = 0;
    for (int i = 0; i < MAX_THREADS; ++i) {
        if (thread_times[i].thread_id == thread_id) {
            time_ms = thread_times[i].last_write_ms;
            break;
        }
    }
    return time_ms;
}

const char *ta_log_source_str(ta_log_source src) {
    switch(src) {
        case SRC_ASSERT:        return "ASSERT";
        case SRC_AUDIO:         return "AUDIO";
        case SRC_CONSOLE:       return "CONSOLE";
        case SRC_EDITOR:        return "EDITOR";
        case SRC_EVENT:         return "EVENT";
        case SRC_FILE:          return "FILE";
        case SRC_FONT:          return "FONT";
        case SRC_GAME:          return "GAME";
        case SRC_GLTF:          return "GLTF";
        case SRC_JSON:          return "JSON";
        case SRC_KEYBIND:       return "KEYBIND";
        case SRC_LIGHT:         return "LIGHT";
        case SRC_MATH:          return "MATH";
        case SRC_OPENGL:        return "OPENGL";
        case SRC_PRIMITIVE:     return "PRIMITIVE";
        case SRC_RENDER:        return "RENDER";
        case SRC_RIGID_BODY:    return "RIGID_BODY";
        case SRC_SCENE:         return "SCENE";
        case SRC_SHADER:        return "SHADER";
        case SRC_SYSTEM:        return "SYSTEM";
        case SRC_TEXTURE:       return "TEXTURE";
        case SRC_WINDOW:        return "WINDOW";
        default:                return "UNKNOWN";
    }
}

void ta_log_init(ta_log *log, FILE *stream, bool flush, bool echo,
    u32 src_include, u32 src_exclude)
{
    assert(log);
    assert(stream);
    log->stream = stream;
    log->flush = flush;
    log->echo = echo;
    log->src_include = src_include;
    log->src_exclude = src_exclude;
    //log->mutex = SDL_CreateMutex();
    //assert(log->mutex);

    TA_LOCK(log->mutex);
    fprintf(log->stream,
        "[     Timestamp     ][Thread][  Elapsed  ][  Delta   ][ Source  ][       Message       ]\n"
        "----------------------------------------------------------------------------------------\n");
    TA_UNLOCK(log->mutex);
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
    char timestamp[32] = "1970-01-01 00:00:00";
    ta_log_timestamp(CSTR(timestamp));
    double elapsed_sec = ta_timer_elapsed_sec();
    double elapsed_ms = ta_timer_elapsed_ms();

    ta_thread_id thread_id = 0; //SDL_ThreadID();
    double ms_since_last_write = elapsed_ms - thread_get_last_write(thread_id);
    thread_set_last_write(thread_id, elapsed_ms);

    fprintf(log->stream, "[%s][%6u][%10.3fs][%7.3fms][ %10s] ", timestamp,
        thread_id, elapsed_sec, ms_since_last_write, ta_log_source_str(src));
    if (log->echo) {
        fprintf(stdout, "[%s][%6u][%10.3fs][%9.3fms][ %10s] ", timestamp,
            thread_id, elapsed_sec, ms_since_last_write, ta_log_source_str(src));
    }
}

#if 0
// Not a good idea when multi-threaded logging is enabled, would need to buffer
// the whole line before sending it.
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
#endif

void ta_log_write(ta_log *log, u32 src, const char *fmt, ...)
{
    if (log->src_include & src && !(log->src_exclude & src)) {
        TA_LOCK(log->mutex);
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
        TA_UNLOCK(log->mutex);
    }
}

void ta_log_free(ta_log *log)
{
    ta_log_flush(&tg_debug_log);
    if (log->filename) {
        fclose(log->stream);
    }
    if (log->mutex) {
        //SDL_DestroyMutex(log->mutex);
    }
}