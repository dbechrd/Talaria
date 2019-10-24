#include "ta_game.h"
#include "ta_event.h"
#include "ta_mouse.h"
#include "ta_keybind.h"
#include "ta_log.h"
#include "ta_rigid_body.h"
#include "ta_symbol.h"
#include "ta_timer.h"
#include "ta_button.h"
#include "ta_collider.h"
#include "ta_scene.h"
#include "ta_node.h"
#include "ta_audio.h"
#include "ta_editor.h"
#include "ta_ui.h"
#include "ta_camera.h"
#include "ta_entity.h"
#include "ta_player.h"
#include "dlb/dlb_vector.h"
#include "misc/gl3w.h"
#include "SDL/SDL.h"

ta_game tg_game;

const char *game_state_str(ta_game_state state)
{
    switch(state) {
        case TA_GAME_STATE_STARTUP:  return "TA_GAME_STATE_STARTUP";
        case TA_GAME_STATE_PLAY:     return "TA_GAME_STATE_PLAY";
        case TA_GAME_STATE_FREE_CAM: return "TA_GAME_STATE_FREE_CAM";
        case TA_GAME_STATE_EDITOR:   return "TA_GAME_STATE_EDITOR";
        case TA_GAME_STATE_SHUTDOWN: return "TA_GAME_STATE_SHUTDOWN";
        default: DLB_ASSERT(!"Unknown game state");  return 0;
    }
};

