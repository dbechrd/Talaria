#pragma once
#include "misc/gl3w.h"

typedef struct ta_scene_s ta_scene;

typedef struct ta_texture_2d_s {
    ta_scene *scene;
    const char *name;
    const char *path;
	int width;
	int height;
	int channels;
	GLuint gl_id;
} ta_texture_2d;

void ta_texture_init(ta_texture_2d *texture, const char *name, const char *path);
void ta_texture_create(ta_texture_2d *texture);
void ta_texture_free(ta_texture_2d *texture);