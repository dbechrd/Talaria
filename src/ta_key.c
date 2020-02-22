#include "ta_key.h"
#include "ta_event.h"
#include "ta_timer.h"

static ta_key_state keys[TA_KEY_COUNT];

bool ta_key_down(ta_key key)
{
    bool down = keys[key].down && !keys[key].handled;
    return down;
}

bool ta_key_up(ta_key key)
{
    bool up = !keys[key].down;
    return up;
}

bool ta_key_pressed(ta_key key)
{
    bool pressed = keys[key].down && keys[key].changed && !keys[key].handled;
    return pressed;
}

bool ta_key_released(ta_key key)
{
    bool released = !keys[key].down && keys[key].changed && !keys[key].handled;
    return released;
}

void ta_key_mark_handled(ta_key key)
{
    keys[key].handled = true;
}

void ta_key_clear_all()
{
    for (int i = 0; i < TA_KEY_COUNT; ++i) {
        keys[i].changed = false;
    }
}

void ta_key_event(ta_event *event)
{
    switch (event->type) {
        case INPUT_EVENT_KEY_PRESS:
        case INPUT_EVENT_KEY_RELEASE: {
            keys[event->data.key.key].down = (event->type == INPUT_EVENT_KEY_PRESS);
            keys[event->data.key.key].handled = false;
            keys[event->data.key.key].changed = true;
            keys[event->data.key.key].last_change_ms = ta_timer_elapsed_ms();
            break;
        }
    }
}