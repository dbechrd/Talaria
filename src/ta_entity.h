#pragma once
#include "ta_scene.h"
#include "ta_math.h"
#include "ta_material.h"
#include "ta_mesh_group.h"
#include "ta_rigid_body.h"

typedef enum {
    ENTITY_MESH_GROUP = 0,
} ta_entity_type;

typedef struct ta_entity_s ta_entity;
typedef struct ta_entity_s {
    ta_scene *scene;
    const char *uid;
    ta_entity_type type;
    ta_transform transform;
    const char *material_uid;
    const char *mesh_group_uid;
    const char *rigid_body_uid;
    const char *parent_uid;
    ta_aabb aabb;
    ta_mat4 model;
    //ta_entity *next;  // TODO: Is a sibling linked list useful?
    //ta_entity **children;
} ta_entity;

void ta_entity_init(ta_entity *e);
ta_material *ta_entity_material(ta_entity *e);
ta_mesh_group *ta_entity_mesh_group(ta_entity *e);
ta_rigid_body *ta_entity_rigid_body(ta_entity *e);
void ta_entity_update(ta_entity *e, double dt);
void ta_entity_push_aabb(ta_entity *e, ta_rgba color);
void ta_entity_push_normals(ta_entity *e);
void ta_entity_render(ta_entity *e, ta_camera *camera);