#include "ta_event.h"
#include "ta_keyboard.h"
#include "ta_log.h"
#include "ta_mouse.h"
#include "ta_game.h"
#include "ta_window.h"
#include "ta_camera.h"
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

static void event_sdl_poll()
{
    SDL_Event sdl_event;
    while (SDL_PollEvent(&sdl_event)) {
        switch (sdl_event.type) {
            case SDL_QUIT: {
                ta_event event = { 0 };
                event.type = TA_EVENT_SHUTDOWN;
                ta_event_push(&event);
                break;
            } case SDL_WINDOWEVENT: {
                switch (sdl_event.window.event) {
                    case SDL_WINDOWEVENT_RESIZED: {
                        ta_event event = { 0 };
                        event.type = TA_EVENT_WINDOW_RESIZE;
                        event.data.window_resize.width = sdl_event.window.data1;
                        event.data.window_resize.height = sdl_event.window.data2;
                        ta_event_push(&event);
                        break;
                    }
                }
                break;
            } case SDL_MOUSEWHEEL: {
                ta_event event = { 0 };
                event.type = TA_EVENT_MOUSE_SCROLL;
                event.data.mouse_scroll.x = sdl_event.wheel.x;
                event.data.mouse_scroll.y = sdl_event.wheel.y;
                event.data.mouse_scroll.flipped = (u8)sdl_event.wheel.direction;
                ta_event_push(&event);
                break;
            } case SDL_KEYDOWN: {
                text_entry_settings *textbox = tg_game.text_entry.entry;
                if (textbox) {
                    switch (sdl_event.key.keysym.scancode) {
#if 0
                        // NOTE: This doesn't work because it gets double processed
                        //       and the entire application exits.
                        case SDL_SCANCODE_ESCAPE: {
                            ta_game_state_set(textbox->prev_state);
                            textbox = 0;
                            tg_game.text_entry.filter = 0;
                            break;
                        }
#endif
                        case SDL_SCANCODE_HOME: {
                            if (textbox->gap_len) {
                                while (textbox->cursor) {
                                    textbox->cursor--;
                                    textbox->buf[textbox->cursor + textbox->gap_len] = textbox->buf[textbox->cursor];
                                    textbox->buf[textbox->cursor] = 0;
                                }
                            } else {
                                textbox->cursor = 0;
                            }
                            break;
                        }
                        case SDL_SCANCODE_END: {
                            u32 cap = dlb_vec_cap(textbox->buf);
                            if (textbox->gap_len) {
                                while (textbox->cursor + textbox->gap_len < cap) {
                                    textbox->buf[textbox->cursor] = textbox->buf[textbox->cursor + textbox->gap_len];
                                    textbox->buf[textbox->cursor + textbox->gap_len] = 0;
                                    textbox->cursor++;
                                }
                            } else {
                                textbox->cursor = cap - textbox->gap_len;
                            }
                            break;
                        }
                        case SDL_SCANCODE_BACKSPACE: {
                            if (textbox->cursor) {
                                textbox->cursor--;
                                textbox->gap_len++;
                            }
                            break;
                        } case SDL_SCANCODE_DELETE: {
                            u32 cap = dlb_vec_cap(textbox->buf);
                            if (textbox->cursor + textbox->gap_len < cap) {
                                textbox->gap_len++;
                            }
                            break;
                        } case SDL_SCANCODE_RIGHT: {
                            u32 cap = dlb_vec_cap(textbox->buf);
                            if (textbox->cursor + textbox->gap_len < cap) {
                                if (textbox->gap_len) {
                                    textbox->buf[textbox->cursor] = textbox->buf[textbox->cursor + textbox->gap_len];
                                    textbox->buf[textbox->cursor + textbox->gap_len] = 0;
                                }
                                textbox->cursor++;
                            }
                            break;
                        } case SDL_SCANCODE_LEFT: {
                            if (textbox->cursor) {
                                textbox->cursor--;
                                if (textbox->gap_len) {
                                    textbox->buf[textbox->cursor + textbox->gap_len] = textbox->buf[textbox->cursor];
                                    textbox->buf[textbox->cursor] = 0;
                                }
                            }
                            break;
                        } case SDL_SCANCODE_RETURN: {
                            if (!textbox->gap_len) {
                                u32 cap = dlb_vec_cap(textbox->buf);
                                u32 start = textbox->cursor + textbox->gap_len;
                                dlb_vec_reserve(textbox->buf, dlb_vec_cap(textbox->buf) + 1);
                                u32 new_cap = dlb_vec_cap(textbox->buf);
                                textbox->gap_len = new_cap - cap;
                                u32 len = cap - start;
                                dlb_memcpy(
                                    textbox->buf + textbox->cursor + textbox->gap_len,
                                    textbox->buf + textbox->cursor,
                                    len
                                );
                            }
                            textbox->buf[textbox->cursor] = '\n';
                            textbox->cursor++;
                            textbox->gap_len--;
                            break;
                        }
#if 0
                        } case SDL_SCANCODE_DOWN: {
                            break;
                        } case SDL_SCANCODE_UP: {
                            break;
                        } case SDL_SCANCODE_PAGEUP: {
                            SDL_StartTextInput();
                            break;
                        } case SDL_SCANCODE_PAGEDOWN: {
                            SDL_StopTextInput();
                            break;
                        }
#endif
                    }
                }
                break;
            } case SDL_TEXTINPUT: {
                text_entry_settings *textbox = tg_game.text_entry.entry;
                if (textbox) {
                    char *c = sdl_event.text.text;

                    if (tg_game.text_entry.filter && tg_game.text_entry.filter(*c)) {
                        if (!textbox->gap_len) {
                            u32 cap = dlb_vec_cap(textbox->buf);
                            u32 start = textbox->cursor + textbox->gap_len;
                            dlb_vec_reserve(textbox->buf, dlb_vec_cap(textbox->buf) + 1);
                            u32 new_cap = dlb_vec_cap(textbox->buf);
                            textbox->gap_len = new_cap - cap;
                            u32 len = cap - start;
                            dlb_memcpy(
                                textbox->buf + textbox->cursor + textbox->gap_len,
                                textbox->buf + textbox->cursor,
                                len
                            );
                        }
                        textbox->buf[textbox->cursor] = *c;
                        textbox->cursor++;
                        textbox->gap_len--;
                    }
#if 0
                    while (*c) {
                        // TODO: Accept all valid chars, maybe pointer to filter
                        //       function provided by textbox?
                        if (tg_game.text_entry.filter(*c)) {
                            dlb_vec_push(*tg_game.text_entry.buffer, *c);
                        }
                        c++;
                    }
#endif
                }
                break;
            }
        }
    }
}

void ta_event_events()
{
    // TODO: Should I use SDL polling instead of manual polling for mouse/kbd?
    ta_mouse_events();
    ta_keyboard_events();
    event_sdl_poll();

    ta_event event;
    while (ta_event_pop(&event)) {
        // TODO: Short-circuit if event.handled = true, or handler returns true
        ta_window_event(tg_game.window, &event);
        ta_game_event(&tg_game, &event);
        ta_camera_event(tg_game.camera, &event);
    }
}