#include "ta_asset_watcher.h"
#include "ta_audio.h"
#include "ta_button.h"
#include "ta_camera.h"
#include "ta_collider.h"
#include "ta_console.h"
#include "ta_editor.h"
#include "ta_event.h"
#include "ta_font.h"
#include "ta_game.h"
#include "ta_gltf.h"
#include "ta_intersect.h"
#include "ta_keybind.h"
#include "ta_light.h"
#include "ta_log.h"
#include "ta_material.h"
#include "ta_mesh.h"
#include "ta_model.h"
#include "ta_mouse.h"
#include "ta_ogx.h"
#include "ta_ogx_parser.h"
#include "ta_player.h"
#include "ta_primitive.h"
#include "ta_rigid_body.h"
#include "ta_scene.h"
#include "ta_shader.h"
#include "ta_support.h"
#include "ta_symbol.h"
#include "ta_timer.h"
#include "ta_transform.h"
#include "ta_ui.h"
#include "ta_ui_barchart.h"
#include "ta_viewport.h"
#include "ta_window.h"
#include "dlb/dlb_vector.h"
#include "dlb/dlb_rand.h"
#include "misc/stb_image.h"
#include "misc/glad.h"
#include "SDL/SDL.h"

// TODO: Ewwww globals
const char *tg_font;
const char *tg_tex_bullet_icon;
const char *tg_tex_bullet_icon_gray;
const char *tg_tex_audio_icon;

// Default textures
const char *tg_tex_invalid_albedo;      // magenta/white checkerboard
const char *tg_tex_default_albedo;      // vec4(1.0)
const char *tg_tex_default_emission;    // vec3(1.0)
const char *tg_tex_default_height;      // 0.0
const char *tg_tex_default_metallic;    // 0.0
const char *tg_tex_default_normal;      // vec3(0.5, 0.5, 1.0)
const char *tg_tex_default_occlusion;   // 1.0
const char *tg_tex_default_roughness;   // 0.5

const char *tg_e_background_music;
const char *tg_e_freecam;
const char *tg_e_player_camera;
const char *tg_e_player_one;
const char *tg_e_can;
const char *tg_e_ground;

const char *tg_mesh_prim_sphere;

ta_game tg_game;

static const float player_move_impulse = 0.1f;
static const float player_jump_impulse = 8.0f;

const char *game_state_str(ta_game_state state)
{
    switch (state) {
        case TA_STATE_STARTUP:  return "TA_STATE_STARTUP";
        case TA_STATE_PLAY:     return "TA_STATE_PLAY";
        case TA_STATE_FREE_CAM: return "TA_STATE_FREE_CAM";
        case TA_STATE_EDITOR:   return "TA_STATE_EDITOR";
        case TA_STATE_SHUTDOWN: return "TA_STATE_SHUTDOWN";
        default: DLB_ASSERT(0); return "TA_STATE_???";
    }
};

const char *ta_command_str(ta_command cmd)
{
    switch (cmd) {
        // Game events
        case COMMAND_PLAY                    : return "COMMAND_PLAY";
        case COMMAND_FREE_CAM                : return "COMMAND_FREE_CAM";
        case COMMAND_CONSOLE_TOGGLE          : return "COMMAND_CONSOLE_TOGGLE";
        case COMMAND_CONSOLE_HIDE            : return "COMMAND_CONSOLE_HIDE";
        case COMMAND_EDITOR                  : return "COMMAND_EDITOR";
        case COMMAND_SHUTDOWN                : return "COMMAND_SHUTDOWN";
        case COMMAND_TOGGLE_FULLSCREEN       : return "COMMAND_TOGGLE_FULLSCREEN";
        // Player events
        case COMMAND_PLAYER_MOVE_FORWARD     : return "COMMAND_PLAYER_MOVE_FORWARD";
        case COMMAND_PLAYER_MOVE_BACKWARD    : return "COMMAND_PLAYER_MOVE_BACKWARD";
        case COMMAND_PLAYER_MOVE_RIGHT       : return "COMMAND_PLAYER_MOVE_RIGHT";
        case COMMAND_PLAYER_MOVE_LEFT        : return "COMMAND_PLAYER_MOVE_LEFT";
        case COMMAND_PLAYER_JUMP             : return "COMMAND_PLAYER_JUMP";
        case COMMAND_PLAYER_SHOOT            : return "COMMAND_PLAYER_SHOOT";
        // Camera events
        case COMMAND_CAMERA_MOVE_FORWARD     : return "COMMAND_CAMERA_MOVE_FORWARD";
        case COMMAND_CAMERA_MOVE_BACKWARD    : return "COMMAND_CAMERA_MOVE_BACKWARD";
        case COMMAND_CAMERA_MOVE_RIGHT       : return "COMMAND_CAMERA_MOVE_RIGHT";
        case COMMAND_CAMERA_MOVE_LEFT        : return "COMMAND_CAMERA_MOVE_LEFT";
        case COMMAND_CAMERA_MOVE_UP          : return "COMMAND_CAMERA_MOVE_UP";
        case COMMAND_CAMERA_MOVE_DOWN        : return "COMMAND_CAMERA_MOVE_DOWN";
        // Debug events
        case COMMAND_DEBUG_MOUSE_LOCK        : return "COMMAND_DEBUG_MOUSE_LOCK";
        case COMMAND_DEBUG_MOUSE_UNLOCK      : return "COMMAND_DEBUG_MOUSE_UNLOCK";
        case COMMAND_DEBUG_MOUSE_LOCK_TOGGLE : return "COMMAND_DEBUG_MOUSE_LOCK_TOGGLE";
        case COMMAND_DEBUG_TOGGLE_WIREFRAME  : return "COMMAND_DEBUG_TOGGLE_WIREFRAME";
        case COMMAND_DEBUG_TOGGLE_MESH       : return "COMMAND_DEBUG_TOGGLE_MESH";
        case COMMAND_DEBUG_TOGGLE_COLLIDERS  : return "COMMAND_DEBUG_TOGGLE_COLLIDERS";
        case COMMAND_DEBUG_TOGGLE_NAMETAGS   : return "COMMAND_DEBUG_TOGGLE_NAMETAGS";
        case COMMAND_DEBUG_TOGGLE_NORMALS    : return "COMMAND_DEBUG_TOGGLE_NORMALS";
        // Editor commands
        case COMMAND_EDITOR_SELECT           : return "COMMAND_EDITOR_SELECT";
        case COMMAND_EDITOR_SELECT_RELEASE   : return "COMMAND_EDITOR_SELECT_RELEASE";
        case COMMAND_EDITOR_CANCEL           : return "COMMAND_EDITOR_CANCEL";
        case COMMAND_EDITOR_CLOSE            : return "COMMAND_EDITOR_CLOSE";
        case COMMAND_EDITOR_SIM_PAUSE_RESUME : return "COMMAND_EDITOR_SIM_PAUSE_RESUME";
        case COMMAND_EDITOR_SIM_NEXT         : return "COMMAND_EDITOR_SIM_NEXT";
        case COMMAND_EDITOR_SIM_NEXT_10      : return "COMMAND_EDITOR_SIM_NEXT_10";
        case COMMAND_EDITOR_SIM_WHILE_HELD   : return "COMMAND_EDITOR_SIM_WHILE_HELD";
        default: DLB_ASSERT(0);                return "COMMAND_???";
    }
}

