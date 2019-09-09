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
    enum ta_event_type event_type;
    u32 triggers;
    ta_key keys[3];
    ta_button_state key_state;
} ta_keybind;

void ta_keybind_bind1(ta_keybind **keybinds, enum ta_event_type event_type,
    u32 triggers, ta_key key1);
void ta_keybind_bind2(ta_keybind **keybinds, enum ta_event_type event_type,
    u32 triggers, ta_key key1, ta_key key2);
void ta_keybind_bind3(ta_keybind **keybinds, enum ta_event_type event_type,
    u32 triggers, ta_key key1, ta_key key2, ta_key key3);
//bool ta_keybind_down(ta_keybind *keybind);
//bool ta_keybind_pressed(ta_keybind *keybind);
//bool ta_keybind_released(ta_keybind *keybind);
//bool ta_keybind_triggered(ta_keybind *keybind);
void ta_keybind_trigger(ta_keybind *keybinds);