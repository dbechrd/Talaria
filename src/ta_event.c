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

static ta_event_queue queue;

void ta_event_push(ta_event *event)
{
    u32 cap = dlb_vec_cap(queue.buffer);
    if (queue.count == cap) {
        bool has_items = cap > 0;
        cap = MAX(16, cap * 2);
        dlb_vec_reserve(queue.buffer, cap);
        if (has_items) {
            // Before resize: [D, A, B, C]
            // After resize : [-, A, B, C, D, -, -, -]
            if (queue.head > 0) {
                int bytes = queue.head * sizeof(queue.buffer[0]);
                dlb_memcpy(&queue.buffer[queue.count],
                    queue.buffer, bytes);
#if _DEBUG
                dlb_memset(queue.buffer, 0, bytes);
#endif
            }
        }
    }
    int next = (queue.head + queue.count) % cap;
    // TODO(perf): This copy could be avoided by allocating events from a pool
    // and passing the index to this method.
    queue.buffer[next] = *event;
    queue.count++;
}

bool ta_event_pop(ta_event *event)
{
    if (queue.count) {
        *event = queue.buffer[queue.head];
        queue.head = (queue.head + 1) % dlb_vec_cap(queue.buffer);
        queue.count--;
        return true;
    } else {
        return false;
    }
}

bool ta_event_peek(ta_event *event)
{
    if (queue.count) {
        *event = queue.buffer[queue.head];
        return true;
    } else {
        return false;
    }
}

// Convert SDL button code to custom scancode for mouse "keys"
static inline s32 mouse_button_scancode(u8 sdl_button)
{
    s32 scancode = 0;
    switch (sdl_button) {
        case SDL_BUTTON_LEFT: {
            scancode = SDL_SCANCODE_MOUSE_LEFT;
            break;
        } case SDL_BUTTON_MIDDLE: {
            scancode = SDL_SCANCODE_MOUSE_MIDDLE;
            break;
        } case SDL_BUTTON_RIGHT: {
            scancode = SDL_SCANCODE_MOUSE_RIGHT;
            break;
        } case SDL_BUTTON_X1: {
            scancode = SDL_SCANCODE_MOUSE_X1;
            break;
        } case SDL_BUTTON_X2: {
            scancode = SDL_SCANCODE_MOUSE_X2;
            break;
        }
    }
    return scancode;
}

#if 0
int
TA_SDL_WaitEventTimeout(SDL_Event * event, int timeout)
{
    Uint32 expiration = 0;

    ta_log_write(&tg_debug_log, SRC_EVENT, "   WaitEventTimeout - GetTicks\n");
    if (timeout > 0)
        expiration = SDL_GetTicks() + timeout;

    for (;;) {
        ta_log_write(&tg_debug_log, SRC_EVENT, "   WaitEventTimeout - PumpEvents\n");
        SDL_PumpEvents();
        ta_log_write(&tg_debug_log, SRC_EVENT, "   WaitEventTimeout - PeepEvents\n");
        switch (SDL_PeepEvents(event, 1, SDL_GETEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT)) {
            case -1:
                return 0;
            case 0:
                if (timeout == 0) {
                    /* Polling and no events, just return */
                    return 0;
                }
                if (timeout > 0 && SDL_TICKS_PASSED(SDL_GetTicks(), expiration)) {
                    /* Timeout expired and no events */
                    return 0;
                }
                ta_log_write(&tg_debug_log, SRC_EVENT, "   WaitEventTimeout - Delay\n");
                SDL_Delay(1);
                break;
            default:
                /* Has events */
                return 1;
        }
    }
}

int
TA_SDL_PollEvent(SDL_Event * event)
{
    return TA_SDL_WaitEventTimeout(event, 0);
}
#endif

static void event_sdl_poll()
{
    SDL_Event sdl_event;
    //while (TA_SDL_PollEvent(&sdl_event)) {
    while (SDL_PollEvent(&sdl_event)) {
        ta_log_write(&tg_debug_log, SRC_EVENT, "  SDL event type = %d\n", sdl_event.type);
        ta_event event = { 0 };
        switch (sdl_event.type) {
            case SDL_QUIT: {
                event.type = GAME_EVENT_SHUTDOWN;
                break;
            } case SDL_WINDOWEVENT: {
                switch (sdl_event.window.event) {
                    case SDL_WINDOWEVENT_RESIZED: {
                        event.type = WINDOW_EVENT_RESIZE;
                        event.data.window_resize.width = sdl_event.window.data1;
                        event.data.window_resize.height = sdl_event.window.data2;
                        break;
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
                s32 scancode = mouse_button_scancode(sdl_event.button.button);
                if (scancode) {
                    event.type = INPUT_EVENT_KEY_PRESS;
                    event.data.key_press.scancode = scancode;
                }
                break;
            } case SDL_MOUSEBUTTONUP: {
                s32 scancode = mouse_button_scancode(sdl_event.button.button);
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
            }
        }
        if (event.type) {
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
        //ta_log_write(&tg_debug_log, SRC_EVENT, "   window event...\n");
        ta_window_event(tg_window, &event);
        if (event.handled) continue;

        ta_log_write(&tg_debug_log, SRC_EVENT, "   editor event...\n");
        ta_editor_event(&event);
        if (event.handled) continue;

        ta_log_write(&tg_debug_log, SRC_EVENT, "   game event...\n");
        ta_game_event(&event);
        if (event.handled) continue;
    }
}