#pragma once
#include "ta_schema.h"
#include "ta_file.h"
#include "ta_math.h"
#include "dlb_types.h"
#include "dlb_hash.h"

typedef struct ta_camera_s ta_camera;
typedef struct ta_scene_s ta_scene;
typedef struct ta_shader_s ta_shader;

typedef struct ta_scene_ref_s {
    ta_scene *scene;
    ta_schema_field_type type;
    const char *uid;
    void *ptr;
} ta_scene_ref;

typedef struct ta_scene_s {
    const char *name;

    ta_scene_ref *refs;
    dlb_hash refs_by_uid;
	u32 refs_placeholder_count;

    const char *default_material_uid;
    const char *default_texture_uid;
    const char *default_mesh_group_uid;

    void *pools[F_TA_COUNT];
    //ta_camera *cameras;
    //ta_light *lights;
    //ta_material *materials;
    //ta_mesh_group *mesh_groups;
    //ta_shader *shaders;
    //ta_texture *textures;
    //ta_rigid_body *rigid_bodies;
    //ta_node *entities;
} ta_scene;

ta_scene *ta_scene_init(const char *name);
ta_scene *ta_scene_load(ta_file *f);
void ta_scene_free(ta_scene *scn);
void ta_scene_print(ta_scene *scn, FILE *hnd);
void *ta_scene_obj_alloc(ta_scene *scene, ta_schema_field_type type,
    const char *uid);
void ta_scene_initialize_objects(ta_scene *scn);
void *ta_scene_find(ta_scene *scene, ta_schema_field_type type, const char *uid);
void *ta_scene_find_by_ref(ta_scene_ref *ref, ta_schema_field_type type,
    const char *uid);
void ta_scene_update(ta_scene *scene, float dt);
void ta_scene_shadow_pass(ta_scene *scene, ta_shader *shader, float alpha);
void ta_scene_render(ta_scene *scene, ta_camera *camera, float alpha);