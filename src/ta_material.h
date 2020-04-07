#pragma once
#include "ta_schema.h"
#include "ta_math.h"

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

typedef struct ta_material {
    TA_RESOURCE_HEADER
    const char *shader;
    const char *albedo_texture;
    ta_rgba     albedo_factor;
    const char *emission_texture;
    ta_rgb      emission_factor;
    const char *metallic_texture;
    float       metallic_factor;
    const char *roughness_texture;
    float       roughness_factor;
    const char *height_texture;
    float       height_factor;
    const char *normal_texture;
    const char *occlusion_texture;
} ta_material;

ta_material *tg_material_default;

void ta_material_init(ta_material *material);