void ta_game_init()
{
    ta_log_write(&tg_debug_log, SRC_GAME, "Setting state to startup...\n");
    // TODO: What other startup states would be useful (e.g. LOADING_MESHES)?
    //       Could use this for a progress bar during load and better logging.
    //       Maybe also have JUMPING, CLIMBING, etc.? Could use bit flags to
    //       capture overall state as well (e.g. PLAYING, EDITING, etc.)
    ta_game_state_set(TA_STATE_STARTUP);

    TracyCZoneN(ctxGetBasePath, "SDL_GetBasePath", true);
    ta_log_write(&tg_debug_log, SRC_GAME, "Determining base path...\n");
#if _DEBUG
    char buf[512] = { 0 };
    DWORD len = GetCurrentDirectoryA(sizeof(buf), buf);
    DLB_ASSERT(len < sizeof(buf) - 2);
    buf[len] = '\\';
    len++;
    dlb_str_replace_char(buf, '\\', '/');
    tg_game.base_path = ta_symbol_intern(buf, (size_t)len);
#else
    char *buf = SDL_GetBasePath();
    size_t len = strlen(buf);
    for (size_t i = 0; i < len; ++i) {
        if (buf[i] == '\\') buf[i] = '/';
    }
    tg_game.base_path = ta_symbol_intern(buf, len);
    SDL_free(buf);
#endif
    ta_log_write(&tg_debug_log, SRC_GAME, "Base path: %s\n", tg_game.base_path);
    TracyCZoneEnd(ctxGetBasePath);

    TracyCZoneN(ctxLoadKeybinds, "Loading keybinds", true);
    ta_log_write(&tg_debug_log, SRC_GAME, "Loading keybinds\n");

    // TODO: Read keybinds from file
    //dlb_vec_reserve(tg_keybinds, 16);

    // TODO: None of these are going to respect the user's OS key repeat settings. Maybe we should check if keybinds
    // are active via switch in event handlers for _everything_. We can_body still have remappable keys and undoable command
    // indirection and don't have to worry about handling repeat anymore.
    //-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    //                                  Command                          Game States                                          Triggers            Keys
    ta_keybind_init1(&tg_game.keybinds, COMMAND_PLAY                   ,                 TA_STATE_FREE_CAM                  , TA_KEYBIND_PRESS,   SDL_SCANCODE_X);
    ta_keybind_init1(&tg_game.keybinds, COMMAND_FREE_CAM               , TA_STATE_PLAY                                      , TA_KEYBIND_PRESS,   SDL_SCANCODE_X);
    ta_keybind_init2(&tg_game.keybinds, COMMAND_CONSOLE_TOGGLE         , TA_STATE_PLAY | TA_STATE_FREE_CAM | TA_STATE_EDITOR, TA_KEYBIND_PRESS,   SDL_SCANCODE_GRAVE, SDL_SCANCODE_LSHIFT);
    ta_keybind_init1(&tg_game.keybinds, COMMAND_CONSOLE_HIDE           , TA_STATE_PLAY | TA_STATE_FREE_CAM | TA_STATE_EDITOR, TA_KEYBIND_PRESS,   SDL_SCANCODE_ESCAPE);
    ta_keybind_init1(&tg_game.keybinds, COMMAND_EDITOR                 , TA_STATE_PLAY | TA_STATE_FREE_CAM                  , TA_KEYBIND_PRESS,   SDL_SCANCODE_GRAVE);
    ta_keybind_init1(&tg_game.keybinds, COMMAND_SHUTDOWN               , TA_STATE_PLAY | TA_STATE_FREE_CAM                  , TA_KEYBIND_PRESS,   SDL_SCANCODE_ESCAPE);
    ta_keybind_init1(&tg_game.keybinds, COMMAND_TOGGLE_FULLSCREEN      , TA_STATE_PLAY | TA_STATE_FREE_CAM | TA_STATE_EDITOR, TA_KEYBIND_PRESS,   SDL_SCANCODE_F11);
    ta_keybind_init1(&tg_game.keybinds, COMMAND_PLAYER_MOVE_FORWARD    , TA_STATE_PLAY                                      , TA_KEYBIND_HOLD,    SDL_SCANCODE_W);
    ta_keybind_init1(&tg_game.keybinds, COMMAND_PLAYER_MOVE_BACKWARD   , TA_STATE_PLAY                                      , TA_KEYBIND_HOLD,    SDL_SCANCODE_S);
    ta_keybind_init1(&tg_game.keybinds, COMMAND_PLAYER_MOVE_RIGHT      , TA_STATE_PLAY                                      , TA_KEYBIND_HOLD,    SDL_SCANCODE_D);
    ta_keybind_init1(&tg_game.keybinds, COMMAND_PLAYER_MOVE_LEFT       , TA_STATE_PLAY                                      , TA_KEYBIND_HOLD,    SDL_SCANCODE_A);
    ta_keybind_init1(&tg_game.keybinds, COMMAND_PLAYER_JUMP            , TA_STATE_PLAY                                      , TA_KEYBIND_PRESS,   SDL_SCANCODE_SPACE);
    ta_keybind_init1(&tg_game.keybinds, COMMAND_PLAYER_SHOOT           , TA_STATE_PLAY                                      , TA_KEYBIND_PRESS,   SDL_SCANCODE_MOUSE_LEFT);
    ta_keybind_init1(&tg_game.keybinds, COMMAND_PLAYER_MOVE_FORWARD    ,                 TA_STATE_FREE_CAM | TA_STATE_EDITOR, TA_KEYBIND_HOLD,    SDL_SCANCODE_I);
    ta_keybind_init1(&tg_game.keybinds, COMMAND_PLAYER_MOVE_BACKWARD   ,                 TA_STATE_FREE_CAM | TA_STATE_EDITOR, TA_KEYBIND_HOLD,    SDL_SCANCODE_K);
    ta_keybind_init1(&tg_game.keybinds, COMMAND_PLAYER_MOVE_RIGHT      ,                 TA_STATE_FREE_CAM | TA_STATE_EDITOR, TA_KEYBIND_HOLD,    SDL_SCANCODE_L);
    ta_keybind_init1(&tg_game.keybinds, COMMAND_PLAYER_MOVE_LEFT       ,                 TA_STATE_FREE_CAM | TA_STATE_EDITOR, TA_KEYBIND_HOLD,    SDL_SCANCODE_J);
    ta_keybind_init1(&tg_game.keybinds, COMMAND_PLAYER_JUMP            ,                 TA_STATE_FREE_CAM | TA_STATE_EDITOR, TA_KEYBIND_PRESS,   SDL_SCANCODE_SEMICOLON);
    ta_keybind_init1(&tg_game.keybinds, COMMAND_CAMERA_MOVE_FORWARD    ,                 TA_STATE_FREE_CAM | TA_STATE_EDITOR, TA_KEYBIND_HOLD,    SDL_SCANCODE_W);
    ta_keybind_init1(&tg_game.keybinds, COMMAND_CAMERA_MOVE_BACKWARD   ,                 TA_STATE_FREE_CAM | TA_STATE_EDITOR, TA_KEYBIND_HOLD,    SDL_SCANCODE_S);
    ta_keybind_init1(&tg_game.keybinds, COMMAND_CAMERA_MOVE_RIGHT      ,                 TA_STATE_FREE_CAM | TA_STATE_EDITOR, TA_KEYBIND_HOLD,    SDL_SCANCODE_D);
    ta_keybind_init1(&tg_game.keybinds, COMMAND_CAMERA_MOVE_LEFT       ,                 TA_STATE_FREE_CAM | TA_STATE_EDITOR, TA_KEYBIND_HOLD,    SDL_SCANCODE_A);
    ta_keybind_init1(&tg_game.keybinds, COMMAND_CAMERA_MOVE_UP         ,                 TA_STATE_FREE_CAM | TA_STATE_EDITOR, TA_KEYBIND_HOLD,    SDL_SCANCODE_E);
    ta_keybind_init1(&tg_game.keybinds, COMMAND_CAMERA_MOVE_DOWN       ,                 TA_STATE_FREE_CAM | TA_STATE_EDITOR, TA_KEYBIND_HOLD,    SDL_SCANCODE_Q);
    ta_keybind_init1(&tg_game.keybinds, COMMAND_DEBUG_MOUSE_LOCK       ,                 TA_STATE_FREE_CAM | TA_STATE_EDITOR, TA_KEYBIND_PRESS,   SDL_SCANCODE_MOUSE_RIGHT);
    ta_keybind_init1(&tg_game.keybinds, COMMAND_DEBUG_MOUSE_UNLOCK     ,                 TA_STATE_FREE_CAM | TA_STATE_EDITOR, TA_KEYBIND_RELEASE, SDL_SCANCODE_MOUSE_RIGHT);
    ta_keybind_init1(&tg_game.keybinds, COMMAND_DEBUG_MOUSE_LOCK_TOGGLE, TA_STATE_PLAY | TA_STATE_FREE_CAM | TA_STATE_EDITOR, TA_KEYBIND_PRESS,   SDL_SCANCODE_M);
    ta_keybind_init1(&tg_game.keybinds, COMMAND_DEBUG_TOGGLE_WIREFRAME , TA_STATE_PLAY | TA_STATE_FREE_CAM | TA_STATE_EDITOR, TA_KEYBIND_PRESS,   SDL_SCANCODE_1);
    ta_keybind_init1(&tg_game.keybinds, COMMAND_DEBUG_TOGGLE_MESH      , TA_STATE_PLAY | TA_STATE_FREE_CAM | TA_STATE_EDITOR, TA_KEYBIND_PRESS,   SDL_SCANCODE_2);
    ta_keybind_init1(&tg_game.keybinds, COMMAND_DEBUG_TOGGLE_COLLIDERS , TA_STATE_PLAY | TA_STATE_FREE_CAM | TA_STATE_EDITOR, TA_KEYBIND_PRESS,   SDL_SCANCODE_3);
    ta_keybind_init1(&tg_game.keybinds, COMMAND_DEBUG_TOGGLE_NAMETAGS  , TA_STATE_PLAY | TA_STATE_FREE_CAM | TA_STATE_EDITOR, TA_KEYBIND_PRESS,   SDL_SCANCODE_4);
    ta_keybind_init1(&tg_game.keybinds, COMMAND_DEBUG_TOGGLE_NORMALS   , TA_STATE_PLAY | TA_STATE_FREE_CAM | TA_STATE_EDITOR, TA_KEYBIND_PRESS,   SDL_SCANCODE_5);
    ta_keybind_init1(&tg_game.keybinds, COMMAND_EDITOR_SELECT          ,                                     TA_STATE_EDITOR, TA_KEYBIND_PRESS,   SDL_SCANCODE_MOUSE_LEFT);
    ta_keybind_init1(&tg_game.keybinds, COMMAND_EDITOR_SELECT_RELEASE  ,                                     TA_STATE_EDITOR, TA_KEYBIND_RELEASE, SDL_SCANCODE_MOUSE_LEFT);
    ta_keybind_init1(&tg_game.keybinds, COMMAND_EDITOR_CANCEL          ,                                     TA_STATE_EDITOR, TA_KEYBIND_PRESS,   SDL_SCANCODE_ESCAPE);
    ta_keybind_init1(&tg_game.keybinds, COMMAND_EDITOR_CLOSE           ,                                     TA_STATE_EDITOR, TA_KEYBIND_PRESS,   SDL_SCANCODE_GRAVE);
    ta_keybind_init1(&tg_game.keybinds, COMMAND_EDITOR_SIM_PAUSE_RESUME,                                     TA_STATE_EDITOR, TA_KEYBIND_PRESS,   SDL_SCANCODE_F5);
    ta_keybind_init1(&tg_game.keybinds, COMMAND_EDITOR_SIM_NEXT        ,                                     TA_STATE_EDITOR, TA_KEYBIND_PRESS,   SDL_SCANCODE_F6);
    ta_keybind_init1(&tg_game.keybinds, COMMAND_EDITOR_SIM_NEXT_10     ,                                     TA_STATE_EDITOR, TA_KEYBIND_PRESS,   SDL_SCANCODE_F7);
    ta_keybind_init1(&tg_game.keybinds, COMMAND_EDITOR_SIM_WHILE_HELD  ,                                     TA_STATE_EDITOR, TA_KEYBIND_HOLD,    SDL_SCANCODE_F8);
    //-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    TracyCZoneEnd(ctxLoadKeybinds);

    //--------------------------------------------------------------------------
    // Random David shit
    //--------------------------------------------------------------------------
    tg_e_can = INTERN("can");

    //--------------------------------------------------------------------------
    // Texturing
    //--------------------------------------------------------------------------
    ta_texturing_init(&tg_game.texturing);

    //--------------------------------------------------------------------------
    // Scene
    //--------------------------------------------------------------------------
    ta_log_write(&tg_debug_log, SRC_GAME, "Loading first scene...\n");
    ta_scene_load_file(&tg_game.scene, "data/scene/scene.dml");
    //ta_scene_save_file_json(&game.scene, "data/scene/scene.json");

    //ta_game_load_gltf("data/mesh/MetalRoughSpheres.glb"); // stride != 0 (interleaved attributes)
    //ta_game_load_gltf("data/mesh/bee.glb");               // diffuse fails to load
    //ta_game_load_gltf("data/mesh/Dodecahedron.gltf");     // has external URIs

    // TODO: If file doesn't exist, just let it not load and fallback on default placeholder. Do same for textures.
    //ta_game_load_gltf("data/mesh/hier_test.gltf");
    //ta_game_load_gltf("data/mesh/rock_0001.gltf");
    //ta_game_load_gltf("data/mesh/button_silly.gltf");
    //ta_game_load_gltf("data/mesh/button.gltf");
    //ta_game_load_gltf("data/mesh/dude.gltf");
    //ta_game_load_gltf("data/mesh/skeleton_test.gltf");

    TracyCZoneN(ctxGenDefaultBoneXforms, "Generate default bone xforms", true);

    DLB_ASSERT(!tg_mesh_gl_default_bone_xforms);
    DLB_ASSERT(!tg_mesh_gl_default_bone_normal_xforms);

    glGenBuffers(1, &tg_mesh_gl_default_bone_xforms);
    glBindBuffer(GL_UNIFORM_BUFFER, tg_mesh_gl_default_bone_xforms);
    glBufferData(GL_UNIFORM_BUFFER, FIELD_SIZEOF(ta_mesh, skin.bone_xforms), 0, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    glBindBufferBase(GL_UNIFORM_BUFFER, TA_GLSL_UBO_BONE_XFORMS, tg_mesh_gl_default_bone_xforms);

    glGenBuffers(1, &tg_mesh_gl_default_bone_normal_xforms);
    glBindBuffer(GL_UNIFORM_BUFFER, tg_mesh_gl_default_bone_normal_xforms);
    glBufferData(GL_UNIFORM_BUFFER, FIELD_SIZEOF(ta_mesh, skin.bone_xforms), 0, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    glBindBufferBase(GL_UNIFORM_BUFFER, TA_GLSL_UBO_BONE_NORMAL_XFORMS, tg_mesh_gl_default_bone_normal_xforms);

    TracyCZoneEnd(ctxGenDefaultBoneXforms);

    //--------------------------------------------------------------------------
    // Lighting
    //--------------------------------------------------------------------------
    // TODO: Find closest 8 lights and store them in tg_game.lights
    ta_light_ubo_init(&tg_game.light_ubo);
    ta_material_ubo_init(&tg_game.material_ubo);

    //--------------------------------------------------------------------------
    // Scene (OGX) ** MUST COME AFTER game.scene.index_by_name[*] INITIALIZED
    //--------------------------------------------------------------------------
    ta_log_write(&tg_debug_log, SRC_GAME, "Loading ogex test file...\n");
    //dml_document_load("data/mesh/skeleton_test.ogex");
    //dml_document_load("data/mesh/button.ogex");
    //dml_document_load("data/mesh/dude.ogex");

    ogx_scene *scenes = 0;
    ogx_scene *scene = 0;
    scene = (ogx_scene *)dlb_vec_alloc(scenes);
    if (ogx_scene_from_file(scene, "data/mesh/dude.ogex") == OGX_SUCCESS) {
        ta_ogx_load(scene);
    }
    scene = (ogx_scene *)dlb_vec_alloc(scenes);
    if (ogx_scene_from_file(scene, "data/mesh/button.ogex") == OGX_SUCCESS) {
        ta_ogx_load(scene);
    }
    scene = (ogx_scene *)dlb_vec_alloc(scenes);
    if (ogx_scene_from_file(scene, "data/mesh/skeleton_test.ogex") == OGX_SUCCESS) {
        ta_ogx_load(scene);
    }
    scene = (ogx_scene *)dlb_vec_alloc(scenes);
    if (ogx_scene_from_file(scene, "data/mesh/test_bone_1.ogex") == OGX_SUCCESS) {
        ta_ogx_load(scene);
    }
    scene = (ogx_scene *)dlb_vec_alloc(scenes);
    if (ogx_scene_from_file(scene, "data/mesh/skeleton_test_skin.ogex") == OGX_SUCCESS) {
        ta_ogx_load(scene);
    }
    //scene = (ogx_scene *)dlb_vec_alloc(scenes);
    //if (ogx_scene_from_file(scene, "data/mesh/chamber_0002.ogex") == OGX_SUCCESS) {
    //    ta_ogx_load(scene);
    //}
    //scene = (ogx_scene *)dlb_vec_alloc(scenes);
    //if (ogx_scene_from_file(scene, "data/mesh/sponza.ogex") == OGX_SUCCESS) {
    //    ta_ogx_load(scene);
    //}
    // TODO: Free the ogx scene if it's not needed after being loaded

    //--------------------------------------------------------------------------
    // Simulation
    //--------------------------------------------------------------------------
    tg_game.simulate = 0; //-1;

    tg_game.debug_physics_render_penetration_vectors      = true;
    tg_game.debug_physics_render_static_friction_vectors  = true;
    tg_game.debug_physics_render_dynamic_friction_vectors = true;
    tg_game.debug_physics_render_damping_vectors          = true;
    tg_game.debug_physics_render_restitution_vectors      = true;

    tg_game.debug_physics_color_penetration_vectors      = TA_COLOR_RED;
    tg_game.debug_physics_color_static_friction_vectors  = TA_COLOR_PINK;
    tg_game.debug_physics_color_dynamic_friction_vectors = TA_COLOR_CYAN;
    tg_game.debug_physics_color_damping_vectors          = TA_COLOR_YELLOW;
    tg_game.debug_physics_color_restitution_vectors      = TA_COLOR_ORANGE;

    //--------------------------------------------------------------------------
    // Player
    //--------------------------------------------------------------------------
    tg_e_player_camera = SYM_ENTITY_PLAYER_CAMERA;
    DLB_ASSERT(tg_e_player_camera);
    tg_e_player_one = SYM_ENTITY_PLAYER_ONE;
    DLB_ASSERT(tg_e_player_one);

    //--------------------------------------------------------------------------
    // World
    //--------------------------------------------------------------------------
    tg_e_ground = INTERN("ground");
    DLB_ASSERT(tg_e_ground);

    //--------------------------------------------------------------------------
    // Cameras
    //--------------------------------------------------------------------------
    tg_e_freecam = SYM_ENTITY_FREECAM;
    DLB_ASSERT(tg_e_freecam);
    tg_game.active_camera = tg_e_freecam;

    TracyCZoneN(ctxMinimapCamera, "Init minimap_camera", true);
    tg_game.minimap_camera.fov = 90.0f;
    tg_game.minimap_camera.up = VEC3_NZ;
    tg_game.minimap_camera.ortho = true;
    ta_camera_init(&tg_game.minimap_camera);
    TracyCZoneEnd(ctxMinimapCamera);

    //--------------------------------------------------------------------------
    // Audio
    //--------------------------------------------------------------------------
    // TODO: Parent this node to the active player
    tg_e_background_music = SYM_ENTITY_BACKGROUND_MUSIC;
    DLB_ASSERT(tg_e_background_music);

    TracyCZoneN(ctxFindBGMusic, "Find background music", true);
    ta_audio_source *bg_music_src = (ta_audio_source *)ta_game_component_try(tg_e_background_music, RES_COMP_AUDIO_SOURCE);
    TracyCZoneEnd(ctxFindBGMusic);
    if (bg_music_src) {
        TracyCZoneN(ctxPlayBGMusic, "Loop background music", true);
        ta_audio_source_play_loop(bg_music_src);
        TracyCZoneEnd(ctxPlayBGMusic);
    }

    TracyCZoneN(ctxSetVolume, "Set volume", true);
    ta_audio_listener_set_volume(&tg_audio_listener, 0.1f);
    TracyCZoneEnd(ctxSetVolume);
    //ta_audio_listener_mute(&tg_audio_listener);

    //--------------------------------------------------------------------------
    // Textures
    //--------------------------------------------------------------------------
    TracyCZoneN(ctxAllocDefaultTextures, "Init default textures", true);

    tg_font                 = INTERN("data/font/UbuntuMono-Regular.ttf");
    tg_tex_audio_icon       = INTERN("data/texture/audio_icon.tga");
    tg_tex_bullet_icon      = INTERN("data/texture/bullet_icon.tga");
    tg_tex_bullet_icon_gray = INTERN("data/texture/bullet_icon_gray.tga");

    tg_tex_invalid_albedo    = INTERN("#invalid_albedo");
    tg_tex_default_albedo    = INTERN("#default_albedo");
    tg_tex_default_emission  = INTERN("#default_emission");
    tg_tex_default_height    = INTERN("#default_height");
    tg_tex_default_metallic  = INTERN("#default_metallic");
    tg_tex_default_normal    = INTERN("#default_normal");
    tg_tex_default_occlusion = INTERN("#default_occlusion");
    tg_tex_default_roughness = INTERN("#default_roughness");

    ta_game_alloc(RES_TEXTURE, SYM(tg_tex_invalid_albedo   ));
    ta_game_alloc(RES_TEXTURE, SYM(tg_tex_default_albedo   ));
    ta_game_alloc(RES_TEXTURE, SYM(tg_tex_default_emission ));
    ta_game_alloc(RES_TEXTURE, SYM(tg_tex_default_height   ));
    ta_game_alloc(RES_TEXTURE, SYM(tg_tex_default_metallic ));
    ta_game_alloc(RES_TEXTURE, SYM(tg_tex_default_normal   ));
    ta_game_alloc(RES_TEXTURE, SYM(tg_tex_default_occlusion));
    ta_game_alloc(RES_TEXTURE, SYM(tg_tex_default_roughness));

    ta_texture *tex_invalid_albedo    = (ta_texture *)ta_game_by_sym_try(RES_TEXTURE, tg_tex_invalid_albedo   );
    ta_texture *tex_default_albedo    = (ta_texture *)ta_game_by_sym_try(RES_TEXTURE, tg_tex_default_albedo   );
    ta_texture *tex_default_emission  = (ta_texture *)ta_game_by_sym_try(RES_TEXTURE, tg_tex_default_emission );
    ta_texture *tex_default_height    = (ta_texture *)ta_game_by_sym_try(RES_TEXTURE, tg_tex_default_height   );
    ta_texture *tex_default_metallic  = (ta_texture *)ta_game_by_sym_try(RES_TEXTURE, tg_tex_default_metallic );
    ta_texture *tex_default_normal    = (ta_texture *)ta_game_by_sym_try(RES_TEXTURE, tg_tex_default_normal   );
    ta_texture *tex_default_occlusion = (ta_texture *)ta_game_by_sym_try(RES_TEXTURE, tg_tex_default_occlusion);
    ta_texture *tex_default_roughness = (ta_texture *)ta_game_by_sym_try(RES_TEXTURE, tg_tex_default_roughness);

    // TODO(cleanup): This isn't used anywhere. Not sure if we need it.
    // Generate magenta/white grid pattern
    tex_invalid_albedo->type = TA_TEXTURE_2D_ARRAY;
    tex_invalid_albedo->width = 32;
    tex_invalid_albedo->height = 32;
    tex_invalid_albedo->channels = 4;
    u32 bytes = tex_invalid_albedo->width * tex_invalid_albedo->height * tex_invalid_albedo->channels;
    dlb_vec_reserve(tex_invalid_albedo->pixels, bytes);
    u8 toggle = 0;
    u8 toggle_width = 4;
    for (u32 y = 0; y < tex_invalid_albedo->height; y++) {
        if (y % toggle_width == 0) toggle = !toggle;
        for (u32 x = 0; x < tex_invalid_albedo->width; x++) {
            if (x % toggle_width == 0) toggle = !toggle;
            dlb_vec_push(tex_invalid_albedo->pixels, 255);
            dlb_vec_push(tex_invalid_albedo->pixels, toggle * 255);
            dlb_vec_push(tex_invalid_albedo->pixels, 255);
            dlb_vec_push(tex_invalid_albedo->pixels, 255);
        }
    }
    size_t pixels_len = dlb_vec_len(tex_invalid_albedo->pixels);
    DLB_ASSERT(pixels_len == bytes);

    // TODO: Just create 3 white textures, for 1, 2 and 4 channels. Then reuse.
    tex_default_albedo->type = TA_TEXTURE_2D_ARRAY;
    tex_default_albedo->width = 1;
    tex_default_albedo->height = 1;
    tex_default_albedo->channels = 4;
    dlb_vec_push(tex_default_albedo->pixels, 255);
    dlb_vec_push(tex_default_albedo->pixels, 255);
    dlb_vec_push(tex_default_albedo->pixels, 255);
    dlb_vec_push(tex_default_albedo->pixels, 255);

    tex_default_emission->type = TA_TEXTURE_2D_ARRAY;
    tex_default_emission->width = 1;
    tex_default_emission->height = 1;
    tex_default_emission->channels = 3;
    dlb_vec_push(tex_default_emission->pixels, 255);
    dlb_vec_push(tex_default_emission->pixels, 255);
    dlb_vec_push(tex_default_emission->pixels, 255);

    tex_default_height->type = TA_TEXTURE_2D_ARRAY;
    tex_default_height->width = 1;
    tex_default_height->height = 1;
    tex_default_height->channels = 1;
    dlb_vec_push(tex_default_height->pixels, 0);

    tex_default_metallic->type = TA_TEXTURE_2D_ARRAY;
    tex_default_metallic->width = 1;
    tex_default_metallic->height = 1;
    tex_default_metallic->channels = 1;
    dlb_vec_push(tex_default_metallic->pixels, 255);

    tex_default_normal->type = TA_TEXTURE_2D_ARRAY;
    tex_default_normal->width = 1;
    tex_default_normal->height = 1;
    tex_default_normal->channels = 3;
    dlb_vec_push(tex_default_normal->pixels, 127);
    dlb_vec_push(tex_default_normal->pixels, 127);
    dlb_vec_push(tex_default_normal->pixels, 255);

    tex_default_occlusion->type = TA_TEXTURE_2D_ARRAY;
    tex_default_occlusion->width = 1;
    tex_default_occlusion->height = 1;
    tex_default_occlusion->channels = 1;
    dlb_vec_push(tex_default_occlusion->pixels, 255);

    tex_default_roughness->type = TA_TEXTURE_2D_ARRAY;
    tex_default_roughness->width = 1;
    tex_default_roughness->height = 1;
    tex_default_roughness->channels = 1;
    dlb_vec_push(tex_default_roughness->pixels, 255);

    ta_texture_init(tex_invalid_albedo);
    ta_texture_init(tex_default_albedo);
    ta_texture_init(tex_default_emission);
    ta_texture_init(tex_default_height);
    ta_texture_init(tex_default_metallic);
    ta_texture_init(tex_default_normal);
    ta_texture_init(tex_default_occlusion);
    ta_texture_init(tex_default_roughness);

    TracyCZoneEnd(ctxAllocDefaultTextures);

    //--------------------------------------------------------------------------
    // Shaders
    //--------------------------------------------------------------------------
    // TODO: Move these to shaders.dml, they're not scene-specific
    tg_shader_lines   = (ta_shader *)ta_game_by_sym(RES_SHADER, INTERN("lines"));
    tg_shader_quads   = (ta_shader *)ta_game_by_sym(RES_SHADER, INTERN("quads"));
    tg_shader_cubemap = (ta_shader *)ta_game_by_sym(RES_SHADER, INTERN("cubemap"));

#if defined(_WIN32) || defined(__WIN32__) || defined(__WINDOWS__)
    TracyCZoneN(ctxAssetWatcher, "Init asset watcher", true);
    ta_asset_watcher_init(&tg_game.texture_watcher, SYM(tg_game.base_path));
    TracyCZoneEnd(ctxAssetWatcher);
#endif

#if _DEBUG
    ta_game_state_set(TA_STATE_FREE_CAM);
#else
    ta_game_state_set(TA_STATE_PLAY);
#endif
    ta_log_write(&tg_debug_log, SRC_GAME, "Active camera: %s\n", tg_game.active_camera);
    DLB_ASSERT(tg_game.active_camera);
}
ta_game_state ta_game_state_current()
{
    return tg_game.state;
}
void ta_game_state_set(ta_game_state state)
{
    if (state == tg_game.state) {
        return;
    }

    tg_game.state = state;
    ta_log_write(&tg_debug_log, SRC_GAME, "State = %s\n", game_state_str(state));
    switch (tg_game.state) {
        case TA_STATE_PLAY: {
            tg_game.active_camera = tg_e_player_camera;
            ta_mouse_capture_set(true);
            break;
        } case TA_STATE_FREE_CAM: {
            ta_camera *freecam = (ta_camera *)ta_game_component(tg_e_freecam, RES_COMP_CAMERA);
            ta_transform *trans = (ta_transform *)ta_game_component(tg_e_freecam, RES_COMP_TRANSFORM);
            // HACK: Set freecam position instantly to player cam location. This will *not* work correctly if freecam
            // has a parent. Needs to somehow use xform_world instead.
            if (vec3_zero(trans->xform.position)) {
                ta_transform *active_cam = (ta_transform *)ta_game_component(tg_game.active_camera, RES_COMP_CAMERA);
                freecam->target_xform.position = active_cam->xform.position;
                trans->xform.position = freecam->target_xform.position;
            }
            tg_game.active_camera = tg_e_freecam;
            break;
        } case TA_STATE_EDITOR: {
            ta_camera *freecam = (ta_camera *)ta_game_component(tg_e_freecam, RES_COMP_CAMERA);
            ta_transform *trans = (ta_transform *)ta_game_component(tg_e_freecam, RES_COMP_TRANSFORM);
            ta_transform *active_cam = (ta_transform *)ta_game_component(tg_game.active_camera, RES_COMP_TRANSFORM);
            freecam->target_xform.position = active_cam->xform.position;
            trans->xform.position = freecam->target_xform.position;
            //}
            tg_game.active_camera = tg_e_freecam;
            break;
        } default: {
            break;
        }
    }
}
void *ta_game_alloc(ta_res_type type, const char *name, size_t name_len)
{
    DLB_ASSERT(type >= RES_COMP_COUNT && "When would we not use ta_game_component_add for components?");
    const char *type_str = 0;
    ta_res_type_str(type, &type_str);
    ta_log_write(&tg_debug_log, SRC_GAME, "ta_game_alloc %s %.*s\n", type_str, name_len, name);
    return ta_scene_alloc(&tg_game.scene, type, name, name_len);
}
void ta_game_destroy(ta_res_type type, const char *name, size_t name_len)
{
    ta_scene_destroy(&tg_game.scene, type, name, name_len);
    dlb_vec_free(tg_game.keybinds);
}
// If not found, ASSERT
void *ta_game_by_name(ta_res_type type, const char *name, size_t name_len)
{
    return ta_scene_find(&tg_game.scene, type, name, name_len);
}
// If not found, returns NULL
void *ta_game_by_name_try(ta_res_type type, const char *name, size_t name_len)
{
    return ta_scene_find_try(&tg_game.scene, type, name, name_len);
}
// If not found, returns the first resource of the given type
void *ta_game_by_name_or_default(ta_res_type type, const char *name, size_t name_len)
{
    return ta_scene_find_or_default(&tg_game.scene, type, name, name_len);
}
void *ta_game_by_sym(ta_res_type type, const char *sym)
{
    return ta_game_by_name(type, SYM(sym));
}
void *ta_game_by_sym_try(ta_res_type type, const char *sym)
{
    return ta_game_by_name_try(type, SYM(sym));
}
void *ta_game_by_sym_or_default(ta_res_type type, const char *sym)
{
    return ta_game_by_name_or_default(type, SYM(sym));
}
void *ta_game_component_add(const char *entity, ta_res_type type, const char *name, size_t name_len)
{
    return ta_scene_component_add(&tg_game.scene, entity, type, name, name_len);
}
void *ta_game_component(const char *entity, ta_res_type type)
{
    return ta_scene_component(&tg_game.scene, entity, type);
}
void *ta_game_component_try(const char *entity, ta_res_type type)
{
    return ta_scene_component_try(&tg_game.scene, entity, type);
}
void *ta_game_resource_pool(ta_res_type type)
{
    void *pool = tg_game.scene.resource_data[type];
    return pool;
}
void ta_game_load_gltf(const char *filename)
{
    UNUSED(filename);
    DLB_ASSERT(!"GLTF loader broken due to model/pieces refactor");
#if 0
    ta_gltf gltf = { 0 };
    gltf.filename = filename;
    int err = ta_gltf_parse_file(&gltf);
    if (err) {
        ta_log_write(&tg_debug_log, SRC_SYSTEM, "Failed to load gltf model\n");
        DLB_ASSERT(0);
    }
    ta_gltf_load(&gltf);
    ta_gltf_free(&gltf);
#endif
}
ta_camera *ta_game_camera()
{
    return (ta_camera *)ta_game_component(tg_game.active_camera, RES_COMP_CAMERA);
}
// Cast ray from camera directly forward (crosshair)
ta_ray ta_game_camera_ray()
{
    ta_camera *camera = (ta_camera *)ta_game_camera();
    ta_transform *cam_trans = (ta_transform *)ta_game_component(camera->entity, RES_COMP_TRANSFORM);
    ta_ray ray = { 0 };
    ray.origin = cam_trans->xform_world.position;
    ray.direction = camera->front;
    return ray;
}
// Cast ray from mouse screen position
ta_ray ta_game_mouse_ray()
{
    //ta_camera *camera = (ta_camera *)ta_game_component(INTERN("player_camera"), RES_COMP_CAMERA);
    ta_camera *camera = (ta_camera *)ta_game_camera();
    ta_transform *cam_trans = (ta_transform *)ta_game_component(camera->entity, RES_COMP_TRANSFORM);

    // NOTE: Implementation based on this, but with I flipped some signs to make it more logical
    // https://www.scratchapixel.com/lessons/3d-basic-rendering/ray-tracing-generating-camera-rays/generating-camera-rays

    // Get center of pixel
    float x = (float)ta_mouse_x() + 0.5f;
    float y = (float)ta_mouse_y() + 0.5f;

    ta_vec3 dir = { 0 };
    dir.x = x * camera->right.x + -y * camera->up.x + camera->screen_to_world.x;
    dir.y = x * camera->right.y + -y * camera->up.y + camera->screen_to_world.y;
    dir.z = x * camera->right.z + -y * camera->up.z + camera->screen_to_world.z;
    dir = vec3_normalize(dir);

    ta_ray ray;
    ray.origin = cam_trans->xform_world.position;
    ray.direction = dir;
    return ray;
}
ta_player *ta_game_player()
{
    return (ta_player *)ta_game_component(tg_e_player_one, RES_COMP_PLAYER);
}
void ta_game_sim_pause()
{
    tg_game.simulate = 0;
}
void ta_game_sim_resume()
{
    tg_game.simulate = -1;
}
void ta_game_sim_step_n_frames(int frames)
{
    DLB_ASSERT(frames > 0);
    tg_game.simulate = frames;
}
bool ta_game_sim_running()
{
    return tg_game.simulate != 0;
}
bool ta_game_sim_paused()
{
    return tg_game.simulate == 0;
}
u64 ta_game_sim_step()
{
    return tg_game.sim_step;
}
u64 ta_game_frame_num()
{
    return tg_game.frame_num;
}
void ta_game_window_resize()
{
    // Update all cameras to new aspect ratio
    dlb_vec_each(ta_camera *, camera, (ta_camera *)ta_game_resource_pool(RES_COMP_CAMERA)) {
        if (!camera->ortho) {
            ta_camera_recalc_projection(camera);
        }
    }
}
static void game_hotload_textures()
{
    for (size_t i = 0; i < ARRAY_SIZE(tg_game.texture_watcher.changes); ++i) {
        ta_asset_change_record *change = &tg_game.texture_watcher.changes[i];
        // NOTE: Delay change handling 60 frames
        if (change->path && (tg_game.frame_num - change->frame_num) > 60) {
            ta_texture *tex = (ta_texture *)ta_game_by_name_try(RES_TEXTURE, change->path, strlen(change->path));
            if (tex) {
                printf("[GAME] hot-loading: %s\n", change->path);
                ta_texture_hot_reload(tex);
            } else {
                //printf("[GAME] not found: %s\n", filename);
            }
            dlb_free(change->path);
            change->path = 0;
            change->frame_num = 0;
        }
    }
}
static void game_draw_frame_info(u64 frame_num, double ms_frame_logic, double ms_frame_delta, u64 sim_step)
{
    const char *game_state = game_state_str(ta_game_state_current());

    float volume = ta_audio_listener_get_volume(&tg_audio_listener);
    bool muted = ta_audio_listener_muted(&tg_audio_listener);

    ta_camera *camera = (ta_camera *)ta_game_camera();

    ta_size window_size = { 0 };
    ta_rect restore = { 0 };
    bool vsync = false;
    ta_window_get_size(tg_window, &window_size.w, &window_size.h);
    ta_window_get_restore_rect(tg_window, &restore);
    ta_window_get_vsync(tg_window, &vsync);

    // Print frame time on the screen
    char frame_info[512] = { 0 };
    size_t len = snprintf(CSTR(frame_info),
        "Frame\n"
        "  count: %08llu\n"
        "  logic: %5.2f ms\n"
        "  delta: %5.2f ms\n"
        "Game\n"
        "  step:  %08llu\n"
        "  state: %s\n"
        "Audio\n"
        "  volume: %.2f%s\n"
        "Camera\n"
        "  fov: %.2f\n"
        "Window\n"
        "  size:     %d x %d\n"
        "  win size: %d x %d\n"
        "  win pos:  %d, %d\n"
        "  v-sync:   %s",
        frame_num,
        ms_frame_logic,
        ms_frame_delta,
        sim_step,
        game_state,
        volume,
        muted ? " (muted)" : "",
        camera->fov,
        window_size.w,
        window_size.h,
        restore.w,
        restore.h,
        restore.x,
        restore.y,
        vsync ? "On" : "Off"
    );
    DLB_ASSERT(len < sizeof(frame_info));

    static ta_rect_uv *frame_time_rects = 0;
    ta_font *font = (ta_font *)ta_game_by_sym(RES_FONT, tg_font);
    ta_rect font_rect = ta_font_push_text(font, frame_info, len, true, 0, 0, 0, &frame_time_rects);

#if 0
    for (int x = 0; x <= 3; x++) {
        for (int y = 0; y <= 3; y++) {
            ta_primitive_text_shadow_offset(x, y);
            ta_primitive_push_text_shadowed(0, frame_time_rects, TA_COLOR_WHITE, 0.0f, true);
            ta_font_render(font,
                500.0f + x * 10.0f + font_rect.w * x,
                10.0f + y * 10.0f + font_rect.h * y,
                UI_LAYER_HUD, true, &primitive_quads
            );
        }
    }
#else
    ta_primitive_text_shadow_offset(1, 1);
    ta_primitive_push_text_shadowed(0, frame_time_rects, TA_COLOR_WHITE, 0.0f, true);
    ta_font_render(font, SCREEN_WRAP_X(-220.0f), 12, UI_LAYER_HUD, true, &primitive_quads);
#endif
    dlb_vec_zero(frame_time_rects);

    // TODO: Idk why this is here.. do I need it?
    ta_shader *font_shader = ta_font_shader(font);
    ta_shader_reset_pvm(font_shader);
}
static void game_draw_hud()
{
    ta_player *player = (ta_player *)ta_game_player();
    ta_gun *gun = (ta_gun *)ta_game_component(player->e_gun, RES_COMP_GUN);

    ta_ui_next_bg_color_rgba(UI_STATE_ALL, TA_COLOR_INVIS);
    static ta_ui_window_state window = { 0 };
    ta_ui_window_begin(&window, TA_UI_AUTOSIZE);
    ta_ui_row_begin();
    for (u32 i = 0; i < gun->carrying_ammo_max; i++) {
        ta_ui_next_bg_color_rgba(UI_STATE_ALL, TA_COLOR_INVIS);
        ta_ui_next_pad(2, 2, 2, 2);
        if (i < gun->carrying_ammo) {
            ta_ui_image(tg_tex_bullet_icon);
        } else {
            ta_ui_image(tg_tex_bullet_icon_gray);
        }
    }
    //ta_ui_pad(0, 4);
    ta_ui_row_begin();
    for (u32 i = 0; i < gun->loaded_ammo_max; i++) {
        ta_ui_next_bg_color_rgba(UI_STATE_ALL, TA_COLOR_INVIS);
        ta_ui_next_pad(2, 2, 2, 2);
        if (i < gun->loaded_ammo) {
            ta_ui_image(tg_tex_bullet_icon);
        } else {
            ta_ui_image(tg_tex_bullet_icon_gray);
        }
    }
    ta_ui_window_end();
    ta_ui_render();
}
static void collision_broadphase(ta_rigid_body_pair **pairs, ta_rigid_body *rigid_bodies, double dt)
{
    // Box2D supports 16 collision categories. For each fixture you can_body
    // specify which category it belongs to. You also specify what other
    // categories this fixture can_body collide with.
    //
    //   if ((categoryA & maskB) != 0 && (categoryB & maskA) != 0)
    //
    // Collision groups let you specify an integral group index. You can_body
    // have all fixtures with the same group index always collide
    // (positive index) or never collide (negative index). Group indices
    // are usually used for things that are somehow related, like the
    // parts of a bicycle.
    //
    // Collisions between fixtures of different group indices are
    // filtered according the category and mask bits. In other words,
    // group filtering has higher precedence than category filtering.
    //
    // - A fixture on a static body can_body only collide with a dynamic
    //   body.
    // - A fixture on a kinematic body can_body only collide with a dynamic
    //   body.
    // - Fixtures on the same body never collide with each other.
    // - You can_body optionally enable/disable collision between fixtures on
    //   bodies connected by a joint.
    //
    // Sensor: Fixture which only detects collision, no response. a.k.a. trigger
    // -----------------------------------------------------------------
    // Depth-first traversal of AABB tree to find islands. Put islands
    // to sleep when all objects in island are resting. Wake up when
    // anything interacts or applies a force to any body in the island.

    UNUSED(dt);

    dlb_vec_each(ta_rigid_body *, a, rigid_bodies) {
        dlb_vec_range(ta_rigid_body *, b, a + 1, dlb_vec_end(rigid_bodies)) {
            // Skip self collision checks
            // NOTE: This isn't currently possible due to the loop starting at a + 1 and the fact that entites can only
            // have a single rigid body, but that might change in the future.
            if (a->entity == b->entity) {
                DLB_ASSERT(!"Entity has multiple rigid bodies?");
                continue;
            }
            // Ignore collisions between two immovable objects
            if (a->inv_mass == 0.0f && b->inv_mass == 0.0f) {
                continue;
            }
            // Ignore collisions between two sensors
            if (a->sensor && b->sensor) {
                continue;
            }
            // Ignore collisions between infinite planes
            if (a->collider.type == TA_COLLIDER_PLANE && b->collider.type == TA_COLLIDER_PLANE) {
                continue;
            }

            // NOTE: Skip broadphase for infinite planes (always collect them as potential pairs)
            if (a->collider.type == TA_COLLIDER_PLANE ||
                b->collider.type == TA_COLLIDER_PLANE ||
                ta_aabb_v_aabb(&a->aabb, &b->aabb))
            {
                ta_rigid_body_pair *pair = (ta_rigid_body_pair *)dlb_vec_alloc(*pairs);
                pair->a = a;
                pair->b = b;
            }
        }
    }
}
static void collision_narrowphase(ta_manifold **manifolds, ta_rigid_body_pair *pairs, double dt)
{
    UNUSED(dt);

    dlb_vec_each(ta_rigid_body_pair *, pair, pairs) {
        ta_manifold manifold = { 0 };
        if (ta_rigid_body_intersect(&manifold, pair->a, pair->b)) {
            dlb_vec_push(*manifolds, manifold);
        }
    }
}
static void game_simulate(float dt)
{
    static const float GRAVITY = -9.81f;

    ta_log_write(&tg_debug_log, SRC_GAME, " Sim step...\n");

#if 0
    // TODO: Use mesh instancing for primitives (need scale :/)
    // TODO: Use circular buffer for perma lines instead of dumping everything at arbitrary threshold
    if (dlb_vec_len(primitive_lines_perma.buffers[0]) > 100000) {
        ta_mesh_clear_buffers(&primitive_lines_perma);
    }
#else
    ta_mesh_clear_buffers(&primitive_lines_perma);
#endif

    dlb_vec_zero(tg_game.pairs);
    dlb_vec_zero(tg_game.manifolds);

    ta_rigid_body *rigid_bodies = (ta_rigid_body *)ta_game_resource_pool(RES_COMP_RIGID_BODY);

    // for n particles do
    //     x_prev = x;
    //     v = v + h * f_ext / m;
    //     x = x + h * v;
    // end
    dlb_vec_each(ta_rigid_body *, body, rigid_bodies) {
        {
            // TODO: We can't simulate rigid bodies on things that have parents, that would be super complex. We should consider
            // all of the childrens' colliders though, if they have any. Let's ignore this issue for now.
            ta_transform *transform = (ta_transform *)ta_game_component(body->entity, RES_COMP_TRANSFORM);
            DLB_ASSERT(!transform->parent);

            body->xform.position = transform->xform.position;
            body->xform.orientation = quat_normalize(transform->xform.orientation);
        }

        // Clear per-body collision list
        // HACK: Cast const away to prevent MSVC warnings about nonsensical const incompatibility
        dlb_vec_zero((char **)body->colliding_with);

        // Apply external forces
        if (!body->no_gravity) {
            ta_vec3 gravity = { 0.0f, GRAVITY, 0.0f };
            //gravity = vec3_scalef(gravity, body->mass);
            ta_rigid_body_apply_force(body, gravity);
        }

        // Store starting transform
        body->xform_prev = body->xform;

        // DEBUG: Cleanup
        const char *e_selected = 0;
        ta_editor_selected_entity(&e_selected);
        if (body->name == e_selected) {
            DLB_ASSERT(1);
        }

        // Update position (unless infinite mass)
        if (body->inv_mass) {
            ta_vec3 acceleration = vec3_scalef(body->force_accum, body->inv_mass);
            body->velocity = vec3_add(body->velocity, vec3_scalef(acceleration, dt));
            body->xform.position = vec3_add(body->xform.position, vec3_scalef(body->velocity, dt));

            float dtheta_mag = vec3_len(body->ang_velocity);
            if (dtheta_mag) {
                ta_vec4 delta_orient = quat_from_axis_angle(vec3_normalize(body->ang_velocity), dtheta_mag * dt);
                body->xform.orientation = quat_normalize(quat_mul(delta_orient, body->xform.orientation));
            }
        }

        // TODO: This will also need to update the AABB tree
        // Recalculate AABB
        body->aabb = ta_collider_world_bounds(&body->collider, &body->xform);

        // Reset accumulators
        body->force_accum = VEC3_ZERO;
        body->torque_accum = VEC3_ZERO;
        body->dbg_broadphase = false;
        body->dbg_narrowphase = false;
    }

    // Broad phase
    //
    // for iterations = 1 do
    //     SolvePositions(x1, ... xn, q1, ... qn);
    // end
    //
    // TODO: AABB tree and expand search distance based on body's velocity:
    // > To save computational cost we collect potential collision pairs once per time step instead of once per
    // > sub-step using a tree of axis aligned bounding boxes. We expand the boxes by a distance (k * dt * velocity),
    // > where k >= 1 is a safety multiplier accounting for potential accelerations during the time step. We use k = 2
    // > in our examples.  - PBDBodies.pdf, section 3.5
    collision_broadphase(&tg_game.pairs, rigid_bodies, dt);
    if (tg_game.pairs) {
        // Narrow phase
        collision_narrowphase(&tg_game.manifolds, tg_game.pairs, dt);
        dlb_vec_each(ta_manifold *, manifold, tg_game.manifolds) {
            DLB_ASSERT(manifold->a != manifold->b);

            // Update colliding_with lists
            dlb_vec_push(manifold->a->colliding_with, manifold->b->entity);
            dlb_vec_push(manifold->b->colliding_with, manifold->a->entity);

            ta_rigid_body *a = manifold->a;
            ta_rigid_body *b = manifold->b;

            // Sensors don't need any resolution
            if (a->sensor || b->sensor) {
                continue;
            }

            // All of this code currently assumes body->xform == collider position/rotation, we would need to be
            // more clever to get this to work with arbitrary offsets. "More clever" probably means making the rigid
            // body be the parent of the mesh and letting transform_update handle the offset
            //DLB_ASSERT(vec3_zero(a->centroid_local));
            //DLB_ASSERT(vec3_zero(b->centroid_local));

            // https://github.com/RandyGaul/ImpulseEngine/blob/master/Manifold.cpp#L57
            if (a->inv_mass == 0.0f && b->inv_mass == 0.0f) {
                DLB_ASSERT(0);
                ta_log_write(&tg_debug_log, SRC_RIGID_BODY, "WARNING: Cannot resolve contact between two infinite mass bodies\n");
                a->velocity = VEC3_ZERO;
                b->velocity = VEC3_ZERO;
                a->ang_velocity = VEC3_ZERO;
                b->ang_velocity = VEC3_ZERO;
                continue;
            }

            const char *e_selected = 0;
            ta_editor_selected_entity(&e_selected);
            if (a->name == e_selected || b->name == e_selected) {
                DLB_ASSERT(1);
            }

            for (u32 i = 0; i < manifold->contact_count; i++) {
                // world space contact points
                const ta_vec3 normal_world = manifold->normal_world;
                const ta_vec3 ca_world = rigid_body_local_to_world(a, manifold->contacts[i].ra_local);
                const ta_vec3 cb_world = rigid_body_local_to_world(b, manifold->contacts[i].rb_local);

                //---------------------------------------
                // Penetration correction
                //---------------------------------------
                float d = vec3_dot(vec3_sub(ca_world, cb_world), normal_world);
                if (d <= 0.0f) {
                    continue;
                }

                // Apply Dx = dn with a = 0 and lambda_normal
                ta_vec3 penetration_correction_world = vec3_scalef(normal_world, -d);
                ta_physics_apply_position_correction(
                    a,
                    b,
                    manifold->contacts[i].ra_local,
                    manifold->contacts[i].rb_local,
                    penetration_correction_world,
                    0.0f,
                    &manifold->contacts[i].lambda_n,
                    dt,
                    tg_game.debug_physics_render_penetration_vectors,
                    tg_game.debug_physics_color_penetration_vectors
                );

                //---------------------------------------
                // Static friction correction
                //---------------------------------------

                // relative motion of contacts
                const ta_vec3 ca_world_prev = rigid_body_local_to_world_prev(a, manifold->contacts[i].ra_local);
                const ta_vec3 cb_world_prev = rigid_body_local_to_world_prev(b, manifold->contacts[i].rb_local);

                // "dp" = delta position, i.e. relative motion of contacts
                ta_vec3 dp = vec3_sub(vec3_sub(cb_world, cb_world_prev), vec3_sub(ca_world, ca_world_prev));
                ta_vec3 dp_normal = vec3_scalef(normal_world, vec3_dot(dp, normal_world));
                ta_vec3 dp_tangent = vec3_sub(dp, dp_normal);

                // NOTE: There shouldn't be a fabsf() here for lambda_n according to PBDBodies.pdf, but I clearly have
                // a sign error somewhere. Need to figure this out (I believe the worst case for the current code is
                // a small over-correction when lambda is negative).
                if (vec3_len2(dp_tangent) &&
                    manifold->contacts[i].lambda_t < manifold->coef_static * -manifold->contacts[i].lambda_n)
                {
                    // Apply Dx = Dp_t with a = 0
                    ta_vec3 static_friction_world = dp_tangent;
#if 1
                    ta_physics_apply_position_correction(
                        a,
                        b,
                        manifold->contacts[i].ra_local,
                        manifold->contacts[i].rb_local,
                        static_friction_world,
                        0.0f,
                        &manifold->contacts[i].lambda_t,
                        dt,
                        tg_game.debug_physics_render_static_friction_vectors,
                        tg_game.debug_physics_color_static_friction_vectors
                    );
#endif
                }
            }
        }
    }

    // for n particles do
    //     v = (x − x_prev)/h;
    // end
    const float dt_inv = 1.0f / dt;
    dlb_vec_each(ta_rigid_body *, body, rigid_bodies) {
        // Update world space AABB
        body->aabb = ta_collider_world_bounds(&body->collider, &body->xform);

        body->velocity_prev = body->velocity;
        body->ang_velocity_prev = body->ang_velocity;

        // DEBUG: Cleanup
        const char *e_selected = 0;
        ta_editor_selected_entity(&e_selected);
        if (body->name == e_selected) {
            DLB_ASSERT(1);
        }

        ta_vec3 dp = vec3_sub(body->xform.position, body->xform_prev.position);
        body->velocity = vec3_scalef(dp, dt_inv);

        ta_vec4 dq = quat_mul(body->xform.orientation, quat_inverse(body->xform_prev.orientation));
        dq = quat_normalize(dq);
        body->ang_velocity = axis_angle_from_quat(dq);
        body->ang_velocity = vec3_scalef(body->ang_velocity, dt_inv);

        // check for NaN and infinity
        DLB_ASSERT(vec3_good(body->velocity));
        DLB_ASSERT(vec3_good(body->ang_velocity));

        // TODO: Allow transform to be offset from rigid body position/orientation? Or just make rigidbody the parent..
        ta_transform *transform = (ta_transform *)ta_game_component(body->entity, RES_COMP_TRANSFORM);
        transform->xform.position = body->xform.position;
        transform->xform.orientation = body->xform.orientation;
    }

    // Velocity corrections
    dlb_vec_each(ta_manifold *, manifold, tg_game.manifolds) {
        DLB_ASSERT(manifold->a != manifold->b);

        ta_rigid_body *a = manifold->a;
        ta_rigid_body *b = manifold->b;

        // Sensors don't need any resolution
        if (a->sensor || b->sensor) {
            continue;
        }

        const char *e_selected = 0;
        ta_editor_selected_entity(&e_selected);
        if (a->name == e_selected || b->name == e_selected) {
            DLB_ASSERT(1);
        }

        for (u32 i = 0; i < manifold->contact_count; i++) {
            const ta_vec3 n = manifold->normal_world;
            const ta_vec3 ra = rigid_body_oriented_vector(a, manifold->contacts[i].ra_local);
            const ta_vec3 rb = rigid_body_oriented_vector(b, manifold->contacts[i].rb_local);

            // relative velocity
            ta_vec3 va = vec3_add(a->velocity, vec3_cross(a->ang_velocity, ra));
            ta_vec3 vb = vec3_add(b->velocity, vec3_cross(b->ang_velocity, rb));
            ta_vec3 v = vec3_sub(vb, va);

            // normal/tangential velocity
            float vn = vec3_dot(n, v);
            ta_vec3 n_vn = vec3_scalef(n, vn);
            ta_vec3 vt = vec3_sub(v, n_vn);

            // dynamic friction
            if (!vec3_zero(vt)) {
                float vt_mag = vec3_len(vt);
                DLB_ASSERT(vt_mag);
                ta_vec3 vt_dir = vec3_scalef(vt, 1.0f/vt_mag);

                float kd = manifold->coef_dynamic;

                // NOTE: Can simplify later by removing redundant dt's
                // PBDBodies eq. 31
                //float fn = -manifold->contacts[i].lambda_n / (dt * dt);
                float fn = -manifold->contacts[i].lambda_n / (dt * dt);
                float dv_mag = -MIN(dt * kd * fn, vt_mag);
                ta_vec3 dv_dynamic_friction = vec3_scalef(vt_dir, dv_mag);
#if 1
                ta_physics_apply_velocity_correction(
                    a,
                    b,
                    manifold->contacts[i].ra_local,
                    manifold->contacts[i].rb_local,
                    dv_dynamic_friction,
                    tg_game.debug_physics_render_dynamic_friction_vectors,
                    tg_game.debug_physics_color_dynamic_friction_vectors
                );
#endif
            }

            // damping
            {
                float coef_linear_damping = 0.99f;
                float coef_angular_damping = 0.99f;

                float linear_damping = MIN(coef_linear_damping, 1.0f);
                float angular_damping = MIN(coef_angular_damping, 1.0f);

                // This doesn't seem to do anything useful to prevent jittering..
                //vec3_scalef(a->velocity, linear_damping * dt);
                //vec3_scalef(b->velocity, linear_damping * dt);
                //vec3_scalef(a->ang_velocity, angular_damping * dt);
                //vec3_scalef(b->ang_velocity, angular_damping * dt);

#if 0
                // TODO: Is "joint damping" relevant in the context of contact constraints? (eq. 32 & 33 in PBDBodies)
                // If so, are the contact radii the correct location to apply the impulse?
                ta_vec3 dv_damping = vec3_scalef(vec3_sub(b->velocity, a->velocity), linear_damping);
                ta_physics_apply_velocity_correction(
                    a,
                    b,
                    manifold->contacts[i].ra_local,
                    manifold->contacts[i].rb_local,
                    dv_damping,
                    tg_game.debug_physics_render_damping_vectors,
                    tg_game.debug_physics_color_damping_vectors
                );
#endif

                // TODO: Angular damping
            }

            // restitution
            {
                // previous relative velocity (before velocity integration)
                ta_vec3 va_prev = vec3_add(a->velocity_prev, vec3_cross(a->ang_velocity_prev, ra));
                ta_vec3 vb_prev = vec3_add(b->velocity_prev, vec3_cross(b->ang_velocity_prev, rb));
                ta_vec3 v_prev = vec3_sub(vb_prev, va_prev);
                // previous normal velocity
                float vn_mag_prev = vec3_dot(n, v_prev);
                // TODO: for small vn, |vn| <= 2|g|h, set restitution to 0 to avoid jittering
                //float e = manifold->e; // * (float)(fabs(vn) > (2.0f * fabs(GRAVITY) * dt));
                float e = manifold->e * (fabsf(vn_mag_prev) > (2.0f * fabsf(GRAVITY) * dt));
                //e = 0.9f;
                // NOTE: Using MIN instead of MAX here because my signs are reversed (opposite normal I think) vs.
                // PBDBodies Eq. 35.
                //float impulse_mag = -vn + MIN(-e * vn_mag_prev, 0);
                //float impulse_mag = -vn + MAX(-e * vn_mag_prev, 0);
                float impulse_mag = vn + e * vn_mag_prev;
                ta_vec3 dv_restitution = vec3_scalef(n, impulse_mag);
                ta_physics_apply_velocity_correction(
                    a,
                    b,
                    manifold->contacts[i].ra_local,
                    manifold->contacts[i].rb_local,
                    dv_restitution,
                    tg_game.debug_physics_render_restitution_vectors,
                    tg_game.debug_physics_color_restitution_vectors
                );
            }

            //ta_vec3 dv_total = vec3_add3(dv_dynamic_friction, dv_damping, dv_restitution);
            //ta_vec3 dv_total_a = vec3_scalef(dv_total, 1.0f/(wa + wb));
            //ta_vec3 dv_total_b = vec3_neg(dv_total_a);
            //ta_rigid_body_apply_velocity_correction(a, dv_total_a, ra);
            //ta_rigid_body_apply_velocity_correction(b, dv_total_b, rb);
        }
    }

#if 0
    // TODO: Implement damping in a way that doesn't vary with different timesteps
    body->velocity = vec3_scalef(body->velocity, 0.99f);
    body->ang_velocity = vec3_scalef(body->ang_velocity, 0.99f);
#endif
}
static void game_render_skybox()
{
    //ta_cubemap *skybox = (ta_cubemap *)ta_game_by_name_try(RES_CUBEMAP, SYM(INTERN("miramar_skybox")));
    ta_cubemap *skybox = (ta_cubemap *)ta_game_by_name_try(RES_CUBEMAP, SYM(INTERN("bad_galaxy_skybox")));
    if (skybox) {
        ta_camera *camera = (ta_camera *)ta_game_camera();
        ta_shader *shader = (ta_shader *)ta_game_by_name(RES_SHADER, SYM(INTERN("skybox")));
        ta_mesh *mesh = (ta_mesh *)ta_game_by_name(RES_MESH, SYM(INTERN("prim_skybox")));
        ta_shader_set_mat4(shader, SYM_U_PROJ, &camera->projection);
        ta_shader_set_mat4(shader, SYM_U_VIEW, &camera->frustum);

        // NOTE: Assume all textures are in the same pool (asserts)
        ta_texture *first_tex = (ta_texture *)ta_game_by_sym(RES_TEXTURE, skybox->textures[0]);
        GLuint *layers = 0;
        for (int i = 0; i < 6; i++) {
            ta_texture *tex = (ta_texture *)ta_game_by_sym(RES_TEXTURE, skybox->textures[i]);
            // TODO: This is a stupid memory allocation, make it a static buffer and pass size explicitly
            dlb_vec_push(layers, tex->gl_texture_pool_layer);
            DLB_ASSERT(tex->gl_texture_pool_index == first_tex->gl_texture_pool_index);
        }
        ta_shader_set_uint(shader, SYM_U_TEXTURE_POOL_INDEX, first_tex->gl_texture_pool_index);
        ta_shader_set_uint_array(shader, SYM_U_TEXTURE_ARRAY_LAYERS, layers);

        //ta_shader_set_sampler_cube(shader, SYM_U_TEX, skybox->gl_id);

        ta_texture_pool *pool = ta_texture_texture_pool(first_tex);
        ta_texture_pool_bind(pool);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, first_tex->gl_filter_min);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, first_tex->gl_filter_mag);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        ta_texture_pool_unbind();

        glDisable(GL_CULL_FACE);
        glDepthMask(GL_FALSE);
        ta_shader_bind(shader);
        ta_mesh_render(mesh);
        ta_shader_unbind();
        glDepthMask(GL_TRUE);
        glEnable(GL_CULL_FACE);

        ta_texture_pool_bind(pool);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
        ta_texture_pool_unbind();
    }
}
static void game_render_manifolds_debug()
{
    const float radius = 0.025f;
    dlb_vec_each(ta_manifold *, manifold, tg_game.manifolds) {
        for (u32 i = 0; i < manifold->contact_count; ++i) {
            // world space contact points
            const ta_vec3 normal_world = manifold->normal_world;
            const ta_vec3 ra_world = rigid_body_oriented_vector(manifold->a, manifold->contacts[i].ra_local);
            const ta_vec3 rb_world = rigid_body_oriented_vector(manifold->b, manifold->contacts[i].rb_local);
            const ta_vec3 ca_world = rigid_body_local_to_world(manifold->a, manifold->contacts[i].ra_local);
            const ta_vec3 cb_world = rigid_body_local_to_world(manifold->b, manifold->contacts[i].rb_local);

            ta_sphere sphere = { 0 };
            sphere.center = ca_world;
            sphere.radius = radius;
            ta_primitive_push_sphere(0, sphere, TA_COLOR_WHITE);

            ta_vec3 origin    = ca_world;
            ta_vec3 direction = vec3_scalef(normal_world, 0.05f);
            ta_primitive_push_arrow(0, origin, direction, TA_COLOR_WHITE);

            ta_primitive_push_arrow(0, manifold->a->xform.position, ra_world, TA_COLOR_BLUE5);
            ta_primitive_push_arrow(0, manifold->b->xform.position, rb_world, TA_COLOR_YELLOW);

            sphere.center = cb_world;
            sphere.radius = radius;
            ta_primitive_push_sphere(0, sphere, TA_COLOR_GRAY6);
        }
    }
}
static void game_render_colliders_debug()
{
    ta_rigid_body *rigid_bodies = (ta_rigid_body *)ta_game_resource_pool(RES_COMP_RIGID_BODY);

    // Local space
    dlb_vec_each(ta_rigid_body *, body, rigid_bodies) {
        ta_transform *transform = (ta_transform *)ta_game_component(body->entity, RES_COMP_TRANSFORM);
        ta_shader_set_mat4(tg_shader_lines, SYM_U_MODEL, &transform->world);
        ta_shader_set_mat4(tg_shader_quads, SYM_U_MODEL, &transform->world);

        ta_sphere centroid_local = { 0 };
        centroid_local.center = body->centroid_local;
        centroid_local.radius = 0.06f;
        ta_primitive_push_sphere(0, centroid_local, TA_COLOR_GREEN);

        ta_rgba narrowphase_color = body->dbg_narrowphase ? TA_COLOR_RED : TA_COLOR_GRAY6;
        ta_collider_push(&body->collider, narrowphase_color);
        ta_primitive_dump(true);
    }
    ta_shader_set_mat4(tg_shader_lines, SYM_U_MODEL, &MAT4_IDENT);
    ta_shader_set_mat4(tg_shader_quads, SYM_U_MODEL, &MAT4_IDENT);

    // World space
    if (tg_game.debug_aabbs) {
        dlb_vec_each(ta_rigid_body *, body, rigid_bodies) {
            ta_transform *transform = (ta_transform *)ta_game_component(body->entity, RES_COMP_TRANSFORM);

            //ta_sphere local_origin = { 0 };
            //local_origin.center = transform->xform_world.position;
            //local_origin.radius = 0.04f;
            //ta_primitive_push_sphere(0, local_origin, TA_COLOR_PINK);
            //
            //ta_sphere centroid_world = { 0 };
            //centroid_world.center = body->centroid_world;
            //centroid_world.radius = 0.08f;
            //ta_primitive_push_sphere(0, centroid_world, TA_COLOR_BLUE);

            ta_rgba broadphase_color = body->dbg_broadphase ? TA_COLOR_ORANGE : TA_COLOR_GRAY3;
            ta_primitive_push_aabb(0, body->aabb, broadphase_color);
        }
        ta_primitive_dump(true);
    }
}
static void game_render_nametags_debug(ta_camera *camera)
{
    ta_font *font = (ta_font *)ta_game_by_sym(RES_FONT, tg_font);
    static ta_rect_uv *tag_rects = 0;

    ta_transform *cam_trans = (ta_transform *)ta_game_component(camera->entity, RES_COMP_TRANSFORM);
    ta_shader *font_shader = ta_font_shader(font);
    ta_shader_reset_pvm(font_shader);

    const float max_render_distance = 15.0f;

    dlb_vec_each(ta_transform *, transform, (ta_transform *)ta_game_resource_pool(RES_COMP_TRANSFORM)) {
        // Don't render nametag for the camera, heh
        if (transform->entity == tg_game.active_camera) {
            continue;
        }

        ta_vec3 tag_pos = transform->xform_world.position;
        ta_vec3 tag_to_cam = vec3_sub(cam_trans->xform_world.position, tag_pos);

        // TODO: Use AABB tree to find N closest transforms to the camera
        if (vec3_len2(tag_to_cam) > max_render_distance * max_render_distance) {
            continue;
        }

#if 0
        tag_to_cam.z *= -1.0f;
        tag_to_cam.y *= 0.0f;
        float tag_scalef = 1.0f; //MAX(vec3_len(tag_to_cam), 4.0f);

        ta_rect tag_rect = ta_font_push_text(font, SYM(transform->name), true, 0, 0, 0, &tag_rects);
        ta_vec3 tag_offset = vec3_scalef(camera->right, NDC_W(tag_rect.w) / 2.0f * tag_scalef);
        ta_vec3 tag_pos_off = vec3_sub(tag_pos, tag_offset);

        ta_mat4 tag_rot_x = mat4_rotate_x(camera->pitch);
        float yaw = camera->yaw - 90.0f;
        if (yaw < 0.0f) yaw += 360.0f;
        ta_mat4 tag_rot_y = mat4_rotate_y(yaw);
        ta_mat4 tag_rot = mat4_mul(&tag_rot_y, &tag_rot_x);

        ta_mat4 tag_trans_bg = mat4_translate(tag_pos_off);
        ta_mat4 tag_xform_bg = mat4_scalef(tag_scalef);
        tag_xform_bg = mat4_mul(&tag_rot, &tag_xform_bg);
        tag_xform_bg = mat4_mul(&tag_trans_bg, &tag_xform_bg);

        ta_vec3 tag_pos_off_fg = tag_pos_off;
        tag_pos_off_fg.y += NDC_H(tag_rect.h) * tag_scalef;
        ta_mat4 tag_trans_fg = mat4_translate(tag_pos_off_fg);
        ta_mat4 tag_xform_fg = mat4_scalef(tag_scalef);
        tag_xform_fg = mat4_mul(&tag_rot, &tag_xform_fg);
        tag_xform_fg = mat4_mul(&tag_trans_fg, &tag_xform_fg);

        // Name tag background
        ta_shader_set_mat4(tg_shader_quads, SYM_U_PROJ, &projection);
        ta_shader_set_mat4(tg_shader_quads, SYM_U_VIEW, &camera->look_at);
        ta_shader_set_mat4(tg_shader_quads, SYM_U_MODEL, &tag_xform_bg);
        //ta_texture *tex_orange = (ta_texture *)ta_game_by_sym(RES_TEXTURE, tg_tex_orange);
        //ta_shader_set_sampler2d(tg_shader_quads, SYM_U_TEX, tex_orange->gl_id);
        ta_rect_uv tag_background = { 0 };
        tag_background.rect.x -= (int)NDC_W(5.0f);
        tag_background.rect.w = (int)(NDC_W(tag_rect.w) + NDC_W(10.0f));
        tag_background.rect.h = (int)NDC_H(tag_rect.h); //tg_game.font->pixel_height * 1.5f;
        ta_primitive_push_rect_uv(0, tag_background, TA_COLOR_DARK_RED, UI_LAYER_HUD_BG, true, true);
        ta_primitive_render_mesh(&primitive_quads, tg_shader_quads, true);
        //ta_shader_set_sampler_2d(tg_shader_quads, SYM_U_TEX, 0);

        // Name tag text
        ta_shader_set_mat4(font_shader, SYM_U_PROJ, &projection);
        ta_shader_set_mat4(font_shader, SYM_U_VIEW, &camera->look_at);
        ta_shader_set_mat4(font_shader, SYM_U_MODEL, &tag_xform_fg);

        // TODO: Move UI_LAYER_HUD out of push_rect_uv into tag_xform, or
        //       make font_render's xform arguments stack with current value
        //       of SYM_U_MODEL.
        dlb_vec_each(ta_rect_uv *, rect, tag_rects) {
            ta_primitive_push_rect_uv(0, *rect, TA_COLOR_WHITE, UI_LAYER_HUD, true, true);
        }
        dlb_vec_zero(tag_rects);
        ta_font_render(font, 0, 0, 0, true, &primitive_quads);
        ta_shader_reset_pvm(font_shader);
#else
        ta_vec4 p_world = vec4_init_vec3_w(transform->xform_world.position, 1.0f);
        ta_mat4 proj = camera->projection;
        ta_mat4 view = camera->look_at;
        ta_mat4 pv = mat4_mul(&proj, &view);
        ta_vec4 p_screen = mat4_mul_vec4(&pv, p_world);

        // Behind camera
        if (p_screen.z < 0.0f) {
            continue;
        }
        // Degenerate case, can't perform divide
        if (p_screen.w == 0.0f) {
            continue;
        }

        ta_vec3 p_ndc = vec3_init(p_screen.x, p_screen.y, p_screen.z);
        if (p_screen.w != 0.0f) {
            p_ndc = vec3_scalef(p_ndc, 1.0f / p_screen.w);
        } else {
            DLB_ASSERT(!"U wot mate?");
        }

        //if (fabsf(p_ndc.x) > 1.0f || fabsf(p_ndc.y) > 1.0f) {
        //    continue;
        //}

        float x = NDC_TO_SCREEN_X(p_ndc.x / p_ndc.z);
        float y = NDC_TO_SCREEN_Y(p_ndc.y / p_ndc.z);


        //ta_shader_set_mat4(font_shader, SYM_U_PROJ, &projection);
        //ta_shader_set_mat4(font_shader, SYM_U_VIEW, &camera->look_at);

        //ta_mat4 trans = mat4_translate(vec3_init(x, y, 0.0f));
        //ta_shader_set_mat4(font_shader, SYM_U_MODEL, &trans);
        //ta_shader_set_mat4(font_shader, SYM_U_MODEL, &c);

        ta_rect tag_rect = ta_font_push_text(font, SYM(transform->name), true, 0, 0, 0, &tag_rects);
        x -= tag_rect.w / 2.0f;
        y -= tag_rect.h / 2.0f;

        ta_primitive_text_shadow_offset(1, 1);
        ta_primitive_push_text_shadowed(0, tag_rects, TA_COLOR_WHITE, UI_LAYER_HUD, true);

        dlb_vec_zero(tag_rects);
        glDisable(GL_CULL_FACE);
        glDisable(GL_DEPTH_TEST);
        ta_font_render(font, x, y, 0, true, &primitive_quads);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
#endif
    }

    ta_shader_reset_pvm(font_shader);
    ta_shader_reset_pvm(tg_shader_quads);
}
void ta_game_loop()
{
    ////////////////////////////////////////////////////////////////////////////
    // Main loop
    ////////////////////////////////////////////////////////////////////////////

    // TODO: Cleanup
    ta_ui_barchart chart = { 0 };
    ta_ui_barchart_init(&chart, 10, 10, MAX(0, WINDOW_W - 20), 30);

    // Eric Catto - Soft Constraints (GDC 2011)
    // Semi-implicit Euler will eventually blow up if you take big time steps. A
    // general rule is to take at least 4 time steps per period of oscillation.
    // For example, if the oscillation frequency is 60Hz, then you shouldn’t
    // take time steps slower than 15Hz.
    //
    // Randy Gaul
    // https://gamedevelopment.tutsplus.com/series/how-to-create-a-custom-physics-engine--gamedev-12715
    const float ms_sim_dt = 16;             // fixed dt milliseconds
    const float sim_dt = ms_sim_dt / 1000;  // fixed dt seconds
    const int sim_max_steps = 3;            // max simulation steps per frame (0 = unlimited; may cause spiral of death)
    const int sim_substeps = 1;             // number of substeps to perform for each step
    double ms_sim_t = 0;                    // current simulation time
    double ms_frame_accum = 0;

    double ms_frame_prev = 0;  // Last frame started
    double ms_frame_start = 0; // This frame started
    double ms_frame_delta = 0; // Total delta time (including v-sync)
    double ms_frame_logic = 0;  // Actual frame time before v-sync

    ta_transform *transforms = (ta_transform *)ta_game_resource_pool(RES_COMP_TRANSFORM);
    ta_mesh *meshes = (ta_mesh *)ta_game_resource_pool(RES_MESH);
    ta_model *models = (ta_model *)ta_game_resource_pool(RES_COMP_MODEL);
    ta_light *lights = (ta_light *)ta_game_resource_pool(RES_COMP_LIGHT);
    ta_camera *cameras = (ta_camera *)ta_game_resource_pool(RES_COMP_CAMERA);
    ta_shader *shaders = (ta_shader *)ta_game_resource_pool(RES_SHADER);

    while (ta_game_state_current() != TA_STATE_SHUTDOWN) {
        ms_frame_start = ta_timer_elapsed_ms();
        ms_frame_delta = ms_frame_start - ms_frame_prev;
        ms_frame_prev = ms_frame_start;

        game_hotload_textures();

        ta_camera *active_camera = (ta_camera *)ta_game_camera();
        ta_transform *active_camera_trans = (ta_transform *)ta_game_component(active_camera->entity, RES_COMP_TRANSFORM);

        //----------------------------------------------------------------------
        // Handle events
        //----------------------------------------------------------------------
        ta_log_write(&tg_debug_log, SRC_GAME, " Handling events...\n");
        ta_event_events();

        //----------------------------------------------------------------------
        // Simulation
        //----------------------------------------------------------------------
        ta_log_write(&tg_debug_log, SRC_GAME, " Accumulating...\n");
        ms_frame_accum += ms_frame_delta;
        // Prevent spiral of death
        // NOTE: This breaks determinism when simulation is under duress
        if (sim_max_steps) {
            float sim_max_ms = ms_sim_dt * sim_max_steps;
            if (ms_frame_accum > sim_max_ms) {
                ta_log_write(&tg_debug_log, SRC_GAME,
                    "WARNING: Physics accumulator spiraling; truncating %f to %f\n",
                    ms_frame_accum, sim_max_ms);
                ms_frame_accum = sim_max_ms;
            }
        }

        while (ms_frame_accum >= ms_sim_dt && tg_game.simulate) {
            if (tg_game.simulate > 0) {
                tg_game.simulate--;
            }

            float h = sim_dt / sim_substeps;
            for (int i = 0; i < sim_substeps; ++i) {
                game_simulate(h);
            }
            ms_sim_t += ms_sim_dt;
            ms_frame_accum -= ms_sim_dt;

            tg_game.sim_step++;
        }

        float sim_alpha = (float)(ms_frame_accum / ms_sim_dt);

        //----------------------------------------------------------------------
        // Animation
        // TODO: Should this go before or after physics simulation..?
        //----------------------------------------------------------------------

        // TODO: Track which animations are currently playing.. do blending.
        //ta_animation *animations = (ta_animation *)ta_game_resource_pool(RES_ANIMATION);
        //dlb_vec_each(ta_animation *, animation, animations) {
        //    UNUSED(animation);
        //}

        const char *dude_wave = INTERN("dude.clip.wave");
        const char *dude_squat = INTERN("dude.clip.squat");

        static const char **animation_names = 0;
        if (!animation_names) {
            dlb_vec_push(animation_names, INTERN("test_bone_1.clip.bob"));
            dlb_vec_push(animation_names, INTERN("skeleton_test.clip.lean_forward"));
            dlb_vec_push(animation_names, INTERN("skeleton_test_skin.clip.lean_forward"));
            dlb_vec_push(animation_names, dude_wave);
        }

        static float animation_time_sec = 0.0f;
        animation_time_sec += (float)(ms_frame_delta / 1000.0);
        while (animation_time_sec > 2.0f) {
            animation_time_sec -= 2.0f;
            if (*dlb_vec_last(animation_names) == dude_wave) {
                dlb_vec_pop(animation_names);
                dlb_vec_push(animation_names, dude_squat);
            } else if (*dlb_vec_last(animation_names) == dude_squat) {
                dlb_vec_pop(animation_names);
                dlb_vec_push(animation_names, dude_wave);
            }
        }

        // DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG
        //animation_time_sec = 0.0f;
        // DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG

#if 1
        dlb_vec_each(const char **, animation_name, animation_names) {
            ta_animation *animation = (ta_animation *)ta_game_by_sym_try(RES_ANIMATION, *animation_name);
            if (animation && animation->tracks) {
                size_t animation_frames = dlb_vec_len(animation->tracks[0].time.key.values.as_float);
                dlb_vec_each(ta_animation_track *, track, animation->tracks) {
                    // Samples must be linearly interpolated values
                    DLB_ASSERT(track->time.curve == TA_ANIMATION_TRACK_CURVE_LINEAR);
                    DLB_ASSERT(track->value.curve == TA_ANIMATION_TRACK_CURVE_LINEAR);
                    DLB_ASSERT(track->time.key.kind == TA_ANIMATION_TRACK_KEY_VALUE);
                    DLB_ASSERT(track->value.key.kind == TA_ANIMATION_TRACK_KEY_VALUE);

                    // Samples must all be equal in length (including across tracks, for now)
                    const size_t time_count = dlb_vec_len(track->time.key.values.as_float);
                    const size_t value_count = dlb_vec_len(track->value.key.values.as_float);
                    DLB_ASSERT(time_count == value_count);  // NOTE: This is strictly necessary
                    DLB_ASSERT(time_count == animation_frames);  // NOTE: This is not necessary; enforces fully sampled

                    // Target node must have a valid transform component
                    ta_transform *transform = (ta_transform *)ta_game_component(track->target_node, RES_COMP_TRANSFORM);

                    // Times must always be floats
                    DLB_ASSERT(track->time.key.type == ATOM_FLOAT);

                    // TODO: Binary search time values, where mid defaults to the index that we found last frame
                    int right = 0;
                    int count = (int)dlb_vec_len(track->time.key.values.as_float);
                    while (right < count && track->time.key.values.as_float[right] < animation_time_sec) {
                        right++;
                    }

                    // We only know how to handle transforms and rotations for now
                    switch (track->value.key.type) {
                        case TYP_VEC3: {
                            DLB_ASSERT(track->target_path == SYM_TRANSLATION);
                            ta_vec3 lerp = { 0 };

                            if (right == 0) {
                                lerp = track->value.key.values.as_vec3[0];
                            } else if (right == count) {
                                lerp = track->value.key.values.as_vec3[count - 1];
                            } else {
                                //float t0 = track->time.key.values.as_float[right - 1];
                                //float t1 = track->time.key.values.as_float[right];
                                //float alpha = (animation_time_sec - t0) * (t1 - t0);
                                //ta_vec3 a = track->value.key.values.as_vec3[right - 1];
                                //ta_vec3 b = track->value.key.values.as_vec3[right];
                                //lerp.x = a.x + (b.x - a.x) * alpha;
                                //lerp.y = a.y + (b.y - a.y) * alpha;
                                //lerp.z = a.z + (b.z - a.z) * alpha;

                                // HACK: No interpolation for now
                                lerp = track->value.key.values.as_vec3[right];
                            }

                            //float y = lerp.y;
                            //lerp.y = -lerp.z;
                            //lerp.z = y;

                            transform->xform.position = lerp;

                            break;
                        } case TYP_VEC4: {
                            DLB_ASSERT(track->target_path == SYM_ROTATION);
                            ta_vec4 lerp = { 0 };
                            if (right == 0) {
                                lerp = track->value.key.values.as_vec4[0];
                            } else if (right == count) {
                                lerp = track->value.key.values.as_vec4[count - 1];
                            } else {
                                // HACK: No interpolation for now
                                lerp = track->value.key.values.as_vec4[right];
                            }
                            transform->xform.orientation = quat_normalize(lerp);
                            break;
                        } default: {
                            DLB_ASSERT(!"That target_path is not currently handled");
                            break;
                        }
                    }
                }
            }
        }
#endif

        //----------------------------------------------------------------------
        // Post-simulation updates (e.g. recalculate cached transform matrices)
        //----------------------------------------------------------------------

        // Update cameras
        // TODO: This might belong in game_simulate. It definitely needs to handle dt properly. Maybe
        // cameras should just have rigid_body components? Hmm..
        dlb_vec_each(ta_camera *, cam, (ta_camera *)ta_game_resource_pool(RES_COMP_CAMERA)) {
            const float dt = 0.0f;
            ta_camera_update(cam, dt);
        }

        // HACK: Hacky stuff for player movement
        {
            // Rotate player to match player camera, camera is parented to player object
            // Calculate camera rotation in X/Z plane
            ta_transform *camera_transform = (ta_transform *)ta_game_component(tg_e_player_camera, RES_COMP_TRANSFORM);
            ta_vec3 camera_rot_vec = quat_mul_vec3(camera_transform->xform.orientation, VEC3_NZ);
            camera_rot_vec.y = 0.0f;
            // NOTE: We are negating because player mesh faces +Z in Blender
            camera_rot_vec = vec3_neg(vec3_normalize(camera_rot_vec));
            ta_vec4 camera_rot_quat = quat_from_vec_vec(VEC3_NZ, camera_rot_vec);

            ta_transform *player_transform = (ta_transform *)ta_game_component(tg_e_player_one, RES_COMP_TRANSFORM);
            player_transform->xform.orientation = camera_rot_quat;

            //ta_vec3 player_cam_offset = { 0.0f, 4.0f, 0.7f };
            ta_vec3 player_cam_offset = { 0.0f, 1.7f, 0.7f };
            player_cam_offset = quat_mul_vec3(camera_rot_quat, player_cam_offset);
            camera_transform->xform.position = vec3_add(player_transform->xform.position, player_cam_offset);

            // HACK: Bring guy back into the world!
            if (player_transform->xform.position.y < -10.0f) {
                player_transform->xform.position.y = 1.0f;
            }

            // HACK: Bring can back into the world!
            ta_transform *can_xform = (ta_transform *)ta_game_component_try(tg_e_can, RES_COMP_TRANSFORM);
            ta_rigid_body *can_body = (ta_rigid_body *)ta_game_component_try(tg_e_can, RES_COMP_RIGID_BODY);
            if (can_xform->xform.position.y < -10.0f) {
                can_xform->xform.position = vec3_init(0.0f, 3.2f, -5.0f);
                can_xform->xform.orientation = QUAT_IDENT;
                can_body->velocity = VEC3_ZERO;
                can_body->ang_velocity = VEC3_ZERO;
            }
        }

        // Update transforms (model matrix and lerp)
        ta_transform_update_all(transforms, sim_alpha);

        // Update buttons
        dlb_vec_each(ta_e_button *, button, (ta_e_button *)ta_game_resource_pool(RES_COMP_BUTTON)) {
            e_button_update(button);
        }

        // Update camera cached stuff
        dlb_vec_each(ta_camera *, camera, (ta_camera *)ta_game_resource_pool(RES_COMP_CAMERA)) {
            ta_camera_update_world_view(camera);
        }

        // TODO: Only do this if the lights are dirty? Tracking that might not be worth the effort..
        // Update light data UBO
        ta_light_ubo_bind(&tg_game.light_ubo);

        // TODO: Only do this if the materials are dirty? Tracking that might not be worth the effort..
        // Update material UBO
        ta_material_ubo_bind(&tg_game.material_ubo);

        //----------------------------------------------------------------------
        // Skinning
        //----------------------------------------------------------------------
        dlb_vec_each(ta_mesh *, mesh, meshes) {
            // Always set bone_xforms dirty for first frame to populate with zeros
            mesh->skin.bone_xforms_dirty = (tg_game.frame_num == 0);

            if (!mesh->skin.bone_count_array) {
                continue;
            }

            // TODO: Handle skin transforms
            //DLB_ASSERT(vec3_zero(mesh->skin.transform.position));
            //DLB_ASSERT(quat_ident(mesh->skin.transform.orientation));

            ta_mat4 skin_pos = mat4_translate(mesh->skin.transform.position);
            ta_mat4 skin_rot = mat4_rotate_quat(mesh->skin.transform.orientation);
            ta_mat4 skin_pose = mat4_mul(&skin_pos, &skin_rot);

            size_t bone_count = dlb_vec_len(mesh->skin.skeleton.bones);
            DLB_ASSERT(dlb_vec_len(mesh->skin.skeleton.bind_pose_positions) == bone_count);
            DLB_ASSERT(dlb_vec_len(mesh->skin.skeleton.bind_pose_orientations) == bone_count);

            ta_bone *bone = (ta_bone *)ta_game_component(mesh->skin.skeleton.bones[0], RES_COMP_BONE);
            ta_transform *armature = (ta_transform *)ta_game_component(bone->armature, RES_COMP_TRANSFORM);

            // TODO: Could cache this, but I'm not sure how much duplication there is here (seems like an armature
            // would only ever be used by one mesh at a time, right?)
            ta_mat4 armature_inv = { 0 };
            DLB_ASSERT(mat4_inverse(&armature->world, &armature_inv));

            size_t bone_idx = 0;
            dlb_vec_each(const char **, bone_name, mesh->skin.skeleton.bones) {
                ta_transform *transform = (ta_transform *)ta_game_component(*bone_name, RES_COMP_TRANSFORM);

                // TODO: Pre-calculate inverse bind poses as mat4 to avoid duplicating this work every frame
                ta_mat4 bone_bind_pos = mat4_translate(mesh->skin.skeleton.bind_pose_positions[bone_idx]);
                ta_mat4 bone_bind_rot = mat4_rotate_quat(mesh->skin.skeleton.bind_pose_orientations[bone_idx]);
                ta_mat4 bone_bind_pose = mat4_mul(&bone_bind_pos, &bone_bind_rot);
                ta_mat4 bone_bind_pose_inv = { 0 };
                DLB_ASSERT(mat4_inverse(&bone_bind_pose, &bone_bind_pose_inv));

                // TODO: This could also be cached
                ta_mat4 skinned = mat4_mul(&bone_bind_pose_inv, &skin_pose);

                skinned = mat4_mul(&transform->world, &skinned);
                skinned = mat4_mul(&armature_inv, &skinned);
                mesh->skin.bone_xforms[bone_idx] = skinned;

                // TODO: Update mesh->skin.bone_normal_xforms[bone_idx]... but how?

                bone_idx++;
            }

            mesh->skin.bone_xforms_dirty = true;
        }

        //----------------------------------------------------------------------
        // Shadow pass
        //----------------------------------------------------------------------
        ta_log_write(&tg_debug_log, SRC_GAME, " Shadow pass...\n");

        // TODO: Try VSM, then CSM.
        // NOTE: If we switch to back face culling it will prevent light leaks, but
        // cause a lot more jitter on the lit side. :(
        //glCullFace(GL_FRONT);
        //glClearColor(FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX);
        //glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

        dlb_vec_each(ta_light *, light, lights) {
            ta_light_shadowpass_render(light, models);
        }
        ta_shader_unbind();
        //glCullFace(GL_BACK);
        glViewport(0, 0, WINDOW_W, WINDOW_H);

        //----------------------------------------------------------------------
        // Render pass
        //----------------------------------------------------------------------
        ta_log_write(&tg_debug_log, SRC_GAME, " Render pass...\n");

        // TODO: Render into post-process framebuffer, then apply FXAA or MSAA before rendering HUD/UI
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glEnable(GL_MULTISAMPLE);
        glStencilMask(0xFF);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        glStencilMask(0x00);

        size_t lights_len = dlb_vec_len(lights);
        u32 u_lights_count = 0;
        for (u32 i = 0; i < lights_len; ++i) {
            if (!lights[i].disabled) {
                //ta_shader_set_light(shader, SYM_U_LIGHTS, u_lights_count, &lights[i]);
                u_lights_count++;
            }
        }

        // TODO: Use a UBO?
        dlb_vec_each(ta_shader *, shader, shaders) {
            ta_shader_set_mat4_try(shader, SYM_U_PROJ, &active_camera->projection);
            ta_shader_set_mat4_try(shader, SYM_U_VIEW, &active_camera->look_at);
            // NOTE: These only happen for "mesh" shader atm but wutevs..
            ta_shader_set_vec3_try(shader, SYM_U_CAMERA_POS, &active_camera_trans->xform_world.position);
            ta_shader_set_int_try(shader, SYM_U_DEBUG_CHANNEL, active_camera->dbg_channel);
            ta_shader_set_uint_try(shader, SYM_U_LIGHTS_COUNT, u_lights_count);
        }

        // Dump any prims from the collision pass
        game_render_manifolds_debug();
        ta_primitive_render_mesh(&primitive_lines_perma, tg_shader_lines, false);
        ta_primitive_dump(true);

        //----------------------------------------------------------------------
        // Render models
        //----------------------------------------------------------------------
        if (tg_game.debug_wireframe) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        }

        dlb_vec_each(ta_model *, model, models) {
            ta_model_render(model);
        }

        if (tg_game.debug_wireframe) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }

        ta_primitive_dump(true);

        //----------------------------------------------------------------------
        // Render skybox
        //----------------------------------------------------------------------
        game_render_skybox();

        //----------------------------------------------------------------------
        // Debug rendering
        //----------------------------------------------------------------------
        if (tg_game.debug_colliders) {
            ta_log_write(&tg_debug_log, SRC_GAME, " Debug colliders pass...\n");
            glDisable(GL_CULL_FACE);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            game_render_colliders_debug();
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glEnable(GL_CULL_FACE);
        }
        if (tg_game.debug_nametags) {
            ta_log_write(&tg_debug_log, SRC_GAME, " Debug nametags pass...\n");
            game_render_nametags_debug(active_camera);
        }

