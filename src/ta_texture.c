#include "ta_texture.h"
#include "ta_log.h"
#include "dlb_types.h"
#include "dlb_memory.h"
#include "dlb_vector.h"

#define STBI_ASSERT(x) DLB_ASSERT(x)
#define STBI_MALLOC dlb_malloc
#define STBI_REALLOC dlb_realloc
#define STBI_FREE dlb_free
#define STBI_ONLY_PNG
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
        ta_texture_load(tex);
    }
}

void ta_texture_load_path(ta_texture *tex, const char *path)
{
    tex->path = path;

    // Load pixel data from file
    int w, h, channels;
    stbi_set_flip_vertically_on_load(true);
    u8 *pixels = stbi_load(tex->path, &w, &h, &channels, tex->channels);
    if (!pixels) {
        const char *reason = stbi_failure_reason();
        ta_log_write(tg_debug_log, "Failed to load tex: %s\nSTBI Reason: %s\n",
            tex->path, reason);
        DLB_ASSERT(!"ta_texture_init: Failed to load tex");
    }

    tex->width = w;
    tex->height = h;
    if (!tex->channels) {
        tex->channels = channels;
    }
    tex->pixels = pixels;
    ta_texture_load(tex);
    stbi_image_free(pixels);
}

void ta_texture_set_pixels(ta_texture *tex, u8 *pixels)
{
    size_t bytes = tex->width * tex->height * tex->channels * sizeof(*tex->pixels);
    tex->pixels = calloc(1, bytes);
    memcpy(tex->pixels, pixels, bytes);
}

void ta_texture_load(ta_texture *tex)
{
    GLint format_internal = 0;
    GLenum format = 0;

    switch (tex->channels)
    {
        case 1: // Grayscale
            DLB_ASSERT(tex->linear);  // OpenGL doesn't support sRGB for grayscale
            format_internal = GL_R8;
            format = GL_RED;
            break;
        case 3: // RGB
            format_internal = tex->linear ? GL_RGB8 : GL_SRGB8;
            format = GL_RGB;
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
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, format_internal, tex->width, tex->height,
            0, format, GL_UNSIGNED_BYTE, tex->pixels);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    //GLuint *gl_id = dlb_vec_alloc(gl_ids[queue]);
    //*gl_id = texture->gl_id;
}

void ta_texture_delete(ta_texture *tex)
{
    if (tex->pixels) {
        free(tex->pixels);
    }
    if (tex->gl_id) {
        glDeleteTextures(1, &tex->gl_id);
    }
    tex->gl_id = 0;
}

void ta_texture_free(ta_texture *tex)
{
    ta_texture_delete(tex);

    // TODO(perf): Delete all scene textures in a single GL call by aggregating
    //             gl_ids during texture initialization.
	//glDeleteTextures(dlb_vec_len(gl_ids[queue]), gl_ids[queue]);
	//dlb_vec_clear(tex[queue]);
	//dlb_vec_clear(gl_ids[queue]);
}