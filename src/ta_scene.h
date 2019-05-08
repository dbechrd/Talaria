#pragma once
#include "ta_schema.h"
#include "ta_camera.h"
#include "ta_mesh.h"
#include "ta_shader.h"
#include "ta_texture.h"
#include "ta_entity.h"
#include "ta_file.h"
#include "dlb_types.h"
#include "dlb_hash.h"

typedef struct ta_scene_s {
    const char *name;

    ta_schema_ref *refs;
    dlb_hash refs_by_uid;

    ta_camera *cameras;
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
void *ta_scene_obj_alloc(ta_scene *scn, ta_schema_field_type type);
void ta_scene_obj_init(ta_scene *scn);
void *ta_scene_find(ta_scene *scene, ta_schema_field_type type, const char *uid);
#if 0
void *ta_scene_find(ta_scene *scene, const char *name, int len, ta_schema_field_type type);
ta_material *ta_scene_find_material_by_name(ta_scene *scene, const char *name, int len);
ta_mesh *ta_scene_find_mesh_by_name(ta_scene *scene, const char *name, int len);
ta_shader *ta_scene_find_shader_by_name(ta_scene *scene, const char *name, int len);
ta_texture *ta_scene_find_texure_by_name(ta_scene *scene, const char *name, int len);
ta_entity *ta_scene_find_entity_by_name(ta_scene *scene, const char *name, int len);
#endif