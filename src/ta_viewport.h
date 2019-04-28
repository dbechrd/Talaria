#pragma once
#include "ta_primitive.h"
#include "dlb_types.h"

typedef struct {
	ta_rect rect;
	float fov;
	float nearz;
	ta_mat4 projection;
	ta_rgba background;
} ta_viewport;

ta_viewport ta_viewport_init(int left, int top, int width, int height,
	float fov, float nearz, ta_rgba background);
void ta_viewport_bind(ta_viewport *view, bool aspect);
void ta_viewport_unbind();