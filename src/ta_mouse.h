#pragma once
#include "dlb/dlb_types.h"

struct ta_event;

#define MOUSE_X ta_mouse_x()
#define MOUSE_Y ta_mouse_y()

void ta_mouse_init();
void ta_mouse_capture_set(bool capture);
void ta_mouse_capture_toggle();
bool ta_mouse_captured();
int ta_mouse_x();
int ta_mouse_y();
void ta_mouse_event(struct ta_event *event);