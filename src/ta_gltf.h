#pragma once

struct ta_scene;

typedef struct ta_gltf {
    struct cgltf_data *data;
} ta_gltf;

enum cgltf_result ta_gltf_parse_file(ta_gltf *gltf, const char *filename);
void ta_gltf_load(ta_gltf *gltf);
void ta_gltf_free(ta_gltf *gltf);