#if 0
        // TODO(cleanup): Temporarily render support point of OBB
        ta_rigid_body *can_body = ta_game_by_sym(RES_COMP_RIGID_BODY, tg_e_can);
        if (can_body) {
            DLB_ASSERT(can_body->collider.type == TA_COLLIDER_OBB);
            ta_obb world_obb = can_body->collider.data.obb;
            world_obb.center = rigid_body_local_to_world(can_body, world_obb.center);
            world_obb.orientation = rigid_body_oriented_quaternion(can_body, world_obb.orientation);
            ta_vec3 camera_back = vec3_neg(active_camera->front);
            ta_vec3 sup = ta_support_obb(&world_obb, camera_back);
            ta_sphere sphere = { 0 };
            sphere.center = sup;
            sphere.radius = 0.01f;
            ta_primitive_push_sphere(0, sphere, TA_COLOR_RED);
            ta_primitive_render_mesh(&primitive_lines, tg_shader_lines, true);
        }
#endif
#if 0
        // TODO(cleanup): Temporary sphere benchmark shenanigans to compare to instanced rendering later
        {
            float sphere_hell_radius = 16.0f;  // note: cubed radius = # of spheres
            ta_vec3 sphere_pos = { 0 };
            ta_sphere sphere = { 0 };
            for (float x = 0.0f; x < sphere_hell_radius; x += 1.0f) {
                for (float y = 0.0f; y < sphere_hell_radius; y += 1.0f) {
                    for (float z = 0.0f; z < sphere_hell_radius; z += 1.0f) {
                        sphere.center = vec3_init(x, y, z);
                        sphere.radius = 0.4f;
                        ta_primitive_push_sphere(0, sphere, TA_COLOR_RED);
                    }
                }
            }
            ta_primitive_dump(true);
        }
