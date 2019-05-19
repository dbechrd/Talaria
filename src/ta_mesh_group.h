#pragma once
#include "ta_scene.h"
#include "ta_mesh.h"
#include "ta_rigid_body.h"
#include "dlb_hash.h"

typedef struct ta_mesh_group_s {
    ta_scene *scene;
    const char *uid;
    const char *path;
    ta_mesh *meshes;
    //dlb_hash meshes_by_name;
    ta_aabb aabb;
} ta_mesh_group;

void ta_mesh_group_init(ta_mesh_group *group, const char *uid, const char *path);
void ta_mesh_group_load(ta_mesh_group *group);
ta_aabb ta_mesh_group_aabb(ta_mesh_group *group);
void ta_mesh_group_push_normals(ta_mesh_group *group);
void ta_mesh_group_render(ta_mesh_group *group);
void ta_mesh_group_free(ta_mesh_group *group);