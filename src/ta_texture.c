#include "ta_texture.h"
#include "ta_log.h"
#include "dlb/dlb_types.h"
#include "dlb/dlb_memory.h"
#include "dlb/dlb_vector.h"

#define STBI_ASSERT(x) DLB_ASSERT(x)
#define STBI_MALLOC dlb_malloc
#define STBI_REALLOC dlb_realloc
#define STBI_FREE dlb_free
#define STBI_ONLY_PNG
#define STBI_ONLY_TGA
#define STB_IMAGE_IMPLEMENTATION
#include "misc/stb_image.h"

void ta_texture_init(ta_texture *tex)
{
    if (tex->path) {
        ta_texture_load_path(tex, tex->path);
    } else if(tex->pixels) {
        DLB_ASSERT(tex->width);
        DLB_ASSERT(tex->height);
        DLB_ASSERT(tex->channels);
        ta_texture_load(tex, tex->pixels, 0);
    }
}

static short le_short(unsigned char *bytes)
{
    return bytes[0] | ((char)bytes[1] << 8);
}

static void *read_tga(const char *filename, int *width, int *height, int *channels)
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
    size_t i, color_map_size, pixels_size;
    FILE *f;
    size_t read;
    void *pixels;
    int row;

    f = fopen(filename, "rb");

    if (!f) {
        fprintf(stderr, "Unable to open %s for reading\n", filename);
        return NULL;
    }

    read = fread(&header, 1, sizeof(header), f);

    if (read != sizeof(header)) {
        fprintf(stderr, "%s has incomplete tga header\n", filename);
        fclose(f);
        return NULL;
    }
    if (header.data_type_code != 2 && header.data_type_code != 3) {
        fprintf(stderr, "%s is not an uncompressed RGB tga file\n", filename);
        fclose(f);
        return NULL;
    }
    //if (header.bits_per_pixel != 24) {
    //    fprintf(stderr, "%s is not a 24-bit uncompressed RGB tga file\n",
    //        filename);
    //    fclose(f);
    //    return NULL;
    //}
    DLB_ASSERT(header.bits_per_pixel == 8 || header.bits_per_pixel == 24 ||
        header.bits_per_pixel == 32);

    for (i = 0; i < header.id_length; ++i) {
        if (getc(f) == EOF) {
            fprintf(stderr, "%s has incomplete id string\n", filename);
            fclose(f);
            return NULL;
        }
    }

    color_map_size = le_short(header.color_map_length) *
        (header.color_map_depth / 8);
    for (i = 0; i < color_map_size; ++i) {
        if (getc(f) == EOF) {
            fprintf(stderr, "%s has incomplete color map\n", filename);
            fclose(f);
            return NULL;
        }
    }

    *width = le_short(header.width);
    *height = le_short(header.height);
    *channels = header.bits_per_pixel / 8;
    pixels_size = *width * *height * *channels;
    pixels = dlb_malloc(pixels_size);
    DLB_ASSERT(pixels);

#if 0
    // Vertical flip
    read = 0;
    int row_width = *width * *channels;
    for (row = *height - 1; row >= 0; --row) {
        read += fread((char *)pixels + row * row_width, 1, row_width, f);
    }
#else
    read = fread(pixels, 1, pixels_size, f);
#endif
    fclose(f);

    if (read != pixels_size) {
        fprintf(stderr, "%s has incomplete image\n", filename);
        dlb_free(pixels);
        return NULL;
    }

    return pixels;
}

void ta_texture_load_path(ta_texture *tex, const char *path)
{
    tex->path = path;

    ta_log_write(&tg_debug_log, "Texture", "Loading texture from disk %s...\n",
        path);
    // Load pixel data from file
    int w, h, channels;
#if 0
    stbi_set_flip_vertically_on_load(true);
    u8 *pixels = stbi_load(tex->path, &w, &h, &channels, tex->channels);
    if (!pixels) {
        const char *reason = stbi_failure_reason();
        ta_log_write(&tg_debug_log, "Texture",
            "Failed to load tex: %s\nSTBI Reason: %s\n", tex->path, reason);
        DLB_ASSERT(!"ta_texture_init: Failed to load tex");
    }

    tex->width = w;
    tex->height = h;
    tex->channels = channels;
    ta_texture_load(tex, pixels, 0);
    stbi_image_free(pixels);
#else
    u8 *pixels = read_tga(path, &w, &h, &channels);
    if (!pixels) {
        ta_log_write(&tg_debug_log, "Texture", "Failed to load tex: %s\n",
            tex->path);
        DLB_ASSERT(!"ta_texture_init: Failed to load tex");
    }

    tex->width = w;
    tex->height = h;
    tex->channels = channels;
    ta_texture_load(tex, pixels, GL_BGR);
    dlb_free(pixels);
#endif
}

void ta_texture_load(ta_texture *tex, u8 *pixels, GLenum format)
{
    DLB_ASSERT(tex->width);
    DLB_ASSERT(tex->height);
    DLB_ASSERT(tex->channels);
    DLB_ASSERT(pixels);

    ta_log_write(&tg_debug_log, "Texture",
        "Creating OpenGL texture w: %d, h: %d, channels: %d...\n",
        tex->width, tex->height, tex->channels);

    GLint format_internal = 0;

    switch (tex->channels)
    {
        case 1: // Grayscale
            //DLB_ASSERT(tex->linear);  // OpenGL doesn't support sRGB for grayscale
            format_internal = GL_R8;
            format = GL_RED;
            break;
        case 3: // RGB
            format_internal = tex->linear ? GL_RGB8 : GL_SRGB8;
            format = format ? format : GL_RGB;
            break;
        case 4: // RGBA
            format_internal = tex->linear ? GL_RGBA8 : GL_SRGB8_ALPHA8;
            format = GL_RGBA;
            break;
        default: // Unsupported BPP
            DLB_ASSERT(!"Sorry, don't know what to do with this texture, man.");
    }

    glCreateTextures(GL_TEXTURE_2D, 1, &tex->gl_id);
    glBindTexture(GL_TEXTURE_2D, tex->gl_id);

    GLint param = tex->repeat ? GL_REPEAT : GL_CLAMP_TO_EDGE;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, param);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, param);

    // GL_NEAREST                 texel nearest
    // GL_NEAREST_MIPMAP_NEAREST  texel nearest, mipmap nearest
    // GL_NEAREST_MIPMAP_LINEAR   texel nearest, mipmap blend
    // GL_LINEAR                  texel 2x2 avg
    // GL_LINEAR_MIPMAP_NEAREST   texel 2x2 avg, mipmap nearest
    // GL_LINEAR_MIPMAP_LINEAR    texel 2x2 avg, mipmap blend

    // TODO: Allow each texture to set its own filtering mode
#if 0
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
#else
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
#endif

    glTexImage2D(GL_TEXTURE_2D, 0, format_internal, tex->width, tex->height,
        0, format, GL_UNSIGNED_BYTE, pixels);
    glGenerateMipmap(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, 0);

    //GLuint *gl_id = dlb_vec_alloc(gl_ids[queue]);
    //*gl_id = texture->gl_id;
}

void ta_texture_delete(ta_texture *tex)
{
    if (tex->pixels) {
        dlb_free(tex->pixels);
    }
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