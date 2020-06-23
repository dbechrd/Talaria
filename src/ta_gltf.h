#pragma once
#include "misc/cgltf.h"

struct ta_scene;

typedef struct ta_gltf {
    const char *filename;
    struct cgltf_data *data;    // [cgltf] data representing the entire GLTF scene
} ta_gltf;

cgltf_result ta_gltf_parse_file (ta_gltf *gltf);
void ta_gltf_load               (ta_gltf *gltf);
void ta_gltf_free               (ta_gltf *gltf);