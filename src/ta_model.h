#pragma once
#include "ta_uid.h"
#include "ta_math.h"
#include "dlb/dlb_types.h"

typedef struct ta_model {
    u32 index;
    const char *name;
    const char *entity_name;
    // TODO: Do I need mesh groups, or should i just have multiple mesh components?
    const char **mesh_groups;
    // TODO: If I need multiple materials per mesh group, probably easier to just
    //       split into multiple mesh components, with mesh/material pairs.
    const char *material;
    bool invisible;
    bool cast_shadows;
    bool receive_shadows;  // TODO: Pass as flag to PBR shader, skip shadows if false
} ta_model;

struct ta_camera;
struct ta_shader;

// TODO: Move all this to ta_entity.h or get rid of it entirely (via "systems")
void ta_model_shadow_pass(struct ta_model *model, struct ta_shader *shader,
    ta_mat4 *light_pv, float alpha);
void ta_model_render(struct ta_model *model, struct ta_camera *camera,
    float alpha);
void ta_model_render_shader(struct ta_model *model, struct ta_camera *camera,
    struct ta_shader *shader, float alpha, float scale);