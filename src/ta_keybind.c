#include "ta_keybind.h"
#include "ta_timer.h"
#include "ta_log.h"
#include "ta_event.h"
#include "ta_key.h"
#include "dlb/dlb_vector.h"

void ta_keybind_init1(ta_keybind **keybinds, ta_command command, u32 game_states, u32 triggers, ta_key key1)
{
    DLB_ASSERT(triggers);
    DLB_ASSERT(key1 < TA_KEY_COUNT);
    ta_keybind *keybind = dlb_vec_alloc(*keybinds);
    keybind->command = command;
    keybind->game_states = game_states;
    keybind->triggers = triggers;
    keybind->keys[0] = key1;
}

void ta_keybind_init2(ta_keybind **keybinds, ta_command command, u32 game_states, u32 triggers, ta_key key1, ta_key key2)
{
    DLB_ASSERT(triggers);
    DLB_ASSERT(key1 < TA_KEY_COUNT);
    DLB_ASSERT(key2 < TA_KEY_COUNT);
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
    DLB_ASSERT(key1 < TA_KEY_COUNT);
    DLB_ASSERT(key2 < TA_KEY_COUNT);
    DLB_ASSERT(key3 < TA_KEY_COUNT);
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
    if (!(keybind->keys[0] || keybind->keys[1] || keybind->keys[2])) {
        return;
    }

    bool keys_down = (
        (!keybind->keys[0] || (ta_key_available(keybind, keybind->keys[0]) && ta_key_down(keybind->keys[0]))) &&
        (!keybind->keys[1] || (ta_key_available(keybind, keybind->keys[1]) && ta_key_down(keybind->keys[1]))) &&
        (!keybind->keys[2] || (ta_key_available(keybind, keybind->keys[2]) && ta_key_down(keybind->keys[2])))
    );
    bool keys_pressed = keys_down && (
        (!keybind->keys[0] || ta_key_pressed(keybind->keys[0])) ||
        (!keybind->keys[1] || ta_key_pressed(keybind->keys[1])) ||
        (!keybind->keys[2] || ta_key_pressed(keybind->keys[2]))
    );
    bool keys_released = keybind->triggered && !keys_down && (
        (!keybind->keys[0] || ta_key_released(keybind->keys[0])) ||
        (!keybind->keys[1] || ta_key_released(keybind->keys[1])) ||
        (!keybind->keys[2] || ta_key_released(keybind->keys[2]))
    );

    if (keys_down && !ta_key_available(keybind, keybind->keys[0])) {
        printf("\n[%p][%d][%llu] REJECTED: %s", keybind, keybind->keys[0], ta_game_frame_num(), ta_command_str(keybind->command));
    }

    bool triggered = (keybind->game_states & game_state) && (
        ((keybind->triggers & TA_KEYBIND_PRESS  ) && keys_pressed ) ||
        ((keybind->triggers & TA_KEYBIND_HOLD   ) && keys_down    ) ||
        ((keybind->triggers & TA_KEYBIND_RELEASE) && keys_released)
    );

    if (triggered != keybind->triggered) {
        keybind->triggered = triggered;
        keybind->last_change_ms = ta_timer_elapsed_ms();
        if (keybind->triggered) {
            printf("\n[%p][%d][%llu]     LOCK: %s", keybind, keybind->keys[0], ta_game_frame_num(), ta_command_str(keybind->command));
            if (keybind->keys[0]) ta_key_lock(keybind, keybind->keys[0]);
            if (keybind->keys[1]) ta_key_lock(keybind, keybind->keys[1]);
            if (keybind->keys[2]) ta_key_lock(keybind, keybind->keys[2]);
        } else {
            printf("\n[%p][%d][%llu]   UNLOCK: %s", keybind, keybind->keys[0], ta_game_frame_num(), ta_command_str(keybind->command));
            if (keybind->keys[0]) ta_key_unlock(keybind, keybind->keys[0]);
            if (keybind->keys[1]) ta_key_unlock(keybind, keybind->keys[1]);
            if (keybind->keys[2]) ta_key_unlock(keybind, keybind->keys[2]);
        }
    }
}