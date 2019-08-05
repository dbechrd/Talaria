#pragma once
#include "ta_uid.h"
#include "dlb_types.h"
#include "misc/stb_truetype.h"
#include "misc/gl3w.h"

typedef struct ta_font {
    ta_uid uid;
    const char *path;
    float pixel_height;
    const char *shader_uid;
    GLuint gl_id;
    stbtt_bakedchar *chars;
} ta_font;

void ta_font_init(ta_font *font);
void ta_font_load_path(ta_font *font, const char *path);
void ta_font_delete(ta_font *font);
void ta_font_free(ta_font *font);
struct ta_shader *ta_font_shader(ta_font *font);
float ta_font_push_text(ta_font *font, float x, float y, const char *text,
    bool screen);
void ta_font_render(ta_font *font, bool clear_queues, bool reset_uniforms);