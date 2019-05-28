#pragma once
#include "ta_math.h"
#include "ta_rigid_body.h"

typedef struct {
    ta_vec2 p0;
    ta_vec2 p1;
} ta_line_2d;

typedef struct {
    ta_vec3 p0;
    ta_vec3 p1;
} ta_line_3d;

typedef struct {
    int x;
    int y;
    int w;
    int h;
} ta_rect;

void ta_primitive_init();
void ta_primitive_push_line_2d(ta_line_2d line_2d, ta_rgba color0, ta_rgba color1);
void ta_primitive_push_line_3d(ta_line_3d line_3d, ta_rgba color0, ta_rgba color1);
void ta_primitive_push_rect(int x, int y, ta_rect rect, ta_rgba color);
void ta_primitive_push_axes(float scale);
void ta_primitive_push_sphere(ta_sphere sphere, ta_rgba color);
void ta_primitive_push_aabb(ta_aabb aabb, ta_rgba color);
void ta_primitive_render();
void ta_primitive_clear();