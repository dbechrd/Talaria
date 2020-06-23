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
const char *tg_tex_orange;
const char *tg_tex_red;
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
const char *tg_e_active_camera;

ta_game tg_game;

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

    ta_log_write(&tg_debug_log, SRC_GAME, "Determining base path...\n");
#if _DEBUG
    char buf[512] = { 0 };
    DWORD len = GetCurrentDirectoryA(sizeof(buf), buf);
    DLB_ASSERT(len < sizeof(buf) - 2);
    for (DWORD i = 0; i < len; ++i) {
        if (buf[i] == '\\') buf[i] = '/';
    }
    buf[len] = '/';
    tg_game.base_path = ta_symbol_intern(buf, len + 1);
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

    ta_log_write(&tg_debug_log, SRC_GAME, "Initializing key binds\n");

    // TODO: Read keybinds from file
    //dlb_vec_reserve(tg_keybinds, 16);

    // TODO: None of these are going to respect the user's OS key repeat settings. Maybe we should check if keybinds
    // are active via switch in event handlers for _everything_. We can still have remappable keys and undoable command
    // indirection and don't have to worry about handling repeat anymore.
    //-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    //                               Command                          Game States                                          Triggers            Keys
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

    tg_mesh_default = ta_game_by_name_try(RES_MESH, SYM(INTERN("prim_unknown")));
    tg_material_default = ta_game_by_name_try(RES_MATERIAL, SYM(INTERN("material_unknown")));

    //--------------------------------------------------------------------------
    // Lighting
    //--------------------------------------------------------------------------
    // TODO: Find closest 8 lights and store them in tg_game.lights
    ta_lighting_init(&tg_game.lighting);

    //--------------------------------------------------------------------------
    // Scene (OGX) ** MUST COME AFTER game.scene.index_by_name[*] INITIALIZED
    //--------------------------------------------------------------------------
    ta_log_write(&tg_debug_log, SRC_GAME, "Loading ogex test file...\n");
    //dml_document_load("data/mesh/skeleton_test.ogex");
    //dml_document_load("data/mesh/button.ogex");
    //dml_document_load("data/mesh/dude.ogex");

    ogx_scene scene = { 0 };
    if (ogx_scene_from_file(&scene, "data/mesh/chamber_0002.ogex") == OGX_SUCCESS) {
        ta_ogx_load(&scene);
    }

    //--------------------------------------------------------------------------
    // Simulation
    //--------------------------------------------------------------------------
    tg_game.simulate = -1;

    //--------------------------------------------------------------------------
    // Player
    //--------------------------------------------------------------------------
    tg_e_player_camera = SYM_ENTITY_PLAYER_CAMERA;
    DLB_ASSERT(tg_e_player_camera);
    tg_e_player_one = SYM_ENTITY_PLAYER_ONE;
    DLB_ASSERT(tg_e_player_one);

    //--------------------------------------------------------------------------
    // Cameras
    //--------------------------------------------------------------------------
    tg_e_freecam = SYM_ENTITY_FREECAM;
    DLB_ASSERT(tg_e_freecam);

    tg_game.minimap_camera.fov = 90.0f;
    tg_game.minimap_camera.up = VEC3_NZ;
    tg_game.minimap_camera.ortho = true;
    ta_camera_init(&tg_game.minimap_camera);

    //--------------------------------------------------------------------------
    // Audio
    //--------------------------------------------------------------------------
    // TODO: Parent this node to the active player
    tg_e_background_music = SYM_ENTITY_BACKGROUND_MUSIC;
    DLB_ASSERT(tg_e_background_music);

    ta_audio_source *bg_music_src = ta_game_component_try(tg_e_background_music, RES_COMP_AUDIO_SOURCE);
    if (bg_music_src) {
        //ta_audio_source_play_loop(bg_music_src);
    }

    ta_audio_listener_set_volume(&tg_audio_listener, 0.2f);
    //ta_audio_listener_mute(&tg_audio_listener);

    //--------------------------------------------------------------------------
    // Textures
    //--------------------------------------------------------------------------
    tg_font            = INTERN("data/font/UbuntuMono-Regular.ttf");
    tg_tex_orange      = INTERN("test_diff");
    tg_tex_red         = INTERN("test_mrao");
    tg_tex_audio_icon  = INTERN("data/texture/audio_icon.tga");

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

    ta_texture *tex_invalid_albedo    = ta_game_by_sym_try(RES_TEXTURE, tg_tex_invalid_albedo   );
    ta_texture *tex_default_albedo    = ta_game_by_sym_try(RES_TEXTURE, tg_tex_default_albedo   );
    ta_texture *tex_default_emission  = ta_game_by_sym_try(RES_TEXTURE, tg_tex_default_emission );
    ta_texture *tex_default_height    = ta_game_by_sym_try(RES_TEXTURE, tg_tex_default_height   );
    ta_texture *tex_default_metallic  = ta_game_by_sym_try(RES_TEXTURE, tg_tex_default_metallic );
    ta_texture *tex_default_normal    = ta_game_by_sym_try(RES_TEXTURE, tg_tex_default_normal   );
    ta_texture *tex_default_occlusion = ta_game_by_sym_try(RES_TEXTURE, tg_tex_default_occlusion);
    ta_texture *tex_default_roughness = ta_game_by_sym_try(RES_TEXTURE, tg_tex_default_roughness);

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

    //--------------------------------------------------------------------------
    // Shaders
    //--------------------------------------------------------------------------
    // TODO: Move these to shaders.dml, they're not scene-specific
    tg_shader_lines   = ta_game_by_sym(RES_SHADER, INTERN("lines"));
    tg_shader_quads   = ta_game_by_sym(RES_SHADER, INTERN("quads"));
    tg_shader_cubemap = ta_game_by_sym(RES_SHADER, INTERN("cubemap"));

#if defined(_WIN32) || defined(__WIN32__) || defined(__WINDOWS__)
    ta_asset_watcher_init(&tg_game.texture_watcher, SYM(tg_game.base_path));
#endif

#if _DEBUG
    ta_game_state_set(TA_STATE_FREE_CAM);
#else
    ta_game_state_set(TA_STATE_PLAY);
