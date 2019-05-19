#include "ta_game.h"
#include "ta_event.h"
#include "ta_mouse.h"
#include "ta_log.h"

ta_game tg_game;

static const char *game_state_str(ta_game_state state)
{
    switch(state) {
        case TA_STATE_INIT:         return "TA_STATE_INIT";
        case TA_STATE_PLAY:         return "TA_STATE_PLAY";
        case TA_STATE_FREE_CAM:     return "TA_STATE_FREE_CAM";
        case TA_STATE_QUIT:         return "TA_STATE_QUIT";
        default: DLB_ASSERT(!"Unknown game state");  return 0;
    }
};

static void game_state_set(ta_game_state state)
{
    tg_game.state = state;
    ta_log_write(tg_debug_log, "[Game] State = %s\n", game_state_str(state));
}

void ta_game_update()
{
    ta_event event;
    while (ta_event_pop(&event, TA_EVENT_QUEUE_GAME)) {
        switch (event.type) {
            case TA_EVENT_GAME_STATE_QUIT: {
                game_state_set(TA_STATE_QUIT);
                break;
            } case TA_EVENT_GAME_STATE_INIT: {
                game_state_set(TA_STATE_INIT);
                break;
            } case TA_EVENT_GAME_STATE_FREE_CAM: {
                game_state_set(TA_STATE_FREE_CAM);
                break;
            } case TA_EVENT_GAME_STATE_PLAY: {
                game_state_set(TA_STATE_PLAY);
                break;
            } case TA_EVENT_GAME_MOUSE_MOVE: {
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
                    }
                }
                break;
            } case TA_EVENT_GAME_MOUSE_CLICK: {
                break;
            } case TA_EVENT_GAME_MOUSE_SCROLL: {
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
                tg_debug_render_normals = !tg_debug_render_normals;
                break;
            } case TA_EVENT_DEBUG_TOGGLE_BBOX: {
                tg_debug_render_bounding_boxes = !tg_debug_render_bounding_boxes;
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