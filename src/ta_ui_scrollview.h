#pragma once
#include "ta_primitive.h"
#include "ta_viewport.h"
#include "ta_texture.h"

typedef enum ta_ui_type {
	TA_UI_SCROLLBAR,
	TA_UI_SCROLLVIEW,
	TA_UI_IMAGE,
	TA_UI_COUNT
} ta_ui_type;

typedef struct ta_ui_scrollbar_widget {
    ta_rect rect;
    int offset;
} ta_ui_scrollbar_widget;

typedef struct ta_ui_scrollbar {
    ta_rect rect;
    ta_ui_scrollbar_widget widget;
    bool visible;
} ta_ui_scrollbar;

struct ta_ui_control;
typedef struct ta_ui_scrollview {
    //ta_ui_scrollbar scrollbar_y;
    struct ta_ui_control *content;
} ta_ui_scrollview;

typedef struct ta_ui_image {
    ta_texture *tex;
} ta_ui_image;

typedef struct ta_ui_control {
    ta_ui_type type;
    ta_size size;
    union {
        ta_ui_scrollbar scrollbar;
        ta_ui_scrollview scrollview;
        ta_ui_image image;
    } data;
} ta_ui_control;

void ta_ui_draw(ta_ui_control *control, ta_vec2i position);
void ta_ui_clear();

ta_ui_control *ta_ui_create_image(ta_size size, ta_texture *tex);
ta_ui_control *ta_ui_create_scrollview(ta_size size, ta_ui_control *content);
void ta_ui_scrollview_scroll(ta_ui_scrollview *sv, int scroll);
