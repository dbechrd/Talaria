#pragma once
#include "dlb_types.h"
#include "ta_button_state.h"

#define TA_SDL_NUM_SCANCODES 512

enum {
    TA_SCANCODE_MOUSE_LEFT = TA_SDL_NUM_SCANCODES,
    TA_SCANCODE_MOUSE_MIDDLE,
    TA_SCANCODE_MOUSE_RIGHT,
    TA_SCANCODE_COUNT
};

typedef struct ta_mouse {
    int x;
    int y;
    ta_button_state left;
    ta_button_state middle;
    ta_button_state right;
    bool captured;
} ta_mouse;

extern ta_mouse tg_mouse;

void ta_mouse_init();
void ta_mouse_toggle_capture();
void ta_mouse_events();