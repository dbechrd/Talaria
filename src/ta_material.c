#include "ta_material.h"

void ta_material_init(ta_material *material)
{
    if (!material->shader) {
        const char *sym_mesh = 0;
        if (!sym_mesh) sym_mesh = INTERN("mesh");
        material->shader = sym_mesh;
    }
    if (!material->albedo_factor.a) {
        material->albedo_factor.r = 1.0f;
        material->albedo_factor.g = 1.0f;
        material->albedo_factor.b = 1.0f;
        material->albedo_factor.a = 1.0f;
    }
    if (material->emission_texture && !(material->emission_factor.r || material->emission_factor.g || material->emission_factor.b)) {
        material->emission_factor.r = 1.0f;
        material->emission_factor.g = 1.0f;
        material->emission_factor.b = 1.0f;
    }
    if (material->metallic_texture && !material->metallic_factor) {
        material->metallic_factor = 1.0f;
    }
    if (material->roughness_texture && !material->roughness_factor) {
        material->roughness_factor = 1.0f;
    }
    if (material->height_texture && !material->height_factor) {
        material->height_factor = 0.02f;
    }
}
