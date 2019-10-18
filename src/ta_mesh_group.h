#pragma once
#include "ta_uid.h"
#include "ta_math.h"

typedef struct ta_mesh_group {
    u32 id;
    const char *path;
    u32 *mesh_ids;
    ta_aabb aabb;
} ta_mesh_group;

void ta_mesh_group_init(ta_mesh_group *group, const char *path);
void ta_mesh_group_load(ta_mesh_group *group);
void ta_mesh_group_push_normals(ta_mesh_group *group);
void ta_mesh_group_render(ta_mesh_group *group);
void ta_mesh_group_free(ta_mesh_group *group);