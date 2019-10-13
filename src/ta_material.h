#pragma once
#include "ta_uid.h"

typedef struct ta_material {
    ta_handle shader;
    ta_handle tex_albedo;
    ta_handle tex_metallic;
} ta_material;