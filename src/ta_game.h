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
    TA_GAME_STATE_TEXT_ENTRY,
    TA_GAME_STATE_QUIT,
    TA_GAME_STATE_COUNT
} ta_game_state;

typedef struct text_entry_settings {
    char *buffer;   // "Some text| is here"
    char *lbuffer;  // "Some text"
    char *rbuffer;  // "ereh si "
    bool dirty;
    u32 text_len;
    u32 cursor;  // index of next character, 0 = before first char, len = after last char
    u32 selection_start;
    u32 selection_len;
    ta_game_state prev_state;
} text_entry_settings;

typedef bool text_entry_filter(char c);

// WARNING: Any of these pointers will be invalidated if their pool resizes
typedef struct ta_game {
    ta_game_state state;
    ta_audio_listener *audio;
    ta_audio_source *background_music;
    ta_font *font;
    ta_texture *tex_orange;
    ta_texture *tex_red;
    ta_texture *tex_audio_icon;
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

    struct {
        text_entry_settings *entry;
        text_entry_filter *filter;
    } text_entry;
} ta_game;

extern ta_game tg_game;
extern GLenum tg_polygon_mode;

void ta_game_init();
void ta_game_state_set(ta_game_state state);
void ta_game_events();