#pragma once
#include "dlb_types.h"
#include "SDL/SDL.h"

enum {
    TA_SCANCODE_MOUSE_LEFT = SDL_NUM_SCANCODES,
    TA_SCANCODE_MOUSE_MIDDLE,
    TA_SCANCODE_MOUSE_RIGHT,
    TA_SCANCODE_COUNT
};

typedef struct {
    int x;
    int y;
    bool left;
    bool middle;
    bool right;
    bool captured;
} ta_mouse;

extern ta_mouse tg_mouse;

void ta_mouse_init();
void ta_mouse_toggle_capture();
void ta_mouse_update();