#pragma once
#include "ta_primitive.h"
#include "dlb_types.h"

typedef struct {
	ta_rect rect;
	int sample_count;
	u32 *samples;
	int next_index;
	int smooth_val;
} ta_barchart;

ta_barchart ta_barchart_init(int x, int y, int w, int h);
void ta_barchart_free(ta_barchart *chart);
void ta_barchart_draw(int x, int y, ta_barchart *chart);