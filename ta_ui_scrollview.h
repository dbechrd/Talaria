#pragma once
#include "ta_primitive.h"
#include "dlb_types.h"

typedef enum {
	TA_UI_SCROLLBAR,
	TA_UI_SCROLLVIEW,
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
} ta_ui_scrollbar;

typedef struct {
	ta_ui_type type;
	ta_rect rect;
	ta_ui_scrollbar scrollbar;
	ta_ui_base *content;
} ta_ui_scrollview;

ta_ui_scrollview *ta_ui_scrollview_init(int x, int y, int w, int h, ta_ui_base *content);
void ta_ui_scrollview_scroll(ta_ui_scrollview *view, int scroll);
void ta_ui_scrollview_draw(ta_ui_scrollview *view);
void ta_ui_clear();