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

static void ta_texture_pool_create_and_bind(ta_texture_pool *texture_pool)
{
    glGenTextures(1, &texture_pool->gl_id);
    glBindTexture(GL_TEXTURE_2D_ARRAY, texture_pool->gl_id);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, texture_pool->width, texture_pool->height,
        (GLsizei)dlb_vec_cap(texture_pool->layers), 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);  //GL_CLAMP_TO_EDGE
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);  //GL_CLAMP_TO_EDGE
}

static void ta_texture_pool_init_and_bind(ta_texture_pool *texture_pool, int width, int height, size_t layers)
{
    texture_pool->width = width;
    texture_pool->height = height;
    dlb_vec_reserve_fixed(texture_pool->layers, layers);
    dlb_vec_alloc_count(texture_pool->layers, layers);
    ta_texture_pool_create_and_bind(texture_pool);
}

void ta_texture_pool_bind(ta_texture_pool *texture_pool)
{
    DLB_ASSERT(texture_pool->gl_id);
    glBindTexture(GL_TEXTURE_2D_ARRAY, texture_pool->gl_id);
}

void ta_texture_pool_unbind()
{
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
}

void ta_texture_pool_set_filter_mode(ta_texture_pool *texture_pool, GLint min, GLint mag)
{
    texture_pool->gl_filter_min = min;
    texture_pool->gl_filter_mag = mag;
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, texture_pool->gl_filter_min);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, texture_pool->gl_filter_mag);
}

void ta_texture_pool_set_layer_texels(ta_texture_pool *texture_pool, int layer, GLenum format, u8 *texels)
{
    DLB_ASSERT(texture_pool);
    DLB_ASSERT(texture_pool->gl_id);
    DLB_ASSERT(texels);

    glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, layer, texture_pool->width, texture_pool->height, 1, format,
        GL_UNSIGNED_BYTE, texels);
}

void ta_texturing_init(ta_texturing *texturing)
{
    UNUSED(texturing);

    ta_texture_pool_init_and_bind(dlb_vec_alloc(texturing->texture_pools),    1,    1, 32);
    ta_texture_pool_init_and_bind(dlb_vec_alloc(texturing->texture_pools),   64,   64, 32);
    ta_texture_pool_init_and_bind(dlb_vec_alloc(texturing->texture_pools),  512,  512, 32);
    ta_texture_pool_init_and_bind(dlb_vec_alloc(texturing->texture_pools), 1024, 1024, 32);
    ta_texture_pool_init_and_bind(dlb_vec_alloc(texturing->texture_pools), 2048, 2048, 32);
    ta_texture_pool_init_and_bind(dlb_vec_alloc(texturing->texture_pools), 4096, 4096, 32);
    ta_texture_pool_unbind();

    // TODO: Generate mipmaps whenever we change the array? After we load everything? Hmm..
    //glGenerateMipmap(GL_TEXTURE_2D_ARRAY);

    // TODO: Fill unused textures with some default texture data to make it obvious when we accidentally use the wrong
    // layer or forget to regenerate mipmaps? I think we can use glTexSubImage3D to fill all layers at the same time.
    //// 1x1, RGBA
    //u8 texels[4] = { 0 };
    //texels[3] = 0xff;
    //for (u32 i = 0; i < 32; ++i) {
    //    texels[0] = (u8)(i * 256 / 32);
    //    texels[1] = (u8)(i * 256 / 32);
    //    texels[2] = (u8)(i * 256 / 32);
    //    ta_texture_pool_set_layer_texels(&texturing->texture_pools[TA_TEXTURE_POOL_1], i, texels);
    //}
}

static void ta_texturing_add_texture(ta_texturing *texturing, ta_texture *tex)
{
    DLB_ASSERT(tex->type == TA_TEXTURE_2D_ARRAY);
    DLB_ASSERT(!tex->gl_id);

    bool found = false;
    u32 index = 0;
    dlb_vec_each(ta_texture_pool *, tex_pool, texturing->texture_pools) {
        if (tex_pool->width == tex->width && tex_pool->height == tex->height) {
            tex->gl_texture_pool_index = index;
            u32 layer = 0;
            dlb_vec_each(const char **, layer_name, tex_pool->layers) {
                if (*layer_name == 0 || *layer_name == tex->name) {
                    *layer_name = tex->name;
                    tex->gl_texture_pool_layer = layer;
                    found = true;
                    break;
                }
                layer++;
            }
            if (found) break;
        }
        index++;
    }
    // TODO: What to do when no matching pool found? For now, just hard crash, but we could do something fancy like
    // make a new pool automatically, or even pack non power-of-two textures into atlases. Avoid complexity if possible.
    DLB_ASSERT(found);
}

