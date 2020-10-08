#pragma once
#include "ta_asset_watcher.h"
#include "ta_camera.h"
#include "ta_light.h"
#include "ta_scene.h"
#include "ta_texture.h"
#include "dlb/dlb_types.h"

struct ta_event;
struct ta_player;
struct ta_ray;
enum ta_res_type;

extern const char *tg_font;
extern const char *tg_tex_orange;
extern const char *tg_tex_red;
extern const char *tg_tex_audio_icon;
extern const char *tg_e_background_music;
extern const char *tg_e_freecam;
extern const char *tg_e_player_one;
extern const char *tg_e_active_camera;
extern float tg_hard_morph;

typedef enum ta_game_state {
    TA_STATE_STARTUP  = 1 << 0,
    TA_STATE_PLAY     = 1 << 1,
    TA_STATE_FREE_CAM = 1 << 2,
    TA_STATE_EDITOR   = 1 << 3,
    TA_STATE_SHUTDOWN = 1 << 4,
} ta_game_state;
const char *game_state_str(ta_game_state state);

typedef enum ta_command {
    // Game events
    COMMAND_PLAY,
    COMMAND_FREE_CAM,
    COMMAND_CONSOLE_TOGGLE,
    COMMAND_CONSOLE_HIDE,
    COMMAND_EDITOR,
    COMMAND_SHUTDOWN,
    COMMAND_TOGGLE_FULLSCREEN,

    // Player events
    COMMAND_PLAYER_MOVE_FORWARD,
    COMMAND_PLAYER_MOVE_BACKWARD,
    COMMAND_PLAYER_MOVE_RIGHT,
    COMMAND_PLAYER_MOVE_LEFT,
    COMMAND_PLAYER_JUMP,
    COMMAND_PLAYER_SHOOT,

    // Camera events
    COMMAND_CAMERA_MOVE_FORWARD,
    COMMAND_CAMERA_MOVE_BACKWARD,
    COMMAND_CAMERA_MOVE_RIGHT,
    COMMAND_CAMERA_MOVE_LEFT,
    COMMAND_CAMERA_MOVE_UP,
    COMMAND_CAMERA_MOVE_DOWN,

    // Debug events
    COMMAND_DEBUG_MOUSE_LOCK,
    COMMAND_DEBUG_MOUSE_UNLOCK,
    COMMAND_DEBUG_MOUSE_LOCK_TOGGLE,
    COMMAND_DEBUG_TOGGLE_WIREFRAME,
    COMMAND_DEBUG_TOGGLE_MESH,
    COMMAND_DEBUG_TOGGLE_COLLIDERS,
    COMMAND_DEBUG_TOGGLE_NAMETAGS,
    COMMAND_DEBUG_TOGGLE_NORMALS,

    // Editor commands
    COMMAND_EDITOR_SELECT,
    COMMAND_EDITOR_SELECT_RELEASE,
    COMMAND_EDITOR_CANCEL,
    COMMAND_EDITOR_CLOSE,
    COMMAND_EDITOR_SIM_PAUSE_RESUME,
    COMMAND_EDITOR_SIM_NEXT,
    COMMAND_EDITOR_SIM_NEXT_10,
    COMMAND_EDITOR_SIM_WHILE_HELD,

    COMMAND_COUNT
} ta_command;

typedef struct ta_game {
    ta_game_state state;        // current game state
    u64 frame_num;              // current frame number
    int simulate;               // physics sim: -1 = on, 0 = off, 1+ = simulate N frames
    u64 sim_step;               // current simulation step
    ta_scene scene;             // active scene
    ta_texturing texturing;     // texture manager
    ta_lighting lighting;       // active lighting
    ta_camera minimap_camera;   // HACK: Just having fun..     // TODO(cleanup): Move this to DML?
    struct ta_keybind *keybinds;
    bool console_visible;
    const char *base_path;      // symbol (working dir in debug mode, otherwise the .exe directory)
    ta_asset_watcher texture_watcher;

    bool debug_wireframe;       // [DEBUG] render everything from as wireframe when using this camera
    bool debug_normals;         // [DEBUG] render normals for every mesh in the world
    bool debug_colliders;       // [DEBUG] render colliders for every rigid_body in the world
    bool debug_nametags;        // [DEBUG] render name tags for every transform in the world
    bool debug_no_mesh;         // [DEBUG] disable mesh rendering for every mesh in the world

    bool debug_physics_render_penetration_vectors;
    bool debug_physics_render_static_friction_vectors;
    bool debug_physics_render_dynamic_friction_vectors;
    bool debug_physics_render_damping_vectors;
    bool debug_physics_render_restitution_vectors;

    ta_rgba debug_physics_color_penetration_vectors;
    ta_rgba debug_physics_color_static_friction_vectors;
    ta_rgba debug_physics_color_dynamic_friction_vectors;
    ta_rgba debug_physics_color_damping_vectors;
    ta_rgba debug_physics_color_restitution_vectors;

    // HACK: Temp data, need to persist between frames for debug rendering to work when sim is paused
    // TODO(cleanup): Holding pointers across frames is a _BAD IDEA_. At the very least, hold names instead.
    struct ta_rigid_body_pair *pairs;  // Array of most recent broadphase pairs
    struct ta_manifold *manifolds;     // Array of most recent collision manifolds
} ta_game;

extern ta_game tg_game;

const char *ta_command_str(ta_command cmd);

void ta_game_init                           ();
ta_game_state ta_game_state_current         ();
void ta_game_state_set                      (ta_game_state state);
void *ta_game_alloc                         (enum ta_res_type type, const char *name, size_t name_len);
void ta_game_destroy                        (enum ta_res_type type, const char *name, size_t name_len);
void *ta_game_by_name                       (enum ta_res_type type, const char *name, size_t name_len);
void *ta_game_by_name_try                   (enum ta_res_type type, const char *name, size_t name_len);
void *ta_game_by_name_or_default            (enum ta_res_type type, const char *name, size_t name_len);
void *ta_game_by_sym                        (enum ta_res_type type, const char *sym);
void *ta_game_by_sym_try                    (enum ta_res_type type, const char *sym);
void *ta_game_by_sym_or_default             (enum ta_res_type type, const char *sym);
void *ta_game_component_add                 (const char *entity, enum ta_res_type type, const char *name, size_t name_len);
void *ta_game_component                     (const char *entity, enum ta_res_type type);
void *ta_game_component_try                 (const char *entity, enum ta_res_type type);
void *ta_game_resource_pool                 (enum ta_res_type type);
void ta_game_load_gltf                      (const char *filename);
struct ta_camera *ta_game_camera            ();
struct ta_ray ta_game_camera_ray            ();
struct ta_ray ta_game_mouse_ray             ();
struct ta_player *ta_game_player            ();
void ta_game_sim_pause                      ();
void ta_game_sim_resume                     ();
void ta_game_sim_step_n_frames              (int frames);
bool ta_game_sim_running                    ();
bool ta_game_sim_paused                     ();
u64 ta_game_sim_step                        ();
u64 ta_game_frame_num                       ();
void ta_game_window_resize                  ();
void ta_game_loop                           ();
void ta_game_update_keybinds                ();
void ta_game_event                          (struct ta_event *event);
void ta_game_save                           ();