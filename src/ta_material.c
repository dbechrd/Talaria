#include "ta_material.h"
#include "ta_symbol.h"

void ta_material_init(ta_material *material)
{
    if (!material->albedo_texture)    { material->albedo_texture    = SYM_MISSING_ALBEDO; }
    if (!material->emission_texture)  { material->emission_texture  = SYM_MISSING_EMISSION; }
    if (!material->metallic_texture)  { material->metallic_texture  = SYM_MISSING_METALLIC; }
    if (!material->roughness_texture) { material->roughness_texture = SYM_MISSING_ROUGHNESS; }
    if (!material->height_texture)    { material->height_texture    = SYM_MISSING_HEIGHT; }
    if (!material->normal_texture)    { material->normal_texture    = SYM_MISSING_NORMAL; }
    if (!material->occlusion_texture) { material->occlusion_texture = SYM_MISSING_OCCLUSION; }
}
