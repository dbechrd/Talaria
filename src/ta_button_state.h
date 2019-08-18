#pragma once
#include "dlb/dlb_types.h"

typedef struct ta_button_state {
    bool down;              // button is currently down
    bool changed;           // state changed since last frame
    double last_change_ms;  // time of last state change in milliseconds
} ta_button_state;

inline bool ta_button_state_down(ta_button_state *state)
{
    return state->down;
}

inline bool ta_button_state_pressed(ta_button_state *state)
{
    return state->down && state->changed;
}

inline bool ta_button_state_released(ta_button_state *state)
{
    return !state->down && state->changed;
}