#pragma once
#include "dlb_types.h"

typedef struct ta_key_state {
    bool down;              // button is currently down
    bool changed;           // state changed since last frame
    double last_change_ms;  // time of last state change in milliseconds
} ta_key_state;

inline bool ta_key_state_down(ta_key_state *state)
{
    return state->down;
}

inline bool ta_key_state_pressed(ta_key_state *state)
{
    return state->down && state->changed;
}

inline bool ta_key_state_released(ta_key_state *state)
{
    return !state->down && state->changed;
}