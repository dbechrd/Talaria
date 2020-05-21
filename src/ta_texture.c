#include "ta_texture.h"
#include "ta_log.h"
#include "dlb/dlb_types.h"
#include "dlb/dlb_memory.h"
#include "dlb/dlb_vector.h"

#pragma warning(push)
#pragma warning(disable: 26451)
#define STBI_ASSERT(x) DLB_ASSERT(x)
#define STBI_MALLOC dlb_malloc
#define STBI_REALLOC dlb_realloc
#define STBI_FREE dlb_free
#define STBI_ONLY_PNG
#define STBI_ONLY_TGA
#define STB_IMAGE_IMPLEMENTATION
#include "misc/stb_image.h"
#pragma warning(pop)

const char *ta_texture_type_str(int type)
{
    switch (type) {
        case TA_TEXTURE_2D:      return "TA_TEXTURE_2D     ";
        case TA_TEXTURE_CUBEMAP: return "TA_TEXTURE_CUBEMAP";
        default: DLB_ASSERT(0);  return "TA_TEXTURE_???    ";
    }
}

void ta_texture_init(ta_texture *tex)
{
    // GL_NEAREST                 texel nearest
    // GL_NEAREST_MIPMAP_NEAREST  texel nearest, mipmap nearest
    // GL_NEAREST_MIPMAP_LINEAR   texel nearest, mipmap blend
    // GL_LINEAR                  texel 2x2 avg
    // GL_LINEAR_MIPMAP_NEAREST   texel 2x2 avg, mipmap nearest
    // GL_LINEAR_MIPMAP_LINEAR    texel 2x2 avg, mipmap blend

    if (!tex->gl_filter_min) {
        tex->gl_filter_min = GL_LINEAR_MIPMAP_LINEAR;
        //tex->gl_filter_min = GL_NEAREST;
    }
    if (!tex->gl_filter_mag) {
        tex->gl_filter_mag = GL_NEAREST;
    }

    if (tex->pixels || tex->data.path) {
        ta_texture_load(tex);
    }
}

static inline GLenum texture_target(ta_texture *tex)
{
    GLenum target = tex->type == TA_TEXTURE_CUBEMAP ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;
    return target;
}

void ta_texture_bind(ta_texture *tex)
{
    GLenum target = texture_target(tex);
    glBindTexture(target, tex->gl_id);
}

void ta_texture_unbind(ta_texture *tex)
{
    GLenum target = texture_target(tex);
    glBindTexture(target, 0);
}

void ta_texture_create_and_bind(ta_texture *tex)
{
    DLB_ASSERT(!tex->gl_id);

    ta_log_write(&tg_debug_log, SRC_TEXTURE,
        "Generating GPU texture %s (w: %d, h: %d, channels: %d)\n",
        tex->name, tex->width, tex->height, tex->channels);

// TODO: Too much variance in engine load times.. not obvious if this is better or not.
#if 0
    static GLuint *tex_id_pool = 0;
    static size_t next_id = 0;
    size_t pool_len = dlb_vec_len(tex_id_pool);
    if (next_id >= pool_len) {
        size_t new_pool_len = MAX(pool_len * 2, 32);
        dlb_vec_alloc_count(tex_id_pool, new_pool_len - pool_len);
        glGenTextures((GLsizei)(new_pool_len - pool_len), tex_id_pool + pool_len);
        ta_log_write(&tg_debug_log, SRC_TEXTURE, "tex_id_pool resized\n", tex->gl_id);
    }
    tex->gl_id = tex_id_pool[next_id];
    next_id++;
    ta_log_write(&tg_debug_log, SRC_TEXTURE, "Texture ID %u claimed\n", tex->gl_id);
#else
    glGenTextures(1, &tex->gl_id);
    ta_log_write(&tg_debug_log, SRC_TEXTURE, "Texture ID %u generated\n", tex->gl_id);
#endif

    GLenum target = texture_target(tex);
    glBindTexture(target, tex->gl_id);
    ta_log_write(&tg_debug_log, SRC_TEXTURE, "Texture ID bound\n");

    GLint param = tex->repeat ? GL_REPEAT : GL_CLAMP_TO_EDGE;
    glTexParameteri(target, GL_TEXTURE_WRAP_S, param);
    glTexParameteri(target, GL_TEXTURE_WRAP_T, param);
    if (target == GL_TEXTURE_CUBE_MAP) {
        glTexParameteri(target, GL_TEXTURE_WRAP_R, param);
    }

    glTexParameteri(target, GL_TEXTURE_MIN_FILTER, tex->gl_filter_min);
    glTexParameteri(target, GL_TEXTURE_MAG_FILTER, tex->gl_filter_mag);
    ta_log_write(&tg_debug_log, SRC_TEXTURE, "Texture parameters set\n");

    //GLuint *gl_id = dlb_vec_alloc(gl_ids[queue]);
    //*gl_id = texture->gl_id;

    ta_log_write(&tg_debug_log, SRC_TEXTURE, "Generation complete.\n");
}

