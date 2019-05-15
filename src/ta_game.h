#pragma once
#include "ta_scene.h"
#include "ta_entity.h"
#include "ta_camera.h"

typedef enum {
    TA_STATE_INIT,
    TA_STATE_PLAY,
    TA_STATE_FREE_CAM,
    TA_STATE_QUIT,
    TA_STATE_COUNT
} ta_game_state;

typedef struct {
    ta_game_state state;
    ta_scene *scene;
    ta_camera *camera;
    ta_entity *player;
} ta_game;

extern ta_game tg_game;