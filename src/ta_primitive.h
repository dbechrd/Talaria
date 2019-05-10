#pragma once
#include "ta_math.h"

void ta_primitive_init();
void ta_primitive_push_line_2d(ta_line_2d line_2d, ta_rgba color0, ta_rgba color1);
void ta_primitive_push_line_3d(ta_line_3d line_3d, ta_rgba color0, ta_rgba color1);
void ta_primitive_push_rect(int x, int y, ta_rect rect, ta_rgba color);
void ta_primitive_render();
void ta_primitive_clear();