const char *ta_texture_type_str(int type)
{
    switch (type) {
        case TA_TEXTURE_2D:         return "TA_TEXTURE_2D     ";
        case TA_TEXTURE_2D_ARRAY:   return "TA_TEXTURE_2D_POOLED";
        default: DLB_ASSERT(0);     return "TA_TEXTURE_???    ";
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

    ta_texture_load(tex);
}
void ta_texture_init_void(void *tex)
{
    ta_texture_init(tex);
}

GLenum ta_texture_target(ta_texture *tex)
{
    GLenum target = 0;
    switch (tex->type) {
        case TA_TEXTURE_2D:       target = GL_TEXTURE_2D;       break;  // 3553
        case TA_TEXTURE_2D_ARRAY: target = GL_TEXTURE_2D_ARRAY; break;  // 35966
        //case TA_TEXTURE_CUBEMAP:  target = GL_TEXTURE_CUBE_MAP; break;  // 34067
        default: DLB_ASSERT(!"Invalid texture type");
    }
    return target;
}

ta_texture_pool *ta_texture_texture_pool(ta_texture *tex)
{
    DLB_ASSERT(!tex->gl_id);
    return &tg_game.texturing.texture_pools[tex->gl_texture_pool_index];
}

void ta_texture_bind(ta_texture *tex)
{
    GLenum target = ta_texture_target(tex);
    if (tex->gl_id) {
        glBindTexture(target, tex->gl_id);
    } else {
        ta_texture_pool *tex_pool = &tg_game.texturing.texture_pools[tex->gl_texture_pool_index];
        glBindTexture(target, tex_pool->gl_id);
    }
}

void ta_texture_unbind(ta_texture *tex)
{
    GLenum target = ta_texture_target(tex);
    glBindTexture(target, 0);
}

#if 0
void ta_texture_create_and_bind(ta_texture *tex)
{
    ta_log_write(&tg_debug_log, SRC_TEXTURE,
        "Generating GPU texture %s (w: %d, h: %d, channels: %d)\n",
        tex->name, tex->width, tex->height, tex->channels);

    glGenTextures(1, &tex->gl_id);
    ta_log_write(&tg_debug_log, SRC_TEXTURE, "Texture ID %u generated\n", tex->gl_id);

    GLenum target = ta_texture_target(tex);
    glBindTexture(target, tex->gl_id);
    // Do we ever need GL_CLAMP_TO_EDGE? We weren't using it anywhere so I got rid of the "repeat" flag.
    glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_REPEAT);
    if (target == GL_TEXTURE_CUBE_MAP) {
        glTexParameteri(target, GL_TEXTURE_WRAP_R, GL_REPEAT);
    }
    glTexParameteri(target, GL_TEXTURE_MIN_FILTER, tex->gl_filter_min);
    glTexParameteri(target, GL_TEXTURE_MAG_FILTER, tex->gl_filter_mag);

    ta_log_write(&tg_debug_log, SRC_TEXTURE, "Generation complete.\n");
}
#endif

