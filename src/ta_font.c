#include "ta_font.h"
#include "ta_file.h"
#include "ta_texture.h"
#include "ta_scene.h"
#include "ta_game.h"
#include "ta_primitive.h"
#include "ta_log.h"
#include "ta_symbol.h"
#include "ta_window.h"
#include "dlb_memory.h"
#include "misc/gl3w.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "misc/stb_truetype.h"

void ta_font_init(ta_font *font)
{
    if (!font->pixel_height) {
        font->pixel_height = 16.0f;
    }
    if (font->path) {
        ta_font_load_path(font, font->path);
    }
}

void ta_font_load_path(ta_font *font, const char *path)
{
    ta_buffer *buf = ta_file_read_all(path);

    // no guarantee this fits!
    u8 *pixels = dlb_calloc(512*512, sizeof(*pixels));
    // ASCII 32..126 is 95 glyphs
    font->chars = dlb_calloc(96, sizeof(*font->chars));
    // TODO: Don't hard code all of these values
    stbtt_BakeFontBitmap(buf->data, 0, font->pixel_height, pixels, 512, 512, 32,
        96, font->chars);
    ta_buffer_free(buf);
    ta_font_load(font, pixels);
    dlb_free(pixels);
}

void ta_font_load(ta_font *font, u8 *pixels)
{
    // TODO: Maybe just make a ta_texture? Don't really want to save this to the
    // scene file though, do we? Need to way to flag things as not serializable.
    glGenTextures(1, &font->gl_id);
    glBindTexture(GL_TEXTURE_2D, font->gl_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, 512, 512, 0, GL_RED,
        GL_UNSIGNED_BYTE, pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
}

void ta_font_delete(ta_font *font)
{
    if (font->gl_id) {
        glDeleteTextures(1, &font->gl_id);
        font->gl_id = 0;
    }
}

void ta_font_free(ta_font *font)
{
    ta_font_delete(font);
    dlb_free(font->chars);
}

void ta_font_print(ta_font *font, float x, float y, char *text)
{
    //ta_shader_set_mat4(tg_shader_quads, SYM_U_PROJ, &MAT4_IDENT);
    //ta_shader_set_mat4(tg_shader_quads, SYM_U_VIEW, &MAT4_IDENT);
    //ta_shader_set_mat4(tg_shader_quads, SYM_U_MODEL, &MAT4_IDENT);
    ta_shader_set_sampler2d(tg_shader_quads, SYM_U_TEX, font->gl_id);

    float x_start = x;
    float x_start_ndc = NDC_X(x_start);
    float y_max = 0;

    y += font->pixel_height;

    while (*text) {
        if (*text == '\n') {
            DLB_ASSERT(y_max <= font->pixel_height);
            x = x_start;
            y += font->pixel_height;  // TODO: This probably should be relative to baseline
            y_max = 0;
        } else if (*text >= 32 && *text < 128) {
            stbtt_aligned_quad q;
            stbtt_GetBakedQuad(font->chars, 512, 512, *text - 32, &x, &y, &q, 1);

            // HACK: Cull characters that would be cut off by edge of screen
            if (NDC_X(q.x0) >= x_start_ndc && NDC_X(q.x1) > x_start_ndc) {
                ta_rect_uv rect_uv = { 0 };
                rect_uv.rect.x = (int)q.x0;
                rect_uv.rect.y = (int)q.y0;
                rect_uv.rect.w = (int)(q.x1 - q.x0);
                rect_uv.rect.h = (int)(q.y1 - q.y0);
                rect_uv.uv0.u = q.s0;
                rect_uv.uv0.v = q.t1;
                rect_uv.uv1.u = q.s1;
                rect_uv.uv1.v = q.t0;
                ta_primitive_push_rect_uv(rect_uv, TA_COLOR_WHITE);

                y_max = MAX(y_max, (float)rect_uv.rect.h);
            }
        }
        ++text;
    }

    ta_primitive_render();
    ta_primitive_clear(false);
    ta_shader_set_sampler2d(tg_shader_quads, SYM_U_TEX, 0);
}