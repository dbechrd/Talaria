#include "ta_keyboard.h"
#include "ta_mouse.h"
#include "ta_timer.h"
#include "ta_log.h"
#include "ta_game.h"
#include "dlb_vector.h"
#include "SDL/SDL.h"

static u8 keys[TA_SCANCODE_COUNT];
static ta_keybind *keybinds[TA_GAME_STATE_COUNT];

void ta_keyboard_init()
{
    //ta_log_write(tg_debug_log, "[Keyboard] Initializing keyboard\n");
    //ta_log_write(tg_debug_log, "[Keyboard] Keyboard initialized\n");
}

void ta_keybind_bind1(enum ta_game_state state_type, ta_event_type event_type,
    u32 triggers, ta_key key1)
{
    ta_keybind *bind = dlb_vec_alloc(keybinds[state_type]);
    bind->event_type = event_type;
    bind->triggers = triggers;
    bind->keys[0] = key1;
}

void ta_keybind_bind2(enum ta_game_state state_type, ta_event_type event_type,
    u32 triggers, ta_key key1, ta_key key2)
{
    ta_keybind *bind = dlb_vec_alloc(keybinds[state_type]);
    bind->event_type = event_type;
    bind->triggers = triggers;
    bind->keys[0] = key1;
    bind->keys[1] = key2;
}

void ta_keybind_bind3(enum ta_game_state state_type, ta_event_type event_type,
    u32 triggers, ta_key key1, ta_key key2, ta_key key3)
{
    ta_keybind *bind = dlb_vec_alloc(keybinds[state_type]);
    bind->event_type = event_type;
    bind->triggers = triggers;
    bind->keys[0] = key1;
    bind->keys[1] = key2;
    bind->keys[2] = key3;
}

bool ta_keybind_down(ta_keybind *keybind)
{
    return keybind->button_state.down;
}

bool ta_keybind_pressed(ta_keybind *keybind)
{
    return keybind->button_state.down && keybind->button_state.changed;
}

bool ta_keybind_released(ta_keybind *keybind)
{
    return !keybind->button_state.down && keybind->button_state.changed;
}

static void keybind_update(ta_keybind *keybind)
{
    bool old_down = keybind->button_state.down;
    keybind->button_state.down =
        (!keybind->keys[0] || keys[keybind->keys[0]]) &&
        (!keybind->keys[1] || keys[keybind->keys[1]]) &&
        (!keybind->keys[2] || keys[keybind->keys[2]]);
    keybind->button_state.changed = keybind->button_state.down != old_down;
    if (keybind->button_state.changed) {
        keybind->button_state.last_change_ms = ta_timer_elapsed_ms();
    }
}

bool ta_keybind_triggered(ta_keybind *keybind)
{
    bool triggered =
        (!keybind->triggers) ||
        ((keybind->triggers & TA_KEY_HOLD) && ta_keybind_down(keybind)) ||
        ((keybind->triggers & TA_KEY_PRESS) && ta_keybind_pressed(keybind)) ||
        ((keybind->triggers & TA_KEY_RELEASE) && ta_keybind_released(keybind));
    return triggered;
}

void ta_keyboard_events()
{
    dlb_memcpy(keys, SDL_GetKeyboardState(0), SDL_NUM_SCANCODES);

    // NOTE: Assumes mouse was updated first
    keys[TA_SCANCODE_MOUSE_LEFT] = (u8)tg_mouse.left.down;
    keys[TA_SCANCODE_MOUSE_MIDDLE] = (u8)tg_mouse.middle.down;
    keys[TA_SCANCODE_MOUSE_RIGHT] = (u8)tg_mouse.right.down;

    dlb_vec_each(ta_keybind *, bind, keybinds[tg_game.state]) {
        keybind_update(bind);
        if (ta_keybind_triggered(bind)) {
            ta_event event = { 0 };
            event.type = bind->event_type;
            ta_event_push(&event);
        }
    }
}