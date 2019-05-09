#pragma once
#include "ta_math.h"

typedef struct ta_scene_s ta_scene;

typedef struct ta_sun_light_s {
    ta_scene *scene;
    const char *uid;
    ta_vec3 direction;
    ta_vec3 color;
} ta_sun_light;

typedef struct ta_point_light_s {
    ta_scene *scene;
    const char *uid;
    ta_vec3 position;
    ta_vec3 color;
} ta_point_light;

typedef struct ta_material_s {
    ta_scene *scene;
    const char *uid;
    const char *shader_uid;
    const char *texture_uid;
} ta_material;

typedef enum {
    ENTITY_DEFAULT
} ta_entity_type;

typedef struct ta_entity_s ta_entity;
typedef struct ta_entity_s {
    ta_scene *scene;
    const char *uid;
    ta_entity_type type;
    const char *material_uid;
    const char *mesh_uid;
    const char *shader_uid;
    const char *texture_uid;
    ta_transform transform;
    ta_entity *parent;
    //ta_entity *next;  // TODO: Is a sibling linked list useful?
    ta_entity **children;
} ta_entity;

ta_material *entity_material(ta_entity *e);