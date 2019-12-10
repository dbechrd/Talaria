#include "ta_audio.h"
#include "ta_button.h"
#include "ta_camera.h"
#include "ta_collider.h"
#include "ta_editor.h"
#include "ta_entity.h"
#include "ta_event.h"
#include "ta_font.h"
#include "ta_game.h"
#include "ta_keybind.h"
#include "ta_light.h"
#include "ta_log.h"
#include "ta_mouse.h"
#include "ta_player.h"
#include "ta_primitive.h"
#include "ta_rigid_body.h"
#include "ta_scene.h"
#include "ta_shader.h"
#include "ta_symbol.h"
#include "ta_timer.h"
#include "ta_ui.h"
#include "ta_window.h"
#include "dlb/dlb_vector.h"
#include "misc/gl3w.h"
#include "SDL/SDL.h"

// TODO: Ewwww globals
const char *tg_font;
const char *tg_tex_orange;
const char *tg_tex_red;
const char *tg_tex_audio_icon;

const char *tg_e_background_music;
const char *tg_e_freecam;
const char *tg_e_player_one;
const char *tg_e_active_camera;

enum {
    // Camera events
    GAME_COMMAND_CAMERA_MOVE_FORWARD,
    GAME_COMMAND_CAMERA_MOVE_BACKWARD,
    GAME_COMMAND_CAMERA_MOVE_RIGHT,
    GAME_COMMAND_CAMERA_MOVE_LEFT,
    GAME_COMMAND_CAMERA_MOVE_UP,
    GAME_COMMAND_CAMERA_MOVE_DOWN,
    GAME_COMMAND_CAMERA_ROTATE,

    // Game events
    GAME_COMMAND_FREE_CAM,
    GAME_COMMAND_PLAY,
    GAME_COMMAND_EDITOR,
    GAME_COMMAND_SHUTDOWN,

    // Player events
    GAME_COMMAND_PLAYER_MOVE_FORWARD,
    GAME_COMMAND_PLAYER_MOVE_BACKWARD,
    GAME_COMMAND_PLAYER_MOVE_RIGHT,
    GAME_COMMAND_PLAYER_MOVE_LEFT,
    GAME_COMMAND_PLAYER_JUMP,
    GAME_COMMAND_PLAYER_SHOOT,

    GAME_COMMAND_COUNT
};

typedef struct ta_game {
    ta_game_state state;
    ta_game_state state_prev;
    int simulate;  // -1 = on, 0 = off, 1+ = simulate N frames
    bool vsync;
    u64 frame_num;
    u64 sim_step;
    ta_keybind keybinds[TA_GAME_STATE_COUNT][GAME_COMMAND_COUNT];
    ta_scene scene;
} ta_game;

static ta_game game;

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

