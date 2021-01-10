#include "ta_material.h"

const char *tg_material_default;

void ta_material_init(ta_material *material)
{
    TracyCZone(ctxMethod, true);

    if (!material->shader) {
        const char *sym_mesh = 0;
        if (!sym_mesh) sym_mesh = INTERN("mesh");
        material->shader = sym_mesh;
    }

    if (vec3_zero(*(ta_vec3 *)&material->albedo_factor)) {
        material->albedo_factor.r = 1.0f;
        material->albedo_factor.g = 1.0f;
        material->albedo_factor.b = 1.0f;
    }
    if (!material->albedo_factor.a) {
        material->albedo_factor.a = 1.0f;
    }
    if (material->emission_texture && vec3_zero(*(ta_vec3 *)&material->emission_factor)) {
        material->emission_factor.r = 1.0f;
        material->emission_factor.g = 1.0f;
        material->emission_factor.b = 1.0f;
    }
    if (material->metallic_texture && !material->metallic_factor) {
        material->metallic_factor = 1.0f;
    }
    if (!material->roughness_factor) {
        material->roughness_factor = material->roughness_texture ? 1.0f : 0.5f;
    }
    if (material->height_texture && !material->height_factor) {
        material->height_factor = 0.02f;
    }

    TracyCZoneEnd(ctxMethod);
}
void ta_material_init_void(void *material)
{
    ta_material_init(material);
}
void ta_material_free(ta_material *material)
{
    UNUSED(material);
}
void ta_material_free_void(void *material)
{
    ta_material_free(material);
}