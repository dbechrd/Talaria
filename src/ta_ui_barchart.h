#pragma once
#include "ta_primitive.h"
#include "dlb/dlb_types.h"

typedef struct ta_ui_barchart {
    ta_rect rect;
    int sample_count;
    u32 *samples;
    int next_index;
    int smooth_val;
} ta_ui_barchart;

void ta_ui_barchart_init(ta_ui_barchart *chart, int x, int y, int w, int h);
void ta_ui_barchart_free(ta_ui_barchart *chart);
void ta_ui_barchart_draw(ta_ui_barchart *chart, int x, int y);