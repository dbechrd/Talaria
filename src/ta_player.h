#pragma once
#include "dlb/dlb_types.h"

typedef struct ta_player {
    u32 index;
    const char *name;
    const char *entity_name;
    const char *e_gun;
    const char **e_guns;
} ta_player;

//void ta_player_equip_next();
//void ta_player_equip_prev();
//void ta_player_equip_by_index(u32 index);

typedef struct ta_gun {
    u32 index;
    const char *name;
    const char *entity_name;
    u16 carrying_ammo;       // # of rounds of extra ammo
    u16 carrying_ammo_max;   // capacity of extra ammo that can be carried
    u16 loaded_ammo;         // # of rounds in gun
    u16 loaded_ammo_max;     // capacity of gun
    const char *sfx_bang;    // fire a round
    const char *sfx_reload;  // reload
    const char *sfx_empty;   // out of ammo
} ta_gun;