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
#define STBI_ONLY_JPEG
#define STBI_ONLY_TGA
#define STB_IMAGE_IMPLEMENTATION
#include "misc/stb_image.h"
#pragma warning(pop)

const char *ta_gl_pixels_format_str(GLenum format)
{
    const char *result = 0;
    switch (format) {
        case GL_DEPTH_COMPONENT: result = "GL_DEPTH_COMPONENT";  break;
        case GL_RED:             result = "GL_RED";              break;
        case GL_RGB:             result = "GL_RGB";              break;
        case GL_RGBA:            result = "GL_RGBA";             break;
        case GL_BGR:             result = "GL_BGR";              break;
        case GL_BGRA:            result = "GL_BGRA";             break;
        default:                 result = "???";                 break;
    }
    return result;
}

const char *ta_gl_pixels_type_str(GLenum type)
{
    const char *result = 0;
    switch (type) {
        case GL_BYTE:           result = "GL_BYTE";             break;
        case GL_UNSIGNED_BYTE:  result = "GL_UNSIGNED_BYTE";    break;
        case GL_SHORT:          result = "GL_SHORT";            break;
        case GL_UNSIGNED_SHORT: result = "GL_UNSIGNED_SHORT";   break;
        case GL_INT:            result = "GL_INT";              break;
        case GL_UNSIGNED_INT:   result = "GL_UNSIGNED_INT";     break;
        case GL_FLOAT:          result = "GL_FLOAT";            break;
        default:                result = "???";                 break;
    }
    return result;
}

static void ta_texture_pool_create_and_bind(ta_texture_pool *texture_pool)
{
    TracyCZone(ctxMethod, true);
    DLB_ASSERT(texture_pool->width);
    DLB_ASSERT(texture_pool->height);
    DLB_ASSERT(texture_pool->format);
    DLB_ASSERT(texture_pool->type);

    glGenTextures(1, &texture_pool->gl_id);
    glBindTexture(GL_TEXTURE_2D_ARRAY, texture_pool->gl_id);
    //glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, texture_pool->width, texture_pool->height,
    //    (GLsizei)dlb_vec_cap(texture_pool->layers), 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, texture_pool->format, texture_pool->width, texture_pool->height,
        (GLsizei)dlb_vec_cap(texture_pool->layers), 0, texture_pool->format, texture_pool->type, 0);

    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);  //GL_CLAMP_TO_EDGE
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);  //GL_CLAMP_TO_EDGE
    TracyCZoneEnd(ctxMethod);
}

static void ta_texture_pool_init_and_bind(ta_texture_pool *texture_pool, int width, int height, size_t layers,
    GLenum format, GLenum type)
{
    TracyCZone(ctxMethod, true);
    texture_pool->width = width;
    texture_pool->height = height;
    dlb_vec_reserve_fixed(texture_pool->layers, layers);
    dlb_vec_alloc_count(texture_pool->layers, layers);
    texture_pool->format = format;
    texture_pool->type = type;
    ta_texture_pool_create_and_bind(texture_pool);
    TracyCZoneEnd(ctxMethod);
}

void ta_texture_pool_bind(ta_texture_pool *texture_pool)
{
    TracyCZone(ctxMethod, true);
    DLB_ASSERT(texture_pool);
    DLB_ASSERT(texture_pool->gl_id);

    // NOTE: Ensures that the texture binding point we use as a temp to change filter/wrap modes doesn't stomp one
    // of the active texture pool bindings.
    DLB_ASSERT(TA_TEXTURE_POOL_MAX < 32);
    glActiveTexture(GL_TEXTURE31);
    glBindTexture(GL_TEXTURE_2D_ARRAY, texture_pool->gl_id);
    TracyCZoneEnd(ctxMethod);
}

void ta_texture_pool_unbind()
{
    TracyCZone(ctxMethod, true);
    glActiveTexture(GL_TEXTURE31);
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
    TracyCZoneEnd(ctxMethod);
}

