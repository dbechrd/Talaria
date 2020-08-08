#pragma once
#include "ta_schema.h"

typedef enum ta_cubemap_face {
    TA_CUBEMAP_FACE_POSITIVE_X,
    TA_CUBEMAP_FACE_NEGATIVE_X,
    TA_CUBEMAP_FACE_POSITIVE_Y,
    TA_CUBEMAP_FACE_NEGATIVE_Y,
    TA_CUBEMAP_FACE_POSITIVE_Z,
    TA_CUBEMAP_FACE_NEGATIVE_Z,
    TA_CUBEMAP_FACE_COUNT
} ta_cubemap_face;

typedef struct ta_cubemap {
    TA_RESOURCE_HEADER
    const char *textures[TA_CUBEMAP_FACE_COUNT];
} ta_cubemap;