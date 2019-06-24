#pragma once
#include "ta_scene.h"
#include "ta_math.h"
#include "ta_material.h"
#include "ta_mesh_group.h"
#include "ta_rigid_body.h"
#include "ta_camera.h"

typedef enum {
    TA_ENT_DEFAULT,  // TODO: Get rid if "default" entities and always have a type?
    TA_ENT_BUTTON,
    TA_ENT_COUNT
} ta_entity_type;

typedef struct ta_entity_s {
    ta_scene_ref ref;
    ta_entity_type type;

    ta_transform transform;
    ta_transform transform_prev;
    ta_mat4 model;

    const char *material_uid;
    const char *mesh_group_uid;
    const char *rigid_body_uid;
    const char *parent_uid;

    ta_aabb aabb;  // AABB in local space

    bool invisible;
    //ta_entity *next;  // TODO: Is a sibling linked list useful?
    //ta_entity **children;
} ta_entity;

void ta_entity_init(ta_entity *e);
ta_material *ta_entity_material(ta_entity *e);
ta_mesh_group *ta_entity_mesh_group(ta_entity *e);
ta_rigid_body *ta_entity_rigid_body(ta_entity *e);
//bool ta_entity_intersect(ta_entity *a, ta_entity *b, ta_manifold *manifold);
void ta_entity_update(ta_entity *e);
void ta_entity_render(ta_entity *e, ta_camera *camera, float alpha);