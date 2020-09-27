#include "ta_font.h"
#include "ta_file.h"
#include "ta_game.h"
#include "ta_log.h"
#include "ta_mesh.h"
#include "ta_primitive.h"
#include "ta_scene.h"
#include "ta_shader.h"
#include "ta_symbol.h"
#include "ta_texture.h"
#include "ta_window.h"
#include "dlb/dlb_memory.h"
#include "dlb/dlb_vector.h"
#include "misc/glad.h"

#pragma warning(push)
#pragma warning(disable: 6385)
#define STB_TRUETYPE_IMPLEMENTATION
#include "misc/stb_truetype.h"
#pragma warning(pop)

void ta_font_init(ta_font *font)
{
    if (!font->pixel_height) {
        font->pixel_height = 16.0f;
    }
    if (font->path) {
        ta_font_load_path(font, font->path);
        if (!font->name) {
            font->name = font->path;
        }
    }
}
void ta_font_init_void(void *font)
{
    ta_font_init(font);
}
void ta_font_load_path(ta_font *font, const char *path)
{
    char *buf = ta_file_read_all(path);
    DLB_ASSERT(buf);

    if (!stbtt_InitFont(&font->font_info, (u8 *)buf, 0)) {
        ta_log_write(&tg_debug_log, SRC_FONT, "Failed to initialize font %s\n",
            path);
        DLB_ASSERT(!"Failed to initialize font!");
    }

    font->tex_w = 512;
    font->tex_h = 512;
    u8 *pixels = dlb_calloc(font->tex_w * font->tex_h, sizeof(*pixels));
    DLB_ASSERT(pixels);

    font->first_char = 32;
    font->last_char = 126;
    int num_chars = (font->last_char + 1) - font->first_char;
    font->chars = dlb_calloc(num_chars, sizeof(*font->chars));
    DLB_ASSERT(font->chars);

    //float scales[32] = { 0 };
    //for (int i = 0; i < ARRAY_SIZE(scales); i++) {
    //    scales[i] = stbtt_ScaleForPixelHeight(&font->font_info, (float)i);
    //}

    font->scale = stbtt_ScaleForPixelHeight(&font->font_info, font->pixel_height);
    //font->scale = font->pixel_height / 1000.0f;
    //font->scale = 12.0f / 1000.0f;
    //
    //int metrics[3] = { 0 };
    //stbtt_GetFontVMetrics(&font->font_info, &metrics[0], &metrics[2], &metrics[2]);
    //font->scale = font->pixel_height / (metrics[0] - metrics[1]);

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
        font->chars[i].xadvance = ceilf(font->scale * advance);
        font->chars[i].xoff = (float)x0;
        font->chars[i].yoff = (float)y0;
        font->ascent = MAX(font->ascent, -y0);
        font->descent = MAX(font->descent, y0 + gh);
        font->line_height = font->ascent + font->descent;
        font->left_bearing = MIN(font->left_bearing, x0);
        x = x + gw + 1;
        if (y + gh + 1 > bottom_y) {
            bottom_y = y + gh + 1;
        }
    }

    while (font->tex_h > bottom_y) {
        font->tex_h >>= 1;
    }
    font->tex_h <<= 1;

    glGenTextures(1, &font->gl_id);
    glBindTexture(GL_TEXTURE_2D, font->gl_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, font->tex_w, font->tex_h, 0, GL_RED, GL_UNSIGNED_BYTE, pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    dlb_vec_free(buf);
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
    // TODO: Fonts probably don't belong in the game scene..
    ta_shader *shader = ta_game_by_sym(RES_SHADER, font->shader);
    return shader;
}

static inline int ta_ifloor(double x)
{
    return (int)x - (x < (int)x);
}

static inline int ta_iceil(double x)
{
    return (int)x + (x > (int)x);
}

static void ta_baked_quad(const stbtt_bakedchar *chardata, int pw, int ph,
    int char_index, int *xpos, int *ypos, ta_rect_uv *rect)
{
    float ipw = 1.0f / pw, iph = 1.0f / ph;
    const stbtt_bakedchar *b = chardata + char_index;
    int round_x = ta_ifloor((*xpos + b->xoff) + 0.5f);
    int round_y = ta_ifloor((*ypos + b->yoff) + 0.5f);

    rect->rect.x = round_x;
    rect->rect.y = round_y;
    rect->rect.w = (b->x1 - b->x0);
    rect->rect.h = (b->y1 - b->y0);
    rect->uv0.u = b->x0 * ipw;
    rect->uv0.v = b->y0 * iph;
    rect->uv1.u = b->x1 * ipw;
    rect->uv1.v = b->y1 * iph;

    *xpos += (int)b->xadvance;

    // TODO(cleanup): returns descent; don't need for now
    //return rect->rect.h + b->yoff;
}

