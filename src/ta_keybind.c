#include "ta_keybind.h"
#include "ta_timer.h"
#include "ta_log.h"
#include "ta_event.h"
#include "ta_key.h"
#include "dlb/dlb_vector.h"
#include "SDL/SDL.h"

void ta_keybind_bind1(ta_keybind **keybinds, ta_event_type event_type,
    u32 triggers, ta_key key1)
{
    DLB_ASSERT(key1 < TA_SCANCODE_COUNT);

    ta_keybind *bind = dlb_vec_alloc(*keybinds);
    bind->event_type = event_type;
    bind->triggers = triggers;
    bind->keys[0] = key1;
}

void ta_keybind_bind2(ta_keybind **keybinds, ta_event_type event_type,
    u32 triggers, ta_key key1, ta_key key2)
{
    DLB_ASSERT(key1 < TA_SCANCODE_COUNT);
    DLB_ASSERT(key2 < TA_SCANCODE_COUNT);

    ta_keybind *bind = dlb_vec_alloc(*keybinds);
    bind->event_type = event_type;
    bind->triggers = triggers;
    bind->keys[0] = key1;
    bind->keys[1] = key2;
}

void ta_keybind_bind3(ta_keybind **keybinds, ta_event_type event_type,
    u32 triggers, ta_key key1, ta_key key2, ta_key key3)
{
    DLB_ASSERT(key1 < TA_SCANCODE_COUNT);
    DLB_ASSERT(key2 < TA_SCANCODE_COUNT);
    DLB_ASSERT(key3 < TA_SCANCODE_COUNT);

    ta_keybind *bind = dlb_vec_alloc(*keybinds);
    bind->event_type = event_type;
    bind->triggers = triggers;
    bind->keys[0] = key1;
    bind->keys[1] = key2;
    bind->keys[2] = key3;
}

static bool ta_keybind_down(ta_keybind *keybind)
{
    return keybind->key_state.down;
}

static bool ta_keybind_pressed(ta_keybind *keybind)
{
    return keybind->key_state.down && keybind->key_state.changed;
}

static bool ta_keybind_released(ta_keybind *keybind)
{
    return !keybind->key_state.down && keybind->key_state.changed;
}

static bool ta_keybind_triggered(ta_keybind *keybind)
{
    bool triggered =
        (!keybind->triggers) ||
        ((keybind->triggers & TA_KEYBIND_HOLD) && ta_keybind_down(keybind)) ||
        ((keybind->triggers & TA_KEYBIND_PRESS) && ta_keybind_pressed(keybind)) ||
        ((keybind->triggers & TA_KEYBIND_RELEASE) && ta_keybind_released(keybind));
    return triggered;
}

static void keybind_update(ta_keybind *keybind)
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

void ta_keybind_trigger(ta_keybind *keybinds)
{
    dlb_vec_each(ta_keybind *, bind, keybinds) {
        keybind_update(bind);
        if (ta_keybind_triggered(bind)) {
            ta_event event = { 0 };
            event.type = bind->event_type;
            ta_event_push(&event);
        }
    }
}