#pragma once
#include "dlb/dlb_types.h"

struct ta_event;
struct ta_player;
enum ta_resource_type;

extern const char *tg_font;
extern const char *tg_tex_orange;
extern const char *tg_tex_red;
extern const char *tg_tex_audio_icon;
extern const char *tg_e_background_music;
extern const char *tg_e_freecam;
extern const char *tg_e_player_one;
extern const char *tg_e_active_camera;

typedef enum ta_game_state {
    TA_GAME_STATE_STARTUP,
    TA_GAME_STATE_PLAY,
    TA_GAME_STATE_FREE_CAM,
    TA_GAME_STATE_EDITOR,
    TA_GAME_STATE_SHUTDOWN,
    TA_GAME_STATE_COUNT
} ta_game_state;
const char *game_state_str(ta_game_state state);

#if 0
// TODO: Pass these around instead of having to explicitly say the resource type
// every time you want to request a component.
typedef struct ta_component {
    ta_resource_type type;
    const char *entity_name;
} ta_component;
#endif

void ta_game_init();
ta_game_state ta_game_state_current();
ta_game_state ta_game_state_prev();
void ta_game_state_set(ta_game_state state);
void *ta_game_alloc(enum ta_resource_type type, const char *name, u32 name_len);
void ta_game_destroy(enum ta_resource_type type, const char *name, u32 name_len);
void *ta_game_by_name(enum ta_resource_type type, const char *name, u32 name_len);
void *ta_game_by_name_try(enum ta_resource_type type, const char *name, u32 name_len);
void *ta_game_by_name_or_default(enum ta_resource_type type, const char *name, u32 name_len);
void *ta_game_by_sym(enum ta_resource_type type, const char *sym);
void *ta_game_by_sym_try(enum ta_resource_type type, const char *sym);
void *ta_game_by_sym_or_default(enum ta_resource_type type, const char *sym);
void *ta_game_component_add(const char *entity, enum ta_resource_type type,
    const char *name, u32 name_len);
void *ta_game_component(const char *entity, enum ta_resource_type type);
void *ta_game_component_try(const char *entity, enum ta_resource_type type);
void *ta_game_resource_pool(enum ta_resource_type type);
void ta_game_load_gltf();
struct ta_camera *ta_game_camera();
struct ta_player *ta_game_player();
void ta_game_sim_pause();
void ta_game_sim_resume();
void ta_game_sim_step_n_frames(int frames);
bool ta_game_sim_running();
bool ta_game_sim_paused();
u64 ta_game_sim_step();
u64 ta_game_frame_num();
void ta_game_window_resize();
void ta_game_loop();
void ta_game_hotkeys();
void ta_game_event(struct ta_event *event);
void ta_game_save();