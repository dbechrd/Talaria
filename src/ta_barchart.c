#include "ta_barchart.h"
#include "ta_primitive.h"
#include "ta_window.h"
#include "ta_shader_lines.h"
#include "dlb_types.h"
#include <time.h>
#include <stdlib.h>

#define x_inc 3
static float alpha = 0.8f;

ta_barchart ta_barchart_init(int x, int y, int w, int h, int sample_count)
{
	ta_barchart barchart = { 0 };
	barchart.rect.x = x;
	barchart.rect.y = y;
	barchart.rect.w = w;
	barchart.rect.h = h;
	barchart.sample_count = sample_count;
	barchart.samples = calloc(sample_count, sizeof(*barchart.samples));
	barchart.next_index = 0;
	barchart.smooth_val = barchart.rect.y / 2;
	return barchart;
}

void ta_barchart_free(ta_barchart *chart)
{
	free(chart->samples);
}

void ta_barchart_draw(int x, int y, ta_barchart *chart)
{
	ta_line_2d line = { 0 };
	ta_color color0 = { 0 };
	ta_color color1 = { 0 };
	for (int i = 0; i < chart->sample_count; i++) {
		int pos_x = x_inc * i;
		line.p0.x = pos_x;
		line.p0.y = chart->rect.h;
		line.p1.x = pos_x;
		if (i == chart->next_index) {
			color0.r = 1;
			color0.g = 0;
			color1.r = 1;
			color1.g = 0;
			line.p1.y = chart->rect.y;
		} else {
			color0.r = 0;
			color0.g = 0.6f;
			color1.r = 1.0f * (chart->samples[i] / (float)chart->rect.h);
			color1.g = 0.6f;
			line.p1.y = chart->rect.h - chart->samples[i];
		}
		line.p0.x += x + chart->rect.x;
		line.p1.x += x + chart->rect.x;
		line.p0.y += y + chart->rect.y;
		line.p1.y += y + chart->rect.y;
		ta_primitive_push_line_2d(&line, &color0, &color1);
	}

	chart->next_index = (chart->next_index + 1) % chart->sample_count;
	u32 next_val = (u32)rand() % chart->rect.h;
	chart->smooth_val = (u32)(alpha * chart->smooth_val + (1.0f - alpha) * next_val);
	chart->samples[chart->next_index] = chart->smooth_val;
}