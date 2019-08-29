#pragma once
#include "dlb/dlb_types.h"

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
} text_entry_settings;

typedef bool text_entry_filter(char c);

typedef struct ta_audio_listener ta_audio_listener;
typedef struct ta_audio_source   ta_audio_source;
typedef struct ta_camera         ta_camera;
typedef struct ta_event          ta_event;
typedef struct ta_font           ta_font;
typedef struct ta_light          ta_light;
typedef struct ta_node           ta_node;
typedef struct ta_scene          ta_scene;
typedef struct ta_texture        ta_texture;
typedef struct ta_window         ta_window;

// WARNING: Any of these pointers will be invalidated if their pool resizes
// TODO: Replace with indexes into pool
typedef struct ta_game {
    ta_game_state state;
    ta_game_state state_prev;

    ta_audio_listener *audio;
    ta_audio_source *background_music;
    ta_camera *camera;
    ta_camera *camera_player;
    ta_camera *camera_freecam;
    ta_font *font;
    ta_light *lights;
    ta_node *player;
    ta_scene *scene;
    ta_texture *tex_orange;
    ta_texture *tex_red;
    ta_texture *tex_audio_icon;
    ta_window *window;

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

void ta_game_init(ta_game *game);
void ta_game_state_set(ta_game *game, ta_game_state state);
void ta_game_event(ta_game *game, ta_event *event);