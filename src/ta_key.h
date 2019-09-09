#pragma once
#include "dlb/dlb_types.h"

struct ta_event;

// HACK: We later ASSERT this is == SDL_NUM_SCANCODES, but this prevents us from
// having to #include massive SDL header and dependencies
#define TA_SDL_NUM_SCANCODES 512

enum {
    SDL_SCANCODE_MOUSE_LEFT = TA_SDL_NUM_SCANCODES,
    SDL_SCANCODE_MOUSE_MIDDLE,
    SDL_SCANCODE_MOUSE_RIGHT,
    SDL_SCANCODE_MOUSE_X1,
    SDL_SCANCODE_MOUSE_X2,
    TA_SCANCODE_COUNT
};

typedef s32 ta_key;

bool ta_key_down(ta_key key);
bool ta_key_up(ta_key key);
bool ta_key_pressed(ta_key key);
bool ta_key_released(ta_key key);
void ta_key_reset_changed();
void ta_key_event(struct ta_event *event);