#include "ta_event.h"
#include "ta_keyboard.h"
#include "ta_log.h"
#include "ta_mouse.h"
#include "dlb_vector.h"
#include "SDL/SDL.h"
#include <string.h>

bool tg_debug_render_normals = false;
bool tg_debug_render_bounding_boxes = false;
bool tg_debug_follow_pinky = false;

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
                memcpy(&queue->buffer[queue->count],
                    queue->buffer, bytes);
#if _DEBUG
                memset(queue->buffer, 0, bytes);
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

void ta_event_sdl_poll()
{
    SDL_Event sdl_event;
    while (SDL_PollEvent(&sdl_event)) {
        switch (sdl_event.type) {
            case SDL_QUIT: {
                ta_event event = { 0 };
                event.type = TA_EVENT_GLOBAL_STATE_QUIT;
                ta_event_push(&event);
                break;
            } case SDL_WINDOWEVENT: {
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

// TODO: Move this to ta_game_update() and rename queue to TA_EVENT_GAME
void ta_event_update()
{
    ta_event event;
    while (ta_event_pop(&event, TA_EVENT_QUEUE_GLOBAL)) {
        switch (event.type) {
            case TA_EVENT_GLOBAL_STATE_PLAY: {
                tg_game.state = TA_STATE_PLAY;
                break;
            } case TA_EVENT_GLOBAL_STATE_FREE_CAM: {
                tg_game.state = TA_STATE_FREE_CAM;
                break;
            } case TA_EVENT_GLOBAL_STATE_QUIT: {
                tg_game.state = TA_STATE_QUIT;
                break;
            } case TA_EVENT_GLOBAL_MOUSE_MOVE: {
                if (!tg_mouse.captured) break;

                switch (tg_game.state) {
                    case TA_STATE_PLAY: // Intentional fall-through
                    case TA_STATE_FREE_CAM: {
                        ta_event cam_rotate_evt = { 0 };
                        cam_rotate_evt.type = TA_EVENT_CAMERA_ROTATE;
                        if (event.data.mouse_move.dx) {
                            cam_rotate_evt.data.camera_rotate.delta_yaw =
                                (float)-event.data.mouse_move.dx;
                        }
                        if (event.data.mouse_move.dy) {
                            cam_rotate_evt.data.camera_rotate.delta_pitch =
                                (float)-event.data.mouse_move.dy;
                        }
                        ta_event_push(&cam_rotate_evt);
                        break;
                    } default: {
                        DLB_ASSERT(!"Unhandled state");
                    }
                }
                break;
            } case TA_EVENT_GLOBAL_MOUSE_CLICK: {
                break;
            } case TA_EVENT_GLOBAL_MOUSE_SCROLL: {
                // TODO: Scroll active element being hovered, if not
                //       handled, bubble up
                //ta_ui_scrollview_scroll(view, event.data.mouse_scroll.y *
                //    -event.data.mouse_scroll.flipped);
                break;
            } case TA_EVENT_DEBUG_TOGGLE_MOUSE_LOCK: {
                ta_mouse_toggle_capture();
                break;
            } case TA_EVENT_DEBUG_TOGGLE_WIREFRAME: {
                ta_camera_toggle_wireframe(tg_game.scene->cameras);
                break;
            } case TA_EVENT_DEBUG_TOGGLE_NORMALS: {
                tg_debug_render_normals = true;
                break;
            } case TA_EVENT_DEBUG_TOGGLE_BBOX: {
                tg_debug_render_bounding_boxes = true;
                break;
            } case TA_EVENT_DEBUG_BOOST_PINKY: {
                ta_rigid_body *rb = ta_entity_rigid_body(tg_game.player);
                rb->transform.position.y += 0.1f;
                break;
            } case TA_EVENT_DEBUG_FOCUS_PINKY: {
                tg_debug_follow_pinky = !tg_debug_follow_pinky;
                break;
            } default: {
                DLB_ASSERT(!"Unhandled event type");
            }
        }
    }
}