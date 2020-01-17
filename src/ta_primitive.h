#pragma once
#include "ta_math.h"
#include "dlb/dlb_types.h"

struct ta_shader;
struct ta_mesh;

extern struct ta_mesh primitive_lines;
extern struct ta_mesh primitive_lines_perma;

extern struct ta_mesh primitive_quads;
extern struct ta_mesh primitive_quads_tooltip_bg;
extern struct ta_mesh primitive_quads_tooltip_fg;

void ta_primitive_init();
void ta_primitive_push_line_2d(ta_line_2d line_2d, ta_rgba color0,
    ta_rgba color1);
void ta_primitive_push_line_3d_q(struct ta_mesh *mesh, ta_line_3d line_3d,
    ta_rgba color0, ta_rgba color1);
void ta_primitive_push_line_3d(ta_line_3d line_3d, ta_rgba color0,
    ta_rgba color1);
void ta_primitive_push_rect_q(struct ta_mesh *mesh, ta_rect rect, ta_rgba color,
    float z);
void ta_primitive_push_rect(ta_rect rect, ta_rgba color, float z);
void ta_primitive_push_rect_uv_q(struct ta_mesh *mesh, ta_rect_uv rect_uv,
    ta_rgba color, float z, bool screen, bool top_left);
void ta_primitive_push_rect_uv(ta_rect_uv rect_uv, ta_rgba color, float z,
    bool screen, bool top_left);
void ta_primitive_push_plane(struct ta_plane plane, float radius, ta_rgba color);
void ta_primitive_push_crosshair(s32 length, s32 thickness);
void ta_primitive_push_sphere(ta_sphere sphere, ta_rgba color);
void ta_primitive_push_rgb_sphere(ta_sphere sphere);
void ta_primitive_push_cone(ta_cone cone, ta_rgba color);
void ta_primitive_push_arrow(ta_vec3 origin, ta_vec3 direction, ta_rgba color);
void ta_primitive_push_aabb(ta_aabb aabb, ta_rgba color);
void ta_primitive_push_obb(ta_obb obb, ta_rgba color);
void ta_primitive_push_axes_arrow(ta_vec3 position, float scale);
void ta_primitive_push_axes_cube(ta_vec3 position, float scale);
void ta_primitive_render_lines(struct ta_mesh *mesh, struct ta_shader *shader,
    bool clear_buffers, bool reset_uniforms);
void ta_primitive_render_quads(struct ta_mesh *mesh, struct ta_shader *shader,
    bool clear_buffers, bool reset_uniforms);
void ta_primitive_render(bool clear_buffers, bool reset_uniforms);