#endif
#if 0
        // TODO(cleanup): Temporary sphere benchmark shenanigans to compare to instanced rendering later
        ta_model *sphere_model = ta_game_by_sym_try(RES_COMP_MODEL, INTERN("ball2"));
        if (sphere_model) {
            ta_transform *sphere_transform = ta_game_by_sym(RES_COMP_TRANSFORM, sphere_model->entity);
            ta_vec3 pos_orig = sphere_transform->xform.position;

            float sphere_hell_radius = 16.0f;  // note: cubed radius = # of spheres
            ta_vec3 sphere_pos = { 0 };
            for (float x = 0.0f; x < sphere_hell_radius; x += 1.0f) {
                for (float y = 0.0f; y < sphere_hell_radius; y += 1.0f) {
                    for (float z = 0.0f; z < sphere_hell_radius; z += 1.0f) {
                        sphere_transform->xform.position = vec3_init(x, y, z);
                        sphere_transform->dirty_flag = ta_transform_dirty_flag;
                        ta_transform_update(sphere_transform, 1.0f, ta_transform_dirty_flag);
                        ta_model_render(sphere_model);
                    }
                }
            }

            sphere_transform->xform.position = pos_orig;
            sphere_transform->dirty_flag = ta_transform_dirty_flag;
            ta_transform_update(sphere_transform, 1.0f, ta_transform_dirty_flag);
        }
