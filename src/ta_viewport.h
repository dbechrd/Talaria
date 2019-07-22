#pragma once
#include "ta_primitive.h"
#include "ta_camera.h"

typedef struct ta_viewport {
	ta_size size;
	ta_rgba background;
} ta_viewport;

ta_viewport ta_viewport_init(ta_size size, ta_rgba background);
void ta_viewport_bind(ta_viewport *view, ta_vec2i position, bool relative);
void ta_viewport_unbind();