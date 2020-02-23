#include "ta_key.h"
#include "ta_keybind.h"
#include "ta_event.h"
#include "ta_timer.h"

static ta_key_state keys[TA_KEY_COUNT];

bool ta_key_down(ta_key key)
{
    bool down = keys[key].down;
    return down;
}

bool ta_key_up(ta_key key)
{
    bool up = !keys[key].down;
    return up;
}

bool ta_key_pressed(ta_key key)
{
    bool pressed = keys[key].down && keys[key].changed;
    return pressed;
}

bool ta_key_released(ta_key key)
{
    bool released = !keys[key].down && keys[key].changed;
    return released;
}

void ta_key_reset_changed()
{
    for (int i = 0; i < TA_KEY_COUNT; ++i) {
        keys[i].changed = false;
    }
}

bool ta_key_available(const ta_keybind *keybind, ta_key key)
{
    bool available = !keys[key].lock || keys[key].lock == keybind;
    return available;
}

void ta_key_lock(const ta_keybind *keybind, ta_key key)
{
    DLB_ASSERT(!keys[key].lock);
    keys[key].lock = keybind;
}

void ta_key_unlock(const ta_keybind *keybind, ta_key key)
{
    DLB_ASSERT(!keys[key].lock || keys[key].lock == keybind);
    keys[key].lock = 0;
}

void ta_key_unlock_all()
{
    for (int i = 0; i < TA_KEY_COUNT; ++i) {
        keys[i].lock = 0;
    }
}

void ta_key_event(ta_event *event)
{
    switch (event->type) {
        case INPUT_EVENT_KEY_PRESS:
        case INPUT_EVENT_KEY_RELEASE: {
            u8 down = (event->type == INPUT_EVENT_KEY_PRESS);
            keys[event->data.key.key].down = down;
            keys[event->data.key.key].changed = true;
            keys[event->data.key.key].last_change_ms = ta_timer_elapsed_ms();
            break;
        }
    }
}