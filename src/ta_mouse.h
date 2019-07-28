#pragma once
#include "dlb_types.h"
#include "ta_button_state.h"

#define TA_SDL_NUM_SCANCODES 512

enum {
    SDL_SCANCODE_MOUSE_LEFT = TA_SDL_NUM_SCANCODES,
    SDL_SCANCODE_MOUSE_MIDDLE,
    SDL_SCANCODE_MOUSE_RIGHT,
    TA_SCANCODE_COUNT
};

typedef struct ta_mouse {
    int x;
    int y;
    ta_key_state left;
    ta_key_state middle;
    ta_key_state right;
    bool captured;
} ta_mouse;

extern ta_mouse tg_mouse;

void ta_mouse_init();
void ta_mouse_toggle_capture();
void ta_mouse_events();