void ta_game_init()
{
    ta_log_write(&tg_debug_log, SRC_GAME, "Setting state to startup...\n");
    // TODO: What other startup states would be useful (e.g. LOADING_MESHES)?
    //       Could use this for a progress bar during load and better logging.
    //       Maybe also have JUMPING, CLIMBING, etc.? Could use bit flags to
    //       capture overall state as well (e.g. PLAYING, EDITING, etc.)
    ta_game_state_set(TA_GAME_STATE_STARTUP);

    ta_log_write(&tg_debug_log, SRC_GAME, "Initializing key binds\n");

    // TODO: Read keybinds from file
    //dlb_vec_reserve(tg_keybinds, 16);

    //--------------------------------------------------------------------------
    // PLAY
    ta_keybind_init1(&game.keybinds[TA_GAME_STATE_PLAY][GAME_COMMAND_SHUTDOWN],             TA_KEYBIND_RELEASE, SDL_SCANCODE_ESCAPE);
    ta_keybind_init1(&game.keybinds[TA_GAME_STATE_PLAY][GAME_COMMAND_FREE_CAM],             TA_KEYBIND_RELEASE, SDL_SCANCODE_X);
    ta_keybind_init1(&game.keybinds[TA_GAME_STATE_PLAY][GAME_COMMAND_EDITOR],               TA_KEYBIND_RELEASE, SDL_SCANCODE_GRAVE);
    ta_keybind_init1(&game.keybinds[TA_GAME_STATE_PLAY][GAME_COMMAND_PLAYER_MOVE_FORWARD],  TA_KEYBIND_HOLD,    SDL_SCANCODE_W);
    ta_keybind_init1(&game.keybinds[TA_GAME_STATE_PLAY][GAME_COMMAND_PLAYER_MOVE_BACKWARD], TA_KEYBIND_HOLD,    SDL_SCANCODE_S);
    ta_keybind_init1(&game.keybinds[TA_GAME_STATE_PLAY][GAME_COMMAND_PLAYER_MOVE_RIGHT],    TA_KEYBIND_HOLD,    SDL_SCANCODE_D);
    ta_keybind_init1(&game.keybinds[TA_GAME_STATE_PLAY][GAME_COMMAND_PLAYER_MOVE_LEFT],     TA_KEYBIND_HOLD,    SDL_SCANCODE_A);
    ta_keybind_init1(&game.keybinds[TA_GAME_STATE_PLAY][GAME_COMMAND_PLAYER_JUMP],          TA_KEYBIND_PRESS,   SDL_SCANCODE_SPACE);
    ta_keybind_init1(&game.keybinds[TA_GAME_STATE_PLAY][GAME_COMMAND_PLAYER_SHOOT],         TA_KEYBIND_PRESS,   SDL_SCANCODE_MOUSE_LEFT);
    ta_keybind_init1(&game.keybinds[TA_GAME_STATE_PLAY][DEBUG_EVENT_MOUSE_LOCK_TOGGLE],     TA_KEYBIND_PRESS,   SDL_SCANCODE_M);
    ta_keybind_init1(&game.keybinds[TA_GAME_STATE_PLAY][DEBUG_EVENT_TOGGLE_WIREFRAME],      TA_KEYBIND_PRESS,   SDL_SCANCODE_2);
    ta_keybind_init1(&game.keybinds[TA_GAME_STATE_PLAY][DEBUG_EVENT_TOGGLE_BBOX],           TA_KEYBIND_PRESS,   SDL_SCANCODE_3);
    ta_keybind_init1(&game.keybinds[TA_GAME_STATE_PLAY][DEBUG_EVENT_TOGGLE_NORMALS],        TA_KEYBIND_PRESS,   SDL_SCANCODE_4);
    ta_keybind_init1(&game.keybinds[TA_GAME_STATE_PLAY][DEBUG_EVENT_TOGGLE_MESH],           TA_KEYBIND_PRESS,   SDL_SCANCODE_5);

    //--------------------------------------------------------------------------
    // FREE_CAM
    // TODO: Move to camera->keybinds
    ta_keybind_init1(&game.keybinds[TA_GAME_STATE_FREE_CAM][GAME_COMMAND_SHUTDOWN],             TA_KEYBIND_RELEASE, SDL_SCANCODE_ESCAPE);
    ta_keybind_init1(&game.keybinds[TA_GAME_STATE_FREE_CAM][GAME_COMMAND_PLAY],                 TA_KEYBIND_RELEASE, SDL_SCANCODE_X);
    ta_keybind_init1(&game.keybinds[TA_GAME_STATE_FREE_CAM][GAME_COMMAND_EDITOR],               TA_KEYBIND_RELEASE, SDL_SCANCODE_GRAVE);
    ta_keybind_init1(&game.keybinds[TA_GAME_STATE_FREE_CAM][GAME_COMMAND_PLAYER_MOVE_FORWARD],  TA_KEYBIND_HOLD,    SDL_SCANCODE_UP);
    ta_keybind_init1(&game.keybinds[TA_GAME_STATE_FREE_CAM][GAME_COMMAND_PLAYER_MOVE_BACKWARD], TA_KEYBIND_HOLD,    SDL_SCANCODE_DOWN);
    ta_keybind_init1(&game.keybinds[TA_GAME_STATE_FREE_CAM][GAME_COMMAND_PLAYER_MOVE_RIGHT],    TA_KEYBIND_HOLD,    SDL_SCANCODE_RIGHT);
    ta_keybind_init1(&game.keybinds[TA_GAME_STATE_FREE_CAM][GAME_COMMAND_PLAYER_MOVE_LEFT],     TA_KEYBIND_HOLD,    SDL_SCANCODE_LEFT);
    ta_keybind_init1(&game.keybinds[TA_GAME_STATE_FREE_CAM][GAME_COMMAND_PLAYER_JUMP],          TA_KEYBIND_PRESS,   SDL_SCANCODE_J);
    ta_keybind_init1(&game.keybinds[TA_GAME_STATE_FREE_CAM][GAME_COMMAND_CAMERA_MOVE_FORWARD],  TA_KEYBIND_HOLD,    SDL_SCANCODE_W);
    ta_keybind_init1(&game.keybinds[TA_GAME_STATE_FREE_CAM][GAME_COMMAND_CAMERA_MOVE_BACKWARD], TA_KEYBIND_HOLD,    SDL_SCANCODE_S);
    ta_keybind_init1(&game.keybinds[TA_GAME_STATE_FREE_CAM][GAME_COMMAND_CAMERA_MOVE_RIGHT],    TA_KEYBIND_HOLD,    SDL_SCANCODE_D);
    ta_keybind_init1(&game.keybinds[TA_GAME_STATE_FREE_CAM][GAME_COMMAND_CAMERA_MOVE_LEFT],     TA_KEYBIND_HOLD,    SDL_SCANCODE_A);
    ta_keybind_init1(&game.keybinds[TA_GAME_STATE_FREE_CAM][GAME_COMMAND_CAMERA_MOVE_UP],       TA_KEYBIND_HOLD,    SDL_SCANCODE_E);
    ta_keybind_init1(&game.keybinds[TA_GAME_STATE_FREE_CAM][GAME_COMMAND_CAMERA_MOVE_DOWN],     TA_KEYBIND_HOLD,    SDL_SCANCODE_Q);
    ta_keybind_init1(&game.keybinds[TA_GAME_STATE_FREE_CAM][DEBUG_EVENT_MOUSE_LOCK],            TA_KEYBIND_PRESS,   SDL_SCANCODE_MOUSE_RIGHT);
    ta_keybind_init1(&game.keybinds[TA_GAME_STATE_FREE_CAM][DEBUG_EVENT_MOUSE_UNLOCK],          TA_KEYBIND_RELEASE, SDL_SCANCODE_MOUSE_RIGHT);
    ta_keybind_init1(&game.keybinds[TA_GAME_STATE_FREE_CAM][DEBUG_EVENT_MOUSE_LOCK_TOGGLE],     TA_KEYBIND_PRESS,   SDL_SCANCODE_M);
    ta_keybind_init1(&game.keybinds[TA_GAME_STATE_FREE_CAM][DEBUG_EVENT_TOGGLE_WIREFRAME],      TA_KEYBIND_PRESS,   SDL_SCANCODE_2);
    ta_keybind_init1(&game.keybinds[TA_GAME_STATE_FREE_CAM][DEBUG_EVENT_TOGGLE_BBOX],           TA_KEYBIND_PRESS,   SDL_SCANCODE_3);
    ta_keybind_init1(&game.keybinds[TA_GAME_STATE_FREE_CAM][DEBUG_EVENT_TOGGLE_NORMALS],        TA_KEYBIND_PRESS,   SDL_SCANCODE_4);
    ta_keybind_init1(&game.keybinds[TA_GAME_STATE_FREE_CAM][DEBUG_EVENT_TOGGLE_MESH],           TA_KEYBIND_PRESS,   SDL_SCANCODE_5);

    //--------------------------------------------------------------------------
    // Scene
    //--------------------------------------------------------------------------
    ta_log_write(&tg_debug_log, SRC_GAME, "Loading first scene...\n");
    ta_scene_load_file(&game.scene, "data/scene/scene1_gen.dml");

    //--------------------------------------------------------------------------
    // Simulation
    //--------------------------------------------------------------------------
    game.simulate = -1;

    // TODO: Find closest 8 lights and store them in tg_game.lights

    //--------------------------------------------------------------------------
    // Player
    //--------------------------------------------------------------------------
    // HACK: Find first entity with a player component, assume it's the player
    tg_e_player_one = SYM_ENTITY_PLAYER_ONE;
    DLB_ASSERT(tg_e_player_one);

    //--------------------------------------------------------------------------
    // Cameras
    //--------------------------------------------------------------------------
    // TODO: idk how best to find static resources other than by name. Maybe
    // have a lookup table in the scene file whose only purpose is to populate
    // the scene with ids of static objects (root node, free cam, player, etc.)?
    tg_e_freecam = SYM_ENTITY_FREECAM;
    DLB_ASSERT(tg_e_freecam);

    //--------------------------------------------------------------------------
    // Audio
    //--------------------------------------------------------------------------
    ta_audio_listener_set_volume(&tg_audio, 1.0f);
    ta_audio_listener_mute(&tg_audio);

    // TODO: Parent this node to the active player
    tg_e_background_music = SYM_ENTITY_BACKGROUND_MUSIC;
    DLB_ASSERT(tg_e_background_music);

    ta_audio_source *bg_music_src = ta_game_component(RES_COMP_AUDIO_SOURCE,
        tg_e_background_music);
    DLB_ASSERT(bg_music_src);
    ta_audio_source_play_loop(bg_music_src);

    //--------------------------------------------------------------------------
    // Textures
    //--------------------------------------------------------------------------
    tg_font            = INTERN("consola");
    tg_tex_orange      = INTERN("test_diff");
    tg_tex_red         = INTERN("test_mrao");
    tg_tex_audio_icon  = INTERN("audio_icon");

    //--------------------------------------------------------------------------
    // Shaders
    //--------------------------------------------------------------------------
    // TODO: Move these to shaders.dml, they're not scene-specific
    tg_shader_lines   = ta_game_by_name(RES_SHADER, INTERN("lines"));
    tg_shader_quads   = ta_game_by_name(RES_SHADER, INTERN("quads"));
    tg_shader_cubemap = ta_game_by_name(RES_SHADER, INTERN("cubemap"));
    tg_shader_shadow  = ta_game_by_name(RES_SHADER, INTERN("shadow"));
    DLB_ASSERT(tg_shader_lines);
    DLB_ASSERT(tg_shader_quads);
    DLB_ASSERT(tg_shader_cubemap);
    DLB_ASSERT(tg_shader_shadow);

#if _DEBUG
    ta_game_state_set(TA_GAME_STATE_FREE_CAM);
#else
    ta_game_state_set(TA_GAME_STATE_PLAY);
#endif
    ta_log_write(&tg_debug_log, SRC_GAME, "Active camera: %s\n", tg_e_active_camera);
    DLB_ASSERT(tg_e_active_camera);
}
ta_game_state ta_game_state_current()
{
    return game.state;
}
ta_game_state ta_game_state_prev()
{
    return game.state_prev;
}
void ta_game_state_set(ta_game_state state)
{
    if (state == game.state) {
        return;
    }

    game.state_prev = game.state;
    game.state = state;
    ta_log_write(&tg_debug_log, SRC_GAME, "State = %s\n", game_state_str(state));
    switch (game.state) {
        case TA_GAME_STATE_PLAY: {
            tg_e_active_camera = tg_e_player_one;
            ta_mouse_capture_set(true);
            break;
        } case TA_GAME_STATE_FREE_CAM: {
            ta_camera *freecam = ta_game_component(RES_COMP_CAMERA, tg_e_freecam);
            if (vec3_zero(freecam->position)) {
                ta_camera *active_cam = ta_game_component(RES_COMP_CAMERA, tg_e_active_camera);
                freecam->target_xform.position = active_cam->target_xform.position;
                freecam->position = freecam->target_xform.position;
            }
            tg_e_active_camera = tg_e_freecam;
            break;
        } case TA_GAME_STATE_EDITOR: {
            break;
        }
    }
}
void *ta_game_alloc(enum ta_resource_type type, const char *name)
{
    return ta_scene_alloc(&game.scene, type, name);
}
void ta_game_destroy(enum ta_resource_type type, const char *name)
{
    ta_scene_destroy(&game.scene, type, name);
}
// If not found, ASSERT
void *ta_game_by_name(ta_resource_type type, const char *name)
{
    return ta_scene_find_by_name(&game.scene, type, name);
}
// If not found, returns NULL
void *ta_game_by_name_try(ta_resource_type type, const char *name)
{
    return ta_scene_find_by_name_try(&game.scene, type, name);
}
// If not found, returns the first resource of the given type
void *ta_game_by_name_or_default(ta_resource_type type, const char *name)
{
    return ta_scene_find_by_name_or_default(&game.scene, type, name);
}
void *ta_game_component(ta_resource_type type, const char *entity)
{
    return ta_scene_component(&game.scene, type, entity);
}
void *ta_game_component_try(ta_resource_type type, const char *entity)
{
    return ta_scene_component_try(&game.scene, type, entity);
}
void *ta_game_resource_pool(ta_resource_type type)
{
    return game.scene.resource_data[type];
}
ta_camera *ta_game_camera()
{
    return ta_game_component(RES_COMP_CAMERA, tg_e_active_camera);
}
ta_player *ta_game_player()
{
    return ta_game_component(RES_COMP_PLAYER, tg_e_player_one);
}
void ta_game_sim_pause()
{
    game.simulate = 0;
}
void ta_game_sim_resume()
{
    game.simulate = -1;
}
void ta_game_sim_step_n_frames(int frames)
{
    DLB_ASSERT(frames > 0);
    game.simulate = frames;
}
bool ta_game_sim_running()
{
    return game.simulate != 0;
}
bool ta_game_sim_paused()
{
    return game.simulate == 0;
}
u64 ta_game_sim_step()
{
    return game.sim_step;
}
u64 ta_game_frame_num()
{
    return game.frame_num;
}
void ta_game_window_resize()
{
    // Update all cameras to new aspect ratio
    dlb_vec_each(ta_camera *, camera, ta_game_resource_pool(RES_COMP_CAMERA)) {
        if (!camera->ortho) {
            ta_camera_recalc_projection(camera);
        }
    }
}
static void game_draw_frame_info(u64 frame_num, double ms_frame_time,
    double ms_frame_delta, u64 sim_step)
{
    // Print frame time on the screen
    char frame_time_buf[256] = { 0 };
    int len = snprintf(CSTR(frame_time_buf),
        "Frame\n"
        "  count: %08llu\n"
        "  time : %5.2f ms\n"
        "  delta: %5.2f ms (v-sync: %s)\n"
        "Game\n"
        "  sim step: %08llu\n"
        "  state   : %s\n"
        "  prev    : %s\n"
        "Audio\n"
        "  master volume: %.2f%s",
        frame_num,
        ms_frame_time,
        ms_frame_delta,
        ta_window_vsync(tg_window) ? "On" : "Off",
        sim_step,
        game_state_str(ta_game_state_current()),
        game_state_str(ta_game_state_prev()),
        ta_audio_listener_get_volume(&tg_audio),
        ta_audio_listener_muted(&tg_audio) ? " (muted)" : ""
    );

    static ta_rect_uv *frame_time_rects = 0;
    ta_font *font = ta_game_by_name(RES_FONT, tg_font);
    ta_font_push_text(&frame_time_rects, font, frame_time_buf, len, true, 0, 0, 0, 0);
    dlb_vec_each(ta_rect_uv *, rect, frame_time_rects) {
        ta_primitive_push_rect_uv(&quads_queue, *rect, TA_COLOR_WHITE,
            0, true, false);
    }
    dlb_vec_zero(frame_time_rects);

    ta_shader *font_shader = ta_game_by_name(RES_SHADER, font->shader);
    ta_shader_set_mat4(font_shader, SYM_U_PROJ, &MAT4_IDENT);
    ta_shader_set_mat4(font_shader, SYM_U_VIEW, &MAT4_IDENT);
    ta_shader_set_mat4(font_shader, SYM_U_MODEL, &MAT4_IDENT);
    ta_font_render(quads_queue, font, SCREEN_WRAP_X(-310.0f), 0,
        UI_LAYER_HUD, true, true);
}
static void game_draw_hud()
{
    ta_player *player = ta_game_player();
    ta_gun *gun = ta_game_component(RES_COMP_GUN, player->e_gun);

    static ta_ui_window_state window = { 0 };
    ta_ui_window_begin(0, &window, TA_UI_AUTOSIZE);
    ta_ui_row_begin();
    for (u32 i = 0; i < gun->carrying_ammo_max; i++) {
        ta_ui_next_size(20, 20);
        ta_ui_next_pad(2, 2, 2, 2);
        if (i < gun->carrying_ammo) {
            ta_texture *tex = ta_game_by_name(RES_TEXTURE, tg_tex_orange);
            ta_ui_image(0, tex, 0);
        } else {
            ta_texture *tex = ta_game_by_name(RES_TEXTURE, tg_tex_red);
            ta_ui_image(0, tex, 0);
        }
    }
    //ta_ui_pad(0, 4);
    ta_ui_row_begin();
    for (u32 i = 0; i < gun->loaded_ammo_max; i++) {
        ta_ui_next_size(20, 20);
        ta_ui_next_pad(2, 2, 2, 2);
        if (i < gun->loaded_ammo) {
            ta_texture *tex = ta_game_by_name(RES_TEXTURE, tg_tex_orange);
            ta_ui_image(0, tex, 0);
        } else {
            ta_texture *tex = ta_game_by_name(RES_TEXTURE, tg_tex_red);
            ta_ui_image(0, tex, 0);
        }
    }
    ta_ui_window_end();
    ta_ui_render();
}
void ta_game_loop()
{
    ////////////////////////////////////////////////////////////////////////////
    // Main loop
    ////////////////////////////////////////////////////////////////////////////

    // TODO: Move this to DML (e.g. editor.dml)
    ta_camera minimap_camera = { 0 };
    minimap_camera.fov = 90.0f;
    minimap_camera.up = VEC3_NZ;
    minimap_camera.ortho = true;
    ta_camera_init(&minimap_camera);

    // Eric Catto - Soft Constraints (GDC 2011)
    // Semi-implicit Euler will eventually blow up if you take big time steps. A
    // general rule is to take at least 4 time steps per period of oscillation.
    // For example, if the oscillation frequency is 60Hz, then you shouldn’t
    // take time steps slower than 15Hz.
    //
    // Randy Gaul
    // https://gamedevelopment.tutsplus.com/series/how-to-create-a-custom-physics-engine--gamedev-12715
    const double ms_sim_dt = 20;             // fixed dt milliseconds
    const double sim_dt = ms_sim_dt / 1000;  // fixed dt seconds
    const double sim_max_steps = 0;          // max simulation steps per frame
    double ms_sim_t = 0;                     // current simulation time
    double ms_frame_accum = 0;

    double ms_frame_prev = 0;   // Last frame started
    double ms_frame_start;      // This frame started
    double ms_frame_delta;      // Total delta time (including v-sync)
    double ms_frame_time;       // Actual frame time before v-sync

    while (ta_game_state_current() != TA_GAME_STATE_SHUTDOWN) {
        ms_frame_start = ta_timer_elapsed_ms();
        ms_frame_delta = ms_frame_start - ms_frame_prev;
        ms_frame_prev = ms_frame_start;

        // Engine events
        ta_log_write(&tg_debug_log, SRC_GAME, " Handling events...\n");
        ta_event_events();

        ta_log_write(&tg_debug_log, SRC_GAME, " Accumulating...\n");
        if (sim_max_steps == 0) {
            // TODO: This *requires* vsync to work correctly!
            // If sim_max_steps == 0, assume we want lockstep physics
            ms_frame_accum = ms_sim_dt;
        } else {
            ms_frame_accum += ms_frame_delta;
            // Prevent spiral of death
            // NOTE: This breaks determinism when simulation is under duress
            if (ms_frame_accum > ms_sim_dt * sim_max_steps) {
                ta_log_write(&tg_debug_log, SRC_GAME,
                    "WARNING: Physics accumulator spiraling; truncating %f to %f\n",
                    ms_frame_accum, ms_sim_dt * sim_max_steps);
                ms_frame_accum = ms_sim_dt * sim_max_steps;
            }
        }

        ta_log_write(&tg_debug_log, SRC_GAME, " Finding components...\n");
        ta_camera *active_camera = ta_game_camera();
        ta_camera *player_cam = 0;
        ta_rigid_body *player_body = 0;

        if (ms_frame_accum >= ms_sim_dt) {
            ta_log_write(&tg_debug_log, SRC_GAME, " Finding sim components...\n");
            player_cam = ta_game_component(RES_COMP_CAMERA, tg_e_player_one);
            player_body = ta_game_component(RES_COMP_RIGID_BODY, tg_e_player_one);
        }

        while (ms_frame_accum >= ms_sim_dt) {
            ta_log_write(&tg_debug_log, SRC_GAME, " Sim step...\n");
            // Target player camera
            ta_camera_set_target_pos_absolute(player_cam,
                vec3_add(player_body->position, (ta_vec3) { 0.0f, 2.0f, 0.0f }));

            // Target minimap camera
            ta_vec3 minimap_camera_target_pos = active_camera->position;
            minimap_camera_target_pos.y += 50.0f;
            minimap_camera.focal_point = active_camera->position;
            ta_camera_set_target_pos_absolute(&minimap_camera,
                minimap_camera_target_pos);

            // Update cameras
            dlb_vec_each(ta_camera *, cam, ta_game_resource_pool(RES_COMP_CAMERA)) {
                ta_camera_update(cam, sim_dt);
            }

            if (game.simulate) {
                if (game.simulate > 0) {
                    game.simulate--;
                }
    #if 0
                ta_mat3 rotate_sun = mat3_rotate_z(1.0f);
                tg_game.sun->data.sun.direction =
                    mat3_mul_vec3(&rotate_sun, tg_game.sun->data.sun.direction);
    #endif

    #if 1
                // HACK: Make point light rotate in a circle
                static float light_deg = 0.0f;
                light_deg += 0.005f;
                if (light_deg >= 360.0f) light_deg = 0.0f;

                ta_light *lights = ta_game_resource_pool(RES_COMP_LIGHT);
                lights[1].position.x = cosf(light_deg) * 16.0f;
                lights[1].position.z = sinf(light_deg) * 16.0f;

                ta_audio_source *bg_source = ta_game_component_try(
                    RES_COMP_AUDIO_SOURCE, tg_e_background_music);
                alSourcei(bg_source->al_source_id, AL_SOURCE_RELATIVE, AL_FALSE);
    #if 1
                alSourcefv(bg_source->al_source_id, AL_POSITION, (float *)&lights[1].position);
    #else
                alSourcefv(bg_source->al_source_id, AL_POSITION, (float *)&VEC3_ZERO);
    #endif
                alSourcefv(bg_source->al_source_id, AL_VELOCITY, (float *)&VEC3_ZERO);

    #else
                // HACK: Make point light follow player camera
                lights[1]->position = tg_game.camera_player->position;
                // HACK: Make point light follow camera
                lights[1]->position = vec3_add(
                    tg_game.camera_freecam->position,
                    tg_game.camera_freecam->front
                );
    #endif

                // Update scene
                ta_scene_update(&game.scene, (float)sim_dt);
                game.sim_step++;
            }

            // TODO: Put this somewhere intelligent
            // Update audio listener position
            ta_vec3 fwd_up[2];
            fwd_up[0] = active_camera->front;
            fwd_up[1] = active_camera->up;
            alListenerfv(AL_ORIENTATION, (float *)fwd_up);
            alListenerfv(AL_POSITION, (float *)&active_camera->position);
            //alListenerfv(AL_VELOCITY, (float *)&tg_game.camera->velocity);

            ms_sim_t += ms_sim_dt;
            ms_frame_accum -= ms_sim_dt;
        }

        float sim_alpha = (float)(ms_frame_accum / ms_sim_dt);

        // Draw models
        ta_log_write(&tg_debug_log, SRC_GAME, " Shadow pass...\n");
        ta_scene_shadow_pass(&game.scene, tg_shader_shadow, sim_alpha);
        ta_log_write(&tg_debug_log, SRC_GAME, " Render pass...\n");
        ta_scene_render(&game.scene, active_camera, sim_alpha);

        // World axes
        ta_primitive_push_axes(1.0f);
        ta_primitive_render(true, true);

        if (game.state == TA_GAME_STATE_EDITOR) {
            ta_log_write(&tg_debug_log, SRC_GAME, " Editor pass...\n");
            ta_editor_draw(sim_alpha);
        }

        // Cursor
        glClear(GL_DEPTH_BUFFER_BIT);
        ta_primitive_push_crosshair(10, 2);

    #if 0
        // Render minimap
        ta_rect map_rect = { 10, 50, 200, 200 };
        ta_viewport_bind(map_rect, TA_COLOR_GRAY7, true);
        ta_scene_render(tg_game.scene, &minimap_camera, sim_alpha);
        ta_viewport_unbind();
        ta_primitive_render(true, true);

        // Red dot on map
        int dot_radius = 2;
        ta_rect dot_rect = { 0 };
        dot_rect.x = map_rect.x + map_rect.w / 2 - dot_radius;
        dot_rect.y = map_rect.y + map_rect.h / 2 - dot_radius;
        dot_rect.w = dot_radius * 2;
        dot_rect.h = dot_radius * 2;
        ta_primitive_push_rect(dot_rect, TA_COLOR_RED, UI_LAYER_HUD);
        ta_primitive_render(true, true);
    #endif

    #if 0
        // TODO: Mesh selector, highlight and rotate mesh while mouse hover
        //ta_mat4 model = mat4_rotate_y(model_deg);
        //model_deg += 1.0f;
        //if (model_deg >= 360.0f) {
        //    model_deg = 0.0f;
        //}

        // Barchart
        ta_shader_set_mat4(tg_shader_lines, SYM_U_PROJ, &MAT4_IDENT);
        ta_shader_set_mat4(tg_shader_lines, SYM_U_VIEW, &MAT4_IDENT);
        ta_shader_set_mat4(tg_shader_lines, SYM_U_MODEL, &MAT4_IDENT);
        ta_ui_barchart_draw(0, 0, &chart);
        ta_primitive_render();
        ta_primitive_clear();
    #endif

        //debug_nametag(active_cam);
        // TODO: Make HUD drawing suck less.. way too many draw calls
        //       Use texture atlas, batch everything into one draw call. Import
        //       textures from Rico; stop using stupid RGB placeholders
        ta_log_write(&tg_debug_log, SRC_GAME, " HUD pass...\n");
        //ta_game_hud_draw(&tg_game);

        ms_frame_time = ta_timer_elapsed_ms() - ms_frame_start;
        game.frame_num++;
        ta_log_write(&tg_debug_log, SRC_GAME, " FPS pass...\n");
        game_draw_frame_info(game.frame_num, ms_frame_time, ms_frame_delta,
            game.sim_step);

        // NOTE: This confirms rendering is being deferred until swap buffers,
        // but it's much slower (~5ms), so don't actually use it.
        //ta_log_write(&tg_debug_log, SRC_GAME, " glFinish...\n");
        //glFinish();

        ta_log_write(&tg_debug_log, SRC_GAME, " Swap...\n");
        ta_window_swap(tg_window);

        ta_log_write(&tg_debug_log, SRC_GAME,
            "Frame %llu displayed. time: %5.3f delta: %5.3f\n",
            game.frame_num, ms_frame_time, ms_frame_delta);

        if (ms_frame_time > 16) {
            // TODO: Debug more long frames (turn on SRC_GAME logging)
            ta_log_write(&tg_debug_log, SRC_GAME, "!!!!!!!! LONG_FRAME !!!!!!!!\n");
            ta_log_flush(&tg_debug_log);
            //__debugbreak();
        }
    }
}
static void game_player_shoot()
{
    static double last_bang_ms = 0;
    static double last_reload_ms = 0;
    static double last_empty_ms = 0;

    ta_player *player = ta_game_player();
    ta_gun *gun = ta_game_component(RES_COMP_GUN, player->e_gun);
    ta_audio_source *src_gun = ta_game_component(RES_COMP_AUDIO_SOURCE,
        player->e_gun);

    double now_ms = ta_timer_elapsed_ms();

    if (gun->loaded_ammo > 0) {
        static double after_bang_delay_ms = 150;
        static double after_reload_delay_ms = 1000;
        if (now_ms < last_bang_ms + after_bang_delay_ms ||
            now_ms < last_reload_ms + after_reload_delay_ms) {
            return;
        }

        ta_audio_source_play_name(src_gun, gun->sfx_bang);
        last_bang_ms = ta_timer_elapsed_ms();
        gun->loaded_ammo--;
    } else {
        if (gun->carrying_ammo) {
            static double after_bang_delay_ms = 750;
            if (now_ms < last_bang_ms + after_bang_delay_ms) {
                return;
            }

            ta_audio_source_play_name(src_gun, gun->sfx_reload);
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

            ta_audio_source_play_name(src_gun, gun->sfx_empty);
            last_empty_ms = ta_timer_elapsed_ms();
        }
    }
}
void ta_game_hotkeys()
{
    for (u32 command = 0; command < GAME_COMMAND_COUNT; ++command) {
        ta_keybind_update(&game.keybinds[game.state][command]);
    }

    if (ta_keybind_triggered(&game.keybinds[game.state][GAME_COMMAND_PLAY])) {
        ta_game_state_set(TA_GAME_STATE_PLAY);
    } else if (ta_keybind_triggered(&game.keybinds[game.state][GAME_COMMAND_FREE_CAM])) {
        ta_game_state_set(TA_GAME_STATE_FREE_CAM);
    } else if (ta_keybind_triggered(&game.keybinds[game.state][GAME_COMMAND_EDITOR])) {
        ta_game_state_set(TA_GAME_STATE_EDITOR);
    } else if (ta_keybind_triggered(&game.keybinds[game.state][GAME_COMMAND_SHUTDOWN])) {
        ta_game_state_set(TA_GAME_STATE_SHUTDOWN);
    } else if (ta_keybind_triggered(&game.keybinds[game.state][GAME_COMMAND_PLAYER_MOVE_FORWARD])) {
        ta_camera *camera = ta_game_camera();
        ta_vec3 dir = { 0 };
        dir.x = camera->front.x;
        dir.z = camera->front.z;
        dir = vec3_normalize(dir);
        dir = vec3_scalef(dir, 0.1f);
        ta_rigid_body *player_body = ta_game_component(RES_COMP_RIGID_BODY,
            tg_e_player_one);
        ta_rigid_body_apply_impulse(player_body, dir, VEC3_ZERO);
    } else if (ta_keybind_triggered(&game.keybinds[game.state][GAME_COMMAND_PLAYER_MOVE_BACKWARD])) {
        ta_camera *camera = ta_game_camera();
        ta_vec3 dir = { 0 };
        dir.x = -camera->front.x;
        dir.z = -camera->front.z;
        dir = vec3_normalize(dir);
        dir = vec3_scalef(dir, 0.1f);
        ta_rigid_body *player_body = ta_game_component(RES_COMP_RIGID_BODY,
            tg_e_player_one);
        ta_rigid_body_apply_impulse(player_body, dir, VEC3_ZERO);
    } else if (ta_keybind_triggered(&game.keybinds[game.state][GAME_COMMAND_PLAYER_MOVE_RIGHT])) {
        ta_camera *camera = ta_game_camera();
        ta_vec3 dir = { 0 };
        dir.x = camera->right.x;
        dir.z = camera->right.z;
        dir = vec3_normalize(dir);
        dir = vec3_scalef(dir, 0.1f);
        ta_rigid_body *player_body = ta_game_component(RES_COMP_RIGID_BODY,
            tg_e_player_one);
        ta_rigid_body_apply_impulse(player_body, dir, VEC3_ZERO);
    } else if (ta_keybind_triggered(&game.keybinds[game.state][GAME_COMMAND_PLAYER_MOVE_LEFT])) {
        ta_camera *camera = ta_game_camera();
        ta_vec3 dir = { 0 };
        dir.x = -camera->right.x;
        dir.z = -camera->right.z;
        dir = vec3_normalize(dir);
        dir = vec3_scalef(dir, 0.1f);
        ta_rigid_body *player_body = ta_game_component(RES_COMP_RIGID_BODY,
            tg_e_player_one);
        ta_rigid_body_apply_impulse(player_body, dir, VEC3_ZERO);
    } else if (ta_keybind_triggered(&game.keybinds[game.state][GAME_COMMAND_PLAYER_JUMP])) {
        ta_vec3 dir = VEC3_Y;
        dir = vec3_scalef(dir, 5.0f);
        ta_rigid_body *player_body = ta_game_component(RES_COMP_RIGID_BODY,
            tg_e_player_one);
        ta_rigid_body_apply_impulse(player_body, dir, VEC3_ZERO);
    } else if (ta_keybind_triggered(&game.keybinds[game.state][GAME_COMMAND_PLAYER_SHOOT])) {
        game_player_shoot(game);
    } else if (ta_keybind_triggered(&game.keybinds[game.state][GAME_COMMAND_CAMERA_MOVE_FORWARD])) {
        if (ta_mouse_captured()) {
            ta_camera *camera = ta_game_camera();
            camera->move_buffer = vec3_add(camera->move_buffer, camera->front);
        }
    } else if (ta_keybind_triggered(&game.keybinds[game.state][GAME_COMMAND_CAMERA_MOVE_BACKWARD])) {
        if (ta_mouse_captured()) {
            ta_camera *camera = ta_game_camera();
            camera->move_buffer = vec3_sub(camera->move_buffer, camera->front);
        }
    } else if (ta_keybind_triggered(&game.keybinds[game.state][GAME_COMMAND_CAMERA_MOVE_RIGHT])) {
        if (ta_mouse_captured()) {
            ta_camera *camera = ta_game_camera();
            camera->move_buffer = vec3_add(camera->move_buffer, camera->right);
        }
    } else if (ta_keybind_triggered(&game.keybinds[game.state][GAME_COMMAND_CAMERA_MOVE_LEFT])) {
        if (ta_mouse_captured()) {
            ta_camera *camera = ta_game_camera();
            camera->move_buffer = vec3_sub(camera->move_buffer, camera->right);
        }
    } else if (ta_keybind_triggered(&game.keybinds[game.state][GAME_COMMAND_CAMERA_MOVE_UP])) {
        if (ta_mouse_captured()) {
            ta_camera *camera = ta_game_camera();
            camera->move_buffer = vec3_add(camera->move_buffer, camera->up);
        }
    } else if (ta_keybind_triggered(&game.keybinds[game.state][GAME_COMMAND_CAMERA_MOVE_DOWN])) {
        if (ta_mouse_captured()) {
            ta_camera *camera = ta_game_camera();
            camera->move_buffer = vec3_sub(camera->move_buffer, camera->up);
        }
    }
}
void ta_game_event(ta_event *event)
{
    bool handled = true;

    switch (event->type) {
        case INPUT_EVENT_MOUSE_MOVE: {
            if (ta_mouse_captured()) {
                ta_event cam_rotate_evt = { 0 };
                cam_rotate_evt.type = GAME_EVENT_CAMERA_ROTATE;
                if (event->data.mouse_move.dx) {
                    cam_rotate_evt.data.camera_rotate.delta_yaw =
                        (float)-event->data.mouse_move.dx;
                }
                if (event->data.mouse_move.dy) {
                    cam_rotate_evt.data.camera_rotate.delta_pitch =
                        (float)-event->data.mouse_move.dy;
                }
                ta_event_push(&cam_rotate_evt);
            }
            break;
        } case GAME_EVENT_CAMERA_ROTATE: {
            ta_camera *camera = ta_game_camera();
            if (event->data.camera_rotate.delta_yaw) {
                ta_camera_yaw(camera, event->data.camera_rotate.delta_yaw);
            }
            if (event->data.camera_rotate.delta_pitch) {
                ta_camera_pitch(camera, event->data.camera_rotate.delta_pitch);
            }
            break;
        } case GAME_EVENT_BUTTON_ACTIVATED: {
            // TODO: Check which button was activated:
            //event->data.button.button_name;
            ta_player *player = ta_game_player();
            ta_gun *gun = ta_game_component(RES_COMP_GUN, player->e_gun);
            if (gun->carrying_ammo == 0 && gun->loaded_ammo == 0) {
                gun->carrying_ammo = gun->carrying_ammo_max;
            }

#if 0
            // TODO: Should audio source subscribe to this event somehow,
            //       or should the button queue the play request itself?
            e_button *button =
                ta_scene_find(game.scene, TYP_BUTTON, event->data.button.button_uid);
            ta_audio_buffer *buffer = e_button_sfx_activated(button);
            if (buffer) {
                ta_audio_source *source = e_button_audio_source(button);
                ta_audio_source_set_buffer(source, buffer);
                ta_audio_source_play(source);
            }
#endif
#if 0
        // TODO: Put these somewhere more global since they apply to edit mode as well
        } case DEBUG_EVENT_MOUSE_UNLOCK: {
            ta_mouse_capture_set(false);
            break;
        } case DEBUG_EVENT_MOUSE_LOCK_TOGGLE: {
            ta_mouse_capture_toggle();
            break;
        } case DEBUG_EVENT_TOGGLE_WIREFRAME: {
            ta_camera *camera = ta_game_camera();
            TOGGLE(camera->debug_wireframe);
            break;
        } case DEBUG_EVENT_TOGGLE_NORMALS: {
            ta_camera *camera = ta_game_camera();
            TOGGLE(camera->debug_normals);
            break;
        } case DEBUG_EVENT_TOGGLE_BBOX: {
            ta_camera *camera = ta_game_camera();
            TOGGLE(camera->debug_bounding_boxes);
            break;
        } case DEBUG_EVENT_TOGGLE_MESH: {
            ta_camera *camera = ta_game_camera();
            TOGGLE(camera->debug_no_mesh);
            break;
        } case DEBUG_EVENT_MOUSE_LOCK: {
                ta_mouse_capture_set(true);
                break;
#endif
        } default: {
            handled = false;
        }
    }

    event->handled = handled;
}
void ta_game_save()
{
    // TODO: Back up original save file before overwriting, handle errors
    ta_scene_save_file(&game.scene, "data/scene/scene1_gen.dml");
}