#include "ta_material.h"
#include "ta_symbol.h"

void ta_material_init(ta_material *material)
{
    if (!material->tex_albedo)    { material->tex_albedo    = SYM_MISSING_ALBEDO; }
    if (!material->tex_height)    { material->tex_height    = SYM_MISSING_HEIGHT; }
    if (!material->tex_metallic)  { material->tex_metallic  = SYM_MISSING_METALLIC; }
    if (!material->tex_normal)    { material->tex_normal    = SYM_MISSING_NORMAL; }
    if (!material->tex_occlusion) { material->tex_occlusion = SYM_MISSING_OCCLUSION; }
    if (!material->tex_roughness) { material->tex_roughness = SYM_MISSING_ROUGHNESS; }
}
