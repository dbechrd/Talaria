#pragma once

typedef enum {
    TA_STATE_INIT,
    TA_STATE_PLAY,
    TA_STATE_COUNT
} ta_game_state;

typedef struct {
    ta_game_state state;
} ta_game;

extern ta_game tg_game;