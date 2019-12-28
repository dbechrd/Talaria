#pragma once
#include "ta_math.h"
#include "dlb/dlb_types.h"

struct ta_shader;

typedef struct ta_shader_lines_vertex {
    ta_vec3 position;
    ta_rgba color;
} ta_shader_lines_vertex;

typedef struct ta_vert_line {
    ta_shader_lines_vertex verts[2];
} ta_vert_line;

typedef struct ta_shader_quads_vertex {
    ta_vec3 position;
    ta_rgba color;
    ta_vec2 uv;
} ta_shader_quads_vertex;

typedef struct ta_vert_quad {
    ta_shader_quads_vertex verts[6];
} ta_vert_quad;

extern ta_vert_line *lines_queue;
extern ta_vert_quad *quads_queue;
extern ta_vert_quad *tooltip_bg_queue;
extern ta_vert_quad *tooltip_fg_queue;

void ta_primitive_init();
void ta_primitive_push_line_2d(ta_line_2d line_2d, ta_rgba color0, ta_rgba color1);
void ta_primitive_push_line_3d(ta_line_3d line_3d, ta_rgba color0, ta_rgba color1);
void ta_primitive_push_rect_q(ta_vert_quad **queue, ta_rect rect, ta_rgba color,
    float z);
void ta_primitive_push_rect(ta_rect rect, ta_rgba color, float z);
void ta_primitive_push_rect_uv(ta_vert_quad **queue, ta_rect_uv rect_uv,
    ta_rgba color, float z, bool screen, bool top_left);
void ta_primitive_push_plane(struct ta_plane plane, float radius, ta_rgba color);
void ta_primitive_push_crosshair(s32 length, s32 thickness);
void ta_primitive_push_axes(float scale);
void ta_primitive_push_sphere(ta_sphere sphere, ta_rgba color);
void ta_primitive_push_rgb_sphere(ta_sphere sphere);
void ta_primitive_push_aabb(ta_aabb aabb, ta_rgba color);
void ta_primitive_push_obb(ta_obb obb, ta_rgba color);
void ta_primitive_render_lines(struct ta_shader *shader, bool clear_queues,
    bool reset_uniforms);
void ta_primitive_render_quads(ta_vert_quad *queue, struct ta_shader *shader,
    bool clear_queues, bool reset_uniforms);
void ta_primitive_render(bool clear_queues, bool reset_uniforms);