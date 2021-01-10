#pragma once
#include "ta_schema.h"
#include "dlb/dlb_types.h"
#include "misc/glad.h"

// NOTE: Needs to match the GLSL #define
#define TA_TEXTURE_POOL_MAX 16

typedef struct ta_texture_pool {
    u32 width;
    u32 height;
    const char **layers;    // vector[layers], if null layer is unused

    // GL_DEPTH_COMPONENT        0x1902 6402
    // GL_RED                    0x1903 6403
    // GL_RGB                    0x1907 6407
    // GL_RGBA                   0x1908 6408
    // GL_BGR                    0x80E0 32992
    // GL_BGRA                   0x80E1 32993
    GLenum format;

    // GL_BYTE                   0x1400 5120
    // GL_UNSIGNED_BYTE          0x1401 5121
    // GL_SHORT                  0x1402 5122
    // GL_UNSIGNED_SHORT         0x1403 5123
    // GL_INT                    0x1404 5124
    // GL_UNSIGNED_INT           0x1405 5125
    // GL_FLOAT                  0x1406 5126
    GLenum type;

    GLuint gl_id;           // [GL] texture id
    GLint  gl_filter_min;   // [GL] current filter mode
    GLint  gl_filter_mag;   // [GL] current filter mode
} ta_texture_pool;

typedef struct ta_texturing {
    ta_texture_pool *texture_pools;  // NOTE: Fixed array of size TA_TEXTURE_POOL_MAX
} ta_texturing;

typedef enum ta_texture_type {
    TA_TEXTURE_2D_ARRAY
} ta_texture_type;

//#pragma warning(push)
//#pragma warning(disable: 4201) // nameless struct/union
typedef struct ta_texture {
    TA_RESOURCE_HEADER
    ta_texture_type type;       // texture type (2D, cubemap)
    const char *path;           // relative file path
    u32 width;                  // Size of texture (pixels)
    u32 height;
    u8 channels;                // Number of color channels (1, 2, 4)

    // GL_DEPTH_COMPONENT        0x1902 6402
    // GL_RED                    0x1903 6403
    // GL_RGB                    0x1907 6407
    // GL_RGBA                   0x1908 6408
    // GL_BGR                    0x80E0 32992
    // GL_BGRA                   0x80E1 32993
    GLenum pixels_format;

    // GL_BYTE                   0x1400 5120
    // GL_UNSIGNED_BYTE          0x1401 5121
    // GL_SHORT                  0x1402 5122
    // GL_UNSIGNED_SHORT         0x1403 5123
    // GL_INT                    0x1404 5124
    // GL_UNSIGNED_INT           0x1405 5125
    // GL_FLOAT                  0x1406 5126
    GLenum pixels_type;

    u8 *pixels;                 // Array of pixels (if inlined instead of via path)
    GLenum gl_internal_format;

    // GL_NEAREST                0x2600 9728
    // GL_LINEAR                 0x2601 9729
    // GL_NEAREST_MIPMAP_NEAREST 0x2700 9984
    // GL_LINEAR_MIPMAP_NEAREST  0x2701 9985
    // GL_NEAREST_MIPMAP_LINEAR  0x2702 9986
    // GL_LINEAR_MIPMAP_LINEAR   0x2703 9987
    GLint gl_filter_min;        // [GL] requested filter mode (default: GL_LINEAR_MIPMAP_LINEAR)
    GLint gl_filter_mag;        // [GL] requested filter mode (default: GL_NEAREST)

    GLuint gl_id;
    u32 gl_texture_pool_index;  // texture pool index
    u32 gl_texture_pool_layer;  // array texture layer
} ta_texture;
//#pragma warning(pop)

const char *ta_gl_pixels_format_str (GLenum format);
const char *ta_gl_pixels_type_str   (GLenum type);

void ta_texturing_init              (ta_texturing *texturing);
void ta_texture_pool_bind           (ta_texture_pool *texture_pool);
void ta_texture_pool_unbind         ();
void ta_texture_pool_set_filter_mode(ta_texture_pool *texture_pool, GLint min, GLint mag);
void ta_texture_pool_set_layer_texels(ta_texture_pool *texture_pool, int layer, GLenum format, GLenum type, void *texels);

const char *ta_texture_type_str     (int type);
void ta_texture_init                (ta_texture *tex);
void ta_texture_init_void           (void *tex);
GLenum ta_texture_target            (ta_texture *tex);
ta_texture_pool *ta_texture_texture_pool(ta_texture *tex);
//void ta_texture_bind                (ta_texture *tex);
//void ta_texture_unbind              (ta_texture *tex);
// Load data from memory into VRAM
//void ta_texture_create_and_bind     (ta_texture *tex);
// Load data from disk (if `path` instead of `pixels`) then create VRAM textures
void ta_texture_upload              (ta_texture *tex, u8 *pixels);
void ta_texture_load                (ta_texture *tex);
void ta_texture_delete              (ta_texture *tex);
void ta_texture_hot_reload              (ta_texture *tex);
void ta_texture_free                (ta_texture *tex);
void ta_texture_free_void           (void *tex);
