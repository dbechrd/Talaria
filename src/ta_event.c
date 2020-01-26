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
#include "SDL/SDL.h"

typedef struct ta_event_queue {
    u32 head;  // oldest item
    u32 count;
    ta_event *buffer;
} ta_event_queue;

static ta_event_queue event_queue;

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
static void event_sdl_poll()
{
    // Convert SDL button code to custom scancode for mouse "keys"
    static int mouse_scancodes[] = {
        [SDL_BUTTON_LEFT]   = SDL_SCANCODE_MOUSE_LEFT,
        [SDL_BUTTON_MIDDLE] = SDL_SCANCODE_MOUSE_MIDDLE,
        [SDL_BUTTON_RIGHT]  = SDL_SCANCODE_MOUSE_RIGHT,
        [SDL_BUTTON_X1]     = SDL_SCANCODE_MOUSE_X1,
        [SDL_BUTTON_X2]     = SDL_SCANCODE_MOUSE_X2,
    };

    SDL_Event sdl_event;
    //while (TA_SDL_PollEvent(&sdl_event)) {
    while (SDL_PollEvent(&sdl_event)) {
        ta_log_write(&tg_debug_log, SRC_EVENT, "  SDL event type = %d\n", sdl_event.type);
        bool handled = true;
        ta_event event = { 0 };
        switch (sdl_event.type) {
            case SDL_QUIT: {
                event.type = GAME_EVENT_SHUTDOWN;
                break;
            } case SDL_WINDOWEVENT: {
                switch (sdl_event.window.event) {
                    case SDL_WINDOWEVENT_RESIZED: {        /**< Window has been resized to data1xdata2 */
                        event.type = WINDOW_EVENT_RESIZE;
                        event.data.window_resize.width = sdl_event.window.data1;
                        event.data.window_resize.height = sdl_event.window.data2;
                        break;
                    } case SDL_WINDOWEVENT_SHOWN: {         /* Window has been shown */
                    } case SDL_WINDOWEVENT_MOVED: {         /* Window has been moved to data1, data2 */
                    } case SDL_WINDOWEVENT_SIZE_CHANGED: {  /* The window size has changed, either as a result of an API call or through the system or user changing the window size. */
                    } case SDL_WINDOWEVENT_MINIMIZED: {     /* Window has been minimized */
                    } case SDL_WINDOWEVENT_MAXIMIZED: {     /* Window has been maximized */
                    } case SDL_WINDOWEVENT_RESTORED: {      /* Window has been restored to normal size and position */
                    } case SDL_WINDOWEVENT_ENTER: {         /* Window has gained mouse focus */
                    } case SDL_WINDOWEVENT_LEAVE: {         /* Window has lost mouse focus */
                    } case SDL_WINDOWEVENT_FOCUS_GAINED: {  /* Window has gained keyboard focus */
                    } case SDL_WINDOWEVENT_FOCUS_LOST: {    /* Window has lost keyboard focus */
                    } case SDL_WINDOWEVENT_CLOSE: {         /* The window manager requests that the window be closed */
                    } case SDL_WINDOWEVENT_TAKE_FOCUS: {    /* Window is being offered a focus (should SetWindowInputFocus() on itself or a subwindow, or ignore) */
                    } case SDL_WINDOWEVENT_HIT_TEST: {      /* Window had a hit test that wasn't SDL_HITTEST_NORMAL. */
                    } default: {
                        handled = false;
                    }
                }
                break;
            } case SDL_KEYDOWN: {
                event.type = INPUT_EVENT_KEY_PRESS;
                event.data.key_press.scancode = sdl_event.key.keysym.scancode;
                break;
            } case SDL_KEYUP: {
                event.type = INPUT_EVENT_KEY_RELEASE;
                event.data.key_press.scancode = sdl_event.key.keysym.scancode;
                break;
            } case SDL_TEXTINPUT: {
                event.type = INPUT_EVENT_TEXT_INPUT;
                event.data.text_input.chr = sdl_event.text.text[0];
                DLB_ASSERT(!sdl_event.text.text[1]);  // Unicode?
                break;
            } case SDL_MOUSEMOTION: {
                event.type = INPUT_EVENT_MOUSE_MOVE;
                event.data.mouse_move.x  = sdl_event.motion.x;
                event.data.mouse_move.y  = sdl_event.motion.y;
                event.data.mouse_move.dx = sdl_event.motion.xrel;
                event.data.mouse_move.dy = sdl_event.motion.yrel;
                break;
            } case SDL_MOUSEBUTTONDOWN: {
                s32 scancode = mouse_scancodes[sdl_event.button.button];
                if (scancode) {
                    event.type = INPUT_EVENT_KEY_PRESS;
                    event.data.key_press.scancode = scancode;
                }
                break;
            } case SDL_MOUSEBUTTONUP: {
                s32 scancode = mouse_scancodes[sdl_event.button.button];
                if (scancode) {
                    event.type = INPUT_EVENT_KEY_RELEASE;
                    event.data.key_press.scancode = scancode;
                }
                break;
            } case SDL_MOUSEWHEEL: {
                event.type = INPUT_EVENT_MOUSE_SCROLL;
                event.data.mouse_scroll.x = sdl_event.wheel.x;
                event.data.mouse_scroll.y = sdl_event.wheel.y;
                event.data.mouse_scroll.flipped = (u8)sdl_event.wheel.direction;
                break;
            } case SDL_DROPBEGIN: {
                printf("dropbegin] file: %s\n", sdl_event.drop.file);
                handled = false;
                break;
            } case SDL_DROPFILE: {
                // TODO: Something useful with dropped files (maybe check mouse
                // position to see where it was dropped?)
                printf("dropfile] file: %s\n", sdl_event.drop.file);
                handled = false;
                break;
            } case SDL_DROPTEXT: {
                printf("droptext] file: %s\n", sdl_event.drop.file);
                handled = false;
                break;
            } case SDL_DROPCOMPLETE: {
                printf("dropcomplete] file: %s\n", sdl_event.drop.file);
                handled = false;
                break;
            } default: {
                handled = false;
            }
        }
        if (handled) {
            ta_log_write(&tg_debug_log, SRC_EVENT, "  TA event type = %d\n", event.type);
            ta_event_push(&event);
            ta_key_event(&event);
            ta_mouse_event(&event);
        }
    }
}
void ta_event_events()
{
    ta_log_write(&tg_debug_log, SRC_EVENT, "  mouse reset relative...\n");
    ta_mouse_reset_relative();
    ta_log_write(&tg_debug_log, SRC_EVENT, "  key reset changed...\n");
    ta_key_reset_changed();
    ta_log_write(&tg_debug_log, SRC_EVENT, "  SDL poll...\n");
    event_sdl_poll();
    ta_log_write(&tg_debug_log, SRC_EVENT, "  SDL poll end\n");

    if (ta_game_state_current() == TA_GAME_STATE_EDITOR) {
        ta_log_write(&tg_debug_log, SRC_EVENT, "  editor hotkeys...\n");
        ta_editor_hotkeys();
    } else {
        ta_log_write(&tg_debug_log, SRC_EVENT, "  game hotkeys...\n");
        ta_game_hotkeys();
    }

    ta_log_write(&tg_debug_log, SRC_EVENT, "  event pop loop...\n");
    ta_event event;
    while (ta_event_pop(&event)) {
        if (ta_game_state_current() == TA_GAME_STATE_EDITOR) {
            ta_log_write(&tg_debug_log, SRC_EVENT, "   editor event...\n");
            ta_editor_event(&event);
            if (event.handled) continue;
        }

        ta_log_write(&tg_debug_log, SRC_EVENT, "   window event...\n");
        ta_window_event(tg_window, &event);
        if (event.handled) continue;

        ta_log_write(&tg_debug_log, SRC_EVENT, "   game event...\n");
        ta_game_event(&event);
        if (event.handled) continue;
    }
}