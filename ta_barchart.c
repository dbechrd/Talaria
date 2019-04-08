#include "ta_barchart.h"
#include "ta_primitive.h"
#include "ta_window.h"
#include "ta_shader_lines.h"
#include "dlb_types.h"
#include <time.h>
#include <stdlib.h>

#define x_max (980 - 40)
#define x_inc 3
#define y_max 20

static u32 samples[x_max / x_inc] = { 0 };
static s32 sample_idx = 0;
static u32 sample_smooth = y_max / 2;
static float alpha = 0.8f;

void ta_barchart_draw()
{
	ta_line_2d line = { 0 };
	ta_color4 color0 = { 0 };
	ta_color4 color1 = { 0 };
	for (int i = 0; i < ARRAY_COUNT(samples); i++) {
		int pos_x = x_inc * i;
		line.p0.x = pos_x;
		line.p0.y = y_max;
		line.p1.x = pos_x;
		if (i == sample_idx) {
			color0.r = 1;
			color0.g = 0;
			color1.r = 1;
			color1.g = 0;
			line.p1.y = 0;
		} else {
			color0.r = 0;
			color0.g = 0.6f;
			color1.r = 1.0f * (samples[i] / (float)y_max);
			color1.g = 0.6f;
			line.p1.y = y_max - samples[i];
		}
		line.p0.x += 20;
		line.p1.x += 20;
		line.p0.y += 20;
		line.p1.y += 20;
		ta_primitive_push_line_2d(&line, &color0, &color1);
	}

	sample_idx = (sample_idx + 1) % ARRAY_COUNT(samples);
	u32 next_val = (u32)rand() % y_max;
	sample_smooth = (u32)(alpha * sample_smooth + (1.0f - alpha) * next_val);
	samples[sample_idx] = sample_smooth;
}