#endif
    ta_log_write(&tg_debug_log, SRC_GAME, "Active camera: %s\n", tg_e_active_camera);
    DLB_ASSERT(tg_e_active_camera);
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
            tg_e_active_camera = tg_e_player_camera;
            ta_mouse_capture_set(true);
            break;
        } case TA_STATE_FREE_CAM: {
            ta_camera *freecam = ta_game_component(tg_e_freecam, RES_COMP_CAMERA);
            ta_transform *trans = ta_game_component(tg_e_freecam, RES_COMP_TRANSFORM);
            // HACK: Set freecam position instantly to player cam location. This will *not* work correctly if freecam
            // has a parent. Needs to somehow use xform_world instead.
            if (vec3_zero(trans->xform.position)) {
                ta_camera *active_cam = ta_game_component(tg_e_active_camera, RES_COMP_CAMERA);
                freecam->target_xform.position = active_cam->target_xform.position;
                trans->xform.position = freecam->target_xform.position;
            }
            tg_e_active_camera = tg_e_freecam;
            break;
        } case TA_STATE_EDITOR: {
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
    ta_gltf gltf = { 0 };
    gltf.filename = filename;
    int err = ta_gltf_parse_file(&gltf);
    if (err) {
        ta_log_write(&tg_debug_log, SRC_SYSTEM, "Failed to load gltf model\n");
        DLB_ASSERT(0);
    }
    ta_gltf_load(&gltf);
    ta_gltf_free(&gltf);
}
ta_camera *ta_game_camera()
{
    return ta_game_component(tg_e_active_camera, RES_COMP_CAMERA);
}
ta_ray ta_game_camera_ray()
{
    ta_camera *camera = ta_game_camera();
    ta_transform *cam_trans = ta_game_component(camera->entity, RES_COMP_TRANSFORM);
    ta_ray ray = { 0 };
    ray.origin = cam_trans->xform_world.position;
    ray.direction = camera->front;
    return ray;
}
ta_player *ta_game_player()
{
    return ta_game_component(tg_e_player_one, RES_COMP_PLAYER);
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
    dlb_vec_each(ta_camera *, camera, ta_game_resource_pool(RES_COMP_CAMERA)) {
        if (!camera->ortho) {
            ta_camera_recalc_projection(camera);
        }
    }
}
static void game_hotload_textures()
{
    for (size_t i = 0; i < ARRAY_SIZE(tg_game.texture_watcher.changed_files); ++i) {
        char *filename = tg_game.texture_watcher.changed_files[i];
        if (filename) {
            ta_texture *tex = ta_game_by_name_try(RES_TEXTURE, filename, strlen(filename));
            if (tex) {
                printf("[GAME] hot-loading: %s\n", filename);
                ta_texture_reload(tex);
            } else {
                //printf("[GAME] not found: %s\n", filename);
            }
            dlb_free(filename);
            tg_game.texture_watcher.changed_files[i] = 0;
        }
    }
}
static void game_draw_frame_info(u64 frame_num, double ms_frame_time, double ms_frame_delta, u64 sim_step)
{
    const char *game_state = game_state_str(ta_game_state_current());

    float volume = ta_audio_listener_get_volume(&tg_audio_listener);
    bool muted = ta_audio_listener_muted(&tg_audio_listener);

    ta_camera *camera = ta_game_camera();

    ta_size window_size = { 0 };
    ta_rect restore = { 0 };
    bool vsync;
    ta_window_get_size(tg_window, &window_size.w, &window_size.h);
    ta_window_get_restore_rect(tg_window, &restore);
    ta_window_get_vsync(tg_window, &vsync);

    // Print frame time on the screen
    char frame_info[512] = { 0 };
    size_t len = snprintf(CSTR(frame_info),
        "Frame\n"
        "  count: %08llu\n"
        "  logic: %5.2f ms\n"
        "   swap: %5.2f ms\n"
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
        ms_frame_time,
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
    ta_font *font = ta_game_by_sym(RES_FONT, tg_font);
    ta_font_push_text(font, frame_info, len, true, 0, 0, 0, &frame_time_rects);
    dlb_vec_each(ta_rect_uv *, rect, frame_time_rects) {
        ta_primitive_push_rect_uv(0, *rect, TA_COLOR_WHITE, 0, true, false);
    }
    dlb_vec_zero(frame_time_rects);

    ta_shader *font_shader = ta_game_by_sym(RES_SHADER, font->shader);
    ta_shader_set_mat4(font_shader, SYM_U_PROJ, &MAT4_IDENT);
    ta_shader_set_mat4(font_shader, SYM_U_VIEW, &MAT4_IDENT);
    ta_shader_set_mat4(font_shader, SYM_U_MODEL, &MAT4_IDENT);
    ta_font_render(font, SCREEN_WRAP_X(-220.0f), 12, UI_LAYER_HUD, true, false, &primitive_quads);
}
static void game_draw_hud()
{
    ta_player *player = ta_game_player();
    ta_gun *gun = ta_game_component(player->e_gun, RES_COMP_GUN);

    static ta_ui_window_state window = { 0 };
    ta_ui_window_begin(&window, TA_UI_AUTOSIZE);
    ta_ui_row_begin();
    for (u32 i = 0; i < gun->carrying_ammo_max; i++) {
        ta_ui_next_size(20, 20);
        ta_ui_next_pad(2, 2, 2, 2);
        if (i < gun->carrying_ammo) {
            ta_texture *tex = ta_game_by_sym(RES_TEXTURE, tg_tex_orange);
            ta_ui_image(tex, 0);
        } else {
            ta_texture *tex = ta_game_by_sym(RES_TEXTURE, tg_tex_red);
            ta_ui_image(tex, 0);
        }
    }
    //ta_ui_pad(0, 4);
    ta_ui_row_begin();
    for (u32 i = 0; i < gun->loaded_ammo_max; i++) {
        ta_ui_next_size(20, 20);
        ta_ui_next_pad(2, 2, 2, 2);
        if (i < gun->loaded_ammo) {
            ta_texture *tex = ta_game_by_sym(RES_TEXTURE, tg_tex_orange);
            ta_ui_image(tex, 0);
        } else {
            ta_texture *tex = ta_game_by_sym(RES_TEXTURE, tg_tex_red);
            ta_ui_image(tex, 0);
        }
    }
    ta_ui_window_end();
    ta_ui_render();
}
static void collision_broadphase(ta_rigid_body_pair **pairs, ta_rigid_body *rigid_bodies, double dt)
{
    // Box2D supports 16 collision categories. For each fixture you can
    // specify which category it belongs to. You also specify what other
    // categories this fixture can collide with.
    //
    //   if ((categoryA & maskB) != 0 && (categoryB & maskA) != 0)
    //
    // Collision groups let you specify an integral group index. You can
    // have all fixtures with the same group index always collide
    // (positive index) or never collide (negative index). Group indices
    // are usually used for things that are somehow related, like the
    // parts of a bicycle.
    //
    // Collisions between fixtures of different group indices are
    // filtered according the category and mask bits. In other words,
    // group filtering has higher precedence than category filtering.
    //
    // - A fixture on a static body can only collide with a dynamic
    //   body.
    // - A fixture on a kinematic body can only collide with a dynamic
    //   body.
    // - Fixtures on the same body never collide with each other.
    // - You can optionally enable/disable collision between fixtures on
    //   bodies connected by a joint.
    //
    // Sensor: Fixture which only detects collision, no response. a.ka. trigger
    // -----------------------------------------------------------------
    // Depth-first traversal of AABB tree to find islands. Put islands
    // to sleep when all objects in island are resting. Wake up when
    // anything interacts or applies a force to any body in the island.

    UNUSED(dt);

    dlb_vec_each(ta_rigid_body *, a, rigid_bodies) {
        dlb_vec_range(ta_rigid_body *, b, a + 1, dlb_vec_end(rigid_bodies)) {
            // Don't let entities collide with themselves
            if (a->entity == b->entity)
                continue;

            // HACK: Skip AABB broadphase for planes, makes no sense (always
            //       collect them as potential pairs)
            if (a->collider.type == TA_COLLIDER_PLANE ||
                b->collider.type == TA_COLLIDER_PLANE ||
                ta_aabb_v_aabb(&a->aabb, &b->aabb))
            {
                ta_rigid_body_pair *pair = dlb_vec_alloc(*pairs);
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
static void game_simulate(ta_camera *active_camera, float dt)
{
    ta_log_write(&tg_debug_log, SRC_GAME, " Sim step...\n");

    ta_transform *player_transform = ta_game_component(tg_e_player_one, RES_COMP_TRANSFORM);
    ta_camera *player_cam = ta_game_component(tg_e_player_camera, RES_COMP_CAMERA);
    ta_transform *active_cam_trans = ta_game_component(active_camera->entity, RES_COMP_TRANSFORM);

    // Target player camera
    ta_camera_set_target_pos_absolute(player_cam, vec3_add(player_transform->xform_world.position, (ta_vec3) { 0.0f, 2.0f, 0.0f }));

    // Target minimap camera
    ta_vec3 minimap_target_pos = active_cam_trans->xform_world.position;
    minimap_target_pos.y += 50.0f;
    tg_game.minimap_camera.focal_point = active_cam_trans->xform_world.position;
    ta_camera_set_target_pos_absolute(&tg_game.minimap_camera, minimap_target_pos);

    if (tg_game.simulate) {
        if (tg_game.simulate > 0) {
            tg_game.simulate--;
        }

        ta_rigid_body *rigid_bodies = ta_game_resource_pool(RES_COMP_RIGID_BODY);

        // Simulate rigid bodies
        dlb_vec_each(ta_rigid_body *, body, rigid_bodies) {
            ta_rigid_body_update(body, dt);
        }

        dlb_vec_zero(tg_game.pairs);
        dlb_vec_zero(tg_game.manifolds);

        // Broad phase
        collision_broadphase(&tg_game.pairs, rigid_bodies, dt);
        if (tg_game.pairs) {
            // Narrow phase
            collision_narrowphase(&tg_game.manifolds, tg_game.pairs, dt);
            dlb_vec_each(ta_manifold *, manifold, tg_game.manifolds) {
                // Resolution
                ta_rigid_body_resolve_collision(manifold, dt);

                // Update colliding_with lists
                dlb_vec_push(manifold->a->colliding_with, manifold->b->entity);
                dlb_vec_push(manifold->b->colliding_with, manifold->a->entity);
            }
        }

        tg_game.sim_step++;
    }

    // Update cameras
    dlb_vec_each(ta_camera *, cam, ta_game_resource_pool(RES_COMP_CAMERA)) {
        ta_camera_update(cam, dt);
    }

    // TODO: dlb_vec_each(ta_audio_source_update)

    //ta_mat3 rotate_sun = mat3_rotate_z(1.0f);
    //tg_game.sun->data.sun.direction =
    //    mat3_mul_vec3(&rotate_sun, tg_game.sun->data.sun.direction);

    //// HACK: Make point light rotate in a circle
    //static float light_deg = 0.0f;
    //light_deg += 0.005f;
    //if (light_deg >= 360.0f) light_deg = 0.0f;

    //ta_light *lights = ta_game_resource_pool(RES_COMP_LIGHT);
    //lights[1].position.x = cosf(light_deg) * 16.0f;
    //lights[1].position.z = sinf(light_deg) * 16.0f;

    // Update buttons
    dlb_vec_each(ta_e_button *, button, ta_game_resource_pool(RES_COMP_BUTTON)) {
        e_button_update(button);
    }
}
static void game_render_skybox()
{
    ta_texture *skybox = ta_game_by_name_try(RES_TEXTURE, SYM(INTERN("miramar_skybox")));
    if (skybox) {
        ta_camera *camera = ta_game_camera();
        ta_shader *shader = ta_game_by_name(RES_SHADER, SYM(INTERN("skybox")));
        ta_mesh *mesh = ta_game_by_name(RES_MESH, SYM(INTERN("prim_skybox")));
        ta_shader_set_sampler_cube(shader, SYM_U_TEX, skybox->gl_id);
        ta_shader_set_mat4(shader, SYM_U_PROJ, &camera->projection);
        ta_shader_set_mat4(shader, SYM_U_VIEW, &camera->frustum);

        glDisable(GL_CULL_FACE);
        glDepthMask(GL_FALSE);
        ta_shader_bind(shader);
        ta_mesh_render(mesh);
        ta_shader_unbind();
        glDepthMask(GL_TRUE);
        glEnable(GL_CULL_FACE);
    }
}
static void game_render_manifolds_debug()
{
    const float radius = 0.05f;
    dlb_vec_each(ta_manifold *, manifold, tg_game.manifolds) {
        for (u32 i = 0; i < manifold->contact_count; ++i) {
            ta_sphere dbg_contact_world;
            dbg_contact_world.center = manifold->contacts[i];
            dbg_contact_world.radius = radius;
            ta_primitive_push_sphere(0, dbg_contact_world, TA_COLOR_DARK_RED);
            ta_line_3d dbg_contact_normal;
            dbg_contact_normal.p0 = vec3_add(manifold->contacts[i], vec3_scalef(manifold->normal, radius));
            dbg_contact_normal.p1 = vec3_add(manifold->contacts[i], vec3_scalef(manifold->normal, 1.0f - radius));
            ta_primitive_push_line_3d(0, dbg_contact_normal, TA_COLOR_DARK_RED, TA_COLOR_DARK_RED);
        }
    }
}
static void game_render_colliders_debug()
{
    ta_rigid_body *rigid_bodies = ta_game_resource_pool(RES_COMP_RIGID_BODY);

    // Local space
    dlb_vec_each(ta_rigid_body *, body, rigid_bodies) {
        ta_transform *transform = ta_game_component(body->entity, RES_COMP_TRANSFORM);
        ta_shader_set_mat4(tg_shader_lines, SYM_U_MODEL, &transform->world);
        ta_shader_set_mat4(tg_shader_quads, SYM_U_MODEL, &transform->world);

        ta_sphere centroid_local = { 0 };
        centroid_local.center = body->centroid_local;
        centroid_local.radius = 0.06f;
        ta_primitive_push_sphere(0, centroid_local, TA_COLOR_GREEN);

        ta_rgba narrowphase_color = body->dbg_narrowphase ? TA_COLOR_MAGENTA : TA_COLOR_ORANGE;
        ta_collider_push(&body->collider, narrowphase_color);
        ta_primitive_render(true, false);
    }

    // World space
    ta_shader_set_mat4(tg_shader_lines, SYM_U_MODEL, &MAT4_IDENT);
    ta_shader_set_mat4(tg_shader_quads, SYM_U_MODEL, &MAT4_IDENT);
    dlb_vec_each(ta_rigid_body *, body, rigid_bodies) {
        ta_transform *transform = ta_game_component(body->entity, RES_COMP_TRANSFORM);

        ta_sphere local_origin = { 0 };
        local_origin.center = transform->xform_world.position;
        local_origin.radius = 0.04f;
        ta_primitive_push_sphere(0, local_origin, TA_COLOR_PINK);

        ta_sphere centroid_global = { 0 };
        centroid_global.center = body->centroid_global;
        centroid_global.radius = 0.08f;
        ta_primitive_push_sphere(0, centroid_global, TA_COLOR_BLUE);

        ta_rgba broadphase_color = body->dbg_broadphase ? TA_COLOR_RED : TA_COLOR_GRAY4;
        ta_primitive_push_aabb(0, body->aabb, broadphase_color);
    }
    ta_primitive_render(true, false);
}
static void game_render_nametags_debug(ta_camera *camera)
{
    glClear(GL_DEPTH_BUFFER_BIT);

    ta_font *font = ta_game_by_sym(RES_FONT, tg_font);
    static ta_rect_uv *tag_rects = 0;

    ta_mat4 projection = camera->projection;
    //ta_mat4 projection = mat4_ortho(-10.0f, 10.0f, -10.0f, 10.0f, -10.0f, 20.0f);

    ta_transform *cam_trans = ta_game_component(camera->entity, RES_COMP_TRANSFORM);

    ta_transform *transforms = ta_game_resource_pool(RES_COMP_TRANSFORM);
    dlb_vec_each(ta_transform *, transform, transforms) {
        ta_rect tag_rect = ta_font_push_text(font, SYM(transform->name), true, 0, 0, 0, &tag_rects);

        ta_vec3 tag_pos = transform->xform_world.position;
        ta_vec3 tag_to_cam = vec3_sub(cam_trans->xform_world.position, tag_pos);
        tag_to_cam.z *= -1.0f;
        tag_to_cam.y *= 0.0f;
        float tag_scalef = MAX(vec3_len(tag_to_cam), 4.0f);

        ta_vec3 tag_offset = tag_offset = vec3_scalef(camera->right, NDC_W(tag_rect.w) / 2.0f * tag_scalef);
        ta_vec3 tag_pos_off = vec3_sub(tag_pos, tag_offset);

        ta_mat4 tag_rot = mat4_lookat(VEC3_ZERO, tag_to_cam, VEC3_Y);

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
        //ta_texture *tex_orange = ta_game_by_sym(RES_TEXTURE, tg_tex_orange);
        //ta_shader_set_sampler2d(tg_shader_quads, SYM_U_TEX, tex_orange->gl_id);
        ta_rect_uv tag_background = { 0 };
        tag_background.rect.x -= (int)NDC_W(5.0f);
        tag_background.rect.w = (int)(NDC_W(tag_rect.w) + NDC_W(10.0f));
        tag_background.rect.h = (int)NDC_H(tag_rect.h); //tg_game.font->pixel_height * 1.5f;
        ta_primitive_push_rect_uv(0, tag_background, TA_COLOR_DARK_RED, UI_LAYER_HUD_BG, false, false);
        ta_primitive_render_mesh(&primitive_quads, tg_shader_quads, TA_TRIANGLES, true, false);
        ta_shader_set_sampler_2d(tg_shader_quads, SYM_U_TEX, 0);

        // Name tag text
        ta_shader *font_shader = ta_game_by_sym(RES_SHADER, font->shader);
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
        ta_font_render(font, 0, 0, 0, true, false, &primitive_quads);
    }
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
    const float ms_sim_dt = 20;             // fixed dt milliseconds
    const float sim_dt = ms_sim_dt / 1000;  // fixed dt seconds
    const int sim_max_steps = 0;            // max simulation steps per frame
    double ms_sim_t = 0;                    // current simulation time
    double ms_frame_accum = 0;

    double ms_frame_prev = 0;   // Last frame started
    double ms_frame_start;      // This frame started
    double ms_frame_delta;      // Total delta time (including v-sync)
    double ms_frame_time;       // Actual frame time before v-sync

    while (ta_game_state_current() != TA_STATE_SHUTDOWN) {
        ms_frame_start = ta_timer_elapsed_ms();
        ms_frame_delta = ms_frame_start - ms_frame_prev;
        ms_frame_prev = ms_frame_start;

        game_hotload_textures();

        ta_camera *active_camera = ta_game_camera();
        ta_transform *transforms = ta_game_resource_pool(RES_COMP_TRANSFORM);
        ta_light *lights = ta_game_resource_pool(RES_COMP_LIGHT);
        ta_camera *cameras = ta_game_resource_pool(RES_COMP_CAMERA);

        //----------------------------------------------------------------------
        // Handle events
        //----------------------------------------------------------------------
        ta_log_write(&tg_debug_log, SRC_GAME, " Handling events...\n");
        ta_event_events();

        //----------------------------------------------------------------------
        // Simulation
        //----------------------------------------------------------------------
        ta_log_write(&tg_debug_log, SRC_GAME, " Accumulating...\n");
        if (sim_max_steps == 0) {
            // TODO: This is a bad idea if vsync != 60hz
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

        if (ms_frame_accum >= ms_sim_dt) {
            while (ms_frame_accum >= ms_sim_dt) {
                game_simulate(active_camera, (float)sim_dt);
                ms_sim_t += ms_sim_dt;
                ms_frame_accum -= ms_sim_dt;
            }
        }

        //----------------------------------------------------------------------
        // Post-simulation updates (e.g. recalculate cached transform matrices)
        //----------------------------------------------------------------------
        float sim_alpha = (float)(ms_frame_accum / ms_sim_dt);

        // Update transforms (model matrix and lerp)
        ta_transform_update_all(transforms, sim_alpha);

        // Update light data UBO
        ta_lighting_bind_lights(&tg_game.lighting);

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
            ta_light_shadowpass_render(light, transforms);
        }
        ta_shader_unbind();
        //glCullFace(GL_BACK);
        glViewport(0, 0, WINDOW_W, WINDOW_H);

        //----------------------------------------------------------------------
        // Render pass
        //----------------------------------------------------------------------
        ta_log_write(&tg_debug_log, SRC_GAME, " Render pass...\n");

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glStencilMask(0xFF);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        glStencilMask(0x00);

        ta_shader_set_mat4(tg_shader_lines, SYM_U_PROJ, &active_camera->projection);
        ta_shader_set_mat4(tg_shader_lines, SYM_U_VIEW, &active_camera->look_at);
        ta_shader_set_mat4(tg_shader_quads, SYM_U_PROJ, &active_camera->projection);
        ta_shader_set_mat4(tg_shader_quads, SYM_U_VIEW, &active_camera->look_at);

        // Dump any prims from the collision pass
        game_render_manifolds_debug();
        ta_primitive_render_mesh(&primitive_lines_perma, tg_shader_lines, TA_LINES, false, false);
        ta_primitive_render(true, false);

        if (active_camera->debug_wireframe) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        }

        // Render models
        // TODO: Group by shader / material to minimize redundant uniform calls
        ta_shader *mesh_shader = ta_game_by_name(RES_SHADER, CSTR("mesh"));
        ta_shader_set_sampler_2d_array(mesh_shader, SYM_U_TEXTURES[0], tg_game.texturing.texture_pools[0].gl_id);
        ta_shader_set_sampler_2d_array(mesh_shader, SYM_U_TEXTURES[1], tg_game.texturing.texture_pools[1].gl_id);
        ta_shader_set_sampler_2d_array(mesh_shader, SYM_U_TEXTURES[2], tg_game.texturing.texture_pools[2].gl_id);
        ta_shader_set_sampler_2d_array(mesh_shader, SYM_U_TEXTURES[3], tg_game.texturing.texture_pools[3].gl_id);
        ta_shader_set_sampler_2d_array(mesh_shader, SYM_U_TEXTURES[4], tg_game.texturing.texture_pools[4].gl_id);
        ta_shader_set_sampler_2d_array(mesh_shader, SYM_U_TEXTURES[5], tg_game.texturing.texture_pools[5].gl_id);
        ta_shader_set_sampler_2d_array(mesh_shader, SYM_U_TEXTURES[6], tg_game.texturing.texture_pools[6].gl_id);
        ta_shader_set_sampler_2d_array(mesh_shader, SYM_U_TEXTURES[7], tg_game.texturing.texture_pools[7].gl_id);
        dlb_vec_each(ta_transform *, transform, transforms) {
            ta_model *model = ta_game_component_try(transform->entity, RES_COMP_MODEL);
            if (model) {
                ta_model_render(model, active_camera);
            }
        }

        if (active_camera->debug_wireframe) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }

        game_render_skybox();

        ta_primitive_render(true, false);

        if (tg_game.state == TA_STATE_EDITOR) {
            // Debug render cameras as RGB spheres
            dlb_vec_each(ta_camera *, camera, cameras) {
                if (camera->name != tg_e_active_camera) {
                    ta_transform *cam_trans = ta_game_component(camera->entity, RES_COMP_TRANSFORM);
                    ta_obb obb = { 0 };
                    obb.center = cam_trans->xform_world.position;
                    obb.extents = (ta_vec3){ 0.2f, 0.2f, 0.2f };
                    obb.orientation = cam_trans->xform_world.orientation;
                    ta_primitive_push_obb(0, obb, TA_COLOR_WHITE);
                    ta_vec3 dir = vec3_rotate_quat(VEC3_NZ, obb.orientation);
                    ta_primitive_push_arrow(0, obb.center, dir, TA_COLOR_WHITE);

                    // TODO: Camera icon on billboarded quad in world space
                    //ta_primitive_push_billboard();
                }
            }
            // Debug render light as spheres of the light's color
            dlb_vec_each(ta_light *, light, lights) {
                ta_transform *transform = ta_game_component(light->entity, RES_COMP_TRANSFORM);

                ta_sphere light_pos = { 0 };
                light_pos.center = transform->xform_world.position;
                light_pos.radius = 0.2f;
                ta_rgba color = { 0 };
                if (light->disabled) {
                    color.r = 0.5f;
                    color.g = 0.5f;
                    color.b = 0.5f;
                } else {
                    color.r = light->color.r;
                    color.g = light->color.g;
                    color.b = light->color.b;
                }
                ta_primitive_push_sphere(0, light_pos, color);

                if (active_camera->debug_colliders) {
                    ta_sphere light_aoe = { 0 };
                    light_aoe.center = transform->xform_world.position;
                    light_aoe.radius = light->shadowmap.zfar;
                    ta_primitive_push_sphere(0, light_aoe, color);
                }
            }
            ta_primitive_render(true, false);
        }

        if (active_camera->debug_colliders) {
            ta_log_write(&tg_debug_log, SRC_GAME, " Debug colliders pass...\n");
            game_render_colliders_debug();
        }

        //-----------------
#if 0
        static ta_ray ray = { 0 };
        if (vec3_zero(ray.origin)) {
            //ray.origin.x = 0.59f;
            //ray.origin.y = 0.36f;
            //ray.origin.z = 0.46f;
            ray.origin.y = 2.0f;
            ray.origin.z = 1.2f;
            ray.direction.x = 0.2f;
            ray.direction.y = 0.2f;
            ray.direction.z = -1.0f;
            ray.direction = vec3_normalize(ray.direction);
        }
        static ta_quad quad = { 0 };
        if (vec4_zero(quad.orientation)) {
            quad.center.y = 2.0f;
            quad.extents = (ta_vec2){ 1.5f, 0.5f };
            quad.orientation = QUAT_IDENT;
        }
        ta_primitive_push_arrow(0, ray.origin, ray.direction, TA_COLOR_RED);
        float ray_t = 0.0f;
        if (ta_ray_v_quad(&ray, &quad, &ray_t)) {
            ta_primitive_push_quad(0, quad, TA_COLOR_DARK_RED);
            ta_sphere contact = { 0 };
            contact.center = vec3_add(ray.origin, vec3_scalef(ray.direction, ray_t));
            contact.radius = 0.05f;
            ta_primitive_push_sphere(0, contact, TA_COLOR_YELLOW);
        } else {
            ta_primitive_push_quad(0, quad, TA_COLOR_DARK_REDA);
        }
        ta_primitive_render(true, false);
#endif
        //-----------------

        //----------------------------------------------------------------------
        // Editor UI (world)
        //----------------------------------------------------------------------
        if (tg_game.state == TA_STATE_EDITOR) {
            ta_log_write(&tg_debug_log, SRC_GAME, " Editor world UI pass...\n");
            ta_editor_update_widgets();
            ta_editor_draw_world();
        }

        if (active_camera->debug_nametags) {
            ta_log_write(&tg_debug_log, SRC_GAME, " Debug nametags pass...\n");
            game_render_nametags_debug(active_camera);
        }

        //----------------------------------------------------------------------
        // Crosshair
        //----------------------------------------------------------------------
        glClear(GL_DEPTH_BUFFER_BIT);
        ta_primitive_push_crosshair(0, 10, 2);
        ta_primitive_render(true, true);

        //----------------------------------------------------------------------
        // Game HUD
        //----------------------------------------------------------------------
        // TODO: Make HUD drawing suck less.. way too many draw calls
        //       Use texture atlas; batch everything into one draw call; stop
        //       using stupid RGB placeholders.
        ta_log_write(&tg_debug_log, SRC_GAME, " HUD pass...\n");
        //game_draw_hud();

#if 0
        //----------------------------------------------------------------------
        // Minimap
        //----------------------------------------------------------------------
        ta_rect map_rect = { 10, 50, 200, 200 };
        ta_viewport_bind(map_rect, TA_COLOR_GRAY7, true);
        ta_scene_render(&tg_game.scene, &minimap_camera, sim_alpha);
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
        // TODO(cleanup): This could be useful to visualize FPS, but it's pretty
        // useless in its current state.
        //----------------------------------------------------------------------
        // Barchart
        //----------------------------------------------------------------------
        ta_shader_set_mat4(tg_shader_lines, SYM_U_PROJ, &MAT4_IDENT);
        ta_shader_set_mat4(tg_shader_lines, SYM_U_VIEW, &MAT4_IDENT);
        ta_shader_set_mat4(tg_shader_lines, SYM_U_MODEL, &MAT4_IDENT);
        ta_ui_barchart_draw(&chart, 0, 0);
        ta_primitive_render(true, true);
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
        //----------------------------------------------------------------------
        // Audio
        //----------------------------------------------------------------------
        // TODO: dlb_vec_each(ta_audio_listener_update)
        ta_transform *active_cam_trans = ta_game_component(active_camera->entity, RES_COMP_TRANSFORM);
        ta_vec3 fwd_up[2];
        fwd_up[0] = active_camera->front;
        fwd_up[1] = active_camera->up;
        alListenerfv(AL_ORIENTATION, (float *)fwd_up);
        alListenerfv(AL_POSITION, (float *)&active_cam_trans->xform_world.position);
        //alListenerfv(AL_VELOCITY, (float *)&tg_game.camera->velocity);

        ta_log_write(&tg_debug_log, SRC_GAME, " Audio update...\n");
        ta_audio_update();

        //----------------------------------------------------------------------
        // BOOM! It's swap time, baby! Show the player all of our hard work.
        //----------------------------------------------------------------------
        // NOTE: This confirms rendering is being deferred until swap buffers,
        // but it's much slower (~5ms), so don't actually use it.
        //ta_log_write(&tg_debug_log, SRC_GAME, " glFinish...\n");
        //glFinish();

        ta_log_write(&tg_debug_log, SRC_GAME, " Update cursor...\n");
        ta_window_update_cursor(tg_window);

        // TODO: Add "show_fps" flag and bind to key; off by default in release
        ms_frame_time = ta_timer_elapsed_ms() - ms_frame_start;
        tg_game.frame_num++;
        ta_log_write(&tg_debug_log, SRC_GAME, " FPS pass...\n");
        game_draw_frame_info(tg_game.frame_num, ms_frame_time, ms_frame_delta, tg_game.sim_step);

        ta_log_write(&tg_debug_log, SRC_GAME, " Swap...\n");
        ta_window_swap(tg_window);

        ta_log_write(&tg_debug_log, SRC_GAME,
            "Frame %llu displayed. time: %5.3f delta: %5.3f\n",
            tg_game.frame_num, ms_frame_time, ms_frame_delta);

        // Sob profusely when frame time goes over 16ms
        if (ms_frame_time > 16) {
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
    bool fullscreen;
    ta_window_get_fullscreen(tg_window, &fullscreen);
    ta_window_set_fullscreen(tg_window, !fullscreen);
}
void game_command_player_move_forward()
{
    ta_camera *camera = ta_game_camera();
    ta_vec3 dir = { 0 };
    dir.x = camera->front.x;
    dir.z = camera->front.z;
    dir = vec3_normalize(dir);
    dir = vec3_scalef(dir, 0.1f);
    ta_rigid_body *player_body = ta_game_component(tg_e_player_one,
        RES_COMP_RIGID_BODY);
    ta_rigid_body_apply_impulse(player_body, dir, VEC3_ZERO);
}
void game_command_player_move_backward()
{
    ta_camera *camera = ta_game_camera();
    ta_vec3 dir = { 0 };
    dir.x = -camera->front.x;
    dir.z = -camera->front.z;
    dir = vec3_normalize(dir);
    dir = vec3_scalef(dir, 0.1f);
    ta_rigid_body *player_body = ta_game_component(tg_e_player_one,
        RES_COMP_RIGID_BODY);
    ta_rigid_body_apply_impulse(player_body, dir, VEC3_ZERO);
}
void game_command_player_move_right()
{
    ta_camera *camera = ta_game_camera();
    ta_vec3 dir = { 0 };
    dir.x = camera->right.x;
    dir.z = camera->right.z;
    dir = vec3_normalize(dir);
    dir = vec3_scalef(dir, 0.1f);
    ta_rigid_body *player_body = ta_game_component(tg_e_player_one,
        RES_COMP_RIGID_BODY);
    ta_rigid_body_apply_impulse(player_body, dir, VEC3_ZERO);
}
void game_command_player_move_left()
{
    ta_camera *camera = ta_game_camera();
    ta_vec3 dir = { 0 };
    dir.x = -camera->right.x;
    dir.z = -camera->right.z;
    dir = vec3_normalize(dir);
    dir = vec3_scalef(dir, 0.1f);
    ta_rigid_body *player_body = ta_game_component(tg_e_player_one,
        RES_COMP_RIGID_BODY);
    ta_rigid_body_apply_impulse(player_body, dir, VEC3_ZERO);
}
void game_command_player_jump()
{
    ta_vec3 dir = VEC3_Y;
    dir = vec3_scalef(dir, 5.0f);
    ta_rigid_body *player_body = ta_game_component(tg_e_player_one,
        RES_COMP_RIGID_BODY);
    ta_rigid_body_apply_impulse(player_body, dir, VEC3_ZERO);
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
    //ta_transform *transform = ta_game_entity_add(name, position);
    //ta_model *model = ta_game_component_add(name, RES_COMP_MODEL, CSTR("who cares"));
    //ta_rigid_body *body = ta_game_component_add(name, RES_COMP_RIGID_BODY, CSTR("who cares"));
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

    ta_player *player = ta_game_player();
    ta_gun *gun = ta_game_component(player->e_gun, RES_COMP_GUN);
    ta_audio_source *src_gun = ta_game_component(player->e_gun,
        RES_COMP_AUDIO_SOURCE);

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
void game_command_camera_move_forward()
{
    if (ta_mouse_captured()) {
        ta_camera *camera = ta_game_camera();
        camera->move_buffer = vec3_add(camera->move_buffer, camera->front);
    }
}
void game_command_camera_move_backward()
{
    if (ta_mouse_captured()) {
        ta_camera *camera = ta_game_camera();
        camera->move_buffer = vec3_sub(camera->move_buffer, camera->front);
    }
}
void game_command_camera_move_right()
{
    if (ta_mouse_captured()) {
        ta_camera *camera = ta_game_camera();
        camera->move_buffer = vec3_add(camera->move_buffer, camera->right);
    }
}
void game_command_camera_move_left()
{
    if (ta_mouse_captured()) {
        ta_camera *camera = ta_game_camera();
        camera->move_buffer = vec3_sub(camera->move_buffer, camera->right);
    }
}
void game_command_camera_move_up()
{
    if (ta_mouse_captured()) {
        ta_camera *camera = ta_game_camera();
        camera->move_buffer = vec3_add(camera->move_buffer, camera->up);
    }
}
void game_command_camera_move_down()
{
    if (ta_mouse_captured()) {
        ta_camera *camera = ta_game_camera();
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

    // HACK: Too lazy to make a proper keybind for this
    for (int i = 0; i < TA_VERTEX_ATTRIB_COUNT; ++i) {
        dlb_vec_clear(primitive_lines_perma.buffers[i]);
    }
}
void game_command_debug_toggle_wireframe()
{
    ta_camera *camera = ta_game_camera();
    camera->debug_wireframe = !camera->debug_wireframe;
}
void game_command_debug_toggle_mesh()
{
    ta_camera *camera = ta_game_camera();
    camera->debug_no_mesh = !camera->debug_no_mesh;
}
void game_command_debug_toggle_colliders()
{
    ta_camera *camera = ta_game_camera();
    camera->debug_colliders = !camera->debug_colliders;
}
void game_command_debug_toggle_nametags()
{
    ta_camera *camera = ta_game_camera();
    camera->debug_nametags = !camera->debug_nametags;
}
void game_command_debug_toggle_normals()
{
    ta_camera *camera = ta_game_camera();
    camera->debug_normals = !camera->debug_normals;
}

void ta_game_update_keybinds()
{
    // TODO: Handle escape as a drag cancel keybind
    // Don't trigger any editor hotkeys while a textbox is focused
    if (editor.textbox_editing || editor.textbox_dragging)
        return;

    static void (*commands[COMMAND_COUNT])() = {
        [COMMAND_PLAY]                    = game_command_play,
        [COMMAND_FREE_CAM]                = game_command_free_cam,
        [COMMAND_CONSOLE_TOGGLE]          = game_command_console_toggle,
        [COMMAND_CONSOLE_HIDE]            = game_command_console_exit,
        [COMMAND_EDITOR]                  = game_command_editor,
        [COMMAND_SHUTDOWN]                = game_command_shutdown,
        [COMMAND_TOGGLE_FULLSCREEN]       = game_command_toggle_fullscreen,
        [COMMAND_CAMERA_MOVE_FORWARD]     = game_command_camera_move_forward,
        [COMMAND_CAMERA_MOVE_BACKWARD]    = game_command_camera_move_backward,
        [COMMAND_CAMERA_MOVE_RIGHT]       = game_command_camera_move_right,
        [COMMAND_CAMERA_MOVE_LEFT]        = game_command_camera_move_left,
        [COMMAND_CAMERA_MOVE_UP]          = game_command_camera_move_up,
        [COMMAND_CAMERA_MOVE_DOWN]        = game_command_camera_move_down,
        [COMMAND_PLAYER_MOVE_FORWARD]     = game_command_player_move_forward,
        [COMMAND_PLAYER_MOVE_BACKWARD]    = game_command_player_move_backward,
        [COMMAND_PLAYER_MOVE_RIGHT]       = game_command_player_move_right,
        [COMMAND_PLAYER_MOVE_LEFT]        = game_command_player_move_left,
        [COMMAND_PLAYER_JUMP]             = game_command_player_jump,
        [COMMAND_PLAYER_SHOOT]            = game_command_player_shoot,
        [COMMAND_DEBUG_MOUSE_LOCK]        = game_command_debug_mouse_lock,
        [COMMAND_DEBUG_MOUSE_UNLOCK]      = game_command_debug_mouse_unlock,
        [COMMAND_DEBUG_MOUSE_LOCK_TOGGLE] = game_command_debug_mouse_lock_toggle,
        [COMMAND_DEBUG_TOGGLE_WIREFRAME]  = game_command_debug_toggle_wireframe,
        [COMMAND_DEBUG_TOGGLE_MESH]       = game_command_debug_toggle_mesh,
        [COMMAND_DEBUG_TOGGLE_COLLIDERS]  = game_command_debug_toggle_colliders,
        [COMMAND_DEBUG_TOGGLE_NAMETAGS]   = game_command_debug_toggle_nametags,
        [COMMAND_DEBUG_TOGGLE_NORMALS]    = game_command_debug_toggle_normals,

        [COMMAND_EDITOR_SELECT]           = editor_command_select,
        [COMMAND_EDITOR_SELECT_RELEASE]   = editor_command_select_release,
        [COMMAND_EDITOR_CANCEL]           = editor_command_cancel,
        [COMMAND_EDITOR_CLOSE]            = editor_command_close,
        [COMMAND_EDITOR_SIM_PAUSE_RESUME] = editor_command_sim_pause_resume,
        [COMMAND_EDITOR_SIM_NEXT]         = editor_command_sim_next,
        [COMMAND_EDITOR_SIM_NEXT_10]      = editor_command_sim_next_ten,
        [COMMAND_EDITOR_SIM_WHILE_HELD]   = editor_command_sim_while_held,
    };

    dlb_vec_each(ta_keybind *, keybind, tg_game.keybinds) {
        ta_keybind_update(keybind, tg_game.state);
        if (ta_keybind_triggered(keybind) && commands[keybind->command]) {
            commands[keybind->command]();
        }
    }
}
void ta_game_event(ta_event *event)
{
    bool handled = true;

    switch (event->type) {
        case WINDOW_EVENT_RESIZE: {
            // TODO: Handle this in ta_window_event
            ta_window_set_size(tg_window, event->data.window_resize.width, event->data.window_resize.height);
            ta_game_window_resize();
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
            }
            break;
        } case GAME_EVENT_SHUTDOWN: {
            game_command_shutdown();
            break;
        } case GAME_EVENT_CAMERA_ROTATE: {
            ta_camera *camera = ta_game_camera();
            static float sensitity = 0.1f;
            if (event->data.camera_rotate.delta_yaw) {
                ta_camera_yaw(camera, event->data.camera_rotate.delta_yaw * sensitity);
            }
            if (event->data.camera_rotate.delta_pitch) {
                ta_camera_pitch(camera, event->data.camera_rotate.delta_pitch * sensitity);
            }
            break;
        }
        case GAME_EVENT_BUTTON_ACTIVATED:
        case GAME_EVENT_BUTTON_DEACTIVATED:
        {
            ta_e_button *button = ta_game_by_sym(RES_COMP_BUTTON, event->data.button.button_name);
            float pressed_weight = 0.0f;
            const char *sfx_name = 0;

            if (event->type == GAME_EVENT_BUTTON_ACTIVATED) {
                ta_player *player = ta_game_player();
                ta_gun *gun = ta_game_component(player->e_gun, RES_COMP_GUN);
                //if (gun->carrying_ammo == 0 && gun->loaded_ammo == 0) {
                gun->carrying_ammo = gun->carrying_ammo_max;
                pressed_weight = 1.0f;
                sfx_name = button->sfx_activated;
            } else {
                pressed_weight = 0.0f;
                sfx_name = button->sfx_deactivated;
            }

            ta_model *model = ta_game_component_try(button->entity, RES_COMP_MODEL);
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
            ta_audio_source *source = ta_game_by_sym_try(RES_COMP_AUDIO_SOURCE, button->entity);
            if (source) {
                float randf = (float)dlb_rand_u32() / (float)UINT32_MAX;
                float vary_by = 0.1f;
                float rand_pitch = 1.0f + (randf * 2 * vary_by - vary_by);  // vary pitch by +/- vary_by
                ta_audio_source_set_pitch(source, rand_pitch);
                if (ta_audio_source_set_buffer(source, sfx_name) == TA_OK) {
                    ta_audio_source_play(source);
                }
            } else {
                ta_log_write(&tg_debug_log, SRC_GAME, "Entity '%s' has no audio source.\n", button->entity);
            }
            break;
        } default: {
            handled = false;
        }
    }

    event->handled = handled;
}
void ta_game_save()
{
    // TODO: Back up original save file before overwriting, handle errors
    ta_scene_save_file(&tg_game.scene, tg_game.scene.filename);
}