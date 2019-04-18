#include "ta_keyboard.h"
#include "ta_timer.h"
#include "dlb_vector.h"

ta_keybind *tg_keybinds[TA_STATE_COUNT];

void ta_keybind_bind1(ta_game_state state_type, ta_event_type event_type,
    u32 triggers, ta_key key1)
{
    ta_keybind *bind = dlb_vec_alloc(tg_keybinds[state_type]);
    bind->event_type = event_type;
    bind->triggers = triggers;
    bind->keys[0] = key1;
}

void ta_keybind_bind2(ta_game_state state_type, ta_event_type event_type,
    u32 triggers, ta_key key1, ta_key key2)
{
    ta_keybind *bind = dlb_vec_alloc(tg_keybinds[state_type]);
    bind->event_type = event_type;
    bind->triggers = triggers;
    bind->keys[0] = key1;
    bind->keys[1] = key2;
}

void ta_keybind_bind3(ta_game_state state_type, ta_event_type event_type,
    u32 triggers, ta_key key1, ta_key key2, ta_key key3)
{
    ta_keybind *bind = dlb_vec_alloc(tg_keybinds[state_type]);
    bind->event_type = event_type;
    bind->triggers = triggers;
    bind->keys[0] = key1;
    bind->keys[1] = key2;
    bind->keys[2] = key3;
}

void ta_keybind_update(ta_keybind *keybind)
{
    bool old_down = keybind->down;
    keybind->down =
        (!keybind->keys[0] || tg_key_states[keybind->keys[0]].down) &&
        (!keybind->keys[1] || tg_key_states[keybind->keys[1]].down) &&
        (!keybind->keys[2] || tg_key_states[keybind->keys[2]].down);
    keybind->changed = keybind->down != old_down;
    if (keybind->changed) {
        keybind->last_change_ms = ta_timer_elapsed_ms();
    }
}

bool ta_keybind_down(ta_keybind *keybind)
{
    return keybind->down;
}

bool ta_keybind_pressed(ta_keybind *keybind)
{
    return keybind->down && keybind->changed;
}

bool ta_keybind_released(ta_keybind *keybind)
{
    return !keybind->down && keybind->changed;
}

bool ta_keybind_triggered(ta_keybind *keybind)
{
    bool triggered =
        (!keybind->triggers) ||
        ((keybind->triggers & TA_KEYBIND_TRIGGER_DOWN) && ta_keybind_down(keybind)) ||
        ((keybind->triggers & TA_KEYBIND_TRIGGER_PRESSED) && ta_keybind_pressed(keybind)) ||
        ((keybind->triggers & TA_KEYBIND_TRIGGER_RELEASED) && ta_keybind_released(keybind));
    return triggered;
}
