#include "ta_event.h"
#include "ta_keybind.h"
#include "ta_log.h"
#include "ta_mouse.h"
#include "ta_editor.h"
#include "ta_window.h"
#include "ta_camera.h"
#include "ta_game.h"
#include "ta_scene.h"
#include "dlb/dlb_vector.h"

typedef struct ta_event_queue {
    u32 head;  // oldest item
    u32 count;
    ta_event *buffer;
} ta_event_queue;

static ta_event_queue event_queue;

const char *event_type_str(ta_event_type type)
{
    switch (type) {
        // Window events
        case WINDOW_EVENT_RESIZE:             return "WINDOW_EVENT_RESIZE";
        // Input events
        case INPUT_EVENT_MOUSE_MOVE:          return "INPUT_EVENT_MOUSE_MOVE";
        case INPUT_EVENT_MOUSE_SCROLL:        return "INPUT_EVENT_MOUSE_SCROLL";
        case INPUT_EVENT_KEY_PRESS:           return "INPUT_EVENT_KEY_PRESS";
        case INPUT_EVENT_KEY_REPEAT:          return "INPUT_EVENT_KEY_REPEAT";
        case INPUT_EVENT_KEY_RELEASE:         return "INPUT_EVENT_KEY_RELEASE";
        case INPUT_EVENT_TEXT_INPUT:          return "INPUT_EVENT_TEXT_INPUT";
        // Game events
        case GAME_EVENT_SHUTDOWN:             return "GAME_EVENT_SHUTDOWN";
        case GAME_EVENT_CAMERA_ROTATE:        return "GAME_EVENT_CAMERA_ROTATE";
        case GAME_EVENT_BUTTON_ACTIVATED:     return "GAME_EVENT_BUTTON_ACTIVATED";
        case GAME_EVENT_BUTTON_DEACTIVATED:   return "GAME_EVENT_BUTTON_DEACTIVATED";
        case GAME_EVENT_BUTTON_STATE_CHANGED: return "GAME_EVENT_BUTTON_STATE_CHANGED";
        default:
            DLB_ASSERT(!"Unknown event type");
            return 0;
    }
}

void ta_event_push(ta_event *event)
{
    size_t cap = dlb_vec_cap(event_queue.buffer);
    if (event_queue.count == cap) {
        bool has_items = cap > 0;
        cap = MAX(16, cap * 2);
        dlb_vec_reserve(event_queue.buffer, cap);
        if (has_items) {
            // Before resize: [D, A, B, C]
            // After resize : [-, A, B, C, D, -, -, -]
            if (event_queue.head > 0) {
                int bytes = event_queue.head * sizeof(event_queue.buffer[0]);
                dlb_memcpy(&event_queue.buffer[event_queue.count],
                    event_queue.buffer, bytes);
#if _DEBUG
                dlb_memset(event_queue.buffer, 0, bytes);
#endif
            }
        }
    }
    int next = (event_queue.head + event_queue.count) % cap;
    // TODO(perf): This copy could be avoided by allocating events from a pool
    // and passing the index to this method.
    event_queue.buffer[next] = *event;
    event_queue.count++;
}
bool ta_event_pop(ta_event *event)
{
    if (event_queue.count) {
        *event = event_queue.buffer[event_queue.head];
        event_queue.head = (event_queue.head + 1) % dlb_vec_cap(event_queue.buffer);
        event_queue.count--;
        return true;
    } else {
        return false;
    }
}
bool ta_event_peek(ta_event *event)
{
    if (event_queue.count) {
        *event = event_queue.buffer[event_queue.head];
        return true;
    } else {
        return false;
    }
}
void ta_event_events()
{
    ta_mouse_reset_relative();
    ta_key_reset_changed();

    ta_log_write(&tg_debug_log, SRC_EVENT, "  glfwPollEvents...\n");
    glfwPollEvents();

    ta_log_write(&tg_debug_log, SRC_EVENT, "  updating keybinds...\n");
    ta_game_update_keybinds();

    ta_log_write(&tg_debug_log, SRC_EVENT, "  event pop loop...\n");
    ta_event event;
    while (ta_event_pop(&event)) {
        ta_log_write(&tg_debug_log, SRC_EVENT, "  event type = %s\n", event_type_str(event.type));

        ta_log_write(&tg_debug_log, SRC_EVENT, "   editor event...\n");
        ta_editor_textbox_event(&event);
        if (event.handled) continue;

        ta_log_write(&tg_debug_log, SRC_EVENT, "   game event...\n");
        ta_game_event(&event);
        if (event.handled) continue;
    }
}