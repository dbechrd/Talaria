#pragma once
#include "ta_schema.h"
#include "dlb/dlb_types.h"

#if 0
// TODO: Binary format should just store component pools as-is, with the uid
// pool intact to map data to entity uid. This struct is for human-readable
// scene files where we want the notion of entities being a collection of
// components more explicitly:
//
//  ta_transform: {
//    uid: "player_transform"
//    position: { x: 0.0, y: 0.0, z: 0.0 }
//  }
//
//  ta_entity: {
//    uid: "player"
//    components: ["player_transform", "player_mesh", "player_audio_source"]
//  }
//
typedef struct ta_entity {
    u32 index;
    const char *name;
    const char *parent_name;
    u32 components[RES_COMP_COUNT];
} ta_entity;

void ta_entity_free(const char *entity);
#endif