#endif
#if 0
        ta_primitive_render_mesh(&primitive_sphere, tg_shader_lines, false);
#endif

        //----------------------------------------------------------------------
        // GJK Debug Shapes
        //----------------------------------------------------------------------
#if 0
        static ta_obb gjk_obb_a = {
            { 0.0f, 0.0f, 0.0f },       // center
            { 0.5f, 0.5f, 0.5f },       // extents
            { 0.0f, 0.0f, 0.0f, 1.0f }  // orientation
        };
#else
        static ta_obb gjk_obb_a = {
            { 1.35f, 0.0f, 0.0f },       // center
            { 0.5f, 0.5f, 0.5f },       // extents
            { 0.0f, 0.444f, 0.0f, 0.896f }  // orientation
        };
#endif
        static ta_obb gjk_obb_b = {
            { 0.0f, 0.0f, 0.0f },       // center
            { 0.5f, 0.5f, 0.5f },       // extents
            { 0.0f, 0.0f, 0.0f, 1.0f }  // orientation
        };
        gjk_obb_a.orientation = quat_normalize(gjk_obb_a.orientation);
        bool intersect = ta_gjk_intersect_obb(&gjk_obb_a, &gjk_obb_b);
        ta_primitive_push_obb(0, gjk_obb_a, intersect ? TA_COLOR_RED : TA_COLOR_GREEN);
        ta_primitive_push_obb(0, gjk_obb_b, intersect ? TA_COLOR_RED : TA_COLOR_GREEN);
        ta_primitive_dump(true);

        //----------------------------------------------------------------------
        // Editor UI (world)
        //----------------------------------------------------------------------
        const char *editor_closest_entity = 0;

        if (tg_game.state == TA_STATE_EDITOR) {
            ta_log_write(&tg_debug_log, SRC_GAME, " Editor world UI pass...\n");

            ta_editor_update_widgets();

            // Grid and world axes
            //ta_primitive_push_grid(0, VEC3_ZERO, VEC3_Y, 1000.0f, 1.0f, TA_COLOR_GRAY3);
            ta_primitive_push_axes_arrow(0, VEC3_ZERO, QUAT_IDENT, 0.3f);

            // Render cameras as OBBs with forward arrows
            dlb_vec_each(ta_camera *, camera, cameras) {
                if (camera->name != tg_game.active_camera) {
                    ta_transform *cam_trans = (ta_transform *)ta_game_component(camera->entity, RES_COMP_TRANSFORM);
                    ta_obb obb = { 0 };
                    obb.center = cam_trans->xform_world.position;
                    obb.extents = (ta_vec3){ 0.2f, 0.2f, 0.2f };
                    obb.orientation = cam_trans->xform_world.orientation;
                    ta_primitive_push_obb(0, obb, TA_COLOR_WHITE);
                    ta_vec3 dir = quat_mul_vec3(obb.orientation, VEC3_NZ);
                    ta_primitive_push_arrow(0, obb.center, dir, TA_COLOR_WHITE);

                    // TODO: Camera icon on billboarded quad in world space
                    //ta_primitive_push_billboard();
                }
            }

            // Render lights as colored spheres
            dlb_vec_each(ta_light *, light, lights) {
                ta_transform *transform = (ta_transform *)ta_game_component(light->entity, RES_COMP_TRANSFORM);

                ta_sphere light_pos = { 0 };
                light_pos.center = transform->xform_world.position;
                light_pos.radius = 0.2f;
                ta_rgba color = { 0 };
                if (!light->disabled) {
                    color.r = light->color.r;
                    color.g = light->color.g;
                    color.b = light->color.b;
                } else {
                    color.r = 0.5f;
                    color.g = 0.5f;
                    color.b = 0.5f;
                }
                ta_primitive_push_sphere(0, light_pos, color);

                if (tg_game.debug_light_radii) {
                    ta_sphere light_aoe = { 0 };
                    light_aoe.center = transform->xform_world.position;
                    light_aoe.radius = 1.0;
                    switch (light->type) {
                        case TA_LIGHT_DIRECTIONAL:
                            light_aoe.radius = light->data.directional.shadow_properties.zfar;
                            break;
                        case TA_LIGHT_POINT:
                            light_aoe.radius = light->data.point.shadow_properties.zfar;
                            break;
                        case TA_LIGHT_SPOT:
                            light_aoe.radius = light->data.spot.shadow_properties.zfar;
                            break;
                    }
                    ta_primitive_push_sphere(0, light_aoe, color);
                }
            }
            ta_primitive_render_mesh(&primitive_lines, tg_shader_lines, true);

            editor_closest_entity = ta_editor_closest_entity();
            if (editor_closest_entity && editor_closest_entity != editor.selected_entity) {
                glEnable(GL_POLYGON_OFFSET_LINE);
                glPolygonOffset(-0.2f, -10.0f);
                ta_editor_draw_entity_wireframe(editor_closest_entity, TA_COLOR_GRAY8, false);
                glDisable(GL_POLYGON_OFFSET_LINE);
            }

            ta_editor_draw_world();
        }

        //----------------------------------------------------------------------
        // Transition boundary from world rendering to screen rendering
        //----------------------------------------------------------------------
        glClear(GL_DEPTH_BUFFER_BIT);

        // TODO: Implement my own AA as a post-process step (*before* rendering UI/font) because
        // this flag has no effect on my NVIDIA card (nor, apparently, does the GL spec require it to).
        //glDisable(GL_MULTISAMPLE);

        // TODO: Use a UBO? or whatever it's called when you share uniforms between multiple shaders
        dlb_vec_each(ta_shader *, shader, shaders) {
            ta_shader_reset_pvm(shader);
        }

        // NOTE: We have to do this after editor_draw_world because that method
        // is using the last frame's hover flag to determine whether or not an
        // uncaptured mouse can interact with the editor widgets.
        ta_ui_flags_reset();

