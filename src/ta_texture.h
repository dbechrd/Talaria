#pragma once
#include "dlb/dlb_types.h"
#include "ta_uid.h"
#include "misc/gl3w.h"

typedef enum ta_texture_type {
    TA_TEXTURE_2D,
    TA_TEXTURE_CUBEMAP,
    TA_TEXTURE_COUNT
} ta_texture_type;

//#pragma warning(push)
//#pragma warning(disable: 4201) // nameless struct/union
typedef struct ta_texture {
    size_t index;
    const char *name;
    ta_texture_type type;
    union {
        const char *path;           // File path
        const char *path_faces[6];  // File paths (6 cubemap faces)
    } data;
    u8 *pixels;        // Pixel data (if inlined instead of via path)
    u32 width;         // Size of texture (pixels)
    u32 height;
    u8 channels;       // Number of color channels (1, 2, 4)
    bool linear;       // True if linear color space. E.g. metallic, etc.
    bool repeat;       // true = REPEAT, false = CLAMP_TO_EDGE
    GLint gl_filter_min;
    GLint gl_filter_mag;
    GLuint gl_id;
} ta_texture;
//#pragma warning(pop)

const char *ta_texture_type_str(int type);
// Initialize any implicit properties
void ta_texture_init(ta_texture *tex);
void ta_texture_bind(ta_texture *tex);
void ta_texture_unbind(ta_texture *tex);
// Load data from memory into VRAM
void ta_texture_create_and_bind(ta_texture *tex);
// Load data from disk (if `path` instead of `pixels`) then create VRAM textures
void ta_texture_load(ta_texture *tex);
void ta_texture_delete(ta_texture *tex);
void ta_texture_free(ta_texture *tex);