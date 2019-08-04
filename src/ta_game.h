#pragma once
#include "ta_audio.h"
#include "ta_scene.h"
#include "ta_light.h"
#include "ta_camera.h"
#include "ta_node.h"
#include "ta_font.h"
#include "ta_texture.h"

typedef enum ta_game_state {
    TA_GAME_STATE_INIT,
    TA_GAME_STATE_PLAY,
    TA_GAME_STATE_FREE_CAM,
    TA_GAME_STATE_QUIT,
    TA_GAME_STATE_COUNT
} ta_game_state;

// WARNING: Any of these pointers will be invalidated if their pool resizes
typedef struct ta_game {
    ta_game_state state;
    ta_audio_listener *audio;
    ta_audio_source *background_music;
    ta_font *font;
    ta_texture *tex_orange;
    ta_texture *tex_gray;
    ta_scene *scene;
    ta_light *lights;
    ta_camera *camera;
    ta_camera *camera_player;
    ta_camera *camera_freecam;
    ta_node *player;
    int player_ammo_max;
    int player_ammo;
    int player_clip_max;
    int player_clip;
} ta_game;

extern ta_game tg_game;
extern GLenum tg_polygon_mode;

void ta_game_init();
void ta_game_state_set(ta_game_state state);
void ta_game_events();