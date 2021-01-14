#pragma once
#include "ta_schema.h"
#include "ta_math.h"
#include "dlb/dlb_types.h"

/*
  Material.002 [0 @ 0000027377221348]
    pbr_metallic_roughness:
      base_color_texture:
        texture: (null) [0 @ 0000027376F69608]
      base_color_factor: 1.000000 1.000000 1.000000 1.000000
      metallic_factor: 1.000000
      roughness_factor: 0.500000
    emissive_factor: 0.000000 0.000000 0.000000
    alpha_mode: 0
    alpha_cutoff: 0.500000
    double_sided: True
    unlit: False
*/

// NOTE: Has to match shader definition
#define TA_MATERIAL_MAX_ACTIVE_MATERIALS 64

typedef struct ta_material_ubo_entry {
    ta_vec4 albedo_factor;
    ta_vec3 emission_factor;
    float   height_factor;
    float   metallic_factor;
    float   roughness_factor;

    u32     albedo_texture_pool_index;
    u32     albedo_texture_pool_layer;
    u32     emission_texture_pool_index;
    u32     emission_texture_pool_layer;
    u32     height_texture_pool_index;
    u32     height_texture_pool_layer;
    u32     metallic_texture_pool_index;
    u32     metallic_texture_pool_layer;
    u32     normal_texture_pool_index;
    u32     normal_texture_pool_layer;
    u32     occlusion_texture_pool_index;
    u32     occlusion_texture_pool_layer;
    u32     roughness_texture_pool_index;
    u32     roughness_texture_pool_layer;
} ta_material_ubo_entry;

typedef struct ta_material_ubo {
    ta_material_ubo_entry materials[TA_MATERIAL_MAX_ACTIVE_MATERIALS];
    GLuint gl_ubo_id;
} ta_material_ubo;

typedef struct ta_material {
    TA_RESOURCE_HEADER
    const char *shader;
    ta_rgba     albedo_factor;
    ta_rgb      emission_factor;
    float       height_factor;
    float       metallic_factor;
    float       roughness_factor;

    const char *albedo_texture;     // note: Expect sRGB texture (shader will convert to linear color space)
    const char *emission_texture;   // note: Expect sRGB texture (shader will convert to linear color space)
    const char *height_texture;
    const char *metallic_texture;
    const char *normal_texture;
    const char *occlusion_texture;
    const char *roughness_texture;
} ta_material;

extern const char *tg_material_default;

void ta_material_ubo_init   (ta_material_ubo *material_ubo);
void ta_material_ubo_bind   (ta_material_ubo *material_ubo);

void ta_material_init       (ta_material *material);
void ta_material_init_void  (void *material);
void ta_material_free       (ta_material *material);
void ta_material_free_void  (void *material);