// Read in little-endian short from char array
static short texture_le_short(unsigned char *bytes)
{
    return bytes[0] | ((char)bytes[1] << 8);
}
static u8 *texture_read_tga(const char *path, u32 *width, u32 *height,
    u8 *channels, bool flip_y)
{
    struct tga_header
    {
        unsigned char  id_length;
        char  color_map_type;
        char  data_type_code;
        unsigned char  color_map_origin[2];
        unsigned char  color_map_length[2];
        char  color_map_depth;
        unsigned char  x_origin[2];
        unsigned char  y_origin[2];
        unsigned char  width[2];
        unsigned char  height[2];
        char  bits_per_pixel;
        char  image_descriptor;
    } header;

    ta_log_write(&tg_debug_log, SRC_TEXTURE, "Reading TGA from disk %s\n", path);

    size_t i, color_map_size, pixels_size;
    FILE *f;
    size_t read;
    u8 *pixels;

    f = fopen(path, "rb");

    if (!f) {
        fprintf(stderr, "Unable to open %s for reading\n", path);
        return NULL;
    }

    read = fread(&header, 1, sizeof(header), f);

    if (read != sizeof(header)) {
        fprintf(stderr, "%s has incomplete tga header\n", path);
        fclose(f);
        return NULL;
    }
    if (header.data_type_code != 2 && header.data_type_code != 3) {
        fprintf(stderr, "%s is not an uncompressed RGB tga file\n", path);
        fclose(f);
        return NULL;
    }
    DLB_ASSERT(header.bits_per_pixel == 8 || header.bits_per_pixel == 24 || header.bits_per_pixel == 32);

    for (i = 0; i < header.id_length; ++i) {
        if (getc(f) == EOF) {
            fprintf(stderr, "%s has incomplete id string\n", path);
            fclose(f);
            return NULL;
        }
    }

    color_map_size = (size_t)texture_le_short(header.color_map_length) * (header.color_map_depth / 8);
    for (i = 0; i < color_map_size; ++i) {
        if (getc(f) == EOF) {
            fprintf(stderr, "%s has incomplete color map\n", path);
            fclose(f);
            return NULL;
        }
    }

    *width = (u32)texture_le_short(header.width);
    *height = (u32)texture_le_short(header.height);
    *channels = (u8)header.bits_per_pixel / 8;
    pixels_size = (size_t)*width * *height * *channels;
    pixels = dlb_malloc(pixels_size);
    DLB_ASSERT(pixels);

    if (flip_y) {
        ta_log_write(&tg_debug_log, SRC_TEXTURE, "Perf Note: Performing vertical flip on %s\n", path);
        // Vertical flip (slower)
        read = 0;
        int row_width = *width * *channels;
        for (int row = *height - 1; row >= 0; --row) {
            read += fread((char *)pixels + (size_t)row * row_width, 1, row_width, f);
        }
    } else {
        read = fread(pixels, 1, pixels_size, f);
    }
    fclose(f);

    if (read != pixels_size) {
        fprintf(stderr, "%s has incomplete image\n", path);
        dlb_free(pixels);
        return NULL;
    }

    ta_log_write(&tg_debug_log, SRC_TEXTURE, "TGA read complete\n", path);
    return pixels;
}
static void texture_upload(ta_texture *tex, int face, u8 *pixels, bool bgr)
{
    DLB_ASSERT(tex->width);
    DLB_ASSERT(tex->height);
    DLB_ASSERT(tex->channels);
    DLB_ASSERT(pixels);

    ta_log_write(&tg_debug_log, SRC_TEXTURE, "Uploading texture to GPU %s\n", tex->name);
    GLint format_internal = 0;
    GLint format = 0;
    switch (tex->channels)
    {
        case 1: // Grayscale
            //DLB_ASSERT(tex->linear);  // OpenGL doesn't support sRGB for grayscale
            format_internal = GL_R8;
            format = GL_RED;
            break;
        case 3: // RGB
            format_internal = tex->linear ? GL_RGB8 : GL_SRGB8;
            format = bgr ? GL_BGR : GL_RGB;
            break;
        case 4: // RGBA
            format_internal = tex->linear ? GL_RGBA8 : GL_SRGB8_ALPHA8;
            format = bgr ? GL_BGRA : GL_RGBA;
            break;
        default: // Unsupported BPP
            DLB_ASSERT(!"Sorry, don't know what to do with this texture, man.");
    }

    GLenum target = texture_target(tex);
    if (target == GL_TEXTURE_CUBE_MAP) {
        target = GL_TEXTURE_CUBE_MAP_POSITIVE_X + face;
    }
    glTexImage2D(target, 0, format_internal, tex->width, tex->height, 0, format, GL_UNSIGNED_BYTE, pixels);

    ta_log_write(&tg_debug_log, SRC_TEXTURE, "Upload complete.\n", tex->name);
}
static void texture_generate_mipmap(ta_texture *tex)
{
    // TODO: Are there any other reasons to generate mipmaps?
    // Only generate mipmap if requested filtering mode requires it
    if (tex->width > 1 && tex->height > 1 &&
        (tex->gl_filter_min != GL_NEAREST && tex->gl_filter_min != GL_LINEAR) ||
        (tex->gl_filter_mag != GL_NEAREST && tex->gl_filter_mag != GL_LINEAR))
    {
        ta_log_write(&tg_debug_log, SRC_TEXTURE, "Generating mipmap for %s\n", tex->name);
        ta_log_timed_region_start(&tg_debug_log, SRC_TEXTURE, CSTR("texture_generate_mipmap"));

        GLenum target = texture_target(tex);
        glGenerateMipmap(target);

        ta_log_timed_region_end(&tg_debug_log, CSTR("texture_generate_mipmap"));
    }
}

