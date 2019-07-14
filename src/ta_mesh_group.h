#pragma once
#include "ta_uid.h"
#include "ta_mesh.h"
#include "ta_rigid_body.h"

typedef struct ta_mesh_group {
    ta_uid uid;
    const char *path;
    ta_mesh *meshes;
    ta_aabb aabb;
} ta_mesh_group;

void ta_mesh_group_init(ta_mesh_group *group, const char *uid, const char *path);
void ta_mesh_group_load(ta_mesh_group *group);
void ta_mesh_group_push_normals(ta_mesh_group *group);
void ta_mesh_group_render(ta_mesh_group *group);
void ta_mesh_group_free(ta_mesh_group *group);