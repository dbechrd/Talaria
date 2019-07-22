#pragma once
#include "ta_primitive.h"
#include "ta_camera.h"

void ta_viewport_bind(ta_rect parent, ta_rect rect, ta_rgba background, bool relative);
void ta_viewport_unbind();