#if 0
        //----------------------------------------------------------------------
        // Crazy bone debug viz; temporary
        //----------------------------------------------------------------------
        //ta_shader_set_mat4(tg_shader_lines, SYM_U_PROJ, &active_camera->projection);
        //ta_shader_set_mat4(tg_shader_lines, SYM_U_VIEW, &active_camera->look_at);
        //ta_shader_set_mat4(tg_shader_quads, SYM_U_PROJ, &active_camera->projection);

        static ta_ui_panel_state bone_labels = { 0 };
        if (ta_mouse_captured()) {
            ta_ui_next_offset(WINDOW_W / 2, WINDOW_H / 2);
        } else {
            ta_ui_next_offset(ta_mouse_x(), ta_mouse_y() + 20);
        }
        ta_ui_next_pad(0, 0, 0, 0);
        ta_ui_panel_begin(&bone_labels, TA_UI_AUTOSIZE);

        dlb_vec_each(ta_transform *, transform, transforms) {
            if (ta_game_component_try(transform->entity, RES_COMP_CAMERA)) {
                continue;
            }

            ta_vec3 pos = transform->xform_world.position;
            ta_vec4 rot = transform->xform_world.orientation;
            float axes_scale = 0.1f;

            ta_sphere sphere = { 0 };
            sphere.center = pos;
            sphere.radius = axes_scale;
            //ta_primitive_push_sphere(0, sphere, node->type == OGX_BONE_NODE ? TA_COLOR_CYAN : TA_COLOR_WHITE);

            ta_ray ray = { 0 };
            ta_editor_select_ray(&ray);
            if (!ta_ui_flag_hovered() && ta_ray_v_sphere(&ray, &sphere, 0)) {
                ta_ui_row_begin();
                ta_ui_label(SYM(transform->name), 0);
                axes_scale *= 2.0f;
            }

            ta_primitive_push_axes_arrow(0, pos, rot, axes_scale);
        }

        ta_ui_panel_end();

        ta_primitive_dump(true, false);
        ta_ui_render();
