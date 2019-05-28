#pragma once
#include "ta_scene.h"
#include "misc/gl3w.h"

typedef struct ta_texture_s {
    ta_scene_ref ref;
    const char *path;
	int width;
	int height;
	int channels;
	GLuint gl_id;
} ta_texture;

void ta_texture_init(ta_texture *texture, const char *name, const char *path);
void ta_texture_create(ta_texture *texture);
void ta_texture_delete(ta_texture *texture);
void ta_texture_free(ta_texture *texture);