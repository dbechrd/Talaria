#include "ta_event.h"
#include "ta_keybind.h"
#include "ta_log.h"
#include "ta_mouse.h"
#include "ta_editor.h"
#include "ta_window.h"
#include "ta_camera.h"
#include "ta_game.h"
#include "ta_text_entry.h"
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

static void event_sdl_poll()
{
    SDL_Event sdl_event;
    while (SDL_PollEvent(&sdl_event)) {
        ta_event event = { 0 };
        switch (sdl_event.type) {
            case SDL_QUIT: {
                event.type = TA_EVENT_GAME_SHUTDOWN;
                break;
            } case SDL_WINDOWEVENT: {
                switch (sdl_event.window.event) {
                    case SDL_WINDOWEVENT_RESIZED: {
                        event.type = TA_EVENT_WINDOW_RESIZE;
                        event.data.window_resize.width = sdl_event.window.data1;
                        event.data.window_resize.height = sdl_event.window.data2;
                        break;
                    }
                }
                break;
            } case SDL_KEYDOWN: {
                event.type = TA_EVENT_KEY_PRESS;
                event.data.key_press.scancode = sdl_event.key.keysym.scancode;
                break;
            } case SDL_KEYUP: {
                event.type = TA_EVENT_KEY_RELEASE;
                event.data.key_press.scancode = sdl_event.key.keysym.scancode;
                break;
            } case SDL_TEXTINPUT: {
                event.type = TA_EVENT_TEXT_INPUT;
                event.data.text_input.chr = sdl_event.text.text[0];
                DLB_ASSERT(!sdl_event.text.text[1]);  // Unicode?
                break;
            } case SDL_MOUSEMOTION: {
                event.type = TA_EVENT_MOUSE_MOVE;
                event.data.mouse_move.x  = sdl_event.motion.x;
                event.data.mouse_move.y  = sdl_event.motion.y;
                event.data.mouse_move.dx = sdl_event.motion.xrel;
                event.data.mouse_move.dy = sdl_event.motion.yrel;
                break;
            } case SDL_MOUSEBUTTONDOWN: {
                s32 scancode = mouse_button_scancode(sdl_event.button.button);
                if (scancode) {
                    event.type = TA_EVENT_KEY_PRESS;
                    event.data.key_press.scancode = scancode;
                }
                break;
            } case SDL_MOUSEBUTTONUP: {
                s32 scancode = mouse_button_scancode(sdl_event.button.button);
                if (scancode) {
                    event.type = TA_EVENT_KEY_RELEASE;
                    event.data.key_press.scancode = scancode;
                }
                break;
            } case SDL_MOUSEWHEEL: {
                event.type = TA_EVENT_MOUSE_SCROLL;
                event.data.mouse_scroll.x = sdl_event.wheel.x;
                event.data.mouse_scroll.y = sdl_event.wheel.y;
                event.data.mouse_scroll.flipped = (u8)sdl_event.wheel.direction;
                break;
            }
        }
        if (event.type) {
            ta_event_push(&event);
            ta_key_event(&event);
            ta_mouse_event(&event);
        }
    }
}

void ta_event_events()
{
    ta_mouse_reset_relative();
    ta_key_reset_changed();
    event_sdl_poll();

    if (tg_game.state == TA_GAME_STATE_EDITOR) {
        ta_game_state blah = tg_game.state;
        UNUSED(blah);
        ta_editor_hotkeys();
    } else {
        ta_game_hotkeys(&tg_game);
    }

    ta_event event;
    while (ta_event_pop(&event)) {
        ta_window_event(tg_game.window, &event);
        if (event.handled) continue;

        ta_editor_event(&event);
        if (event.handled) continue;

        ta_game_event(&tg_game, &event);
        if (event.handled) continue;

        if (ta_mouse_captured()) {
            ta_camera *camera = ta_scene_component(tg_game.scene,
                RES_COMP_CAMERA, tg_game.e_active_camera);
            ta_camera_event(camera, &event);
        }
    }
}