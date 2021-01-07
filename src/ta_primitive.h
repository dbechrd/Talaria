#pragma once
#include "ta_math.h"
#include "dlb/dlb_types.h"

struct ta_shader;
struct ta_mesh;

// NOTE: Must match OpenGL values for now (should probably make a lookup table)
//#define TA_POINTS          0x0000
#define TA_LINES           0x0001
//#define TA_LINE_LOOP       0x0002
//#define TA_LINE_STRIP      0x0003
#define TA_TRIANGLES       0x0004
//#define TA_TRIANGLE_STRIP  0x0005
//#define TA_TRIANGLE_FAN    0x0006
//#define TA_QUADS           0x0007

#define TA_PRIMITIVE_CONE_RADIUS_SCALE 20.0f

extern struct ta_mesh primitive_lines;
extern struct ta_mesh primitive_lines_perma;

extern struct ta_mesh primitive_quads;
extern struct ta_mesh primitive_quads_tooltip_bg;
extern struct ta_mesh primitive_quads_tooltip_fg;

extern struct ta_mesh primitive_sphere;

void ta_primitive_init                  ();
void ta_primitive_push_line_2d          (struct ta_mesh *mesh, ta_line_2d line_2d, ta_rgba color0, ta_rgba color1);
void ta_primitive_push_line_3d          (struct ta_mesh *mesh, ta_line_3d line_3d, ta_rgba color0, ta_rgba color1);
void ta_primitive_push_quad             (struct ta_mesh *mesh, ta_quad quad, ta_rgba color);
void ta_primitive_push_rect             (struct ta_mesh *mesh, ta_rect rect, ta_rgba color, float z);
void ta_primitive_push_rect_uv          (struct ta_mesh *mesh, ta_rect_uv rect_uv, ta_rgba color, float z, bool screen);
void ta_primitive_push_plane            (struct ta_mesh *mesh, struct ta_plane plane, float radius, ta_rgba color);
void ta_primitive_push_crosshair        (struct ta_mesh *mesh, s32 length, s32 thickness);
void ta_primitive_push_sphere           (struct ta_mesh *mesh, ta_sphere sphere, ta_rgba color);
void ta_primitive_push_rgb_sphere       (struct ta_mesh *mesh, ta_sphere sphere);
void ta_primitive_push_cone             (struct ta_mesh *mesh, ta_cone cone, ta_rgba color);
void ta_primitive_push_arrow            (struct ta_mesh *mesh, ta_vec3 origin, ta_vec3 direction, ta_rgba color);
void ta_primitive_push_aabb             (struct ta_mesh *mesh, ta_aabb aabb, ta_rgba color);
void ta_primitive_push_obb              (struct ta_mesh *mesh, ta_obb obb, ta_rgba color);
void ta_primitive_push_capsule          (struct ta_mesh *mesh, ta_capsule capsule, ta_rgba color);
void ta_primitive_push_cube             (struct ta_mesh *mesh, ta_vec3 center, float radius, ta_rgba color);
void ta_primitive_push_grid             (struct ta_mesh *mesh, ta_vec3 center, ta_vec3 normal, float radius, float frequency, ta_rgba color);
void ta_primitive_push_axes_arrow_color (struct ta_mesh *mesh, ta_vec3 position, ta_vec4 orientation, float scale, ta_rgba cx, ta_rgba cy, ta_rgba cz);
void ta_primitive_push_axes_arrow       (struct ta_mesh *mesh, ta_vec3 position, ta_vec4 orientation, float scale);
void ta_primitive_push_axes_cube        (struct ta_mesh *mesh, ta_vec3 position, float scale);
void ta_primitive_render_mesh           (struct ta_mesh *mesh, struct ta_shader *shader, bool clear_buffers);
void ta_primitive_dump                  (bool clear_buffers);

void ta_primitive_generate_sphere       (struct ta_mesh *mesh);