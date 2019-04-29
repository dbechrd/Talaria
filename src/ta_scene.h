#pragma once
#include "ta_schema.h"
#include "ta_mesh.h"
#include "ta_entity.h"
#include "ta_file.h"
#include "dlb_types.h"
#include "dlb_hash.h"

typedef struct scene_ref_s {
    ta_schema_field_type type;
    void *ptr;
} scene_ref;

typedef struct ta_sun_light_s ta_sun_light;
typedef struct ta_point_light_s ta_point_light;
typedef struct ta_material_s ta_material;
typedef struct ta_mesh_s ta_mesh;
typedef struct ta_shader_s ta_shader;
typedef struct ta_texture_s ta_texture;
typedef struct ta_entity_s ta_entity;

typedef struct ta_scene_s {
    const char *name;

    scene_ref *refs;
    dlb_hash refs_by_name;

    ta_sun_light *sun_lights;
    ta_point_light *point_lights;
    ta_material *materials;
    ta_mesh *meshes;
    ta_shader *shaders;
    ta_texture *textures;
    ta_entity *entities;
} ta_scene;

ta_scene *ta_scene_init(const char *name);
ta_scene *ta_scene_load(ta_file *f);
void ta_scene_free(ta_scene *scn);
void ta_scene_print(ta_scene *scn, FILE *hnd);
void *ta_scene_obj_init(ta_scene *scn, ta_schema_field_type type);