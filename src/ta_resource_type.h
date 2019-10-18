#pragma once

typedef enum ta_resource_type {
    // Component types
    RES_COMP_AUDIO_SOURCE,
    RES_COMP_BUTTON,
    RES_COMP_CAMERA,
    RES_COMP_LIGHT,
    RES_COMP_MODEL,
    RES_COMP_POSITION,
    RES_COMP_RIGID_BODY,
    RES_COMP_COUNT,
    // Resource types
    RES_AUDIO_BUFFER = RES_COMP_COUNT,
    RES_ENTITY,
    RES_FONT,
    RES_MATERIAL,
    RES_MESH_GROUP,
    RES_MESH,
    RES_SHADER,
    RES_TEXTURE,
    RES_COUNT,
} ta_resource_type;