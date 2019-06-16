#pragma once
#include "ta_scene.h"
#include "misc/gl3w.h"

typedef struct ta_texture_s {
    ta_scene_ref ref;
    const char *path;  // File path
	int width;         // Size of texture (pixels)
	int height;
	int channels;      // Number of color channels (1, 2, 4)
    bool linear;       // True if linear color space. E.g. metallic, etc.
	GLuint gl_id;
} ta_texture;

void ta_texture_init(ta_texture *tex, const char *name, const char *path);
void ta_texture_create(ta_texture *tex);
void ta_texture_delete(ta_texture *tex);
void ta_texture_free(ta_texture *tex);