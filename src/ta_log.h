#pragma once
#include "dlb/dlb_types.h"

typedef struct _iobuf FILE;

typedef enum ta_log_source {
    SRC_NONE       = 0x00000000,
    SRC_ASSERT     = 0x00000001,
    SRC_AUDIO      = 0x00000002,
    SRC_CONSOLE    = 0x00000004,
    SRC_EDITOR     = 0x00000008,
    SRC_EVENT      = 0x00000010,
    SRC_FILE       = 0x00000020,
    SRC_FONT       = 0x00000040,
    SRC_GAME       = 0x00000080,
    SRC_GLTF       = 0x00000100,
    SRC_JSON       = 0x00000200,
    SRC_MATH       = 0x00000400,
    SRC_OPENGL     = 0x00000800,
    SRC_PRIMITIVE  = 0x00001000,
    SRC_RENDER     = 0x00002000,
    SRC_RIGID_BODY = 0x00004000,
    SRC_SCENE      = 0x00008000,
    SRC_SHADER     = 0x00010000,
    SRC_SYSTEM     = 0x00020000,
    SRC_TEXTURE    = 0x00040000,
    SRC_WINDOW     = 0x00080000,

    //...
    //SRC_LAST              = 0x8fffffff

    SRC_ALL = SRC_ASSERT | SRC_AUDIO | SRC_CONSOLE| SRC_EDITOR | SRC_EVENT | SRC_FILE | SRC_FONT | SRC_GAME | SRC_GLTF
            | SRC_JSON | SRC_MATH | SRC_OPENGL | SRC_PRIMITIVE | SRC_RENDER | SRC_RIGID_BODY | SRC_SCENE | SRC_SHADER
            | SRC_SYSTEM | SRC_TEXTURE | SRC_WINDOW
} ta_log_source;

// TODO: Actually use this
typedef enum ta_log_level {
    LEVEL_NONE  = 0x00000000,
    LEVEL_DEBUG = 0x00000001,
    LEVEL_INFO  = 0x00000002,
    LEVEL_WARN  = 0x00000004,
    LEVEL_ERROR = 0x00000008,
    LEVEL_FATAL = 0x00000010,
} ta_log_level;

typedef struct ta_log {
    const char  *filename;      // relative path to log file
    FILE        *stream;        // file stream to write to
    bool        flush;          // if true, flush log after every write (also flushed stdout when echo = true)
    bool        echo;           // if true, echo all log writes to stdout
    u32         src_include;    // log source bitmap, 1 = log this source
    u32         src_exclude;    // log source bitmap, 1 = exclude this source (overrides include)
    u32         level_filter;   // TODO(unused): log level filter
    void        *mutex;         // TODO(unused): was SDL_mutex when I was testing threading
} ta_log;

extern ta_log tg_debug_log;

const char *ta_log_source_str(ta_log_source src);

void ta_log_init        (ta_log *log, FILE *stream, bool flush, bool echo, u32 src_include, u32 src_exclude);
void ta_log_init_file   (ta_log *log, const char *filename, bool flush, bool echo, u32 src_include, u32 src_exclude);
void ta_log_flush       (ta_log *log);
void ta_log_write       (ta_log *log, u32 src, const char *fmt, ...);
//void ta_log_append      (ta_log *log, const char* fmt, ...);
void ta_log_free        (ta_log *log);