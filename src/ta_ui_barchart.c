#include "ta_ui_barchart.h"
#include "ta_math.h"
#include "ta_primitive.h"
#include "dlb_types.h"
#include "dlb_memory.h"
#include <time.h>
#include <stdlib.h>

#define x_inc 2
static float alpha = 0.8f;
static const ta_rgb color = { 1.0f, 0.0f, 0.0f };

ta_ui_barchart ta_ui_barchart_init(int x, int y, int w, int h)
{
	ta_ui_barchart barchart = { 0 };
	barchart.rect.x = x;
	barchart.rect.y = y;
	barchart.rect.w = w;
	barchart.rect.h = h;
	barchart.sample_count = w / x_inc;
	barchart.samples = dlb_calloc(barchart.sample_count, sizeof(*barchart.samples));
	barchart.next_index = 0;
	barchart.smooth_val = barchart.rect.y / 2;
	return barchart;
}

void ta_ui_barchart_free(ta_ui_barchart *chart)
{
	dlb_free(chart->samples);
}

void ta_ui_barchart_draw(int x, int y, ta_ui_barchart *chart)
{
	ta_line_2d line = { 0 };
	ta_rgba color0 = { 0 };
	ta_rgba color1 = { 0 };
	for (int i = 0; i < chart->sample_count; i++) {
		int pos_x = x_inc * i;
		line.p0.x = (float)pos_x;
		line.p0.y = (float)chart->rect.h;
		line.p1.x = (float)pos_x;
		if (i == chart->next_index) {
			color0.r = 1;
			color0.g = 0;
			color1.r = 1;
			color1.g = 0;
			line.p1.y = (float)chart->rect.y;
		} else {
			color0.r = 0;
			color0.g = 0.6f;
			color1.r = 1.0f * (chart->samples[i] / (float)chart->rect.h);
			color1.g = 0.6f;
			line.p1.y = (float)(chart->rect.h - chart->samples[i]);
		}
		line.p0.x += x + chart->rect.x;
		line.p1.x += x + chart->rect.x;
		line.p0.y += y + chart->rect.y;
		line.p1.y += y + chart->rect.y;

        ta_mat3 mat_hue_rot = mat3_hue_rotation((float)i);
		ta_rgb cc = mat3_mul_rgb(&mat_hue_rot, color);
		float lum = color.r + color.g + color.b;
		float lum2 = cc.r + cc.g + cc.b;
		ta_vec3 res = *(ta_vec3 *)&cc;
		res = vec3_scalef(res, 1.0f + lum / lum2);
		cc = *(ta_rgb *)&res;

		color0.r = cc.r;
		color0.g = cc.g;
		color0.b = cc.b;
		color1.r = cc.r;
		color1.g = cc.g;
		color1.b = cc.b;
		ta_primitive_push_line_2d(line, color0, color1);
	}

	chart->next_index = (chart->next_index + 1) % chart->sample_count;
	u32 next_val = (u32)rand() % chart->rect.h;
	chart->smooth_val = (u32)(alpha * chart->smooth_val + (1.0f - alpha) * next_val);
	chart->samples[chart->next_index] = chart->smooth_val;
}