#pragma once
#include "ta_primitive.h"
#include "dlb_types.h"

typedef struct {
	ta_rect rect;
	ta_color background;
} ta_viewport;

ta_viewport ta_viewport_init(int left, int top, int width, int height,
	ta_color background);
void ta_viewport_bind(ta_viewport *view, bool aspect);
void ta_viewport_unbind();