#endif
#if 1
        // HACK: This probably should go in the editor or something..
        //----------------------------------------------------------------------
        // Screen space tooltips
        //----------------------------------------------------------------------
        if (editor_closest_entity) {
            ta_transform *transform = ta_game_component(editor_closest_entity, RES_COMP_TRANSFORM);

            // TODO: Component-specific tooltip data could be cool
            //if (ta_game_component_try(editor_closest_entity, RES_COMP_CAMERA)) {
            //    continue;
            //}

            static ta_ui_panel_state hover_tooltip = { 0 };
            if (ta_mouse_captured()) {
                ta_ui_next_offset(WINDOW_W / 2, WINDOW_H / 2);
            } else {
                ta_ui_next_offset(ta_mouse_x(), ta_mouse_y() + 20);
            }
            ta_ui_next_pad(0, 0, 0, 0);
            ta_ui_panel_begin(&hover_tooltip, TA_UI_AUTOSIZE);
            ta_ui_label(SYM(transform->name));
            //ta_ui_row_begin();
            //static ta_ui_textbox_vec3_state tx_pos = { 0 };
            //ta_ui_textbox_vec3(&transform->xform_world.position, &tx_pos, false, false);
            //ta_ui_row_begin();
            //static ta_ui_textbox_vec4_state tx_rot = { 0 };
            //ta_ui_textbox_vec4(&transform->xform_world.orientation, &tx_rot, false, false);
            ta_ui_panel_end();

            ta_ui_render();
        }
#endif

        //----------------------------------------------------------------------
        // Crosshair
        //----------------------------------------------------------------------
        if (ta_mouse_captured()) {
            ta_primitive_push_crosshair(0, 10, 2);
            ta_primitive_render_mesh(&primitive_quads, tg_shader_quads, true);
        }

        //----------------------------------------------------------------------
        // Game HUD
        //----------------------------------------------------------------------
        // TODO: Make HUD drawing suck less.. way too many draw calls
        //       Use texture atlas; batch everything into one draw call; stop
        //       using stupid RGB placeholders.
        ta_log_write(&tg_debug_log, SRC_GAME, " HUD pass...\n");
        game_draw_hud();

#if 0
        //----------------------------------------------------------------------
        // Minimap
        //----------------------------------------------------------------------
        {
            // Target minimap camera
            ta_transform *active_cam_trans = (ta_transform *)ta_game_component(active_camera->entity, RES_COMP_TRANSFORM);
            ta_vec3 minimap_target_pos = active_cam_trans->xform_world.position;
            minimap_target_pos.y += 50.0f;
            tg_game.minimap_camera.focal_point = active_cam_trans->xform_world.position;
            ta_camera_set_target_pos_absolute(&tg_game.minimap_camera, minimap_target_pos);

            // Render minimap
            ta_rect map_rect = { 10, 50, 200, 200 };
            ta_viewport_bind(map_rect, TA_COLOR_GRAY7, true);
            ta_scene_render(&tg_game.scene, &minimap_camera, sim_alpha);
            ta_viewport_unbind();
            ta_primitive_dump(true, true);

            // Red dot on map
            int dot_radius = 2;
            ta_rect dot_rect = { 0 };
            dot_rect.x = map_rect.x + map_rect.w / 2 - dot_radius;
            dot_rect.y = map_rect.y + map_rect.h / 2 - dot_radius;
            dot_rect.w = dot_radius * 2;
            dot_rect.h = dot_radius * 2;
            ta_primitive_push_rect(dot_rect, TA_COLOR_RED, UI_LAYER_HUD);
            ta_primitive_dump(true, true);
        }
#endif

#if 0
        // TODO(cleanup): This could be useful to visualize FPS, but it's pretty
        // useless in its current state.
        //----------------------------------------------------------------------
        // Barchart
        //----------------------------------------------------------------------
        ta_shader_set_mat4(tg_shader_lines, SYM_U_PROJ, &MAT4_IDENT);
        ta_shader_set_mat4(tg_shader_lines, SYM_U_VIEW, &MAT4_IDENT);
        ta_shader_set_mat4(tg_shader_lines, SYM_U_MODEL, &MAT4_IDENT);
        ta_ui_barchart_draw(&chart, 0, 0);
        ta_primitive_dump(true, true);
#endif

        //----------------------------------------------------------------------
        // Editor UI (screen)
        //----------------------------------------------------------------------
        if (tg_game.state == TA_STATE_EDITOR) {
            ta_log_write(&tg_debug_log, SRC_GAME, " Editor screen UI pass...\n");
            ta_editor_draw_screen();
        }

        //----------------------------------------------------------------------
        // Console UI (screen)
        //----------------------------------------------------------------------
        if (tg_game.console_visible) {
            ta_log_write(&tg_debug_log, SRC_GAME, " Console screen UI pass...\n");
            ta_console_draw_screen();
        }

#if 1
        static ta_ui_window_state window = { 0 };
        ta_ui_window_begin(&window, TA_UI_AUTOSIZE);

        ta_ui_row_begin();
        ta_ui_label(CSTR("obb_a.center     "));
        static ta_ui_textbox_vec3_state gjk_center_a = { 0 };
        ta_ui_textbox_vec3(&gjk_obb_a.center, &gjk_center_a, false, true);

        ta_ui_row_begin();
        ta_ui_label(CSTR("obb_a.extents    "));
        static ta_ui_textbox_vec3_state gjk_extents_a = { 0 };
        ta_ui_textbox_vec3(&gjk_obb_a.extents, &gjk_extents_a, false, false);

        ta_ui_row_begin();
        ta_ui_label(CSTR("obb_a.orientation"));
        static ta_ui_textbox_vec4_state gjk_orient_a = { 0 };
        ta_ui_textbox_vec4(&gjk_obb_a.orientation, &gjk_orient_a, true, true);

        ta_ui_row_begin();
        ta_ui_label(CSTR("obb_b.center     "));
        static ta_ui_textbox_vec3_state gjk_center_b = { 0 };
        ta_ui_textbox_vec3(&gjk_obb_b.center, &gjk_center_b, false, true);

        ta_ui_row_begin();
        ta_ui_label(CSTR("obb_b.extents    "));
        static ta_ui_textbox_vec3_state gjk_extents_b = { 0 };
        ta_ui_textbox_vec3(&gjk_obb_b.extents, &gjk_extents_b, false, false);

        ta_ui_row_begin();
        ta_ui_label(CSTR("obb_b.orientation"));
        static ta_ui_textbox_vec4_state gjk_orient_b = { 0 };
        ta_ui_textbox_vec4(&gjk_obb_b.orientation, &gjk_orient_b, true, true);

        ta_ui_window_end();
        glDisable(GL_DEPTH_TEST);
        ta_ui_render();
        glEnable(GL_DEPTH_TEST);
#endif

#if 0
        static ta_ui_window_state window = { 0 };
        u32 flags = TA_UI_AUTOSIZE;
        ta_ui_window_begin(&window, flags);
        static ta_ui_textbox_vec3_state tb3[4] = { 0 };
        ta_ui_row_begin();
        ta_ui_label(CSTR("ray.origin      "));
        ta_ui_textbox_vec3(&ray.origin, &tb3[0], 0, 0, 0);
        ta_ui_row_begin();
        ta_ui_label(CSTR("ray.direction   "));
        ta_ui_textbox_vec3(&ray.direction, &tb3[1], 0, 0, 0);
        ta_ui_row_begin();
        ta_ui_label(CSTR("quad.center     "));
        ta_ui_textbox_vec3(&quad.center, &tb3[2], 0, 0, 0);
        static ta_ui_textbox_vec2_state tb2 = { 0 };
        ta_ui_row_begin();
        ta_ui_label(CSTR("quad.extents    "));
        ta_ui_textbox_vec2(&quad.extents, &tb2, 0, 0, 0);
        static ta_ui_textbox_vec4_state tb4 = { 0 };
        ta_ui_row_begin();
        ta_ui_label(CSTR("quad.orientation"));
        ta_ui_textbox_vec4(&quad.orientation, &tb4, 1, 0, 0);
        static ta_ui_textbox_state tb = { 0 };
        ta_ui_row_begin();
        ta_ui_label(CSTR("ray_t           "));
        ta_ui_textbox_float(&ray_t, &tb, TA_UI_AUTOSIZE);
        ta_ui_window_end();
        glClear(GL_DEPTH_BUFFER_BIT);
        ta_ui_render();
#endif

#if 0
        static ta_ui_window_state window = { 0 };
        u32 flags = TA_UI_AUTOSIZE;
        ta_ui_window_begin(&window, flags);

        ta_ui_row_begin();
        ta_ui_label(CSTR("animation_time_sec: "));
        ta_ui_label_float(animation_time_sec);

        ta_ui_window_end();
        glClear(GL_DEPTH_BUFFER_BIT);
        ta_ui_render();
