#include "ta_log.h"
#include "ta_timer.h"
#include "dlb/dlb_vector.h"
#include <stdarg.h>
#include <stdio.h>
#include <time.h>

// Note: Don't use DLB_ASSERT in this file because the assert handler writes
// to the log (will cause infinite recursion).
#undef DLB_ASSERT

ta_log tg_debug_log;

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
        "[Timestamp          ][TID  ][Source    ][Elapsed   ][Message                   ]\n"
        "--------------------------------------------------------------------------------\n");
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

static ta_log_thread_state *log_get_or_create_thread_state(ta_log *log, ta_thread_id thread_id)
{
    //assert(thread_id);

    ta_log_thread_state *state = 0;
    for (int i = 0; i < MAX_THREADS; ++i) {
        if (log->thread_states[i].thread_id == thread_id) {
            state = &log->thread_states[i];
            break;
        } else if (!log->thread_states[i].thread_id) {
            state = &log->thread_states[i];
            state->thread_id = thread_id;
            break;
        }
    }
    if (!state) {
        assert(!"Thread table is full. Do clean-up or increase MAX_THREADS");
    }
    return state;
}

static ta_log_thread_state *log_get_thread_state(ta_log *log, ta_thread_id thread_id)
{
    ta_log_thread_state *state = 0;
    for (int i = 0; i < MAX_THREADS; ++i) {
        if (log->thread_states[i].thread_id == thread_id) {
            state = &log->thread_states[i];
            break;
        }
    }
    return state;
}

void ta_log_indent(ta_log *log)
{
    ta_thread_id thread_id = 0; //SDL_ThreadID();
    ta_log_thread_state *state = log_get_or_create_thread_state(log, thread_id);
    state->indent++;
}

void ta_log_unindent(ta_log *log)
{
    ta_thread_id thread_id = 0; //SDL_ThreadID();
    ta_log_thread_state *state = log_get_thread_state(log, thread_id);
    if (state && state->indent) {
        state->indent--;
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
#define LOG_PRINT(f) fprintf(f, "[%s][%5u][%10s][%8.3fms] ", timestamp, thread_id, ta_log_source_str(src), elapsed_sec)
    LOG_PRINT(log->stream);
    if (log->echo) {
        LOG_PRINT(stdout);
    }
#undef LOG_PRINT

    ta_log_thread_state *state = log_get_thread_state(log, thread_id);
    if (state) {
        dlb_vec_each(ta_log_timed_region *, region, state->timed_regions) {
            double region_elapsed_ms = elapsed_ms - region->start_ms;
            // TODO: Store and print region names
#define LOG_PRINT(f) fprintf(f, "[%s: %6.3fms] ", region->name, region_elapsed_ms)
            LOG_PRINT(log->stream);
            if (log->echo) {
                LOG_PRINT(stdout);
            }
#undef LOG_PRINT
        }
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

        ta_thread_id thread_id = 0; //SDL_ThreadID();
        ta_log_thread_state *state = log_get_thread_state(log, thread_id);
        if (state) {
            for (int i = 0; i < state->indent; ++i) {
                fprintf(log->stream, "    ");
            }
        }

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

// NOTE: Lifetime of name must be at least as long as it takes to call ta_log_timed_region_end()
void ta_log_timed_region_start(ta_log *log, u32 src, const char *name, size_t name_len)
{
    ta_thread_id thread_id = 0; //SDL_ThreadID();
    ta_log_thread_state *state = log_get_or_create_thread_state(log, thread_id);
    ta_log_timed_region region = { 0 };
    region.name = ta_symbol_intern(name, name_len);
    region.src = src;
    region.start_ms = ta_timer_elapsed_ms();

    dlb_vec_push(state->timed_regions, region);
    ta_log_write(log, src, "START\n");
}

void ta_log_timed_region_end(ta_log *log, const char *name, size_t name_len)
{
    ta_thread_id thread_id = 0; //SDL_ThreadID();
    ta_log_thread_state *state = log_get_or_create_thread_state(log, thread_id);
    ta_log_timed_region *region = dlb_vec_last(state->timed_regions);

    const char *name_sym = ta_symbol_intern(name, name_len);
    while (region) {
        //ta_log_unindent(log);
        ta_log_write(log, region->src, "END\n");
        dlb_vec_popz(state->timed_regions);
        if (region->name == name_sym) {
            break;
        }
        region = dlb_vec_last(state->timed_regions);
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
    for (int i = 0; i < MAX_THREADS; ++i) {
        dlb_vec_each(ta_log_timed_region *, region, log->thread_states[i].timed_regions) {
            // TODO: Flush all timed regions on log close? For now, just assume app does that correctly
        }
        dlb_vec_free(log->thread_states[i].timed_regions);
    }
}