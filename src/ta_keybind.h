 #pragma once
#include "ta_key.h"

enum ta_game_state;
struct ta_event;

// Keybind trigger flags
typedef enum ta_keybind_trigger {
    TA_KEYBIND_PRESS   = 1 << 0,  // once when key pressed
    TA_KEYBIND_HOLD    = 1 << 1,  // while key is held down
    TA_KEYBIND_RELEASE = 1 << 2,  // once when key released
} ta_keybind_trigger;

typedef struct ta_keybind {
    ta_command command;  // command to run when keybind is triggered
    u32    game_states;       // [flags] only trigger this keybind while in one of these game states
    u32    triggers;          // trigger flag bitmap (PRESS, HOLD, RELEASE); when to trigger keybind
    ta_key keys[3];           // which keys need to be pressed simultaneously to trigger keybind
    bool   triggered;         // keybind is currently being triggered
    double last_change_ms;    // time of last state change in milliseconds
} ta_keybind;

void ta_keybind_init1       (ta_keybind **keybinds, ta_command command, u32 game_states, u32 triggers, ta_key key1);
void ta_keybind_init2       (ta_keybind **keybinds, ta_command command, u32 game_states, u32 triggers, ta_key key1, ta_key key2);
void ta_keybind_init3       (ta_keybind **keybinds, ta_command command, u32 game_states, u32 triggers, ta_key key1, ta_key key2, ta_key key3);
bool ta_keybind_triggered   (const ta_keybind *keybind);
void ta_keybind_update      (ta_keybind *keybind, enum ta_game_state game_state);
