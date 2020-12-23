#pragma once
#include "ta_schema.h"
#include "ta_math.h"

// NOTE: Needs to match the GLSL #define
#define TA_MODEL_MAX_MORPHS 1

// TODO: This could be named ta_geometry_instance, this comes from geometry_node in OGX file
typedef struct ta_model {
    TA_COMPONENT_HEADER
    ta_xform   xform;                  // Visual transform
    const char *mesh;                  // Mesh name
    const char **materials;            // Vector of material names
    float      *morph_target_weights;  // Vector of morph target weights
    bool       no_render;              // If true, model is not rendered
    bool       no_shadow_cast;         // If true, model will not cast real-time shadows
    bool       no_shadow_receive;      // If true, model will not use shadow maps                    // TODO: Pass as flag to PBR shader, skip shadows if false
    //bool       no_serialize;           // If true, do not serialize this model to the scene file   // HACK: Exclude GLTF models from scene file
} ta_model;

struct ta_camera;
struct ta_shader;

void ta_model_init                      (ta_model *model);
void ta_model_init_void                 (void *model);
void ta_model_free                      (ta_model *model);
void ta_model_free_void                 (void *model);
float ta_model_get_morph_target_weight  (ta_model *model, const char *morph_target_name);
void ta_model_set_morph_target_weight   (ta_model *model, const char *morph_target_name, float weight);
void ta_model_shadow_pass               (ta_model *model, struct ta_shader *shader, ta_mat4 *light_pv);
void ta_model_render                    (ta_model *model);
void ta_model_render_shader             (ta_model *model, struct ta_camera *camera, struct ta_shader *shader);