#pragma once
#include "dlb/dlb_types.h"

typedef struct ta_player {
    u32 id;
    u32 entity_id;
    u32 gun_id;
    u32 *gun_ids;
} ta_player;

//void ta_player_equip_next();
//void ta_player_equip_prev();
//void ta_player_equip_by_index(u32 index);

typedef struct ta_gun {
    u32 id;
    u32 entity_id;
    u16 loaded_ammo;        // # of rounds in gun
    u16 loaded_ammo_max;    // capacity of gun
    u16 carrying_ammo;      // # of rounds of extra ammo
    u16 carrying_ammo_max;  // capacity of extra ammo that can be carried
    u32 sfx_bang;           // fire a round
    u32 sfx_reload;         // reload
    u32 sfx_empty;          // out of ammo
} ta_gun;