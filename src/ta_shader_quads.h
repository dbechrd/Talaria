#pragma once
#include "ta_primitive.h"
#include "misc/gl3w.h"

typedef struct {
    ta_vec3 position;
	ta_color color;
	ta_uv uv;
} ta_shader_quads_vertex;

typedef struct {
	ta_shader_quads_vertex verts[6];
} ta_vert_quad;

GLuint ta_shader_quads_init();
void ta_shader_quads_free(GLuint program);
void ta_shader_quads_attribs();
void ta_shader_quads_set_projection(const ta_mat4 *mat);
void ta_shader_quads_set_view(const ta_mat4 *mat);
void ta_shader_quads_set_model(const ta_mat4 *mat);
void ta_shader_quads_set_texture(int index, GLuint texture);
void ta_shader_quads_prerender();