void ta_texture_pool_set_filter_mode(ta_texture_pool *texture_pool, GLint min, GLint mag)
{
    TracyCZone(ctxMethod, true);
    texture_pool->gl_filter_min = min;
    texture_pool->gl_filter_mag = mag;
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, texture_pool->gl_filter_min);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, texture_pool->gl_filter_mag);
    TracyCZoneEnd(ctxMethod);
}

void ta_texture_pool_set_layer_texels(ta_texture_pool *texture_pool, int layer, GLenum format, GLenum type, void *texels)
{
    TracyCZone(ctxMethod, true);
    DLB_ASSERT(texture_pool);
    DLB_ASSERT(texture_pool->gl_id);
    DLB_ASSERT(format);
    DLB_ASSERT(type);
    DLB_ASSERT(texels);

    TracyCZoneN(ctxSubImage, "glTexSubImage3D", true);
    glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, layer, texture_pool->width, texture_pool->height, 1, format, type,
        texels);
    TracyCZoneEnd(ctxSubImage);
    TracyCZoneEnd(ctxMethod);
}

void ta_texturing_init(ta_texturing *texturing)
{
    TracyCZone(ctxMethod, true);
    UNUSED(texturing);

    dlb_vec_reserve_fixed(texturing->texture_pools, TA_TEXTURE_POOL_MAX);
    ta_texture_pool_init_and_bind(dlb_vec_alloc(texturing->texture_pools),    1,    1, 32, GL_RGBA, GL_UNSIGNED_BYTE);
    ta_texture_pool_init_and_bind(dlb_vec_alloc(texturing->texture_pools),   16,   32, 32, GL_RGBA, GL_UNSIGNED_BYTE);
    ta_texture_pool_init_and_bind(dlb_vec_alloc(texturing->texture_pools),   32,   32, 32, GL_RGBA, GL_UNSIGNED_BYTE);
    ta_texture_pool_init_and_bind(dlb_vec_alloc(texturing->texture_pools),  256,  256, 32, GL_RGBA, GL_UNSIGNED_BYTE);
    ta_texture_pool_init_and_bind(dlb_vec_alloc(texturing->texture_pools),  256, 1024,  4, GL_RGBA, GL_UNSIGNED_BYTE);
    ta_texture_pool_init_and_bind(dlb_vec_alloc(texturing->texture_pools),  512,  512, 32, GL_RGBA, GL_UNSIGNED_BYTE);
    ta_texture_pool_init_and_bind(dlb_vec_alloc(texturing->texture_pools), 1024, 1024, 64, GL_RGBA, GL_UNSIGNED_BYTE);
    ta_texture_pool_init_and_bind(dlb_vec_alloc(texturing->texture_pools), 2048, 2048,  8, GL_RGBA, GL_UNSIGNED_BYTE);
    ta_texture_pool_init_and_bind(dlb_vec_alloc(texturing->texture_pools), 4096, 4096,  1, GL_RGBA, GL_UNSIGNED_BYTE);
    ta_texture_pool_init_and_bind(dlb_vec_alloc(texturing->texture_pools), 1024, 1024, 18, GL_DEPTH_COMPONENT, GL_FLOAT);
    ta_texture_pool_init_and_bind(dlb_vec_alloc(texturing->texture_pools), 4096, 4096,  1, GL_DEPTH_COMPONENT, GL_FLOAT);
    ta_texture_pool_unbind();

    // TODO: Fill unused textures with some default texture data to make it obvious when we accidentally use the wrong
    // layer or forget to regenerate mipmaps. I think we can use glTexSubImage3D to fill all layers at the same time.

    // Generate RGBA texture placeholder
    const size_t rgba_max_width = 4096;
    const size_t rgba_max_height = 4096;
    const size_t rgba_channels = 4;
    u8 *rgba_pixels = 0;
    size_t rgba_elements = rgba_max_width * rgba_max_height * rgba_channels;
    size_t rgba_bytes = rgba_elements * sizeof(*rgba_pixels);
    dlb_vec_reserve(rgba_pixels, rgba_elements);
    size_t rgba_pixel_count = rgba_max_width * rgba_max_height;
    for (size_t i = 0; i < rgba_pixel_count; ++i) {
        // NOTE: 0xC000D3 as if you were saying: "I like to cooode!" Sort of a Barney purple color.
        dlb_vec_push(rgba_pixels, 0xC0);
        dlb_vec_push(rgba_pixels, 0x00);
        dlb_vec_push(rgba_pixels, 0xD3);
        dlb_vec_push(rgba_pixels, 0xFF);
    }
    size_t rgba_pixels_len = dlb_vec_len(rgba_pixels);
    DLB_ASSERT(rgba_pixels_len == rgba_bytes);

    // Generate depth texture placeholder
    const size_t depth_max_width = 4096;
    const size_t depth_max_height = 4096;
    float *depth_pixels = 0;
    size_t depth_elements = depth_max_width * depth_max_height;
    dlb_vec_alloc_count(depth_pixels, depth_elements);
    for (size_t i = 0; i < depth_elements; ++i) {
        // NOTE: Magic number for easy graphics debugging
        depth_pixels[i] = 0.42f;
    }

    dlb_vec_each(ta_texture_pool *, pool, texturing->texture_pools) {
        ta_texture_pool_bind(pool);
        switch (pool->format) {
            case GL_RGBA: {
                DLB_ASSERT(pool->type == GL_UNSIGNED_BYTE);
                for (size_t layer = 0; layer < dlb_vec_len(pool->layers); ++layer) {
                    ta_texture_pool_set_layer_texels(pool, (int)layer, pool->format, pool->type, rgba_pixels);
                }
                break;
            } case GL_DEPTH_COMPONENT: {
                DLB_ASSERT(pool->type == GL_FLOAT);
                for (size_t layer = 0; layer < dlb_vec_len(pool->layers); ++layer) {
                    ta_texture_pool_set_layer_texels(pool, (int)layer, pool->format, pool->type, depth_pixels);
                }
                break;
            } default: {
                DLB_ASSERT(!"I don't know what format this is");
            }
        }
        // TODO: Generate mipmaps whenever we change the array? After we load everything? Hmm..
        //glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
    }
    ta_texture_pool_unbind();

    dlb_vec_free(rgba_pixels);
    dlb_vec_free(depth_pixels);
    TracyCZoneEnd(ctxMethod);
}

