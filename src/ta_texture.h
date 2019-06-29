#pragma once
#include "ta_scene.h"
#include "misc/gl3w.h"

typedef struct ta_texture_s {
    ta_scene_ref ref;
    const char *path;  // File path
    u8 *pixels;        // Pixel data (if inlined instead of via path)
    int width;         // Size of texture (pixels)
	int height;
	int channels;      // Number of color channels (1, 2, 4)
    bool linear;       // True if linear color space. E.g. metallic, etc.
	GLuint gl_id;
} ta_texture;

void ta_texture_init(ta_texture *tex);
void ta_texture_load_path(ta_texture *tex, const char *path);
void ta_texture_load(ta_texture *tex);
void ta_texture_delete(ta_texture *tex);
void ta_texture_free(ta_texture *tex);