#endif

        //----------------------------------------------------------------------
        // Audio
        //----------------------------------------------------------------------
        // TODO: dlb_vec_each(ta_audio_listener_update)
        ta_transform *active_cam_trans = (ta_transform *)ta_game_component(active_camera->entity, RES_COMP_TRANSFORM);
        ta_vec3 fwd_up[2] = { 0 };
        fwd_up[0] = active_camera->front;
        fwd_up[1] = active_camera->up;
        alListenerfv(AL_ORIENTATION, (float *)fwd_up);
        alListenerfv(AL_POSITION, (float *)&active_cam_trans->xform_world.position);
        //alListenerfv(AL_VELOCITY, (float *)&tg_game.camera->velocity);

        ta_log_write(&tg_debug_log, SRC_GAME, " Audio update...\n");
        ta_audio_update();

        ta_log_write(&tg_debug_log, SRC_GAME, " Update cursor...\n");
        ta_window_update_cursor(tg_window);
        ta_ui_set_cursor(TA_CURSOR_ARROW);

        // TODO: Add "show_fps" flag and bind to key; off by default in release
        ta_log_write(&tg_debug_log, SRC_GAME, " FPS pass...\n");
        tg_game.frame_num++;
        ms_frame_logic = ta_timer_elapsed_ms() - ms_frame_start;
        game_draw_frame_info(tg_game.frame_num, ms_frame_logic, ms_frame_delta, tg_game.sim_step);

        //----------------------------------------------------------------------
        // BOOM! It's swap time, baby! Show the player all of our hard work.
        //----------------------------------------------------------------------
        // NOTE: This confirms rendering is being deferred until swap buffers,
        // but it's much slower (~5ms), so don't actually use it.
        //ta_log_write(&tg_debug_log, SRC_GAME, " glFinish...\n");
        //glFinish();
        ta_log_write(&tg_debug_log, SRC_GAME, " Swap...\n");
        ta_window_swap(tg_window);
        TracyCFrameMark;

        ta_log_write(&tg_debug_log, SRC_GAME,
            "Frame %llu displayed. time: %5.3f delta: %5.3f\n",
            tg_game.frame_num, ms_frame_logic, ms_frame_delta);

        // Sob profusely when frame time goes over 16ms
        if (ms_frame_logic > 16) {
            ta_log_write(&tg_debug_log, SRC_GAME, "!!!!!!!! LONG_FRAME !!!!!!!!\n");
            ta_log_flush(&tg_debug_log);
            // TODO: Debug more long frames (turn on SRC_GAME logging)
            //__debugbreak();
        }
    }
}
void game_command_play()
{
    ta_game_state_set(TA_STATE_PLAY);
}
void game_command_free_cam()
{
    ta_game_state_set(TA_STATE_FREE_CAM);
}
void game_command_console_toggle()
{
    tg_game.console_visible = !tg_game.console_visible;
}
void game_command_console_exit()
{
    if (tg_game.console_visible) {
        tg_game.console_visible = false;
    } else {
        ta_keybind_not_handled(&tg_game.keybinds[COMMAND_CONSOLE_HIDE]);
    }
}
void game_command_editor()
{
    editor.prev_state = ta_game_state_current();
    ta_game_state_set(TA_STATE_EDITOR);
}
void game_command_shutdown()
{
    ta_game_state_set(TA_STATE_SHUTDOWN);
}
void game_command_toggle_fullscreen()
{
    bool fullscreen = false;
    ta_window_get_fullscreen(tg_window, &fullscreen);
    ta_window_set_fullscreen(tg_window, !fullscreen);
}
void game_command_player_move_forward()
{
    ta_camera *camera = (ta_camera *)ta_game_camera();
    ta_vec3 dir = { 0 };
    dir.x = camera->front.x;
    dir.z = camera->front.z;
    dir = vec3_normalize(dir);
    dir = vec3_scalef(dir, player_move_impulse);
    ta_rigid_body *player_body = (ta_rigid_body *)ta_game_component(tg_e_player_one, RES_COMP_RIGID_BODY);
    ta_rigid_body_apply_impulse(player_body, dir, VEC3_ZERO);
}
void game_command_player_move_backward()
{
    ta_camera *camera = (ta_camera *)ta_game_camera();
    ta_vec3 dir = { 0 };
    dir.x = -camera->front.x;
    dir.z = -camera->front.z;
    dir = vec3_normalize(dir);
    dir = vec3_scalef(dir, player_move_impulse);
    ta_rigid_body *player_body = (ta_rigid_body *)ta_game_component(tg_e_player_one, RES_COMP_RIGID_BODY);
    ta_rigid_body_apply_impulse(player_body, dir, VEC3_ZERO);
}
void game_command_player_move_right()
{
    ta_camera *camera = (ta_camera *)ta_game_camera();
    ta_vec3 dir = { 0 };
    dir.x = camera->right.x;
    dir.z = camera->right.z;
    dir = vec3_normalize(dir);
    dir = vec3_scalef(dir, player_move_impulse);
    ta_rigid_body *player_body = (ta_rigid_body *)ta_game_component(tg_e_player_one, RES_COMP_RIGID_BODY);
    ta_rigid_body_apply_impulse(player_body, dir, VEC3_ZERO);
}
void game_command_player_move_left()
{
    ta_camera *camera = (ta_camera *)ta_game_camera();
    ta_vec3 dir = { 0 };
    dir.x = -camera->right.x;
    dir.z = -camera->right.z;
    dir = vec3_normalize(dir);
    dir = vec3_scalef(dir, player_move_impulse);
    ta_rigid_body *player_body = (ta_rigid_body *)ta_game_component(tg_e_player_one, RES_COMP_RIGID_BODY);
    ta_rigid_body_apply_impulse(player_body, dir, VEC3_ZERO);
}
void game_command_player_jump()
{
    ta_rigid_body *player_body = (ta_rigid_body *)ta_game_component(tg_e_player_one, RES_COMP_RIGID_BODY);
    if (ta_rigid_body_colliding_with(player_body, tg_e_ground)) {
        ta_vec3 dir = vec3_scalef(VEC3_Y, player_jump_impulse);
        ta_rigid_body_apply_impulse(player_body, dir, VEC3_ZERO);
    }
}
static void spawn_bullet(ta_vec3 position)
{
    static const char *bullet_names[] = {
        "bullet_01",
        "bullet_02",
        "bullet_03",
        "bullet_04",
        "bullet_05",
        "bullet_06",
        "bullet_07",
        "bullet_08",
        "bullet_09",
    };
    static size_t next_idx = 0;
    DLB_ASSERT(next_idx < ARRAY_SIZE(bullet_names));
    //const char *name = ta_symbol_intern(bullet_names[next_idx], strlen(bullet_names[next_idx]));
    next_idx++;

    UNUSED(position);
    //ta_transform *transform = (ta_transform *)ta_game_entity_add(name, position);
    //ta_model *model = (ta_model *)ta_game_component_add(name, RES_COMP_MODEL, CSTR("who cares"));
    //ta_rigid_body *body = (ta_rigid_body *)ta_game_component_add(name, RES_COMP_RIGID_BODY, CSTR("who cares"));
    //DLB_ASSERT(transform);
    //DLB_ASSERT(model);
    //DLB_ASSERT(body);

    //ta_transform_init(transform);
    //transform->xform.position = position;

    //model->
}
void game_command_player_shoot()
{
    static double last_bang_ms = 0;
    static double last_reload_ms = 0;
    static double last_empty_ms = 0;

    ta_player *player = (ta_player *)ta_game_player();
    ta_transform *player_xform = (ta_transform *)ta_game_component(player->entity, RES_COMP_TRANSFORM);
    ta_gun *gun = (ta_gun *)ta_game_component(player->e_gun, RES_COMP_GUN);
    ta_audio_source *src_gun = (ta_audio_source *)ta_game_component(player->e_gun, RES_COMP_AUDIO_SOURCE);

    ta_audio_source_set_position(src_gun, player_xform->xform_world.position);

    double now_ms = ta_timer_elapsed_ms();

    bool fired = false;

    if (gun->loaded_ammo > 0) {
        static double after_bang_delay_ms = 50;
        static double after_reload_delay_ms = 1000;
        if (now_ms < last_bang_ms + after_bang_delay_ms ||
            now_ms < last_reload_ms + after_reload_delay_ms) {
            return;
        }

        float rand_pitch = 1.0f + dlb_rand_variance(0.1f);
        ta_audio_source_set_pitch(src_gun, rand_pitch);
        ta_audio_source_play_name(src_gun, gun->sfx_bang);
        last_bang_ms = ta_timer_elapsed_ms();
        gun->loaded_ammo--;
        fired = true;
    } else {
        if (gun->carrying_ammo) {
            static double after_bang_delay_ms = 500;
            if (now_ms < last_bang_ms + after_bang_delay_ms) {
                return;
            }

            ta_audio_source_play_name(src_gun, gun->sfx_reload);
            last_reload_ms = ta_timer_elapsed_ms();

            gun->loaded_ammo = MIN(gun->loaded_ammo_max, gun->carrying_ammo);
            gun->carrying_ammo -= gun->loaded_ammo;
        } else {
            static double after_bang_delay_ms = 750;
            static double after_empty_delay_ms = 300;
            if (now_ms < last_bang_ms + after_bang_delay_ms ||
                now_ms < last_empty_ms + after_empty_delay_ms) {
                return;
            }

            ta_audio_source_play_name(src_gun, gun->sfx_empty);
            last_empty_ms = ta_timer_elapsed_ms();
        }
    }

    if (fired) {
        ta_transform *can_xform = (ta_transform *)ta_game_component_try(tg_e_can, RES_COMP_TRANSFORM);
        ta_rigid_body *can_body = (ta_rigid_body *)ta_game_component_try(tg_e_can, RES_COMP_RIGID_BODY);
        DLB_ASSERT(can_xform && can_body);
        DLB_ASSERT(can_body->collider.type == TA_COLLIDER_OBB);
        ta_ray bullet = ta_game_camera_ray();
        float t_intersect = 0.0f;

        ta_obb obb = can_body->collider.data.obb;
        obb.center = quat_mul_vec3(can_xform->xform_world.orientation, obb.center);
        obb.center = vec3_add(obb.center, can_xform->xform_world.position);
        obb.orientation = quat_normalize(quat_mul(can_xform->xform_world.orientation, obb.orientation));

        if (ta_ray_v_obb(&bullet, &obb, &t_intersect)) {
            ta_vec3 impulse = vec3_scalef(bullet.direction, 0.1f);;
            ta_vec3 contact_world = vec3_add(bullet.origin, vec3_scalef(bullet.direction, t_intersect));
            ta_vec3 contact_local = rigid_body_world_to_centroid(can_body, contact_world);
            ta_rigid_body_apply_impulse(can_body, impulse, contact_local);
        }
    }
}
void game_command_camera_move_forward()
{
    if (ta_mouse_captured()) {
        ta_camera *camera = (ta_camera *)ta_game_camera();
        camera->move_buffer = vec3_add(camera->move_buffer, camera->front);
    }
}
void game_command_camera_move_backward()
{
    if (ta_mouse_captured()) {
        ta_camera *camera = (ta_camera *)ta_game_camera();
        camera->move_buffer = vec3_sub(camera->move_buffer, camera->front);
    }
}
void game_command_camera_move_right()
{
    if (ta_mouse_captured()) {
        ta_camera *camera = (ta_camera *)ta_game_camera();
        camera->move_buffer = vec3_add(camera->move_buffer, camera->right);
    }
}
void game_command_camera_move_left()
{
    if (ta_mouse_captured()) {
        ta_camera *camera = (ta_camera *)ta_game_camera();
        camera->move_buffer = vec3_sub(camera->move_buffer, camera->right);
    }
}
void game_command_camera_move_up()
{
    if (ta_mouse_captured()) {
        ta_camera *camera = (ta_camera *)ta_game_camera();
        camera->move_buffer = vec3_add(camera->move_buffer, camera->up);
    }
}
void game_command_camera_move_down()
{
    if (ta_mouse_captured()) {
        ta_camera *camera = (ta_camera *)ta_game_camera();
        camera->move_buffer = vec3_sub(camera->move_buffer, camera->up);
    }
}
void game_command_debug_mouse_lock()
{
    ta_mouse_capture_set(true);
}
void game_command_debug_mouse_unlock()
{
    ta_mouse_capture_set(false);
}
void game_command_debug_mouse_lock_toggle()
{
    ta_mouse_capture_toggle();
}
void game_command_debug_toggle_wireframe()
{
    tg_game.debug_wireframe = !tg_game.debug_wireframe;
}
void game_command_debug_toggle_mesh()
{
    tg_game.debug_no_mesh = !tg_game.debug_no_mesh;
}
void game_command_debug_toggle_colliders()
{
    tg_game.debug_colliders = !tg_game.debug_colliders;
}
void game_command_debug_toggle_nametags()
{
    tg_game.debug_nametags = !tg_game.debug_nametags;
}
void game_command_debug_toggle_normals()
{
    tg_game.debug_normals = !tg_game.debug_normals;
}

void ta_game_update_keybinds()
{
    // TODO: Handle escape as a drag cancel keybind
    // Don't trigger any editor hotkeys while a textbox is focused
    if (editor.textbox_editing || editor.textbox_dragging)
        return;

    dlb_vec_each(ta_keybind *, keybind, tg_game.keybinds) {
        ta_keybind_update(keybind, tg_game.state);
        if (ta_keybind_triggered(keybind)) {
            switch (keybind->command) {
                case COMMAND_PLAY                    : game_command_play();                    break;
                case COMMAND_FREE_CAM                : game_command_free_cam();                break;
                case COMMAND_CONSOLE_TOGGLE          : game_command_console_toggle();          break;
                case COMMAND_CONSOLE_HIDE            : game_command_console_exit();            break;
                case COMMAND_EDITOR                  : game_command_editor();                  break;
                case COMMAND_SHUTDOWN                : game_command_shutdown();                break;
                case COMMAND_TOGGLE_FULLSCREEN       : game_command_toggle_fullscreen();       break;
                case COMMAND_CAMERA_MOVE_FORWARD     : game_command_camera_move_forward();     break;
                case COMMAND_CAMERA_MOVE_BACKWARD    : game_command_camera_move_backward();    break;
                case COMMAND_CAMERA_MOVE_RIGHT       : game_command_camera_move_right();       break;
                case COMMAND_CAMERA_MOVE_LEFT        : game_command_camera_move_left();        break;
                case COMMAND_CAMERA_MOVE_UP          : game_command_camera_move_up();          break;
                case COMMAND_CAMERA_MOVE_DOWN        : game_command_camera_move_down();        break;
                case COMMAND_PLAYER_MOVE_FORWARD     : game_command_player_move_forward();     break;
                case COMMAND_PLAYER_MOVE_BACKWARD    : game_command_player_move_backward();    break;
                case COMMAND_PLAYER_MOVE_RIGHT       : game_command_player_move_right();       break;
                case COMMAND_PLAYER_MOVE_LEFT        : game_command_player_move_left();        break;
                case COMMAND_PLAYER_JUMP             : game_command_player_jump();             break;
                case COMMAND_PLAYER_SHOOT            : game_command_player_shoot();            break;
                case COMMAND_DEBUG_MOUSE_LOCK        : game_command_debug_mouse_lock();        break;
                case COMMAND_DEBUG_MOUSE_UNLOCK      : game_command_debug_mouse_unlock();      break;
                case COMMAND_DEBUG_MOUSE_LOCK_TOGGLE : game_command_debug_mouse_lock_toggle(); break;
                case COMMAND_DEBUG_TOGGLE_WIREFRAME  : game_command_debug_toggle_wireframe();  break;
                case COMMAND_DEBUG_TOGGLE_MESH       : game_command_debug_toggle_mesh();       break;
                case COMMAND_DEBUG_TOGGLE_COLLIDERS  : game_command_debug_toggle_colliders();  break;
                case COMMAND_DEBUG_TOGGLE_NAMETAGS   : game_command_debug_toggle_nametags();   break;
                case COMMAND_DEBUG_TOGGLE_NORMALS    : game_command_debug_toggle_normals();    break;
                case COMMAND_EDITOR_SELECT           : editor_command_select();                break;
                case COMMAND_EDITOR_SELECT_RELEASE   : editor_command_select_release();        break;
                case COMMAND_EDITOR_CANCEL           : editor_command_cancel();                break;
                case COMMAND_EDITOR_CLOSE            : editor_command_close();                 break;
                case COMMAND_EDITOR_SIM_PAUSE_RESUME : editor_command_sim_pause_resume();      break;
                case COMMAND_EDITOR_SIM_NEXT         : editor_command_sim_next();              break;
                case COMMAND_EDITOR_SIM_NEXT_10      : editor_command_sim_next_ten();          break;
                case COMMAND_EDITOR_SIM_WHILE_HELD   : editor_command_sim_while_held();        break;
            }
        }
    }
}
void ta_game_event(ta_event *event)
{
    switch (event->type) {
        case WINDOW_EVENT_RESIZE: {
            // TODO: Handle this in ta_window_event
            ta_window_set_size(tg_window, event->data.window_resize.width, event->data.window_resize.height);
            ta_game_window_resize();
            event->handled = true;
            break;
        } case INPUT_EVENT_MOUSE_MOVE: {
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
                event->handled = true;
            }
            break;
        } case INPUT_EVENT_MOUSE_SCROLL: {
            if (ta_mouse_captured() && tg_game.state == TA_STATE_EDITOR) {
                ta_camera *camera = (ta_camera *)ta_game_camera();
                camera->position_target_vel += 0.02f * -ta_mouse_scroll_dy();
                camera->position_target_vel = MAX(0.02f, camera->position_target_vel);
            }
            break;
        } case INPUT_EVENT_DROP_FILE: {
            printf("dropfile] file: %s\n", event->data.drop_file.path);
            event->handled = true;
            break;
        } case GAME_EVENT_SHUTDOWN: {
            game_command_shutdown();
            event->handled = true;
            break;
        } case GAME_EVENT_CAMERA_ROTATE: {
            static float sensitivity = 0.1f;
            ta_camera *camera = (ta_camera *)ta_game_camera();
            if (event->data.camera_rotate.delta_yaw) {
                ta_camera_yaw(camera, event->data.camera_rotate.delta_yaw * sensitivity);
            }
            if (event->data.camera_rotate.delta_pitch) {
                ta_camera_pitch(camera, event->data.camera_rotate.delta_pitch * sensitivity);
            }
            event->handled = true;
            break;
        }
        case GAME_EVENT_BUTTON_ACTIVATED:
        case GAME_EVENT_BUTTON_DEACTIVATED:
        {
            ta_e_button *button = (ta_e_button *)ta_game_by_sym(RES_COMP_BUTTON, event->data.button.button_name);
            float pressed_weight = 0.0f;
            const char *sfx_name = 0;

            if (event->type == GAME_EVENT_BUTTON_ACTIVATED) {
                ta_player *player = (ta_player *)ta_game_player();
                ta_gun *gun = (ta_gun *)ta_game_component(player->e_gun, RES_COMP_GUN);
                gun->carrying_ammo = gun->carrying_ammo_max;
                pressed_weight = 1.0f;
                sfx_name = button->sfx_activated;
            } else {
                pressed_weight = 0.0f;
                sfx_name = button->sfx_deactivated;
            }

            ta_model *model = (ta_model *)ta_game_component_try(button->entity, RES_COMP_MODEL);
            if (model) {
                // TODO: Some sort of lookup table for important gameplay symbols (SYM_PRESSED?)
                static const char *button_pressed_morph_target = 0;
                if (!button_pressed_morph_target) {
                    button_pressed_morph_target = ta_symbol_intern(CSTR("Pressed"));
                }
                ta_model_set_morph_target_weight(model, button_pressed_morph_target, pressed_weight);
            }

            // TODO: Should audio source subscribe to this event somehow,
            //       or should the button queue the play request itself?
            ta_audio_source *source = (ta_audio_source *)ta_game_by_sym_try(RES_COMP_AUDIO_SOURCE, button->entity);
            if (source) {
                float rand_pitch = 1.0f + dlb_rand_variance(0.1f);
                ta_audio_source_set_pitch(source, rand_pitch);
                if (ta_audio_source_set_buffer(source, sfx_name) == TA_OK) {
                    ta_audio_source_play(source);
                }
            } else {
                ta_log_write(&tg_debug_log, SRC_GAME, "Entity '%s' has no audio source.\n", button->entity);
            }
            event->handled = true;
            break;
        }
    }
}
void ta_game_save()
{
    // TODO: Back up original save file before overwriting, handle errors
    ta_scene_save_file(&tg_game.scene, tg_game.scene.filename);
}