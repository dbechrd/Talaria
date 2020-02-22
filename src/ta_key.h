#pragma once
#include "dlb/dlb_types.h"
#include "GLFW/glfw3.h"

struct ta_event;

enum {
    GLFW_KEY_MOUSE_LEFT = GLFW_KEY_LAST + 1,
    GLFW_KEY_MOUSE_RIGHT,
    GLFW_KEY_MOUSE_MIDDLE,
    TA_KEY_COUNT
};

typedef s32 ta_key;

typedef struct ta_key_state {
    u8      down;            // button is currently down
    u8      changed;         // state changed since last frame
    u8      handled;         // global handled state to prevent double actions
    double  last_change_ms;  // time of last state change in milliseconds
} ta_key_state;

bool ta_key_down         (ta_key key);
bool ta_key_up           (ta_key key);
bool ta_key_pressed      (ta_key key);
bool ta_key_released     (ta_key key);
void ta_key_mark_handled (ta_key key);
void ta_key_clear_all    ();
void ta_key_event        (struct ta_event *event);