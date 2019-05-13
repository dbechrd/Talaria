#include "ta_keyboard.h"
#include "ta_mouse.h"
#include "ta_timer.h"
#include "ta_log.h"
#include "dlb_vector.h"
#include <string.h>

static u8 keys[TA_SCANCODE_COUNT];
static ta_keybind *keybinds[TA_STATE_COUNT];

void ta_keyboard_init()
{
    ta_log_write(tg_debug_log, "[Keyboard] Initializing keyboard\n");
    ta_log_write(tg_debug_log, "[Keyboard] Initializing key binds\n");
    // TODO: Read keybinds from file
    //dlb_vec_reserve(tg_keybinds, 16);
    ta_keybind_bind1(TA_STATE_PLAY, TA_EVENT_GLOBAL_QUIT,              TA_KEY_PRESS, SDL_SCANCODE_ESCAPE);
    ta_keybind_bind1(TA_STATE_PLAY, TA_EVENT_GLOBAL_TOGGLE_MOUSE_LOCK, TA_KEY_PRESS, SDL_SCANCODE_M);
    ta_keybind_bind1(TA_STATE_PLAY, TA_EVENT_GLOBAL_TOGGLE_WIREFRAME,  TA_KEY_PRESS, SDL_SCANCODE_Z);
    ta_keybind_bind1(TA_STATE_PLAY, TA_EVENT_GLOBAL_TOGGLE_DEBUG_1,    TA_KEY_PRESS, SDL_SCANCODE_1);
    ta_keybind_bind1(TA_STATE_PLAY, TA_EVENT_GLOBAL_TOGGLE_DEBUG_2,    TA_KEY_PRESS, SDL_SCANCODE_2);
    ta_keybind_bind1(TA_STATE_PLAY, TA_EVENT_CAMERA_MOVE_FORWARD,      TA_KEY_HOLD, SDL_SCANCODE_W);
    ta_keybind_bind1(TA_STATE_PLAY, TA_EVENT_CAMERA_MOVE_BACKWARD,     TA_KEY_HOLD, SDL_SCANCODE_S);
    ta_keybind_bind1(TA_STATE_PLAY, TA_EVENT_CAMERA_MOVE_RIGHT,        TA_KEY_HOLD, SDL_SCANCODE_D);
    ta_keybind_bind1(TA_STATE_PLAY, TA_EVENT_CAMERA_MOVE_LEFT,         TA_KEY_HOLD, SDL_SCANCODE_A);
    ta_keybind_bind1(TA_STATE_PLAY, TA_EVENT_CAMERA_MOVE_UP,           TA_KEY_HOLD, SDL_SCANCODE_SPACE);
    ta_keybind_bind1(TA_STATE_PLAY, TA_EVENT_CAMERA_MOVE_DOWN,         TA_KEY_HOLD, SDL_SCANCODE_LSHIFT);
    ta_log_write(tg_debug_log, "[Keyboard] Keyboard initialized\n");
}

void ta_keybind_bind1(ta_game_state state_type, ta_event_type event_type,
    u32 triggers, ta_key key1)
{
    ta_keybind *bind = dlb_vec_alloc(keybinds[state_type]);
    bind->event_type = event_type;
    bind->triggers = triggers;
    bind->keys[0] = key1;
}

void ta_keybind_bind2(ta_game_state state_type, ta_event_type event_type,
    u32 triggers, ta_key key1, ta_key key2)
{
    ta_keybind *bind = dlb_vec_alloc(keybinds[state_type]);
    bind->event_type = event_type;
    bind->triggers = triggers;
    bind->keys[0] = key1;
    bind->keys[1] = key2;
}

void ta_keybind_bind3(ta_game_state state_type, ta_event_type event_type,
    u32 triggers, ta_key key1, ta_key key2, ta_key key3)
{
    ta_keybind *bind = dlb_vec_alloc(keybinds[state_type]);
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
        (!keybind->keys[0] || keys[keybind->keys[0]]) &&
        (!keybind->keys[1] || keys[keybind->keys[1]]) &&
        (!keybind->keys[2] || keys[keybind->keys[2]]);
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
        ((keybind->triggers & TA_KEY_HOLD) && ta_keybind_down(keybind)) ||
        ((keybind->triggers & TA_KEY_PRESS) && ta_keybind_pressed(keybind)) ||
        ((keybind->triggers & TA_KEY_RELEASE) && ta_keybind_released(keybind));
    return triggered;
}

void ta_keyboard_update()
{
    memcpy(keys, SDL_GetKeyboardState(0), SDL_NUM_SCANCODES);

    // NOTE: Assumes mouse was updated first
    keys[TA_SCANCODE_MOUSE_LEFT] = tg_mouse.left;
    keys[TA_SCANCODE_MOUSE_MIDDLE] = tg_mouse.middle;
    keys[TA_SCANCODE_MOUSE_RIGHT] = tg_mouse.right;

    for (ta_keybind *bind = keybinds[tg_game.state];
        bind != dlb_vec_end(keybinds[tg_game.state]);
        bind++)
    {
        ta_keybind_update(bind);
        if (ta_keybind_triggered(bind)) {
            ta_event event = { 0 };
            event.type = bind->event_type;
            ta_event_push(&event);
        }
    }
}