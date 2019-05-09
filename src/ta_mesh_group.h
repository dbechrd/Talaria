#pragma once
#include "dlb_hash.h"

typedef struct ta_scene_s ta_scene;
typedef struct ta_mesh_s ta_mesh;

typedef struct ta_mesh_group_s {
    ta_scene *scene;
    const char *uid;
    const char *path;
    ta_mesh *meshes;
    //dlb_hash meshes_by_name;
} ta_mesh_group;

void ta_mesh_group_init(ta_mesh_group *group, const char *uid, const char *path);
void ta_mesh_group_load(ta_mesh_group *group);
void ta_mesh_group_push_normals(ta_mesh_group *group);
void ta_mesh_group_render(ta_mesh_group *group);
void ta_mesh_group_free(ta_mesh_group *group);