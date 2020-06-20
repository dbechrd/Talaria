#pragma once
#include "ta_schema.h"
#include "dlb/dlb_types.h"
#include "misc/glad.h"

typedef struct ta_texture_array {
    int width;
    int height;
    int layers;
    const char *layer_textures;  // vector[layers], if null layer is unused
    GLuint gl_id;
} ta_texture_array;

// NOTE: Needs to match #defines in GLSL
typedef enum ta_texture_array_size {
    TA_TEXTURE_POOL_1      = 0,
    //TA_TEXTURE_POOL_2,
    //TA_TEXTURE_POOL_4,
    //TA_TEXTURE_POOL_8,
    //TA_TEXTURE_POOL_16,
    TA_TEXTURE_POOL_32     = 1,
    //TA_TEXTURE_POOL_64,
    //TA_TEXTURE_POOL_128,
    //TA_TEXTURE_POOL_256,
    TA_TEXTURE_POOL_512    = 2,
    TA_TEXTURE_POOL_1024   = 3,
    TA_TEXTURE_POOL_2048   = 4,
    TA_TEXTURE_POOL_4096   = 5,
    TA_TEXTURE_POOL_COUNT  = 6
} ta_texture_array_size;

typedef struct ta_texturing {
    ta_texture_array textures_arrays[TA_TEXTURE_POOL_COUNT];  // sampler2DArray
} ta_texturing;

typedef enum ta_texture_type {
    TA_TEXTURE_2D,
    TA_TEXTURE_CUBEMAP,
    TA_TEXTURE_COUNT
} ta_texture_type;

//#pragma warning(push)
//#pragma warning(disable: 4201) // nameless struct/union
typedef struct ta_texture {
    TA_RESOURCE_HEADER
    ta_texture_type type;   // texture type (2D, cubemap)
    union {
        const char *path;           // relative file path
        const char *path_faces[6];  // relative file paths (6 cubemap faces)
    } data;
    u32     width;          // Size of texture (pixels)
    u32     height;
    u8      channels;       // Number of color channels (1, 2, 4)
    u8      *pixels;        // Array of pixels (if inlined instead of via path)
    bool    linear;         // True if linear color space. E.g. metallic, etc.
    bool    repeat;         // true = REPEAT, false = CLAMP_TO_EDGE
    bool    flip_y;         // vertical flip pixel data when loading
    // GL_NEAREST                 0x2600 = 9728
    // GL_LINEAR                  0x2601 = 9729
    // GL_NEAREST_MIPMAP_NEAREST  0x2700 = 9984
    // GL_LINEAR_MIPMAP_NEAREST   0x2701 = 9985
    // GL_NEAREST_MIPMAP_LINEAR   0x2702 = 9986
    // GL_LINEAR_MIPMAP_LINEAR    0x2703 = 9987
    GLint   gl_filter_min;  // [GL] filter mode for minification  (default: GL_LINEAR_MIPMAP_LINEAR)
    GLint   gl_filter_mag;  // [GL] filter mode for magnification (default: GL_NEAREST)
    GLuint  gl_id;          // [GL] texture id
} ta_texture;
//#pragma warning(pop)

void ta_texturing_init              (ta_texturing *texturing);

const char *ta_texture_type_str     (int type);
void ta_texture_init                (ta_texture *tex);
void ta_texture_bind                (ta_texture *tex);
void ta_texture_unbind              (ta_texture *tex);
// Load data from memory into VRAM
void ta_texture_create_and_bind     (ta_texture *tex, GLuint *gl_id);
// Load data from disk (if `path` instead of `pixels`) then create VRAM textures
void ta_texture_load                (ta_texture *tex);
void ta_texture_delete              (ta_texture *tex);
void ta_texture_reload              (ta_texture *tex);
void ta_texture_free                (ta_texture *tex);