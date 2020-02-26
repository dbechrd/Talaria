#include "ta_keybind.h"
#include "ta_timer.h"
#include "ta_log.h"
#include "ta_event.h"
#include "ta_key.h"
#include "dlb/dlb_vector.h"

void ta_keybind_init1(ta_keybind **keybinds, ta_command command, u32 game_states, u32 triggers, ta_key key1)
{
    DLB_ASSERT(triggers);
    DLB_ASSERT(key1 && key1 < TA_KEY_COUNT);
    ta_keybind *keybind = dlb_vec_alloc(*keybinds);
    keybind->command = command;
    keybind->game_states = game_states;
    keybind->triggers = triggers;
    keybind->keys[0] = key1;
}

void ta_keybind_init2(ta_keybind **keybinds, ta_command command, u32 game_states, u32 triggers, ta_key key1, ta_key key2)
{
    DLB_ASSERT(triggers);
    DLB_ASSERT(key1 && key1 < TA_KEY_COUNT);
    DLB_ASSERT(key2 && key2 < TA_KEY_COUNT);
    ta_keybind *keybind = dlb_vec_alloc(*keybinds);
    keybind->command = command;
    keybind->game_states = game_states;
    keybind->triggers = triggers;
    keybind->keys[0] = key1;
    keybind->keys[1] = key2;
}

void ta_keybind_init3(ta_keybind **keybinds, ta_command command, u32 game_states, u32 triggers, ta_key key1, ta_key key2, ta_key key3)
{
    DLB_ASSERT(triggers);
    DLB_ASSERT(key1 && key1 < TA_KEY_COUNT);
    DLB_ASSERT(key2 && key2 < TA_KEY_COUNT);
    DLB_ASSERT(key3 && key3 < TA_KEY_COUNT);
    ta_keybind *keybind = dlb_vec_alloc(*keybinds);
    keybind->command = command;
    keybind->game_states = game_states;
    keybind->triggers = triggers;
    keybind->keys[0] = key1;
    keybind->keys[1] = key2;
    keybind->keys[2] = key3;
}

bool ta_keybind_triggered(const ta_keybind *keybind)
{
    return keybind->triggered;
}

void ta_keybind_update(ta_keybind *keybind, enum ta_game_state game_state)
{
    // Keybind needs at least one valid key
    DLB_ASSERT(keybind->keys[0]);

    bool keys_down =
        (!keybind->keys[0] || ta_key_down(keybind->keys[0])) &&
        (!keybind->keys[1] || ta_key_down(keybind->keys[1])) &&
        (!keybind->keys[2] || ta_key_down(keybind->keys[2]));

    bool state_match = keybind->game_states & game_state;
    bool trigger_press = state_match &&  keys_down && (keybind->triggers & TA_KEYBIND_PRESS) && (
        ta_key_pressed(keybind->keys[0]) ||
        ta_key_pressed(keybind->keys[1]) ||
        ta_key_pressed(keybind->keys[2])
    );
    bool trigger_hold = state_match &&  keys_down && (keybind->triggers & TA_KEYBIND_HOLD);
    bool trigger_release = state_match && !keys_down && (keybind->triggers & TA_KEYBIND_RELEASE) && keybind->keys_down;
    bool can_trigger = trigger_press || trigger_hold || trigger_release;

    // Only trigger if all keys are available for use (i.e. not already locked by another keybind)
    bool triggered = can_trigger && (
        ta_key_available(keybind, keybind->keys[0]) &&
        ta_key_available(keybind, keybind->keys[1]) &&
        ta_key_available(keybind, keybind->keys[2])
    );

    if (can_trigger && !triggered) {
        ta_log_write(&tg_debug_log, SRC_KEYBIND, "[%p][%d][%llu]  Rejected: %s\n", keybind, keybind->keys[0],
            ta_game_frame_num(), ta_command_str(keybind->command));
    }

    if (triggered != keybind->triggered) {
        if (triggered) {
            ta_log_write(&tg_debug_log, SRC_KEYBIND, "[%p][%d][%llu]   Locking: %s\n", keybind, keybind->keys[0],
                ta_game_frame_num(), ta_command_str(keybind->command));
            ta_key_lock(keybind, keybind->keys[0]);
            ta_key_lock(keybind, keybind->keys[1]);
            ta_key_lock(keybind, keybind->keys[2]);
        } else {
            ta_log_write(&tg_debug_log, SRC_KEYBIND, "[%p][%d][%llu] Unlocking: %s\n", keybind, keybind->keys[0],
                ta_game_frame_num(), ta_command_str(keybind->command));
            ta_key_unlock(keybind, keybind->keys[0]);
            ta_key_unlock(keybind, keybind->keys[1]);
            ta_key_unlock(keybind, keybind->keys[2]);
        }
        keybind->triggered = triggered;
        keybind->last_change_ms = ta_timer_elapsed_ms();
    }

    keybind->keys_down = keys_down;
}