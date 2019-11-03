#pragma once
#include "dlb/dlb_types.h"

struct ta_event;

#define MOUSE_X ta_mouse_x()
#define MOUSE_Y ta_mouse_y()

void ta_mouse_init();
void ta_mouse_capture_set(bool capture);
void ta_mouse_capture_toggle();
bool ta_mouse_captured();
void ta_mouse_drag_begin();
void ta_mouse_drag_end();
bool ta_mouse_dragging();
int ta_mouse_x();
int ta_mouse_y();
int ta_mouse_dx();
int ta_mouse_dy();
void ta_mouse_update();
void ta_mouse_event(struct ta_event *event);