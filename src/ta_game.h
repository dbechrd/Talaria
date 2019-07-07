#pragma once
#include "ta_audio.h"
#include "ta_scene.h"
#include "ta_light.h"
#include "ta_camera.h"
#include "ta_node.h"

typedef enum {
    TA_STATE_INIT,
    TA_STATE_PLAY,
    TA_STATE_FREE_CAM,
    TA_STATE_QUIT,
    TA_STATE_COUNT
} ta_game_state;

typedef struct {
    ta_game_state state;
    ta_audio_listener *audio;
    ta_scene *scene;
    ta_light *lights;
    ta_camera *camera;
    ta_camera *camera_player;
    ta_camera *camera_freecam;
    ta_node *player;
} ta_game;

extern ta_game tg_game;
extern GLenum tg_polygon_mode;

void ta_game_init();
void ta_game_state_set(ta_game_state state);
void ta_game_update();