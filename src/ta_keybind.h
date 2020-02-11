 #pragma once
#include "ta_key.h"
#include "ta_button_state.h"

enum ta_game_state;
struct ta_event;

// Keybind trigger flags
enum {
    TA_KEYBIND_PRESS   = 1 << 0,  // once when key pressed
    TA_KEYBIND_HOLD    = 1 << 1,  // while key is held down
    TA_KEYBIND_RELEASE = 1 << 2,  // once when key released
};

typedef struct ta_keybind {
    u32             triggers;   // trigger flag bitmap (PRESS, HOLD, RELEASE); when to trigger keybind
    ta_key          keys[3];    // which keys need to be pressed simultaneously to trigger keybind
    ta_button_state key_state;  // tracks current/prev state and timings (for delta events and delayed actions)
} ta_keybind;

void ta_keybind_init1       (ta_keybind *keybind, u32 triggers, ta_key key1);
void ta_keybind_init2       (ta_keybind *keybind, u32 triggers, ta_key key1, ta_key key2);
void ta_keybind_init3       (ta_keybind *keybind, u32 triggers, ta_key key1, ta_key key2, ta_key key3);
bool ta_keybind_down        (const ta_keybind *keybind);
bool ta_keybind_pressed     (const ta_keybind *keybind);
bool ta_keybind_released    (const ta_keybind *keybind);
bool ta_keybind_triggered   (const ta_keybind *keybind);
void ta_keybind_update      (ta_keybind *keybind);
