#pragma once
#include "ta_primitive.h"
#include "ta_viewport.h"
#include "ta_texture.h"
#include "dlb_types.h"

typedef enum {
	TA_UI_SCROLLBAR,
	TA_UI_SCROLLVIEW,
	TA_UI_IMAGE,
	TA_UI_COUNT
} ta_ui_type;

typedef struct {
	ta_ui_type type;
	ta_rect rect;
} ta_ui_base;

typedef struct {
	ta_rect rect;
	int offset;
} ta_ui_scrollbar_widget;

typedef struct {
	ta_rect rect;
	ta_ui_scrollbar_widget widget;
	bool visible;
} ta_ui_scrollbar;

typedef struct {
	ta_ui_type type;
	ta_rect rect;
	ta_ui_scrollbar scrollbar_y;
	ta_viewport viewport;
	ta_ui_base *content;
} ta_ui_scrollview;

typedef struct {
	ta_ui_type type;
	ta_rect rect;
	ta_texture_2d *tex;
} ta_ui_image;

void ta_ui_draw(int x, int y, ta_ui_base *ui);
void ta_ui_clear();

ta_ui_image *ta_ui_image_init(int x, int y, int w, int h, ta_texture_2d *tex);
void ta_ui_image_draw(int x, int y, ta_ui_image *image);

ta_ui_scrollview *ta_ui_scrollview_init(int x, int y, int w, int h,
	ta_ui_base *content);
void ta_ui_scrollview_scroll(ta_ui_scrollview *view, int scroll);
void ta_ui_scrollview_draw(int x, int y, ta_ui_scrollview *view);