static void ta_texturing_add_texture(ta_texturing *texturing, ta_texture *tex)
{
    TracyCZone(ctxMethod, true);
    DLB_ASSERT(tex->type == TA_TEXTURE_2D_ARRAY);
    DLB_ASSERT(tex->width);
    DLB_ASSERT(tex->height);
    DLB_ASSERT(tex->gl_internal_format);
    DLB_ASSERT(!tex->gl_id);

    bool found = false;
    u32 index = 0;
    dlb_vec_each(ta_texture_pool *, tex_pool, texturing->texture_pools) {
        if (tex_pool->width == tex->width &&
            tex_pool->height == tex->height &&
            tex_pool->format == tex->gl_internal_format)
        {
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

            // NOTE: The way the code is currently written there could be multiple matching pools, but for now I'm
            // assuming width/height/format are distinct pools and that if no slot is found in this pool, then no slot
            // will be found in any pool.
            DLB_ASSERT(found && "Texture pool has no more free slots, need to resize or reserve a bigger pool");
        }
        index++;
    }
    // TODO: What to do when no matching pool found? For now, just hard crash, but we could do something fancy like
    // make a new pool automatically, or even pack non power-of-two textures into atlases. Avoid complexity if possible.
    DLB_ASSERT(found);
    TracyCZoneEnd(ctxMethod);
}

const char *ta_texture_type_str(int type)
{
    switch (type) {
        case TA_TEXTURE_2D_ARRAY:   return "TA_TEXTURE_2D_ARRAY";
        default: DLB_ASSERT(0);     return "TA_TEXTURE_???     ";
    }
}

