#pragma once
#include "ta_primitive.h"
#include "misc/gl3w.h"

typedef struct {
    ta_vec3 position;
    ta_color4 color;
} ta_shader_lines_vertex;

typedef struct {
	ta_shader_lines_vertex verts[2];
} ta_vert_line;

GLuint ta_shader_lines_init();
void ta_shader_lines_free(GLuint program);
void ta_shader_lines_attribs();