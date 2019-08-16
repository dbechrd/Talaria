#include "ta_game.h"
#include "ta_event.h"
#include "ta_mouse.h"
#include "ta_keyboard.h"
#include "ta_log.h"
#include "ta_rigid_body.h"
#include "ta_symbol.h"
#include "ta_timer.h"
#include "ta_button.h"
#include "SDL/SDL.h"

ta_game tg_game;
GLenum tg_polygon_mode = GL_FILL;

static const char *game_state_str(ta_game_state state)
{
    switch(state) {
        case TA_GAME_STATE_INIT:         return "TA_GAME_STATE_INIT";
        case TA_GAME_STATE_PLAY:         return "TA_GAME_STATE_PLAY";
        case TA_GAME_STATE_FREE_CAM:     return "TA_GAME_STATE_FREE_CAM";
        case TA_GAME_STATE_TEXT_ENTRY:   return "TA_GAME_STATE_TEXT_ENTRY";
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
    BIND1(PLAY, GAME_PLAYER_JUMP,          HOLD, SPACE);
    BIND1(PLAY, GAME_PLAYER_SHOOT,         PRESS, MOUSE_LEFT);

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

    BIND1(FREE_CAM, DEBUG_TOGGLE_MOUSE_LOCK, PRESS, SCROLLLOCK);
    BIND1(FREE_CAM, DEBUG_TOGGLE_WIREFRAME,  PRESS, Z);
    BIND1(FREE_CAM, DEBUG_TOGGLE_BBOX,       PRESS, 1);
    BIND1(FREE_CAM, DEBUG_TOGGLE_NORMALS,    PRESS, 2);
    BIND1(FREE_CAM, GAME_PLAYER_JUMP,        HOLD,  3);

    //--------------------------------------------------------------------------
    // TEXT_ENTRY

#if 0
    //BIND1(TEXT_ENTRY, TEXT_ENTRY_KEYDOWN, PRESS, A);
    //BIND1(TEXT_ENTRY, TEXT_ENTRY_KEYDOWN, PRESS, B);
    //BIND1(TEXT_ENTRY, TEXT_ENTRY_KEYDOWN, PRESS, C);
    //BIND1(TEXT_ENTRY, TEXT_ENTRY_KEYDOWN, PRESS, D);
    //BIND1(TEXT_ENTRY, TEXT_ENTRY_KEYDOWN, PRESS, E);
    //BIND1(TEXT_ENTRY, TEXT_ENTRY_KEYDOWN, PRESS, F);
    //BIND1(TEXT_ENTRY, TEXT_ENTRY_KEYDOWN, PRESS, G);
    //BIND1(TEXT_ENTRY, TEXT_ENTRY_KEYDOWN, PRESS, H);
    //BIND1(TEXT_ENTRY, TEXT_ENTRY_KEYDOWN, PRESS, I);
    //BIND1(TEXT_ENTRY, TEXT_ENTRY_KEYDOWN, PRESS, J);
    //BIND1(TEXT_ENTRY, TEXT_ENTRY_KEYDOWN, PRESS, K);
    //BIND1(TEXT_ENTRY, TEXT_ENTRY_KEYDOWN, PRESS, L);
    //BIND1(TEXT_ENTRY, TEXT_ENTRY_KEYDOWN, PRESS, M);
    //BIND1(TEXT_ENTRY, TEXT_ENTRY_KEYDOWN, PRESS, N);
    //BIND1(TEXT_ENTRY, TEXT_ENTRY_KEYDOWN, PRESS, O);
    //BIND1(TEXT_ENTRY, TEXT_ENTRY_KEYDOWN, PRESS, P);
    //BIND1(TEXT_ENTRY, TEXT_ENTRY_KEYDOWN, PRESS, Q);
    //BIND1(TEXT_ENTRY, TEXT_ENTRY_KEYDOWN, PRESS, R);
    //BIND1(TEXT_ENTRY, TEXT_ENTRY_KEYDOWN, PRESS, S);
    //BIND1(TEXT_ENTRY, TEXT_ENTRY_KEYDOWN, PRESS, T);
    //BIND1(TEXT_ENTRY, TEXT_ENTRY_KEYDOWN, PRESS, U);
    //BIND1(TEXT_ENTRY, TEXT_ENTRY_KEYDOWN, PRESS, V);
    //BIND1(TEXT_ENTRY, TEXT_ENTRY_KEYDOWN, PRESS, W);
    //BIND1(TEXT_ENTRY, TEXT_ENTRY_KEYDOWN, PRESS, X);
    //BIND1(TEXT_ENTRY, TEXT_ENTRY_KEYDOWN, PRESS, Y);
    //BIND1(TEXT_ENTRY, TEXT_ENTRY_KEYDOWN, PRESS, Z);
    //BIND1(TEXT_ENTRY, TEXT_ENTRY_KEYDOWN, PRESS, 0);
    //BIND1(TEXT_ENTRY, TEXT_ENTRY_KEYDOWN, PRESS, 1);
    //BIND1(TEXT_ENTRY, TEXT_ENTRY_KEYDOWN, PRESS, 2);
    //BIND1(TEXT_ENTRY, TEXT_ENTRY_KEYDOWN, PRESS, 3);
    //BIND1(TEXT_ENTRY, TEXT_ENTRY_KEYDOWN, PRESS, 4);
    //BIND1(TEXT_ENTRY, TEXT_ENTRY_KEYDOWN, PRESS, 5);
    //BIND1(TEXT_ENTRY, TEXT_ENTRY_KEYDOWN, PRESS, 6);
    //BIND1(TEXT_ENTRY, TEXT_ENTRY_KEYDOWN, PRESS, 7);
    //BIND1(TEXT_ENTRY, TEXT_ENTRY_KEYDOWN, PRESS, 8);
    //BIND1(TEXT_ENTRY, TEXT_ENTRY_KEYDOWN, PRESS, 9);

    SDL_SCANCODE_A = 4,
    SDL_SCANCODE_B = 5,
    SDL_SCANCODE_C = 6,
    SDL_SCANCODE_D = 7,
    SDL_SCANCODE_E = 8,
    SDL_SCANCODE_F = 9,
    SDL_SCANCODE_G = 10,
    SDL_SCANCODE_H = 11,
    SDL_SCANCODE_I = 12,
    SDL_SCANCODE_J = 13,
    SDL_SCANCODE_K = 14,
    SDL_SCANCODE_L = 15,
    SDL_SCANCODE_M = 16,
    SDL_SCANCODE_N = 17,
    SDL_SCANCODE_O = 18,
    SDL_SCANCODE_P = 19,
    SDL_SCANCODE_Q = 20,
    SDL_SCANCODE_R = 21,
    SDL_SCANCODE_S = 22,
    SDL_SCANCODE_T = 23,
    SDL_SCANCODE_U = 24,
    SDL_SCANCODE_V = 25,
    SDL_SCANCODE_W = 26,
    SDL_SCANCODE_X = 27,
    SDL_SCANCODE_Y = 28,
    SDL_SCANCODE_Z = 29,

    SDL_SCANCODE_1 = 30,
    SDL_SCANCODE_2 = 31,
    SDL_SCANCODE_3 = 32,
    SDL_SCANCODE_4 = 33,
    SDL_SCANCODE_5 = 34,
    SDL_SCANCODE_6 = 35,
    SDL_SCANCODE_7 = 36,
    SDL_SCANCODE_8 = 37,
    SDL_SCANCODE_9 = 38,
    SDL_SCANCODE_0 = 39,

    SDL_SCANCODE_RETURN = 40,
    SDL_SCANCODE_ESCAPE = 41,
    SDL_SCANCODE_BACKSPACE = 42,
    SDL_SCANCODE_TAB = 43,
    SDL_SCANCODE_SPACE = 44,

    SDL_SCANCODE_MINUS = 45,
    SDL_SCANCODE_EQUALS = 46,
    SDL_SCANCODE_LEFTBRACKET = 47,
    SDL_SCANCODE_RIGHTBRACKET = 48,
    SDL_SCANCODE_BACKSLASH = 49,
    SDL_SCANCODE_SEMICOLON = 51,
    SDL_SCANCODE_APOSTROPHE = 52,
    SDL_SCANCODE_GRAVE = 53,
    SDL_SCANCODE_COMMA = 54,
    SDL_SCANCODE_PERIOD = 55,
    SDL_SCANCODE_SLASH = 56,

    SDL_SCANCODE_F1 = 58,
    SDL_SCANCODE_F2 = 59,
    SDL_SCANCODE_F3 = 60,
    SDL_SCANCODE_F4 = 61,
    SDL_SCANCODE_F5 = 62,
    SDL_SCANCODE_F6 = 63,
    SDL_SCANCODE_F7 = 64,
    SDL_SCANCODE_F8 = 65,
    SDL_SCANCODE_F9 = 66,
    SDL_SCANCODE_F10 = 67,
    SDL_SCANCODE_F11 = 68,
    SDL_SCANCODE_F12 = 69,

    SDL_SCANCODE_PRINTSCREEN = 70,
    SDL_SCANCODE_SCROLLLOCK = 71,
    SDL_SCANCODE_PAUSE = 72,

    SDL_SCANCODE_INSERT = 73,
    SDL_SCANCODE_HOME = 74,
    SDL_SCANCODE_PAGEUP = 75,
    SDL_SCANCODE_DELETE = 76,
    SDL_SCANCODE_END = 77,
    SDL_SCANCODE_PAGEDOWN = 78,

    SDL_SCANCODE_RIGHT = 79,
    SDL_SCANCODE_LEFT = 80,
    SDL_SCANCODE_DOWN = 81,
    SDL_SCANCODE_UP = 82,

    SDL_SCANCODE_KP_DIVIDE = 84,
    SDL_SCANCODE_KP_MULTIPLY = 85,
    SDL_SCANCODE_KP_MINUS = 86,
    SDL_SCANCODE_KP_PLUS = 87,
    SDL_SCANCODE_KP_ENTER = 88,
    SDL_SCANCODE_KP_1 = 89,
    SDL_SCANCODE_KP_2 = 90,
    SDL_SCANCODE_KP_3 = 91,
    SDL_SCANCODE_KP_4 = 92,
    SDL_SCANCODE_KP_5 = 93,
    SDL_SCANCODE_KP_6 = 94,
    SDL_SCANCODE_KP_7 = 95,
    SDL_SCANCODE_KP_8 = 96,
    SDL_SCANCODE_KP_9 = 97,
    SDL_SCANCODE_KP_0 = 98,
    SDL_SCANCODE_KP_PERIOD = 99,
#endif
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
        case TA_GAME_STATE_TEXT_ENTRY:
            // TODO: Some sort of global text entry buffer?
            break;
    }
}