void ta_texture_init(ta_texture *tex)
{
    TracyCZone(ctxMethod, true);
    // GL_NEAREST                 texel nearest
    // GL_NEAREST_MIPMAP_NEAREST  texel nearest, mipmap nearest
    // GL_NEAREST_MIPMAP_LINEAR   texel nearest, mipmap blend
    // GL_LINEAR                  texel 2x2 avg
    // GL_LINEAR_MIPMAP_NEAREST   texel 2x2 avg, mipmap nearest
    // GL_LINEAR_MIPMAP_LINEAR    texel 2x2 avg, mipmap blend

    //tex->gl_filter_min += !tex->gl_filter_min * GL_NEAREST;
    tex->gl_filter_min += !tex->gl_filter_min * GL_LINEAR_MIPMAP_LINEAR;
    tex->gl_filter_mag += !tex->gl_filter_mag * GL_NEAREST;

    ta_texture_load(tex);
    TracyCZoneEnd(ctxMethod);
}
void ta_texture_init_void(void *tex)
{
    ta_texture_init(tex);
}

GLenum ta_texture_target(ta_texture *tex)
{
    GLenum target = 0;
    switch (tex->type) {
        //case TA_TEXTURE_2D:       target = GL_TEXTURE_2D;       break;  // 3553
        case TA_TEXTURE_2D_ARRAY: target = GL_TEXTURE_2D_ARRAY; break;  // 35866
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

#if 0
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
    TracyCZone(ctxMethod, true);
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
        TracyCZoneEnd(ctxMethod);
        return NULL;
    }

    read = fread(&header, 1, sizeof(header), f);

    if (read != sizeof(header)) {
        fprintf(stderr, "%s has incomplete tga header\n", path);
        fclose(f);
        TracyCZoneEnd(ctxMethod);
        return NULL;
    }
    if (header.data_type_code != 2 && header.data_type_code != 3) {
        fprintf(stderr, "%s is not an uncompressed RGB tga file\n", path);
        fclose(f);
        TracyCZoneEnd(ctxMethod);
        return NULL;
    }
    DLB_ASSERT(header.bits_per_pixel == 8 || header.bits_per_pixel == 24 || header.bits_per_pixel == 32);

    for (i = 0; i < header.id_length; ++i) {
        if (getc(f) == EOF) {
            fprintf(stderr, "%s has incomplete id string\n", path);
            fclose(f);
            TracyCZoneEnd(ctxMethod);
            return NULL;
        }
    }

    color_map_size = (size_t)texture_le_short(header.color_map_length) * (header.color_map_depth / 8);
    for (i = 0; i < color_map_size; ++i) {
        if (getc(f) == EOF) {
            fprintf(stderr, "%s has incomplete color map\n", path);
            fclose(f);
            TracyCZoneEnd(ctxMethod);
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
        TracyCZoneEnd(ctxMethod);
        return NULL;
    }

    ta_log_write(&tg_debug_log, SRC_TEXTURE, "TGA read complete\n", path);
    TracyCZoneEnd(ctxMethod);
    return pixels;
}
void ta_texture_upload(ta_texture *tex, u8 *pixels)
{
    TracyCZone(ctxMethod, true);
    DLB_ASSERT(tex->width);
    DLB_ASSERT(tex->height);
    DLB_ASSERT(tex->channels);
    DLB_ASSERT(tex->pixels_format);
    DLB_ASSERT(tex->pixels_type);
    DLB_ASSERT(pixels);

    ta_log_write(&tg_debug_log, SRC_TEXTURE, "Uploading texture to GPU %s\n", tex->name);

    GLenum target = ta_texture_target(tex);
    DLB_ASSERT(target == GL_TEXTURE_2D_ARRAY);

    ta_texture_pool *tex_pool = ta_texture_texture_pool(tex);
    //switch (tex_pool->format) {
    //    case GL_R8: {
    //        size_t pixels_len = tex->width * tex->height;
    //        u8 *padded_texels = dlb_calloc(tex->width * tex->height, 4);
    //        u8 *dst = padded_texels;
    //        for (size_t i = 0; i < pixels_len; i += 1, dst += 4) {
    //            dst[0] = pixels[i];
    //        }
    //        ta_texture_pool_set_layer_texels(tex_pool, tex->gl_texture_pool_layer, tex->pixels_format, tex->pixels_type,
    //            padded_texels);
    //        dlb_free(padded_texels);
    //        break;
    //    } case GL_RGB8: {
    //        DLB_ASSERT(tex->channels == 3);
    //        size_t pixels_len = tex->width * tex->height * 3;
    //        u8 *padded_texels = dlb_calloc(tex->width * tex->height, 4);
    //        u8 *src = pixels;
    //        u8 *dst = padded_texels;
    //        for (size_t i = 0; i < pixels_len; i += 3, src += 3, dst += 4) {
    //            dst[0] = src[0];
    //            dst[1] = src[1];
    //            dst[2] = src[2];
    //            dst[3] = 0xff;
    //        }
    //        ta_texture_pool_set_layer_texels(tex_pool, tex->gl_texture_pool_layer, tex->pixels_format, tex->pixels_type,
    //            padded_texels);
    //        dlb_free(padded_texels);
    //        break;
    //    } case GL_RGBA8: {
    //        DLB_ASSERT(tex->channels == 4);
    //        ta_texture_pool_set_layer_texels(tex_pool, tex->gl_texture_pool_layer, tex->pixels_format, tex->pixels_type,
    //            pixels);
    //        break;
    //    } default: {
    //        DLB_ASSERT(!"Uknown texture format");
    //    }
    //}
    ta_texture_pool_set_layer_texels(tex_pool, tex->gl_texture_pool_layer, tex->pixels_format, tex->pixels_type,
        pixels);

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
    TracyCZoneEnd(ctxMethod);
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
        TracyCZone(ctxMethod, true);
        ta_log_write(&tg_debug_log, SRC_TEXTURE, "Generating mipmap for %s\n", tex->name);
        ta_log_timed_region_start(&tg_debug_log, SRC_TEXTURE, CSTR("texture_generate_mipmap"));

        GLenum target = ta_texture_target(tex);
        glGenerateMipmap(target);

        ta_log_timed_region_end(&tg_debug_log, CSTR("texture_generate_mipmap"));
        TracyCZoneEnd(ctxMethod);
    }
}