void ta_game_init(ta_game *game)
{
    ta_log_write(&tg_debug_log, "[Game] Initializing game\n");

    // TODO: What other startup states would be useful (e.g. LOADING_MESHES)?
    //       Could use this for a progress bar during load and better logging.
    //       Maybe also have JUMPING, CLIMBING, etc.? Could use bit flags to
    //       capture overall state as well (e.g. PLAYING, EDITING, etc.)
    ta_game_state_set(game, TA_GAME_STATE_STARTUP);

    ta_log_write(&tg_debug_log, "[Game] Initializing key binds\n");

#define BIND1(state, e, key_state, key) \
    ta_keybind_bind1(&game->keybinds[TA_GAME_STATE_##state], e, \
    TA_KEYBIND_##key_state, SDL_SCANCODE_##key)

    // TODO: Read keybinds from file
    //dlb_vec_reserve(tg_keybinds, 16);

    //--------------------------------------------------------------------------
    // PLAY

    BIND1(PLAY, TA_EVENT_GAME_SHUTDOWN,             RELEASE, ESCAPE);
    BIND1(PLAY, TA_EVENT_GAME_FREE_CAM,             RELEASE, X);
    BIND1(PLAY, TA_EVENT_GAME_EDITOR,               RELEASE, GRAVE);

    BIND1(PLAY, TA_EVENT_GAME_PLAYER_MOVE_FORWARD,  HOLD, W);
    BIND1(PLAY, TA_EVENT_GAME_PLAYER_MOVE_BACKWARD, HOLD, S);
    BIND1(PLAY, TA_EVENT_GAME_PLAYER_MOVE_RIGHT,    HOLD, D);
    BIND1(PLAY, TA_EVENT_GAME_PLAYER_MOVE_LEFT,     HOLD, A);
    BIND1(PLAY, TA_EVENT_GAME_PLAYER_JUMP,          PRESS, SPACE);
    BIND1(PLAY, TA_EVENT_GAME_PLAYER_SHOOT,         PRESS, MOUSE_LEFT);

    BIND1(PLAY, TA_EVENT_DEBUG_MOUSE_LOCK_TOGGLE,   PRESS, M);
    BIND1(PLAY, TA_EVENT_DEBUG_TOGGLE_WIREFRAME,    PRESS, 2);
    BIND1(PLAY, TA_EVENT_DEBUG_TOGGLE_BBOX,         PRESS, 3);
    BIND1(PLAY, TA_EVENT_DEBUG_TOGGLE_NORMALS,      PRESS, 4);
    BIND1(PLAY, TA_EVENT_DEBUG_TOGGLE_MESH,         PRESS, 5);

    //--------------------------------------------------------------------------
    // FREE_CAM

    // TODO: Move to camera->keybinds

    BIND1(FREE_CAM, TA_EVENT_GAME_SHUTDOWN,             RELEASE, ESCAPE);
    BIND1(FREE_CAM, TA_EVENT_GAME_PLAY,                 RELEASE, X);
    BIND1(FREE_CAM, TA_EVENT_GAME_EDITOR,               RELEASE, GRAVE);

    BIND1(FREE_CAM, TA_EVENT_GAME_PLAYER_MOVE_FORWARD,  HOLD, UP);
    BIND1(FREE_CAM, TA_EVENT_GAME_PLAYER_MOVE_BACKWARD, HOLD, DOWN);
    BIND1(FREE_CAM, TA_EVENT_GAME_PLAYER_MOVE_RIGHT,    HOLD, RIGHT);
    BIND1(FREE_CAM, TA_EVENT_GAME_PLAYER_MOVE_LEFT,     HOLD, LEFT);
    BIND1(FREE_CAM, TA_EVENT_GAME_PLAYER_JUMP,          PRESS, J);

    BIND1(FREE_CAM, TA_EVENT_CAMERA_MOVE_FORWARD,       HOLD, W);
    BIND1(FREE_CAM, TA_EVENT_CAMERA_MOVE_BACKWARD,      HOLD, S);
    BIND1(FREE_CAM, TA_EVENT_CAMERA_MOVE_RIGHT,         HOLD, D);
    BIND1(FREE_CAM, TA_EVENT_CAMERA_MOVE_LEFT,          HOLD, A);
    BIND1(FREE_CAM, TA_EVENT_CAMERA_MOVE_UP,            HOLD, E);
    BIND1(FREE_CAM, TA_EVENT_CAMERA_MOVE_DOWN,          HOLD, Q);

    BIND1(FREE_CAM, TA_EVENT_DEBUG_MOUSE_LOCK,          PRESS,   MOUSE_RIGHT);
    BIND1(FREE_CAM, TA_EVENT_DEBUG_MOUSE_UNLOCK,        RELEASE, MOUSE_RIGHT);
    BIND1(FREE_CAM, TA_EVENT_DEBUG_MOUSE_LOCK_TOGGLE,   PRESS, M);
    BIND1(FREE_CAM, TA_EVENT_DEBUG_TOGGLE_WIREFRAME,    PRESS, 2);
    BIND1(FREE_CAM, TA_EVENT_DEBUG_TOGGLE_BBOX,         PRESS, 3);
    BIND1(FREE_CAM, TA_EVENT_DEBUG_TOGGLE_NORMALS,      PRESS, 4);
    BIND1(FREE_CAM, TA_EVENT_DEBUG_TOGGLE_MESH,         PRESS, 5);

    //--------------------------------------------------------------------------

#undef BIND1

    ta_log_write(&tg_debug_log, "[Game] Game initialized\n");
}

void ta_game_state_set(ta_game *game, ta_game_state state)
{
    if (state == game->state) {
        return;
    }

    game->state_prev = game->state;
    game->state = state;
    ta_log_write(&tg_debug_log, "[Game] State = %s\n", game_state_str(state));
    switch (game->state) {
        case TA_GAME_STATE_PLAY: {
            game->camera_active_id = game->camera_player_id;
            ta_mouse_capture_set(true);
            break;
        } case TA_GAME_STATE_FREE_CAM: {
            ta_camera *freecam = tg_scene.cameras[tg_game.camera_freecam_id];
            ta_camera *freecam = ta_scene_find_by_id(tg_game.scene, RES_COMP_CAMERA, tg_game.camera_freecam_id);
            ta_entity *e_player = ta_scene_find_by_id(tg_game.scene, RES_ENTITY,
                tg_game.player_one_id);
            ta_camera *player_cam = ta_scene_entity_component(tg_game.scene,
                e_player, RES_COMP_CAMERA);
            if (vec3_zero(freecam->position)) {
                freecam->target_xform.position = player_cam->target_xform.position;
                freecam->position = freecam->target_xform.position;
            }
            game->camera_active_id = game->camera_freecam_id;
            break;
        } case TA_GAME_STATE_EDITOR: {
            break;
        }
    }
}

static void game_player_shoot(ta_game *game)
{
    static double last_bang_ms = 0;
    static double last_reload_ms = 0;
    static double last_empty_ms = 0;

    ta_entity *e_player = ta_scene_find_by_id(game->scene, RES_ENTITY, game->player_one_id);
    ta_player *player = ta_scene_entity_component(game->scene, e_player, RES_COMP_PLAYER);

    ta_entity *e_gun = ta_scene_find_by_id(game->scene, RES_ENTITY, player->gun_id);
    ta_gun *gun = ta_scene_entity_component(game->scene, e_gun, RES_COMP_GUN);
    ta_audio_source *src_gun = ta_scene_entity_component(game->scene, e_gun, RES_COMP_AUDIO_SOURCE);

    double now_ms = ta_timer_elapsed_ms();

    if (gun->loaded_ammo > 0) {
        static double after_bang_delay_ms = 150;
        static double after_reload_delay_ms = 1000;
        if (now_ms < last_bang_ms + after_bang_delay_ms ||
            now_ms < last_reload_ms + after_reload_delay_ms) {
            return;
        }

        ta_audio_source_play_id(src_gun, gun->sfx_bang);
        last_bang_ms = ta_timer_elapsed_ms();
        gun->loaded_ammo--;
    } else {
        if (gun->carrying_ammo) {
            static double after_bang_delay_ms = 750;
            if (now_ms < last_bang_ms + after_bang_delay_ms) {
                return;
            }

            ta_audio_source_play_id(src_gun, gun->sfx_reload);
            last_reload_ms = ta_timer_elapsed_ms();

            gun->loaded_ammo = MIN(gun->loaded_ammo_max, gun->carrying_ammo);
            gun->carrying_ammo -= gun->loaded_ammo;
        } else {
            static double after_bang_delay_ms = 750;
            static double after_empty_delay_ms = 2000;
            if (now_ms < last_bang_ms + after_bang_delay_ms ||
                now_ms < last_empty_ms + after_empty_delay_ms) {
                return;
            }

            ta_audio_source_play_id(src_gun, gun->sfx_empty);
            last_empty_ms = ta_timer_elapsed_ms();
        }
    }
}

void ta_game_hotkeys(ta_game *game)
{
    ta_keybind_trigger(game->keybinds[game->state]);
}

void ta_game_event(ta_game *game, ta_event *event)
{
    bool handled = true;

    switch (event->type) {
        case TA_EVENT_GAME_STARTUP: {
            ta_game_state_set(game, TA_GAME_STATE_STARTUP);
            break;
        } case TA_EVENT_GAME_PLAY: {
            ta_game_state_set(game, TA_GAME_STATE_PLAY);
            break;
        } case TA_EVENT_GAME_FREE_CAM: {
            ta_game_state_set(game, TA_GAME_STATE_FREE_CAM);
            break;
        } case TA_EVENT_GAME_EDITOR: {
            ta_game_state_set(game, TA_GAME_STATE_EDITOR);
            break;
        } case TA_EVENT_GAME_SHUTDOWN: {
            ta_game_state_set(game, TA_GAME_STATE_SHUTDOWN);
            break;
        } case TA_EVENT_GAME_PLAYER_MOVE_FORWARD: {
            ta_camera *camera = ta_scene_find_by_id(tg_game.scene,
                RES_COMP_CAMERA, tg_game.camera_active_id);
            ta_vec3 dir = { 0 };
            dir.x = camera->front.x;
            dir.z = camera->front.z;
            dir = vec3_normalize(dir);
            dir = vec3_scalef(dir, 0.1f);
            ta_entity *e_player = ta_scene_find_by_id(game->scene, RES_ENTITY,
                game->player_one_id);
            ta_rigid_body *player_body = ta_scene_entity_component(game->scene,
                e_player, RES_COMP_RIGID_BODY);
            ta_rigid_body_apply_impulse(player_body, dir, VEC3_ZERO);
            break;
        } case TA_EVENT_GAME_PLAYER_MOVE_BACKWARD: {
            ta_camera *camera = ta_scene_find_by_id(tg_game.scene,
                RES_COMP_CAMERA, tg_game.camera_active_id);
            ta_vec3 dir = { 0 };
            dir.x = -camera->front.x;
            dir.z = -camera->front.z;
            dir = vec3_normalize(dir);
            dir = vec3_scalef(dir, 0.1f);
            ta_entity *e_player = ta_scene_find_by_id(game->scene, RES_ENTITY,
                game->player_one_id);
            ta_rigid_body *player_body = ta_scene_entity_component(game->scene,
                e_player, RES_COMP_RIGID_BODY);
            ta_rigid_body_apply_impulse(player_body, dir, VEC3_ZERO);
            break;
        } case TA_EVENT_GAME_PLAYER_MOVE_RIGHT: {
            ta_camera *camera = ta_scene_find_by_id(tg_game.scene,
                RES_COMP_CAMERA, tg_game.camera_active_id);
            ta_vec3 dir = { 0 };
            dir.x = camera->right.x;
            dir.z = camera->right.z;
            dir = vec3_normalize(dir);
            dir = vec3_scalef(dir, 0.1f);
            ta_entity *e_player = ta_scene_find_by_id(game->scene, RES_ENTITY,
                game->player_one_id);
            ta_rigid_body *player_body = ta_scene_entity_component(game->scene,
                e_player, RES_COMP_RIGID_BODY);
            ta_rigid_body_apply_impulse(player_body, dir, VEC3_ZERO);
            break;
        } case TA_EVENT_GAME_PLAYER_MOVE_LEFT: {
            ta_camera *camera = ta_scene_find_by_id(tg_game.scene,
                RES_COMP_CAMERA, tg_game.camera_active_id);
            ta_vec3 dir = { 0 };
            dir.x = -camera->right.x;
            dir.z = -camera->right.z;
            dir = vec3_normalize(dir);
            dir = vec3_scalef(dir, 0.1f);
            ta_entity *e_player = ta_scene_find_by_id(game->scene, RES_ENTITY,
                game->player_one_id);
            ta_rigid_body *player_body = ta_scene_entity_component(game->scene,
                e_player, RES_COMP_RIGID_BODY);
            ta_rigid_body_apply_impulse(player_body, dir, VEC3_ZERO);
            break;
        } case TA_EVENT_GAME_PLAYER_JUMP: {
            ta_vec3 dir = VEC3_Y;
            dir = vec3_scalef(dir, 5.0f);
            ta_entity *e_player = ta_scene_find_by_id(game->scene, RES_ENTITY,
                game->player_one_id);
            ta_rigid_body *player_body = ta_scene_entity_component(game->scene,
                e_player, RES_COMP_RIGID_BODY);
            ta_rigid_body_apply_impulse(player_body, dir, VEC3_ZERO);
            break;
        } case TA_EVENT_GAME_PLAYER_SHOOT: {
            game_player_shoot(game);
            break;
        } case TA_EVENT_GAME_BUTTON_ACTIVATED: {
            ta_entity *e_player = ta_scene_find_by_id(game->scene, RES_ENTITY,
                game->player_one_id);
            ta_player *player = ta_scene_entity_component(game->scene,
                e_player, RES_COMP_PLAYER);
            ta_entity *e_gun = ta_scene_find_by_id(game->scene, RES_ENTITY,
                player->gun_id);
            ta_gun *gun = ta_scene_entity_component(game->scene,
                e_gun, RES_COMP_GUN);
            if (gun->carrying_ammo == 0 && gun->loaded_ammo == 0) {
                gun->carrying_ammo = gun->carrying_ammo_max;
            }

#if 0
            // TODO: Should audio source subscribe to this event somehow,
            //       or should the button queue the play request itself?
            e_button *button =
                ta_scene_find(game->scene, TYP_BUTTON, event->data.button.button_uid);
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
        } case TA_EVENT_DEBUG_MOUSE_LOCK: {
            ta_mouse_capture_set(true);
            break;
        } case TA_EVENT_DEBUG_MOUSE_UNLOCK: {
            ta_mouse_capture_set(false);
            break;
        } case TA_EVENT_DEBUG_MOUSE_LOCK_TOGGLE: {
            ta_mouse_capture_toggle();
            break;
        } case TA_EVENT_DEBUG_TOGGLE_WIREFRAME: {
            ta_camera *camera = ta_scene_find_by_id(tg_game.scene,
                RES_COMP_CAMERA, tg_game.camera_active_id);
            TOGGLE(camera->debug_wireframe);
            break;
        } case TA_EVENT_DEBUG_TOGGLE_NORMALS: {
            ta_camera *camera = ta_scene_find_by_id(tg_game.scene,
                RES_COMP_CAMERA, tg_game.camera_active_id);
            TOGGLE(camera->debug_normals);
            break;
        } case TA_EVENT_DEBUG_TOGGLE_BBOX: {
            ta_camera *camera = ta_scene_find_by_id(tg_game.scene,
                RES_COMP_CAMERA, tg_game.camera_active_id);
            TOGGLE(camera->debug_bounding_boxes);
            break;
        } case TA_EVENT_DEBUG_TOGGLE_MESH: {
            ta_camera *camera = ta_scene_find_by_id(tg_game.scene,
                RES_COMP_CAMERA, tg_game.camera_active_id);
            TOGGLE(camera->debug_no_mesh);
            break;
        } default: {
            handled = false;
        }
    }

    event->handled = handled;
}

void ta_game_hud_draw(ta_game *game)
{
    ta_entity *e_player = ta_scene_find_by_id(game->scene, RES_ENTITY,
        game->player_one_id);
    ta_player *player = ta_scene_entity_component(game->scene,
        e_player, RES_COMP_PLAYER);
    ta_entity *e_gun = ta_scene_find_by_id(game->scene, RES_ENTITY,
        player->gun_id);
    ta_gun *gun = ta_scene_entity_component(game->scene,
        e_gun, RES_COMP_GUN);

    // TODO: Remove x,y coords from init() methods and only store size. Pass x,y
    //       at render time (make sure to update viewport correctly).
    ta_ui_next_size(200, 40);
    ta_ui_window_begin(0, 0);
    ta_ui_row_begin();
    for (int i = 0; i < gun->carrying_ammo_max; i++) {
        ta_ui_next_size(20, 20);
        if (i < gun->carrying_ammo) {
            ta_ui_button(0, tg_game.tex_orange);
        } else {
            ta_ui_button(0, tg_game.tex_red);
        }
    }
    // TODO: Allow next_pad to work on rows, or introduce a panel here
    //ta_ui_pad(0, 4);
    ta_ui_row_begin();
    for (int i = 0; i < gun->loaded_ammo_max; i++) {
        ta_ui_next_size(20, 20);
        if (i < gun->loaded_ammo) {
            ta_ui_button(0, tg_game.tex_orange);
        } else {
            ta_ui_button(0, tg_game.tex_red);
        }
    }
    ta_ui_window_end();

    glDisable(GL_SCISSOR_TEST);
}