#include "ta_game.h"
#include "ta_event.h"
#include "ta_mouse.h"
#include "ta_keyboard.h"
#include "ta_log.h"
#include "ta_rigid_body.h"
#include "SDL/SDL.h"

ta_game tg_game;
GLenum tg_polygon_mode = GL_FILL;

static const char *game_state_str(ta_game_state state)
{
    switch(state) {
        case TA_GAME_STATE_INIT:         return "TA_GAME_STATE_INIT";
        case TA_GAME_STATE_PLAY:         return "TA_GAME_STATE_PLAY";
        case TA_GAME_STATE_FREE_CAM:     return "TA_GAME_STATE_FREE_CAM";
        case TA_GAME_STATE_QUIT:         return "TA_GAME_STATE_QUIT";
        default: DLB_ASSERT(!"Unknown game state");  return 0;
    }
};

void ta_game_init()
{
    ta_log_write(tg_debug_log, "[Game] Initializing game\n");

    // TODO: What other startup states would be useful (e.g. LOADING_MESHES)?
    //       Could use this for a progress bar during load and better logging.
    //       Maybe also have JUMPING, CLIMBING, etc.? Could use bit flags to
    //       capture overall state as well (e.g. PLAYING, EDITING, etc.)
    ta_game_state_set(TA_GAME_STATE_INIT);

    ta_log_write(tg_debug_log, "[Game] Initializing key binds\n");
    // TODO: Read keybinds from file
    //dlb_vec_reserve(tg_keybinds, 16);

#define BIND1(state, e, key_state, key) \
    ta_keybind_bind1(TA_GAME_STATE_##state, TA_EVENT_##e, TA_KEY_##key_state, \
    SDL_SCANCODE_##key)

    //--------------------------------------------------------------------------
    // PLAY

    BIND1(PLAY, GAME_QUIT,     RELEASE, ESCAPE);
    BIND1(PLAY, GAME_FREE_CAM, RELEASE, X);

    BIND1(PLAY, GAME_PLAYER_MOVE_FORWARD,  HOLD, W);
    BIND1(PLAY, GAME_PLAYER_MOVE_BACKWARD, HOLD, S);
    BIND1(PLAY, GAME_PLAYER_MOVE_RIGHT,    HOLD, D);
    BIND1(PLAY, GAME_PLAYER_MOVE_LEFT,     HOLD, A);
    BIND1(PLAY, GAME_PLAYER_MOVE_JUMP,     HOLD, SPACE);

    BIND1(PLAY, DEBUG_TOGGLE_MOUSE_LOCK, PRESS, M);
    BIND1(PLAY, DEBUG_TOGGLE_WIREFRAME,  PRESS, Z);
    BIND1(PLAY, DEBUG_TOGGLE_BBOX,       PRESS, 1);
    BIND1(PLAY, DEBUG_TOGGLE_NORMALS,    PRESS, 2);

    //--------------------------------------------------------------------------
    // FREE_CAM

    BIND1(FREE_CAM, GAME_QUIT, RELEASE, ESCAPE);
    BIND1(FREE_CAM, GAME_PLAY, RELEASE, X);

    BIND1(FREE_CAM, GAME_PLAYER_MOVE_FORWARD,  HOLD, UP);
    BIND1(FREE_CAM, GAME_PLAYER_MOVE_BACKWARD, HOLD, DOWN);
    BIND1(FREE_CAM, GAME_PLAYER_MOVE_RIGHT,    HOLD, RIGHT);
    BIND1(FREE_CAM, GAME_PLAYER_MOVE_LEFT,     HOLD, LEFT);

    BIND1(FREE_CAM, CAMERA_MOVE_FORWARD,  HOLD, W);
    BIND1(FREE_CAM, CAMERA_MOVE_BACKWARD, HOLD, S);
    BIND1(FREE_CAM, CAMERA_MOVE_RIGHT,    HOLD, D);
    BIND1(FREE_CAM, CAMERA_MOVE_LEFT,     HOLD, A);
    BIND1(FREE_CAM, CAMERA_MOVE_UP,       HOLD, SPACE);
    BIND1(FREE_CAM, CAMERA_MOVE_DOWN,     HOLD, LSHIFT);

    BIND1(FREE_CAM, DEBUG_TOGGLE_MOUSE_LOCK, PRESS, M);
    BIND1(FREE_CAM, DEBUG_TOGGLE_WIREFRAME,  PRESS, Z);
    BIND1(FREE_CAM, DEBUG_TOGGLE_BBOX,       PRESS, 1);
    BIND1(FREE_CAM, DEBUG_TOGGLE_NORMALS,    PRESS, 2);
    BIND1(FREE_CAM, GAME_PLAYER_MOVE_JUMP,   HOLD,  3);
#undef BIND1

    ta_log_write(tg_debug_log, "[Game] Game initialized\n");
}

void ta_game_state_set(ta_game_state state)
{
    ta_log_write(tg_debug_log, "[Game] State = %s\n", game_state_str(state));
    tg_game.state = state;
    switch (tg_game.state) {
        case TA_GAME_STATE_PLAY:
            tg_game.camera = tg_game.camera_player;
            break;
        case TA_GAME_STATE_FREE_CAM:
            if (vec3_zero(tg_game.camera_freecam->position)) {
                tg_game.camera_freecam->follow_target = tg_game.camera_player->follow_target;
                tg_game.camera_freecam->position = tg_game.camera_freecam->follow_target;
            }
            tg_game.camera = tg_game.camera_freecam;
            break;
    }
}

void ta_game_events()
{
    ta_vec3 dir = { 0 };
    ta_event event;
    while (ta_event_pop(&event, TA_EVENT_QUEUE_GAME)) {
        switch (event.type) {
            case TA_EVENT_GAME_QUIT: {
                ta_game_state_set(TA_GAME_STATE_QUIT);
                break;
            } case TA_EVENT_GAME_INIT: {
                ta_game_state_set(TA_GAME_STATE_INIT);
                break;
            } case TA_EVENT_GAME_FREE_CAM: {
                ta_game_state_set(TA_GAME_STATE_FREE_CAM);
                break;
            } case TA_EVENT_GAME_PLAY: {
                ta_game_state_set(TA_GAME_STATE_PLAY);
                break;
            } case TA_EVENT_GAME_MOUSE_MOVE: {
                if (!tg_mouse.captured) break;

                switch (tg_game.state) {
                    case TA_GAME_STATE_PLAY: // Intentional fall-through
                    case TA_GAME_STATE_FREE_CAM: {
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
            } case TA_EVENT_GAME_PLAYER_MOVE_FORWARD: {
                dir.x += tg_game.camera->front.x;
                dir.z += tg_game.camera->front.z;
                break;
            } case TA_EVENT_GAME_PLAYER_MOVE_BACKWARD: {
                dir.x -= tg_game.camera->front.x;
                dir.z -= tg_game.camera->front.z;
                break;
            } case TA_EVENT_GAME_PLAYER_MOVE_RIGHT: {
                dir.x += tg_game.camera->right.x;
                dir.z += tg_game.camera->right.z;
                break;
            } case TA_EVENT_GAME_PLAYER_MOVE_LEFT: {
                dir.x -= tg_game.camera->right.x;
                dir.z -= tg_game.camera->right.z;
                break;
            } case TA_EVENT_GAME_PLAYER_MOVE_JUMP: {
                //dir.y = 1000.0f;
                ta_rigid_body *player_body = ta_node_rigid_body(tg_game.player);
                ta_rigid_body_apply_impulse(player_body, VEC3_Y, VEC3_ZERO);
                break;
            } case TA_EVENT_DEBUG_TOGGLE_MOUSE_LOCK: {
                ta_mouse_toggle_capture();
                break;
            } case TA_EVENT_DEBUG_TOGGLE_WIREFRAME: {
                tg_game.camera->debug_wireframe =
                    !tg_game.camera->debug_wireframe;
                break;
            } case TA_EVENT_DEBUG_TOGGLE_NORMALS: {
                tg_game.camera->debug_normals = !tg_game.camera->debug_normals;
                break;
            } case TA_EVENT_DEBUG_TOGGLE_BBOX: {
                tg_game.camera->debug_bounding_boxes =
                    !tg_game.camera->debug_bounding_boxes;
                break;
            } case TA_EVENT_DEBUG_TOGGLE_MESH: {
                tg_game.camera->debug_no_mesh =
                    !tg_game.camera->debug_no_mesh;
                break;
            } default: {
                DLB_ASSERT(!"Unhandled event type");
            }
        }
    }
    if (!vec3_zero(dir)) {
        dir = vec3_normalize(dir);
        dir = vec3_scalef(dir, 20.0f);
        ta_rigid_body *player_body = ta_node_rigid_body(tg_game.player);
        ta_rigid_body_apply_force(player_body, dir);
        //player_body->velocity = vec3_add(player_body->velocity, dir);
        //player_body->position = vec3_add(player_body->position, dir);
    }
}