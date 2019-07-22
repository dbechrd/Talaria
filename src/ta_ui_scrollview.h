#pragma once
#include "ta_primitive.h"
#include "ta_viewport.h"
#include "ta_texture.h"

void ta_ui_window_begin(ta_vec2i *pos, ta_size *size, int *scroll_v);
void ta_ui_image(int w, int h, ta_texture *tex);
void ta_ui_window_end();
void ta_ui_test();