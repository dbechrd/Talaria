#pragma once
#include "ta_mesh.h"
#include "misc/gl3w.h"

void ta_shader_mesh_init();
void ta_shader_mesh_free();
void ta_shader_mesh_bind();
void ta_shader_mesh_unbind();
void ta_shader_mesh_set_projection(const ta_mat4 *projection);
void ta_shader_mesh_set_view(const ta_mat4 *view);
void ta_shader_mesh_set_model(const ta_mat4 *model);
void ta_shader_mesh_set_texture(int index, GLuint texture);
void ta_shader_mesh_prerender();
void ta_shader_mesh_render(ta_mesh *mesh);