#include "ta_key.h"
#include "ta_event.h"
#include "ta_button_state.h"
#include "ta_timer.h"

static ta_button_state keys[TA_KEY_COUNT];

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

void ta_key_event(ta_event *event)
{
    switch (event->type) {
        case INPUT_EVENT_KEY_PRESS: case INPUT_EVENT_KEY_RELEASE: {
            u8 down = (event->type == INPUT_EVENT_KEY_PRESS);
            u8 changed = keys[event->data.key.scancode].down != down;
            keys[event->data.key.scancode].down = down;
            keys[event->data.key.scancode].changed = changed;
            keys[event->data.key.scancode].last_change_ms = ta_timer_elapsed_ms();
            break;
        }
    }
}