// Read in little-endian short from char array
static short texture_le_short(unsigned char *bytes)
{
    return bytes[0] | ((char)bytes[1] << 8);
}
static u8 *texture_read_tga(const char *path, u32 *width, u32 *height, u8 *channels)
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

    read = fread(pixels, 1, pixels_size, f);
    fclose(f);

    if (read != pixels_size) {
        ta_log_write(&tg_debug_log, SRC_TEXTURE, "ERROR: TGA pixel data does not match width/height/channels\n", path);
        dlb_free(pixels);
        return NULL;
    }

    ta_log_write(&tg_debug_log, SRC_TEXTURE, "TGA read complete\n", path);
    return pixels;
}
void ta_texture_upload(ta_texture *tex, u8 *pixels, GLint format_internal, GLint format)
{
    DLB_ASSERT(tex->width);
    DLB_ASSERT(tex->height);
    DLB_ASSERT(tex->channels);
    DLB_ASSERT(pixels);

    ta_log_write(&tg_debug_log, SRC_TEXTURE, "Uploading texture to GPU %s\n", tex->name);

    GLenum target = ta_texture_target(tex);
    switch (target) {
        case GL_TEXTURE_2D: {
            glTexImage2D(target, 0, format_internal, tex->width, tex->height, 0, format, GL_UNSIGNED_BYTE, pixels);
            break;
        } case GL_TEXTURE_2D_ARRAY: {
            ta_texture_pool *tex_pool = ta_texture_texture_pool(tex);
            switch (format_internal) {
                case GL_R8: {
                    size_t pixels_len = tex->width * tex->height;
                    u8 *padded_texels = dlb_calloc(tex->width * tex->height, 4);
                    u8 *dst = padded_texels;
                    for (size_t i = 0; i < pixels_len; i += 1, dst += 4) {
                        dst[0] = pixels[i];
                    }
                    ta_texture_pool_set_layer_texels(tex_pool, tex->gl_texture_pool_layer, GL_RGBA, padded_texels);
                    dlb_free(padded_texels);
                    break;
                } case GL_RGB8: {
                    DLB_ASSERT(tex->channels == 3);
                    size_t pixels_len = tex->width * tex->height * 3;
                    u8 *padded_texels = dlb_calloc(tex->width * tex->height, 4);
                    u8 *src = pixels;
                    u8 *dst = padded_texels;
                    for (size_t i = 0; i < pixels_len; i += 3, src += 3, dst += 4) {
                        dst[0] = src[0];
                        dst[1] = src[1];
                        dst[2] = src[2];
                        dst[3] = 0xff;
                    }
                    format = (format == GL_BGR) ? GL_BGRA : GL_RGBA;
                    ta_texture_pool_set_layer_texels(tex_pool, tex->gl_texture_pool_layer, format, padded_texels);
                    dlb_free(padded_texels);
                    break;
                } case GL_RGBA8: {
                    DLB_ASSERT(tex->channels == 4);
                    ta_texture_pool_set_layer_texels(tex_pool, tex->gl_texture_pool_layer, format, pixels);
                    break;
                } default: {
                    DLB_ASSERT(!"Uknown texture format");
                }
            }
            break;
        } default: {
            DLB_ASSERT(!"Uknown texture target");
        }
    }

    // https://github.com/hglm/detex/blob/master/dds.c
    // https://github.com/nothings/stb/blob/master/stb_dxt.h

    // TODO: Handle S3TC formats
    // https://www.khronos.org/opengl/wiki/S3_Texture_Compression
    // http://www.buckarooshangar.com/flightgear/tut_dds.html
    // Extension: EXT_texture_compression_s3tc
    // GL_COMPRESSED_RGB_S3TC_DXT1_EXT  ("BC1")
    // GL_COMPRESSED_RGBA_S3TC_DXT1_EXT ("BC1")
    // GL_COMPRESSED_RGBA_S3TC_DXT3_EXT ("BC2")
    // GL_COMPRESSED_RGBA_S3TC_DXT5_EXT ("BC3")

    // TODO: Possibly use RGTC for normal maps?
    // https://www.khronos.org/opengl/wiki/Red_Green_Texture_Compression
    // Extension: ARB_texture_compression_rgtc
    // GL_COMPRESSED_RED_RGTC1          ("BC4")
    // GL_COMPRESSED_SIGNED_RED_RGTC1   ("BC4")
    // GL_COMPRESSED_RG_RGTC2           ("BC5")
    // GL_COMPRESSED_SIGNED_RG_RGTC2    ("BC5")

    ta_log_write(&tg_debug_log, SRC_TEXTURE, "Upload complete.\n", tex->name);
}
// NOTE: Assumes texture is already bound, should probably assert this
static void texture_generate_mipmap(ta_texture *tex)
{
    // TODO: Are there any other reasons to generate mipmaps?
    // Only generate mipmap if requested filtering mode requires it
    if (tex->width > 1 && tex->height > 1 &&
        ((tex->gl_filter_min != GL_NEAREST && tex->gl_filter_min != GL_LINEAR) ||
        (tex->gl_filter_mag != GL_NEAREST && tex->gl_filter_mag != GL_LINEAR)))
    {
        ta_log_write(&tg_debug_log, SRC_TEXTURE, "Generating mipmap for %s\n", tex->name);
        ta_log_timed_region_start(&tg_debug_log, SRC_TEXTURE, CSTR("texture_generate_mipmap"));

        GLenum target = ta_texture_target(tex);
        glGenerateMipmap(target);

        ta_log_timed_region_end(&tg_debug_log, CSTR("texture_generate_mipmap"));
    }
}

