#pragma once
#include "dlb/dlb_types.h"

struct ta_event;

void ta_mouse_init              ();
void ta_mouse_reset_frame       ();
void ta_mouse_capture_set       (bool capture);
void ta_mouse_capture_toggle    ();
bool ta_mouse_captured          ();
void ta_mouse_drag_begin        ();
void ta_mouse_drag_end          ();
bool ta_mouse_dragging          ();
int ta_mouse_x                  ();
int ta_mouse_y                  ();
int ta_mouse_dx                 ();
int ta_mouse_dy                 ();
int ta_mouse_scroll_dx          ();
int ta_mouse_scroll_dy          ();
void ta_mouse_move              (int x, int y);
void ta_mouse_event             (struct ta_event *event);