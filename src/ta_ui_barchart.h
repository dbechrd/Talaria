#pragma once
#include "ta_math.h"
#include "ta_primitive.h"
#include "dlb/dlb_types.h"

typedef struct ta_ui_barchart {
    ta_rect rect;
    int sample_count;
    u32 *samples;
    int next_index;
    int smooth_val;
} ta_ui_barchart;

ta_ui_barchart ta_ui_barchart_init(int x, int y, int w, int h);
void ta_ui_barchart_free(ta_ui_barchart *chart);
void ta_ui_barchart_draw(int x, int y, ta_ui_barchart *chart);