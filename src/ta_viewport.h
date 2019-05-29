#pragma once
#include "ta_primitive.h"
#include "ta_camera.h"
#include "dlb_types.h"

typedef struct {
	ta_rect rect;
	ta_rgba background;
    ta_camera *camera;
} ta_viewport;

ta_viewport ta_viewport_init(int left, int top, int width, int height,
	ta_rgba background, ta_camera *camera);
void ta_viewport_bind(ta_viewport *view, bool stretch_to_fit);
void ta_viewport_unbind();