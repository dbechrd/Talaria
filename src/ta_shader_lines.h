#pragma once
#include "ta_primitive.h"
#include "misc/gl3w.h"

typedef struct {
    ta_vec3 position;
    ta_color color;
} ta_shader_lines_vertex;

typedef struct {
	ta_shader_lines_vertex verts[2];
} ta_vert_line;

GLuint ta_shader_lines_init();
void ta_shader_lines_free();
void ta_shader_lines_attribs();
void ta_shader_lines_bind();
void ta_shader_lines_unbind();
void ta_shader_lines_set_projection(const ta_mat4 *projection);
void ta_shader_lines_set_view(const ta_mat4 *view);
void ta_shader_lines_set_model(const ta_mat4 *model);
void ta_shader_lines_prerender();
void ta_shader_lines_render(ta_vert_line *lines);