#pragma once
#include "ta_uid.h"

typedef struct ta_material {
    u32 id;
    u32 shader_id;
    u32 tex_albedo_id;
    u32 tex_metallic_id;
} ta_material;