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

enum font_layers {
    LAYER_SHADOW,
    LAYER_TEXT,
    LAYER_COUNT
};

static const ta_rgba *colors[LAYER_COUNT] = {
    [LAYER_SHADOW] = &TA_COLOR_BLACK,
    [LAYER_TEXT]   = &TA_COLOR_WHITE,
};

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
    if (!stbtt_InitFont(&font->font_info, buf->data, 0)) {
        ta_log_write(tg_debug_log, "[font] Failed to initialize font %s\n", path);
        DLB_ASSERT(!"Failed to initialize font!");
    }


    font->tex_w = 512;
    font->tex_h = 512;
    u8 *pixels = dlb_calloc(font->tex_w * font->tex_h, sizeof(*pixels));
    DLB_ASSERT(pixels);

    font->first_char = 32;
    font->last_char = 126;
    int num_chars = font->last_char - font->first_char;
    font->chars = dlb_calloc(num_chars, sizeof(*font->chars));
    DLB_ASSERT(font->chars);

    font->scale = stbtt_ScaleForPixelHeight(&font->font_info, font->pixel_height);

    int x = 1;
    int y = 1;
    int bottom_y = 1;

    for (int i = 0; i < num_chars; ++i) {
        int advance, lsb, x0, y0, x1, y1, gw, gh;
        int g = stbtt_FindGlyphIndex(&font->font_info, font->first_char + i);
        stbtt_GetGlyphHMetrics(&font->font_info, g, &advance, &lsb);
        stbtt_GetGlyphBitmapBox(&font->font_info, g, font->scale, font->scale,
            &x0, &y0, &x1, &y1);
        gw = x1 - x0;
        gh = y1 - y0;
        // advance to next row
        if (x + gw + 1 >= font->tex_w) {
            y = bottom_y;
            x = 1;
        }
        // check if it fits vertically AFTER potentially moving to next row
        if (y + gh + 1 >= font->tex_h) {
            DLB_ASSERT(!"Ran out of space in font atlas");
        }
        DLB_ASSERT(x + gw < font->tex_w);
        DLB_ASSERT(y + gh < font->tex_h);
        stbtt_MakeGlyphBitmap(&font->font_info, pixels + x + y * font->tex_w, gw,
            gh, font->tex_w, font->scale, font->scale, g);
        //----------------------------------------------------------------------
        int padding = 5;
        u8 onedge_value = 180;
        float pixel_dist_scale = 180 / 5.0;
        int sdf_gw;
        int sdf_gh;
        int sdf_gx_off;
        int sdf_gy_off;
        stbtt_GetGlyphSDF(&font->font_info, font->scale, g, padding, onedge_value,
            pixel_dist_scale, &sdf_gw, &sdf_gh, &sdf_gx_off, &sdf_gy_off);
        // TODO: memcpy into pixels buffer, dunno about size of buf
        //----------------------------------------------------------------------
        font->chars[i].x0 = (s16)x;
        font->chars[i].y0 = (s16)y;
        font->chars[i].x1 = (s16)(x + gw);
        font->chars[i].y1 = (s16)(y + gh);
        font->chars[i].xadvance = font->scale * advance;
        font->chars[i].xoff = (float) x0;
        font->chars[i].yoff = (float) y0;
        x = x + gw + 1;
        if (y + gh + 1 > bottom_y) {
            bottom_y = y + gh + 1;
        }
    }

    stbtt_GetFontVMetrics(&font->font_info, &font->ascent, &font->descent, &font->line_gap);
    stbtt_GetFontBoundingBox(&font->font_info, &font->bbox.x, &font->bbox.y, &font->bbox.w, &font->bbox.h);

    while (font->tex_h > bottom_y) {
        font->tex_h >>= 1;
    }
    font->tex_h <<= 1;

    glGenTextures(1, &font->gl_id);
    glBindTexture(GL_TEXTURE_2D, font->gl_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, font->tex_w, font->tex_h, 0, GL_RED,
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

static float ta_GetBakedQuad(const stbtt_bakedchar *chardata, int pw, int ph,
    int char_index, float *xpos, float *ypos, ta_rect_uv *rect)
{
    float ipw = 1.0f / pw, iph = 1.0f / ph;
    const stbtt_bakedchar *b = chardata + char_index;
    int round_x = STBTT_ifloor((*xpos + b->xoff) + 0.5f);
    int round_y = STBTT_ifloor((*ypos + b->yoff) + 0.5f);

    rect->rect.x = (float)round_x;
    rect->rect.y = (float)round_y;
    rect->rect.w = (float)(b->x1 - b->x0);
    rect->rect.h = (float)(b->y1 - b->y0);
    rect->uv0.u = b->x0 * ipw;
    rect->uv0.v = b->y1 * iph;
    rect->uv1.u = b->x1 * ipw;
    rect->uv1.v = b->y0 * iph;

    *xpos += b->xadvance;
    return rect->rect.h + b->yoff;
}

ta_rectf ta_font_push_text(ta_vert_quad **queue, ta_font *font, float x, float y,
    float z, const char *text, u32 text_len, bool screen, u32 cursor_idx,
    float *cursor_x)
{
    DLB_ASSERT(text);
    DLB_ASSERT(text_len);

    ta_rectf rect = { 0 };
    rect.x = x;
    rect.y = y;

    const float shadow_offset_x = 1.0f;
    const float shadow_offset_y = 1.0f;
    const float layer_dir = (screen) ? 1.0f : -1.0f;
    float layer_offset = 0.0f;

    for (int layer = 0; layer < LAYER_COUNT; layer++) {
        // TODO: Try SDFs instead for shadowing. This anti-aliased shadowing
        //       doesn't look right.
        if (layer == LAYER_SHADOW) continue;

        float cur_x = rect.x;
        float cur_y = rect.y + font->pixel_height;
        float ndc_x = NDC_X(cur_x);
        float ndc_y = NDC_Y(cur_y);

        switch (layer) {
            case LAYER_SHADOW: {
                cur_x += shadow_offset_x;
                cur_y += shadow_offset_y;
                layer_offset = (z - (UI_LAYER_EPSILON / 2.0f)) * layer_dir;
                break;
            } case LAYER_TEXT: {
                layer_offset = z * layer_dir;
                break;
            }
        }

        float max_h = 0.0f;

        for (u32 i = 0; i < text_len; i++) {
            // Save cursor position
            if (i == cursor_idx && cursor_x) {
                *cursor_x = cur_x;
            }

            if (text[i] == '\n') {
                //DLB_ASSERT(y_max <= font->pixel_height);
                cur_x = rect.x;
                cur_y += max_h;
                rect.h += max_h;
                max_h = 0.0f;
            } else if (text[i] >= font->first_char && text[i] <= font->last_char) {
                ta_rect_uv rect_uv = { 0 };
                float descent = ta_GetBakedQuad(font->chars, font->tex_w,
                    font->tex_h, text[i] - 32, &cur_x, &cur_y, &rect_uv);

#if 0
                // HACK: Cull characters that would be cut off by edge of screen
                //       to prevent weird wrapping glitches in screen mode.
                float a = NDC_X(rect_uv.rect.x);
                float b = NDC_X(rect_uv.rect.x + rect_uv.rect.w);
                float c = NDC_Y(rect_uv.rect.y);
                float d = NDC_Y(rect_uv.rect.y - rect_uv.rect.h);
#endif
                if (!screen || (
                    NDC_X(rect_uv.rect.x) >= ndc_x &&
                    NDC_X(rect_uv.rect.x + rect_uv.rect.w) > ndc_x
                )) {
                    ta_primitive_push_rect_uv(queue, rect_uv, *colors[layer],
                        layer_offset, screen);
                    max_h = MAX(max_h, font->pixel_height + descent);
                } else {
                    DLB_ASSERT(1);
                }

                rect.w = MAX(rect.w, cur_x - rect.x);
            }
        }

        rect.h += max_h;
    }

    return rect;
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