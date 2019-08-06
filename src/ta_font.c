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

    // NOTE: No guarantee texture is big enough to hold all of the glyphs!
    // ASCII 32..126 is 95 glyphs
    font->chars = dlb_calloc(96, sizeof(*font->chars));

    u8 *pixels = dlb_calloc(512*512, 1);
    stbtt_BakeFontBitmap(buf->data, 0, font->pixel_height, pixels, 512, 512, 32,
        96, font->chars);

    glGenTextures(1, &font->gl_id);
    glBindTexture(GL_TEXTURE_2D, font->gl_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, 512, 512, 0, GL_RED,
        GL_UNSIGNED_BYTE, pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    ta_buffer_free(buf);
    dlb_free(pixels);
}

void ta_font_delete(ta_font *font)
{
    glDeleteTextures(1, &font->gl_id);
    font->gl_id = 0;
}

void ta_font_free(ta_font *font)
{
    ta_font_delete(font);
    dlb_free(font->chars);
}

ta_shader *ta_font_shader(ta_font *font)
{
    ta_shader *shader = ta_scene_find(font->uid.scene, TA_SHADER, font->shader_uid);
    return shader;
}

static const ta_rgba *colors[2] = {
    &TA_COLOR_BLACK,
    &TA_COLOR_WHITE,
};

float ta_font_push_text(ta_vert_quad **queue, ta_font *font, float x, float y,
    const char *text, bool screen)
{
    float start_x = x;
    float start_y = y + font->pixel_height;
    float start_x_ndc = NDC_X(start_x);
    float max_x = 0.0f;

    const float shadow_offset_x = 1.0f;
    const float shadow_offset_y = 1.0f;

    for (int i = 0; i < 2; i++) {
        float cur_x = start_x + (shadow_offset_x * (1 - i));
        float cur_y = start_y + (shadow_offset_y * (1 - i));
        char *chr = (char *)text;

        while (*chr) {
            if (*chr == '\n') {
                //DLB_ASSERT(y_max <= font->pixel_height);
                cur_x = start_x;
                cur_y += font->pixel_height;  // TODO: This probably should be relative to baseline
            } else if (*chr >= 32 && *chr < 128) {
                stbtt_aligned_quad quad;
                stbtt_GetBakedQuad(font->chars, 512, 512, *chr - 32, &cur_x, &cur_y, &quad, 1);

                // HACK: Cull characters that would be cut off by edge of screen
                if (!screen || (NDC_X(quad.x0) >= start_x_ndc && NDC_X(quad.x1) > start_x_ndc)) {
                    ta_rect_uv rect_uv = { 0 };
                    rect_uv.rect.x = quad.x0;
                    rect_uv.rect.y = quad.y0;
                    rect_uv.rect.w = (quad.x1 - quad.x0);
                    rect_uv.rect.h = (quad.y1 - quad.y0);
                    rect_uv.uv0.u = quad.s0;
                    rect_uv.uv0.v = quad.t1;
                    rect_uv.uv1.u = quad.s1;
                    rect_uv.uv1.v = quad.t0;
                    float layer = (i == 0) ? UI_LAYER_SHADOW : UI_LAYER_1;
                    if (screen) layer *= -1.0f;
                    ta_primitive_push_rect_uv(queue, rect_uv, *colors[i], layer, screen);
                }

                max_x = MAX(max_x, cur_x);
            }
            ++chr;
        }
    }

    float max_w = max_x - start_x;
    return max_w;
}

void ta_font_render(ta_vert_quad *queue, ta_font *font, bool clear_queues,
    bool reset_uniforms)
{
    glDisable(GL_CULL_FACE);
    //glDisable(GL_DEPTH_TEST);

    ta_shader *shader = ta_font_shader(font);
    ta_shader_set_sampler2d(shader, SYM_U_TEX, font->gl_id);
    ta_primitive_render_quads(queue, shader, clear_queues, reset_uniforms);
    ta_shader_set_sampler2d(shader, SYM_U_TEX, 0);

    //glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
}