ta_rect ta_font_push_text(ta_font *font, const char *text, size_t text_len, bool screen, size_t *cursor_idx,
    ta_vec2i *cursor_offset, const ta_vec2i *mouse_coords, ta_rect_uv **rects)
{
    DLB_ASSERT(rects);
    if (text_len) {
        dlb_vec_reserve(*rects, dlb_vec_len(*rects) + text_len);
    }

    ta_rect bounds = { 0 };
    if (!text) {
        DLB_ASSERT(0);
        return bounds;
    }

    size_t orig_rects_len = dlb_vec_len(*rects);

    ta_vec2i position = { 0 };
    position.x = 0;
    position.y = font->ascent;
    ta_vec2i cursor = position;
    bool cursor_set = false;

    // Loop until i == text_len or, if text_len is 0, we hit a nil character
    size_t newlines = 0;
    size_t i = 0;
    for (; ((text_len) ? i < text_len : text[i]); i++) {
        if (!cursor_set && !mouse_coords && cursor_idx && *cursor_idx == i) {
            cursor = position;
            cursor_set = true;
        }

        if (text[i] == '\n') {
            //DLB_ASSERT(y_max <= font->pixel_height);
            position.x = bounds.x;
            position.y += font->line_height;
            bounds.h += font->line_height;
            newlines++;
        } else if (text[i] >= font->first_char && text[i] <= font->last_char) {
            ta_vec2i baked_pos = position;
            ta_rect_uv *rect_uv = dlb_vec_alloc(*rects);
            ta_baked_quad(font->chars, font->tex_w, font->tex_h,
                text[i] - 32, &baked_pos.x, &baked_pos.y, rect_uv);
            bounds.w = MAX(bounds.w, (int)baked_pos.x - bounds.x);

            // HACK: Flip world text upside down.. this is super gross,
            //       surely there's a better way?
            if (!screen) {
                rect_uv->rect.y = font->line_height - (rect_uv->rect.y + rect_uv->rect.h);
                float v = rect_uv->uv0.v;
                rect_uv->uv0.v = rect_uv->uv1.v;
                rect_uv->uv1.v = v;
            }

            if (!cursor_set && mouse_coords && screen) {
                int x_advance = baked_pos.x - position.x;
                int y_top = position.y - font->ascent;

                // Check if user clicked on this character
                if (mouse_coords->x >= position.x &&
                    mouse_coords->x <= position.x + x_advance &&
                    mouse_coords->y >= y_top &&
                    mouse_coords->y <= y_top + font->line_height)
                {
                    // If click on left half of char rect, use previous position
                    if (mouse_coords->x < position.x + x_advance/2) {
                        cursor = position;
                        if (cursor_idx) {
                            *cursor_idx = i;
                        }
                    } else {
                        cursor.x = baked_pos.x;
                        cursor.y = baked_pos.y;
                        if (cursor_idx) {
                            *cursor_idx = i + 1;
                        }
                    }
                    cursor_set = true;
                }
            }

            position = baked_pos;
        }
    }
    bounds.h += font->line_height;

    size_t rects_len = dlb_vec_len(*rects) - orig_rects_len;
    DLB_ASSERT(rects_len + newlines == text_len);

    if (cursor_offset) {
        if (!cursor_set) {
            cursor = position;
            if (cursor_idx) {
                *cursor_idx = i;
            }
        }
        cursor_offset->x = cursor.x;
        cursor_offset->y = cursor.y - font->ascent;
    }
    return bounds;
}

// z postiive for screen, negative for world
void ta_font_render(ta_font *font, float x, float y, float z, bool clear_buffers, bool reset_uniforms, ta_mesh *mesh)
{
    //glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);

    ta_shader *shader = ta_font_shader(font);
    if (x || y || z) {
        ta_vec3 offset = { 0 };
        offset.x = NDC_X(x) + 1.0f;
        offset.y = NDC_Y(y) - 1.0f;
        offset.z = z;
        ta_mat4 xform = mat4_translate(offset);
        ta_shader_set_mat4(shader, SYM_U_MODEL, &xform);
    }
    ta_shader_set_sampler_2d(shader, SYM_U_TEX, font->gl_id);
    ta_primitive_render_mesh(mesh, shader, TA_TRIANGLES, clear_buffers, reset_uniforms);
    ta_shader_set_sampler_2d(shader, SYM_U_TEX, 0);

    glEnable(GL_DEPTH_TEST);
}