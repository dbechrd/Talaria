#pragma once
#include "ta_schema.h"
#include "dlb/dlb_types.h"

typedef struct ta_player {
    TA_COMPONENT_HEADER
    const char *e_gun;      // Entity name of currently equipped gun
    const char **e_guns;    // Array of entity names for all guns in inventory
} ta_player;

//void ta_player_equip_next();
//void ta_player_equip_prev();
//void ta_player_equip_by_index(size_t index);

typedef struct ta_gun {
    TA_COMPONENT_HEADER
    u32        carrying_ammo;       // # of rounds of extra ammo
    u32        carrying_ammo_max;   // capacity of extra ammo that can be carried
    u32        loaded_ammo;         // # of rounds in gun
    u32        loaded_ammo_max;     // capacity of gun
    const char *sfx_bang;           // fire a round
    const char *sfx_reload;         // reload
    const char *sfx_empty;          // out of ammo
} ta_gun;

void ta_player_free     (ta_player *player);
void ta_player_free_void(void *player);