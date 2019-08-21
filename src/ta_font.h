#pragma once
#include "ta_uid.h"
#include "ta_primitive.h"
#include "dlb/dlb_types.h"
#include "misc/stb_truetype.h"
#include "misc/gl3w.h"

typedef struct ta_font {
    ta_uid uid;
    const char *path;
    float pixel_height;
    const char *shader_uid;

    int first_char;
    int last_char;
    stbtt_bakedchar *chars;

    stbtt_fontinfo font_info;
    float scale;
    int ascent;
    int descent;
    int line_height;
    int left_bearing;
    ta_rect bbox;

    int tex_w;
    int tex_h;
    GLuint gl_id;
} ta_font;

struct ta_vert_quad;
struct ta_rectf;

void ta_font_init(ta_font *font);
void ta_font_load_path(ta_font *font, const char *path);
void ta_font_delete(ta_font *font);
void ta_font_free(ta_font *font);
struct ta_shader *ta_font_shader(ta_font *font);
ta_rectf ta_font_push_text(ta_rect_uv **rects, ta_font *font, const char *text,
    u32 text_len, bool screen, u32 cursor_idx, ta_vec2 *cursor_offset);
void ta_font_render(ta_vert_quad *queue, ta_font *font, float x, float y,
    float z, bool clear_queues, bool reset_uniforms);