void ta_texture_load(ta_texture *tex)
{
    ta_log_write(&tg_debug_log, SRC_TEXTURE, "Loading texture %s\n", tex->name);
    ta_log_timed_region_start(&tg_debug_log, SRC_TEXTURE, CSTR("ta_texture_load"));

    // Pixel textures contain inlined pixel data, path should be null
    if (tex->path) {
        // NOTE: Assuming all texture paths point to BGR TGA images for now.
        const bool bgr = true;

        // Load image data from file(s) and upload to VRAM
        u32 width = 0;
        u32 height = 0;
        u8 channels = 0;
        u8 *pixels = texture_read_tga(tex->path, &width, &height, &channels);
        if (!pixels) {
            ta_log_write(&tg_debug_log, SRC_TEXTURE, "Failed to load tex: %s\n", tex->path);
            DLB_ASSERT(!"ta_texture_init: Failed to load tex");
        }

        tex->width = width;
        tex->height = height;
        tex->channels = channels;

        // TODO: Always use GL_RGBA textures, but somehow pack 1 and 3 channel textures. This needs to happen on the CPU
        // side, but it would be preferable to have the packed texture data be what gets loaded from disk so we can quickly
        // dump it straight to VRAM without manually interleaving channels at texture load time.
        GLint format_internal = 0;
        GLint format = 0;
        switch (tex->channels)
        {
            case 1: // Grayscale
                format_internal = GL_R8;
                format = GL_RED;
                break;
            case 3: // RGB
                format_internal = GL_RGB8;
                format = bgr ? GL_BGR : GL_RGB;
                break;
            case 4: // RGBA
                format_internal = GL_RGBA8;
                format = bgr ? GL_BGRA : GL_RGBA;
                break;
            default: // Unsupported BPP
                DLB_ASSERT(!"Sorry, don't know what to do with this texture, man.");
        }

        ta_texturing_add_texture(&tg_game.texturing, tex);
        ta_texture_bind(tex);
        ta_texture_upload(tex, pixels, format_internal, format);
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
    } else {
        ta_texturing_add_texture(&tg_game.texturing, tex);

        if (tex->pixels) {
            ta_texture_bind(tex);

            // TODO: Always use GL_RGBA textures, but somehow pack 1 and 3 channel textures. This needs to happen on the CPU
            // side, but it would be preferable to have the packed texture data be what gets loaded from disk so we can quickly
            // dump it straight to VRAM without manually interleaving channels at texture load time.
            GLint format_internal = 0;
            GLint format = 0;
            switch (tex->channels)
            {
                case 1: // Grayscale
                    format_internal = GL_R8;
                    format = GL_RED;
                    break;
                case 3: // RGB
                    format_internal = GL_RGB8;
                    format = GL_RGB;
                    break;
                case 4: // RGBA
                    format_internal = GL_RGBA8;
                    format = GL_RGBA;
                    break;
                default: // Unsupported BPP
                    DLB_ASSERT(!"Sorry, don't know what to do with this texture, man.");
            }

            ta_texture_upload(tex, tex->pixels, format_internal, format);
        }
    }

    texture_generate_mipmap(tex);
    ta_texture_unbind(tex);

    ta_log_timed_region_end(&tg_debug_log, CSTR("ta_texture_load"));
}

void ta_texture_delete(ta_texture *tex)
{
    DLB_ASSERT(tex->gl_id);
    GLuint id = tex->gl_id;
    tex->gl_id = 0;
    if (id) {
        glDeleteTextures(1, &id);
    }
}

void ta_texture_reload(ta_texture *tex)
{
    ta_texture_delete(tex);
    ta_texture_load(tex);
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
void ta_texture_free_void(void *tex)
{
    ta_texture_free(tex);
}
