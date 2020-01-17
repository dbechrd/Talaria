#pragma once
#include "ta_uid.h"
#include "ta_math.h"
#include "dlb/dlb_types.h"
#include "misc/stb_truetype.h"
#include "misc/gl3w.h"

struct ta_mesh;

typedef struct ta_font {
    u32 index;
    const char *name;
    const char *path;
    float pixel_height;
    const char *shader;  // TODO: This doesn't belong here

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

void ta_font_init(ta_font *font);
void ta_font_load_path(ta_font *font, const char *path);
void ta_font_delete(ta_font *font);
void ta_font_free(ta_font *font);
ta_rectf ta_font_push_text(ta_rect_uv **rects, ta_font *font, const char *text,
    u32 text_len, bool screen, u32 *cursor_idx, ta_vec2 *cursor_offset,
    const ta_vec2i *mouse_coords);
void ta_font_render(struct ta_mesh *mesh, ta_font *font, float x, float y,
    float z, bool clear_buffers, bool reset_uniforms);