void ta_texture_load(ta_texture *tex)
{
    ta_log_write(&tg_debug_log, SRC_TEXTURE, "Loading texture %s\n", tex->name);
    ta_log_timed_region_start(&tg_debug_log, SRC_TEXTURE, CSTR("ta_texture_load"));

    if (tex->type == TA_TEXTURE_2D) {
        DLB_ASSERT(tex->pixels || tex->data.path);
    } else if (tex->type == TA_TEXTURE_CUBEMAP) {
        DLB_ASSERT(
            tex->data.path_faces[0] &&
            tex->data.path_faces[1] &&
            tex->data.path_faces[2] &&
            tex->data.path_faces[3] &&
            tex->data.path_faces[4] &&
            tex->data.path_faces[5]
        );
    } else {
        DLB_ASSERT(!"invalid texture type");
    }
    ta_texture_create_and_bind(tex);

    // Pixel textures contain inlined pixel data, path should be null
    if (tex->pixels) {
        DLB_ASSERT(tex->type == TA_TEXTURE_2D);
        texture_upload(tex, 0, tex->pixels, false);
    } else {
        // Load image data from file(s) and upload to VRAM
        int face_count = tex->type == TA_TEXTURE_2D ? 1 : 6;
        for (int i = 0; i < face_count; ++i) {
            const char *path = tex->data.path_faces[i];

            u32 width = 0;
            u32 height = 0;
            u8 channels = 0;
            u8 *pixels = texture_read_tga(path, &width, &height, &channels, tex->flip_y);
            if (!pixels) {
                ta_log_write(&tg_debug_log, SRC_TEXTURE, "Failed to load tex: %s\n", path);
                DLB_ASSERT(!"ta_texture_init: Failed to load tex");
            }

            if (face_count > 1 && tex->width) {
                // Ensure textures are all the same size/type
                DLB_ASSERT(tex->width == width);
                DLB_ASSERT(tex->height == height);
                DLB_ASSERT(tex->channels == channels);
            } else {
                tex->width = width;
                tex->height = height;
                tex->channels = channels;
            }

            texture_upload(tex, i, pixels, true);
            dlb_free(pixels);

            //stbi_set_flip_vertically_on_load(true);
            //u8 *pixels = stbi_load(tex->path, &w, &h, &channels, tex->channels);
            //if (!pixels) {
            //    const char *reason = stbi_failure_reason();
            //    ta_log_write(&tg_debug_log, SRC_TEXTURE,
            //        "Failed to load tex: %s\nSTBI Reason: %s\n", tex->path, reason);
            //    DLB_ASSERT(!"ta_texture_init: Failed to load tex");
            //}
            //
            //tex->width = w;
            //tex->height = h;
            //tex->channels = channels;
            //ta_texture_load(tex, pixels, 0);
            //stbi_image_free(pixels);
        }
    }

    texture_generate_mipmap(tex);
    ta_texture_unbind(tex);

    ta_log_timed_region_end(&tg_debug_log, CSTR("ta_texture_load"));
}

void ta_texture_delete(ta_texture *tex)
{
    if (tex->gl_id) {
        glDeleteTextures(1, &tex->gl_id);
    }
    tex->gl_id = 0;
}

void ta_texture_free(ta_texture *tex)
{
    dlb_vec_free(tex->pixels);
    ta_texture_delete(tex);

    // TODO(perf): Delete all scene textures in a single GL call by aggregating
    //             gl_ids during texture initialization.
    //glDeleteTextures(dlb_vec_len(gl_ids[queue]), gl_ids[queue]);
    //dlb_vec_clear(tex[queue]);
    //dlb_vec_clear(gl_ids[queue]);
}