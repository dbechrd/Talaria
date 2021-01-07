#pragma once
#include "ta_schema.h"
#include "ta_math.h"
#include "dlb/dlb_types.h"
#include "misc/stb_truetype.h"
#include "misc/glad.h"

struct ta_mesh;

typedef struct ta_font {
    TA_RESOURCE_HEADER
    const char  *path;          // relative path to font file
    float       pixel_height;   // height of bitmap font to generate, in pixels
    const char  *shader;        // shader to use to render this font

    int first_char;             // first ASCII code to start generating bitmaps from (inclusive)
    int last_char;              // last ASCII code generate bitmap for (inclusive)
    stbtt_bakedchar *chars;     // [stb_truetype] baked characters (size info per character generated)
    stbtt_fontinfo font_info;   // [stb_truetype] misc. font properties
    float scale;                //
    int ascent;                 //
    int descent;                //
    int line_height;            //
    int left_bearing;           //
    ta_rect bbox;               //

    int tex_w;                  // bitmap font texture width in pixels
    int tex_h;                  // bitmap font texture height in pixels
    GLuint gl_id;               // [OpenGL] bitmap font texture id
} ta_font;

void ta_font_init           (ta_font *font);
void ta_font_init_void      (void *font);
void ta_font_load_path      (ta_font *font, const char *path);
void ta_font_delete         (ta_font *font);
void ta_font_free           (ta_font *font);
ta_rect ta_font_push_text   (ta_font *font, const char *text, size_t text_len, bool screen, size_t *cursor_idx,
                             ta_vec2i *cursor_offset, const ta_vec2i *mouse_coords, ta_rect_uv **rects);
void ta_font_render         (ta_font *font, float x, float y, float z, bool clear_buffers, struct ta_mesh *mesh);
