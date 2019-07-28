#include "ta_event.h"
#include "ta_keyboard.h"
#include "ta_log.h"
#include "ta_mouse.h"
#include "ta_game.h"
#include "dlb_vector.h"
#include "SDL/SDL.h"

void ta_event_push(ta_event *event)
{
    int event_type_queue = TA_EVENT_TYPE_QUEUE(event->type);
    ta_event_queue *queue = &tg_event_queues[event_type_queue];
    if (queue->count == queue->capacity) {
        u32 old_size = queue->capacity;
        u32 new_cap = MAX(16, queue->capacity * 2);
        dlb_vec_reserve(queue->buffer, new_cap);
        if (old_size) {
            // Before resize: [D, A, B, C]
            // After resize : [-, A, B, C, D, -, -, -]
            if (queue->head > 0) {
                int bytes = queue->head * sizeof(queue->buffer[0]);
                dlb_memcpy(&queue->buffer[queue->count],
                    queue->buffer, bytes);
#if _DEBUG
                dlb_memset(queue->buffer, 0, bytes);
#endif
            }
        }
        queue->capacity = new_cap;
    }
    int next = (queue->head + queue->count) % queue->capacity;
    queue->buffer[next] = *event;
    queue->count++;
}

bool ta_event_pop(ta_event *event, ta_event_queue_type queue_type)
{
    ta_event_queue *queue = &tg_event_queues[queue_type];
    if (queue->count) {
        *event = queue->buffer[queue->head];
        queue->head = (queue->head + 1) % queue->capacity;
        queue->count--;
        return true;
    } else {
        return false;
    }
}

bool ta_event_peek(ta_event *event, ta_event_queue_type queue_type)
{
    ta_event_queue *queue = &tg_event_queues[queue_type];
    if (queue->count) {
        *event = queue->buffer[queue->head];
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
                event.type = TA_EVENT_GLOBAL_QUIT;
                ta_event_push(&event);
                break;
            } case SDL_WINDOWEVENT: {
                switch (sdl_event.window.event) {
                    case SDL_WINDOWEVENT_RESIZED: {
                        ta_event event = { 0 };
                        event.type = TA_EVENT_GLOBAL_WINDOW_RESIZE;
                        event.data.window_resize.width = sdl_event.window.data1;
                        event.data.window_resize.height = sdl_event.window.data2;
                        ta_event_push(&event);
                        break;
                    }
                }
                break;
            } case SDL_MOUSEWHEEL: {
                ta_event event = { 0 };
                event.type = TA_EVENT_GLOBAL_MOUSE_SCROLL;
                event.data.mouse_scroll.x = sdl_event.wheel.x;
                event.data.mouse_scroll.y = sdl_event.wheel.y;
                event.data.mouse_scroll.flipped = (u8)sdl_event.wheel.direction;
                ta_event_push(&event);
                break;
            } case SDL_TEXTEDITING: {
                break;
            } case SDL_TEXTINPUT: {
                break;
            }
        }
    }
}

void ta_event_events()
{
    event_sdl_poll();

    ta_event event;
    while (ta_event_pop(&event, TA_EVENT_QUEUE_GLOBAL)) {
        switch (event.type) {
            case TA_EVENT_GLOBAL_QUIT: {
                ta_event e = { 0 };
                e.type = TA_EVENT_GAME_QUIT;
                ta_event_push(&e);
                break;
            } case TA_EVENT_GLOBAL_WINDOW_RESIZE: {
                ta_event e_w = { 0 };
                e_w.type = TA_EVENT_WINDOW_RESIZE;
                e_w.data.window_resize = event.data.window_resize;
                ta_event_push(&e_w);
                ta_event e_c = { 0 };
                e_c.type = TA_EVENT_CAMERA_ASPECT_CHANGE;
                e_c.data.window_resize = event.data.window_resize;
                ta_event_push(&e_c);
                break;
            } case TA_EVENT_GLOBAL_MOUSE_MOVE: {
                ta_event e = { 0 };
                e.type = TA_EVENT_GAME_MOUSE_MOVE;
                e.data.mouse_move = event.data.mouse_move;
                ta_event_push(&e);
                break;
#if 0
            } case TA_EVENT_GLOBAL_MOUSE_CLICK: {
                ta_event e = { 0 };
                e.type = TA_EVENT_GAME_MOUSE_CLICK;
                e.data.mouse_click = event.data.mouse_click;
                ta_event_push(&e);
                break;
            } case TA_EVENT_GLOBAL_MOUSE_SCROLL: {
                ta_event e = { 0 };
                e.type = TA_EVENT_GAME_MOUSE_SCROLL;
                e.data.mouse_scroll = event.data.mouse_scroll;
                ta_event_push(&e);
                break;
#endif
            } default: {
                DLB_ASSERT(!"Unhandled event type");
            }
        }
    }
}