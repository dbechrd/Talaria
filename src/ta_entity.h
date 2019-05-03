#pragma once
#include "ta_math.h"

typedef struct ta_scene_s ta_scene;

typedef struct ta_sun_light_s {
    ta_scene *scene;
    const char *name;
    ta_vec3 direction;
    ta_vec3 color;
} ta_sun_light;

typedef struct ta_point_light_s {
    ta_scene *scene;
    const char *name;
    ta_vec3 position;
    ta_vec3 color;
} ta_point_light;

typedef struct ta_material_s {
    ta_scene *scene;
    const char *name;
    //ta_texture *texture;  // TODO: Use file id?
} ta_material;

typedef struct ta_shader_s {
    ta_scene *scene;
    const char *name;
    const char *path;
} ta_shader;

typedef enum {
    ENTITY_DEFAULT
} ta_entity_type;

typedef struct ta_entity_s ta_entity;
typedef struct ta_entity_s {
    ta_scene *scene;
    const char *name;
    ta_entity_type type;
    const char *material;
    const char *mesh;
    const char *shader;
    const char *texture;
    ta_transform transform;
    ta_entity *parent;
    //ta_entity *next;  // TODO: Is a sibling linked list useful?
    ta_entity **children;
} ta_entity;

ta_material *entity_material(ta_entity *e);