void ta_texture_load(ta_texture *tex)
{
    TracyCZone(ctxMethod, true);
    TracyCZoneText(ctxMethod, tex->name, dlb_symbol_len(tex->name));
    ta_log_write(&tg_debug_log, SRC_TEXTURE, "Loading texture %s\n", tex->name);
    ta_log_timed_region_start(&tg_debug_log, SRC_TEXTURE, CSTR("ta_texture_load"));

    // Pixel textures contain inlined pixel data, path should be null
    if (tex->path) {
        bool use_stb = true;
        bool bgr = false;

        u32 width = 0;
        u32 height = 0;
        u8 channels = 0;
        u8 *pixels = 0;

        if (use_stb) {
            stbi_set_flip_vertically_on_load(true);
            //pixels = stbi_load(tex->path, &width, &height, &channels, tex->channels);
            TracyCZoneN(ctxStbLoad, "stbi_load", true);
            pixels = stbi_load(tex->path, (int *)&width, (int *)&height, (int *)&channels, 4);
            TracyCZoneEnd(ctxStbLoad);
            channels = 4;
            if (!pixels) {
                const char *reason = stbi_failure_reason();
                ta_log_write(&tg_debug_log, SRC_TEXTURE,
                    "Failed to load tex: %s\nSTBI Reason: %s\n", tex->path, reason);
                DLB_ASSERT(!"ta_texture_init: Failed to load tex");
            }
        } else {
            // Load image data from file(s) and upload to VRAM
            pixels = texture_read_tga(tex->path, &width, &height, &channels);
            if (!pixels) {
                ta_log_write(&tg_debug_log, SRC_TEXTURE, "Failed to load tex: %s\n", tex->path);
                DLB_ASSERT(!"ta_texture_init: Failed to load tex");
            }

            // NOTE: Assuming all texture paths point to BGR TGA images for now.
            bgr = true;
        }

        tex->width = width;
        tex->height = height;
        tex->channels = channels;

        // TODO: Always use GL_RGBA textures, but somehow pack 1 and 3 channel textures. This needs to happen on the CPU
        // side, but it would be preferable to have the packed texture data be what gets loaded from disk so we can quickly
        // dump it straight to VRAM without manually interleaving channels at texture load time.
        switch (tex->channels)
        {
            case 1: // Grayscale
                tex->pixels_format = GL_RED;
                break;
            case 3: // RGB
                tex->pixels_format = bgr ? GL_BGR : GL_RGB;
                break;
            case 4: // RGBA
                tex->pixels_format = bgr ? GL_BGRA : GL_RGBA;
                break;
            default: // Unsupported BPP
                DLB_ASSERT(!"Sorry, don't know what to do with this texture, man.");
        }
        tex->pixels_type = GL_UNSIGNED_BYTE;

        // If internal format (GPU side) not specified, assume we want the texture in an RGBA pool
        // NOTE: Branchless just cuz
        tex->gl_internal_format += !tex->gl_internal_format * GL_RGBA;

        ta_texturing_add_texture(&tg_game.texturing, tex);
        ta_texture_pool *pool = ta_texture_texture_pool(tex);
        ta_texture_pool_bind(pool);
        ta_texture_upload(tex, pixels);

        if (use_stb) {
            TracyCZoneN(ctxStbLoad, "stbi_image_free", true);
            stbi_image_free(pixels);
            TracyCZoneEnd(ctxStbLoad);
        } else {
            TracyCZoneN(ctxStbLoad, "dlb_free", true);
            dlb_free(pixels);
            TracyCZoneEnd(ctxStbLoad);
        }
    } else {
        ////////////////////////////////////////////////////////////////////////////////////////
        // HACK: Using this to fix scene file, should be explicit
        if (tex->pixels) {
            switch (tex->channels)
            {
                case 1: // Grayscale
                    tex->pixels_format += !tex->pixels_format * GL_RED;
                    break;
                case 3: // RGB
                    tex->pixels_format += !tex->pixels_format * GL_RGB;
                    break;
                case 4: // RGBA
                    tex->pixels_format += !tex->pixels_format * GL_RGBA;
                    break;
                default: // Unsupported BPP
                    DLB_ASSERT(!"Sorry, don't know what to do with this texture, man.");
            }
            tex->pixels_type += !tex->pixels_type * GL_UNSIGNED_BYTE;

            // If internal format (GPU side) not specified, assume we want the texture in an RGBA pool
            tex->gl_internal_format += !tex->gl_internal_format * GL_RGBA;
        }
        ////////////////////////////////////////////////////////////////////////////////////////

        ta_texturing_add_texture(&tg_game.texturing, tex);
        ta_texture_pool *pool = ta_texture_texture_pool(tex);
        ta_texture_pool_bind(pool);
        if (tex->pixels) {
            ta_texture_upload(tex, tex->pixels);
        }
    }

    // TODO: Don't generate mipmaps for textures that don't need them (e.g. UI textures?)
    texture_generate_mipmap(tex);
    ta_texture_pool_unbind();
    ta_log_timed_region_end(&tg_debug_log, CSTR("ta_texture_load"));
    TracyCZoneEnd(ctxMethod);
}

void ta_texture_delete(ta_texture *tex)
{
    // TODO: Need to fix this for texture pools.. hmm :(
    DLB_ASSERT(tex->gl_id);
    GLuint id = tex->gl_id;
    tex->gl_id = 0;
    if (id) {
        glDeleteTextures(1, &id);
    }
}

void ta_texture_reload(ta_texture *tex)
{
    TracyCZone(ctxMethod, true);
    //ta_texture_delete(tex);
    ta_texture_load(tex);
    TracyCZoneEnd(ctxMethod);
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