static void game_player_shoot()
{
    if (!tg_mouse.captured) {
        return;
    }

    static double last_shoot_ms = 0;
    static double last_cock_ms = 0;
    static double last_oh_no_ms = 0;

    ta_audio_source *src_gun =
        ta_scene_find(tg_game.scene, TA_AUDIO_SOURCE, INTERN("src_gun"));

    double now_ms = ta_timer_elapsed_ms();

    if (tg_game.player_clip > 0) {
        static double after_shoot_delay_ms = 150;
        static double after_cock_delay_ms = 1000;
        if (now_ms < last_shoot_ms + after_shoot_delay_ms ||
            now_ms < last_cock_ms + after_cock_delay_ms) {
            return;
        }

        ta_audio_buffer *sfx_gunshot =
            ta_scene_find(tg_game.scene, TA_AUDIO_BUFFER, INTERN("sfx_gunshot"));
        ta_audio_source_set_buffer(src_gun, sfx_gunshot);
        ta_audio_source_play(src_gun);
        last_shoot_ms = ta_timer_elapsed_ms();
        tg_game.player_clip--;
    } else {
        if (tg_game.player_ammo) {
            static double after_shoot_delay_ms = 750;
            if (now_ms < last_shoot_ms + after_shoot_delay_ms) {
                return;
            }

            ta_audio_buffer *sfx_cock =
                ta_scene_find(tg_game.scene, TA_AUDIO_BUFFER, INTERN("sfx_cock"));
            ta_audio_source_set_buffer(src_gun, sfx_cock);
            ta_audio_source_play(src_gun);
            last_cock_ms = ta_timer_elapsed_ms();

            tg_game.player_clip = MIN(tg_game.player_clip_max, tg_game.player_ammo);
            tg_game.player_ammo -= tg_game.player_clip;
        } else {
            static double after_shoot_delay_ms = 750;
            static double after_oh_no_delay_ms = 2000;
            if (now_ms < last_shoot_ms + after_shoot_delay_ms ||
                now_ms < last_oh_no_ms + after_oh_no_delay_ms) {
                return;
            }

            ta_audio_buffer *sfx_cock =
                ta_scene_find(tg_game.scene, TA_AUDIO_BUFFER, INTERN("sfx_oh_no"));
            ta_audio_source_set_buffer(src_gun, sfx_cock);
            ta_audio_source_play(src_gun);
            last_oh_no_ms = ta_timer_elapsed_ms();
        }
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
#if 0
            } case TA_EVENT_GAME_MOUSE_CLICK: {
                break;
            } case TA_EVENT_GAME_MOUSE_SCROLL: {
                // TODO: Scroll active element being hovered, if not
                //       handled, bubble up
                //ta_ui_scrollview_scroll(view, event.data.mouse_scroll.y *
                //    -event.data.mouse_scroll.flipped);
                break;
#endif
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
            } case TA_EVENT_GAME_PLAYER_JUMP: {
                //dir.y = 1000.0f;
                ta_rigid_body *player_body = ta_node_rigid_body(tg_game.player);
                ta_rigid_body_apply_impulse(player_body, VEC3_Y, VEC3_ZERO);
                break;
            } case TA_EVENT_GAME_PLAYER_SHOOT: {
                game_player_shoot();
                break;
            } case TA_EVENT_GAME_BUTTON_ACTIVATED: {
                if (tg_game.player_ammo == 0 && tg_game.player_clip == 0) {
                    tg_game.player_ammo = tg_game.player_ammo_max;
                }

#if 0
                // TODO: Should audio source subscribe to this event somehow,
                //       or should the button queue the play request itself?
                e_button *button =
                    ta_scene_find(tg_game.scene, TA_BUTTON, event.data.button.button_uid);
                ta_audio_buffer *buffer = e_button_sfx_activated(button);
                if (buffer) {
                    ta_audio_source *source = e_button_audio_source(button);
                    ta_audio_source_set_buffer(source, buffer);
                    ta_audio_source_play(source);
                }
#endif
                break;
            } case TA_EVENT_GAME_BUTTON_DEACTIVATED: {
                break;
            } case TA_EVENT_GAME_BUTTON_STATE_CHANGED: {
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