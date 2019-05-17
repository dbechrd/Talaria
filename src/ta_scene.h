#pragma once
#include "ta_schema.h"
#include "ta_file.h"
#include "ta_math.h"
#include "dlb_types.h"
#include "dlb_hash.h"

typedef struct ta_camera_s      ta_camera;
typedef struct ta_sun_light_s   ta_sun_light;
typedef struct ta_point_light_s ta_point_light;
typedef struct ta_material_s    ta_material;
typedef struct ta_mesh_group_s  ta_mesh_group;
typedef struct ta_shader_s      ta_shader;
typedef struct ta_texture_s     ta_texture;
typedef struct ta_rigid_body_s  ta_rigid_body;
typedef struct ta_entity_s      ta_entity;

typedef struct ta_scene_s {
    const char *name;

    ta_schema_ref *refs;
    dlb_hash refs_by_uid;

    ta_camera *cameras;
    ta_sun_light *sun_lights;
    ta_point_light *point_lights;
    ta_material *materials;
    ta_mesh_group *mesh_groups;
    ta_shader *shaders;
    ta_texture *textures;
    ta_rigid_body *rigid_bodies;
    ta_entity *entities;
} ta_scene;

ta_scene *ta_scene_init(const char *name);
ta_scene *ta_scene_load(ta_file *f);
void ta_scene_free(ta_scene *scn);
void ta_scene_print(ta_scene *scn, FILE *hnd);
void *ta_scene_obj_alloc(ta_scene *scn, ta_schema_field_type type);
void ta_scene_obj_init(ta_scene *scn);
void *ta_scene_find(ta_scene *scene, ta_schema_field_type type, const char *uid);
void ta_scene_update(ta_scene *scene, double dt);
void ta_scene_render(ta_scene *scene, ta_mat4 *proj, ta_mat4 *view);