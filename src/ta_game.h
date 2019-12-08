#pragma once
#include "dlb/dlb_types.h"

struct ta_event;

typedef enum ta_game_state {
    TA_GAME_STATE_STARTUP,
    TA_GAME_STATE_PLAY,
    TA_GAME_STATE_FREE_CAM,
    TA_GAME_STATE_EDITOR,
    TA_GAME_STATE_SHUTDOWN,
    TA_GAME_STATE_COUNT
} ta_game_state;
const char *game_state_str(ta_game_state state);

typedef struct ta_game {
    ta_game_state state;
    ta_game_state state_prev;
    int simulate;  // -1 = on, 0 = off, 1+ = simulate N frames
    bool vsync;
    u64 frame_num;
    u64 sim_step;

    // WARNING: These pointers will be invalidated if their pool resizes
    // TODO: Replace with uid strings
    struct ta_keybind *keybinds[TA_GAME_STATE_COUNT];
    struct ta_scene *scene;

    const char *font;
    const char *tex_orange;
    const char *tex_red;
    const char *tex_audio_icon;

    const char *e_background_music;
    const char *e_freecam;
    const char *e_player_one;
    //struct ta_entity *player;

    const char *e_active_camera;
} ta_game;

extern ta_game tg_game;

void ta_game_init(ta_game *game);
void ta_game_state_set(ta_game *game, ta_game_state state);
void ta_game_hotkeys(ta_game *game);
void ta_game_event(ta_game *game, struct ta_event *event);
void ta_game_hud_draw(ta_game *game);