#include "ta_material.h"
#include "dlb/dlb_types.h"

const char *tg_material_default;

void ta_material_ubo_init(ta_material_ubo *material_ubo)
{
    TracyCZone(ctxMethod, true);
    // NOTE: This is necessary for std140 packing of ta_materialing_records in ubo_materials
    DLB_ASSERT(sizeof(bool) == sizeof(int));

    glGenBuffers(1, &material_ubo->gl_ubo_id);
    glBindBufferBase(GL_UNIFORM_BUFFER, TA_GLSL_UBO_MATERIALS, material_ubo->gl_ubo_id);
    glBindBuffer(GL_UNIFORM_BUFFER, material_ubo->gl_ubo_id);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(ta_material_ubo_entry) * TA_MATERIAL_MAX_ACTIVE_MATERIALS, 0, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    // TODO: If we want to use binding point 0 for other things in other shaders, then this mapping needs to be a bit
    // more abstract.
    glBindBufferBase(GL_UNIFORM_BUFFER, TA_GLSL_UBO_MATERIALS, material_ubo->gl_ubo_id);
    TracyCZoneEnd(ctxMethod);
}
void ta_material_ubo_bind(ta_material_ubo *material_ubo)
{
    TracyCZone(ctxMethod, true);
    ta_material *materials = (ta_material *)ta_game_resource_pool(RES_MATERIAL);
    int material_idx = 0;
    for (size_t i = 0; i < dlb_vec_len(materials) && i < TA_MATERIAL_MAX_ACTIVE_MATERIALS; ++i) {
        const ta_material *material = &materials[i];

        ta_texture *albedo_texture    = (ta_texture *)ta_game_by_sym_try(RES_TEXTURE, material->albedo_texture);
        ta_texture *emission_texture  = (ta_texture *)ta_game_by_sym_try(RES_TEXTURE, material->emission_texture);
        ta_texture *height_texture    = (ta_texture *)ta_game_by_sym_try(RES_TEXTURE, material->height_texture);
        ta_texture *metallic_texture  = (ta_texture *)ta_game_by_sym_try(RES_TEXTURE, material->metallic_texture);
        ta_texture *normal_texture    = (ta_texture *)ta_game_by_sym_try(RES_TEXTURE, material->normal_texture);
        ta_texture *occlusion_texture = (ta_texture *)ta_game_by_sym_try(RES_TEXTURE, material->occlusion_texture);
        ta_texture *roughness_texture = (ta_texture *)ta_game_by_sym_try(RES_TEXTURE, material->roughness_texture);

        // NOTE: Seems dumb to bind a texture only for the multiplication factor to be 0.0, right?
        DLB_ASSERT(material->albedo_factor.a);
        if (emission_texture)  { DLB_ASSERT(material->emission_factor.r || material->emission_factor.g || material->emission_factor.b); }
        if (height_texture)    { DLB_ASSERT(material->height_factor); }
        if (metallic_texture)  { DLB_ASSERT(material->metallic_factor); }
        if (roughness_texture) { DLB_ASSERT(material->roughness_factor); }

        if (!albedo_texture   ) { albedo_texture    = ta_game_by_sym_try(RES_TEXTURE, tg_tex_default_albedo); }
        if (!emission_texture ) { emission_texture  = ta_game_by_sym_try(RES_TEXTURE, tg_tex_default_emission); }
        if (!height_texture   ) { height_texture    = ta_game_by_sym_try(RES_TEXTURE, tg_tex_default_height); }
        if (!metallic_texture ) { metallic_texture  = ta_game_by_sym_try(RES_TEXTURE, tg_tex_default_metallic); }
        if (!normal_texture   ) { normal_texture    = ta_game_by_sym_try(RES_TEXTURE, tg_tex_default_normal); }
        if (!occlusion_texture) { occlusion_texture = ta_game_by_sym_try(RES_TEXTURE, tg_tex_default_occlusion); }
        if (!roughness_texture) { roughness_texture = ta_game_by_sym_try(RES_TEXTURE, tg_tex_default_roughness); }

        DLB_ASSERT(albedo_texture    && albedo_texture   ->type == TA_TEXTURE_2D_ARRAY);
        DLB_ASSERT(emission_texture  && emission_texture ->type == TA_TEXTURE_2D_ARRAY);
        DLB_ASSERT(height_texture    && height_texture   ->type == TA_TEXTURE_2D_ARRAY);
        DLB_ASSERT(metallic_texture  && metallic_texture ->type == TA_TEXTURE_2D_ARRAY);
        DLB_ASSERT(normal_texture    && normal_texture   ->type == TA_TEXTURE_2D_ARRAY);
        DLB_ASSERT(occlusion_texture && occlusion_texture->type == TA_TEXTURE_2D_ARRAY);
        DLB_ASSERT(roughness_texture && roughness_texture->type == TA_TEXTURE_2D_ARRAY);

        material_ubo->materials[material_idx].albedo_factor                 = *(ta_vec4 *)&material->albedo_factor;
        material_ubo->materials[material_idx].emission_factor               = *(ta_vec3 *)&material->emission_factor;
        material_ubo->materials[material_idx].height_factor                 = material->height_factor;
        material_ubo->materials[material_idx].metallic_factor               = material->metallic_factor;
        material_ubo->materials[material_idx].roughness_factor              = material->roughness_factor;
        material_ubo->materials[material_idx].albedo_texture_pool_index     = albedo_texture->gl_texture_pool_index;
        material_ubo->materials[material_idx].albedo_texture_pool_layer     = albedo_texture->gl_texture_pool_layer;
        material_ubo->materials[material_idx].emission_texture_pool_index   = emission_texture->gl_texture_pool_index;
        material_ubo->materials[material_idx].emission_texture_pool_layer   = emission_texture->gl_texture_pool_layer;
        material_ubo->materials[material_idx].height_texture_pool_index     = height_texture->gl_texture_pool_index;
        material_ubo->materials[material_idx].height_texture_pool_layer     = height_texture->gl_texture_pool_layer;
        material_ubo->materials[material_idx].metallic_texture_pool_index   = metallic_texture->gl_texture_pool_index;
        material_ubo->materials[material_idx].metallic_texture_pool_layer   = metallic_texture->gl_texture_pool_layer;
        material_ubo->materials[material_idx].normal_texture_pool_index     = normal_texture->gl_texture_pool_index;
        material_ubo->materials[material_idx].normal_texture_pool_layer     = normal_texture->gl_texture_pool_layer;
        material_ubo->materials[material_idx].occlusion_texture_pool_index  = occlusion_texture->gl_texture_pool_index;
        material_ubo->materials[material_idx].occlusion_texture_pool_layer  = occlusion_texture->gl_texture_pool_layer;
        material_ubo->materials[material_idx].roughness_texture_pool_index  = roughness_texture->gl_texture_pool_index;
        material_ubo->materials[material_idx].roughness_texture_pool_layer  = roughness_texture->gl_texture_pool_layer;

        material_idx++;
    }

    glBindBuffer(GL_UNIFORM_BUFFER, material_ubo->gl_ubo_id);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(material_ubo->materials), material_ubo->materials, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    TracyCZoneEnd(ctxMethod);
}

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