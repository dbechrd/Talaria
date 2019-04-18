#pragma once
#include "ta_event.h"
#include "ta_game.h"
#include "dlb_types.h"
#include "SDL/SDL.h"

#if 0
enum {
    TA_SCANCODE_MOUSE_MOVE = SDL_NUM_SCANCODES,
    TA_SCANCODE_COUNT,
};
#endif

typedef struct {
    bool down;
    bool changed;
} ta_key_state;
ta_key_state tg_key_states[SDL_NUM_SCANCODES];

enum {
    TA_KEYBIND_TRIGGER_DOWN     = 1 << 0,
    TA_KEYBIND_TRIGGER_PRESSED  = 1 << 1,
    TA_KEYBIND_TRIGGER_RELEASED = 1 << 2,
};

typedef SDL_Scancode ta_key;

typedef struct {
    ta_event_type event_type;
    u32 triggers;
    ta_key keys[3];
    bool down;           // all keys are currently down
    bool changed;        // state changed since last frame
    u64 last_change_ms;  // time of last state change in milliseconds
} ta_keybind;

extern ta_keybind *tg_keybinds[TA_STATE_COUNT];

void ta_keybind_bind1(ta_game_state state_type, ta_event_type event_type,
    u32 triggers, ta_key key1);
void ta_keybind_bind2(ta_game_state state_type, ta_event_type event_type,
    u32 triggers, ta_key key1, ta_key key2);
void ta_keybind_bind3(ta_game_state state_type, ta_event_type event_type,
    u32 triggers, ta_key key1, ta_key key2, ta_key key3);
void ta_keybind_update(ta_keybind *keybind);
bool ta_keybind_down(ta_keybind *keybind);
bool ta_keybind_pressed(ta_keybind *keybind);
bool ta_keybind_released(ta_keybind *keybind);
bool ta_keybind_triggered(ta_keybind *keybind);