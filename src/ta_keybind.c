#include "ta_keybind.h"
#include "ta_timer.h"
#include "ta_log.h"
#include "ta_event.h"
#include "ta_key.h"
#include "dlb/dlb_vector.h"
#include "SDL/SDL.h"

void ta_keybind_init1(ta_keybind *keybind, u32 triggers, ta_key key1)
{
    DLB_ASSERT(triggers);
    DLB_ASSERT(key1 < TA_SCANCODE_COUNT);
    keybind->triggers = triggers;
    keybind->keys[0] = key1;
}

void ta_keybind_init2(ta_keybind *keybind, u32 triggers, ta_key key1,
    ta_key key2)
{
    DLB_ASSERT(triggers);
    DLB_ASSERT(key1 < TA_SCANCODE_COUNT);
    DLB_ASSERT(key2 < TA_SCANCODE_COUNT);
    keybind->triggers = triggers;
    keybind->keys[0] = key1;
    keybind->keys[1] = key2;
}

void ta_keybind_init3(ta_keybind *keybind, u32 triggers, ta_key key1,
    ta_key key2, ta_key key3)
{
    DLB_ASSERT(triggers);
    DLB_ASSERT(key1 < TA_SCANCODE_COUNT);
    DLB_ASSERT(key2 < TA_SCANCODE_COUNT);
    DLB_ASSERT(key3 < TA_SCANCODE_COUNT);
    keybind->triggers = triggers;
    keybind->keys[0] = key1;
    keybind->keys[1] = key2;
    keybind->keys[2] = key3;
}

bool ta_keybind_down(const ta_keybind *keybind)
{
    return keybind->key_state.down;
}

bool ta_keybind_pressed(const ta_keybind *keybind)
{
    // TODO: Make "pressed" a state that lasts until end of frame, then is
    // manually cleared in case we get a DOWN and an UP event in the same frame.
    return keybind->key_state.down && keybind->key_state.changed;
}

bool ta_keybind_released(const ta_keybind *keybind)
{
    return !keybind->key_state.down && keybind->key_state.changed;
}

bool ta_keybind_triggered(const ta_keybind *keybind)
{
    bool triggered =
        ((keybind->triggers & TA_KEYBIND_HOLD) && ta_keybind_down(keybind)) ||
        ((keybind->triggers & TA_KEYBIND_PRESS) && ta_keybind_pressed(keybind)) ||
        ((keybind->triggers & TA_KEYBIND_RELEASE) && ta_keybind_released(keybind));
    return triggered;
}

// TODO: Make this happen for all keybinds immediately after SDL keyboard events
// are handled.
void ta_keybind_update(ta_keybind *keybind)
{
    bool old_down = keybind->key_state.down;
    keybind->key_state.down =
        (!keybind->keys[0] || ta_key_down(keybind->keys[0])) &&
        (!keybind->keys[1] || ta_key_down(keybind->keys[1])) &&
        (!keybind->keys[2] || ta_key_down(keybind->keys[2]));
    keybind->key_state.changed = keybind->key_state.down != old_down;
    if (keybind->key_state.changed) {
        keybind->key_state.last_change_ms = ta_timer_elapsed_ms();
    }
}