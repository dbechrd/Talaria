 #pragma once
#include "ta_event.h"

// Keybind trigger flags
enum {
    TA_KEY_PRESS   = 1 << 0,  // once when key pressed
    TA_KEY_HOLD    = 1 << 1,  // while key is held down
    TA_KEY_RELEASE = 1 << 2,  // once when key released
};

typedef enum SDL_Scancode ta_key;

typedef struct ta_keybind {
    ta_event_type event_type;
    u32 triggers;
    ta_key keys[3];
    bool down;              // all keys are currently down
    bool changed;           // state changed since last frame
    double last_change_ms;  // time of last state change in milliseconds
} ta_keybind;

#if 0
typedef struct ta_keyboard {
    ta_key_state key_states[SDL_NUM_SCANCODES];
    ta_keybind *keybinds[TA_GAME_STATE_COUNT];
} ta_keyboard;

extern ta_keyboard tg_keyboard;
#endif

enum ta_game_state;

void ta_keyboard_init();
void ta_keybind_bind1(enum ta_game_state state_type, ta_event_type event_type,
    u32 triggers, ta_key key1);
void ta_keybind_bind2(enum ta_game_state state_type, ta_event_type event_type,
    u32 triggers, ta_key key1, ta_key key2);
void ta_keybind_bind3(enum ta_game_state state_type, ta_event_type event_type,
    u32 triggers, ta_key key1, ta_key key2, ta_key key3);
void ta_keybind_update(ta_keybind *keybind);
bool ta_keybind_down(ta_keybind *keybind);
bool ta_keybind_pressed(ta_keybind *keybind);
bool ta_keybind_released(ta_keybind *keybind);
bool ta_keybind_triggered(ta_keybind *keybind);
void ta_keyboard_events();