#include "ta_ogx.h"

const char *ogx_key_kind_str[OGX_KEY_KIND_COUNT] = {
    [OGX_KEY_KIND_UNKNOWN    ] = "OGX_KEY_KIND_UNKNOWN",
    [OGX_KEY_KIND_VALUE      ] = "OGX_KEY_KIND_VALUE",
    [OGX_KEY_KIND_POS_CONTROL] = "OGX_KEY_KIND_POS_CONTROL",
    [OGX_KEY_KIND_NEG_CONTROL] = "OGX_KEY_KIND_NEG_CONTROL",
};
const char *ogx_type_str[OGX_TYPE_COUNT] = {
    [OGX_TYPE_UNKNOWN] = "OGX_TYPE_UNKNOWN",
    [OGX_TYPE_FLOAT  ] = "OGX_TYPE_FLOAT",
    [OGX_TYPE_VEC2   ] = "OGX_TYPE_VEC2",
    [OGX_TYPE_VEC3   ] = "OGX_TYPE_VEC3",
    [OGX_TYPE_MAT4   ] = "OGX_TYPE_MAT4",
};
const char *ogx_time_curve_str[OGX_TIME_CURVE_COUNT] = {
    [OGX_TIME_CURVE_UNKNOWN] = "OGX_TIME_CURVE_UNKNOWN",
    [OGX_TIME_CURVE_LINEAR ] = "OGX_TIME_CURVE_LINEAR",
    [OGX_TIME_CURVE_BEZIER ] = "OGX_TIME_CURVE_BEZIER",
};
const char *ogx_value_curve_str[OGX_VALUE_CURVE_COUNT] = {
    [OGX_VALUE_CURVE_UNKNOWN] = "OGX_VALUE_CURVE_UNKNOWN",
    [OGX_VALUE_CURVE_LINEAR ] = "OGX_VALUE_CURVE_LINEAR",
    [OGX_VALUE_CURVE_BEZIER ] = "OGX_VALUE_CURVE_BEZIER",
};

static void ta_ogx_load_texture(ogx_texture *tex)
{
    char filepath[1024] = { 0 };
    snprintf(filepath, sizeof(filepath) - 1, "data/mesh/%s", tex->path);

    ta_log_write(&tg_debug_log, SRC_OGX, "stbi_load_from_memory: %s\n", tex->name);
    //stbi_set_flip_vertically_on_load(true);
    int w = 0;
    int h = 0;
    int channels = 0;  // components/channels
    u8 *pixels = stbi_load(filepath, &w, &h, &channels, 0);
    //u8 *pixels = stbi_load_from_memory(buffer, (int)buffer_len, &w, &h, &channels, 0);
    if (!pixels) {
        const char *reason = stbi_failure_reason();
        ta_log_write(&tg_debug_log, SRC_OGX, "Failed to load tex: %s\nSTBI Reason: %s\n", tex->name, reason);
        DLB_ASSERT(!"ta_texture_init: Failed to load tex");
    }

    DLB_ASSERT(w);
    DLB_ASSERT(h);
    DLB_ASSERT(channels);

    ta_log_write(&tg_debug_log, SRC_OGX, "ta_game_alloc TEXTURE %s\n", tex->name);
    ta_texture *texture = ta_game_alloc(RES_TEXTURE, SYM(tex->name));
    texture->width = w;
    texture->height = h;
    texture->channels = (u8)channels;
    texture->pixels = pixels;

    ta_texture_init(texture);

    texture->pixels = 0; // HACK: Don't want ta_texture_free to try to call dlb_vec_free() on this buffer
    stbi_image_free(pixels);
}

static void ta_ogx_load_material(ogx_material *mat)
{
    ta_log_write(&tg_debug_log, SRC_OGX, "ta_game_alloc MATERIAL %s\n", mat->name);
    ta_material *material = ta_game_alloc(RES_MATERIAL, SYM(mat->name));
    material->albedo_texture     = mat->albedo_texture;
    material->albedo_factor.r    = mat->albedo_factor[0];
    material->albedo_factor.g    = mat->albedo_factor[1];
    material->albedo_factor.b    = mat->albedo_factor[2];
    material->albedo_factor.a    = mat->alpha_factor;
    // NOTE: ta_material doesn't support alpha_texture, so ensure we're not discarding anything
    DLB_ASSERT(!mat->alpha_texture);
    material->emission_texture   = mat->emissive_texture;
    material->emission_factor.r  = mat->emissive_factor[0];
    material->emission_factor.g  = mat->emissive_factor[1];
    material->emission_factor.g  = mat->emissive_factor[2];
    material->metallic_texture   = mat->metallic_texture;
    material->metallic_factor    = mat->metallic_factor;
    material->normal_texture     = mat->normal_texture;
    // NOTE: ta_material doesn't support normal_factor for now, so ensure we're not discarding anything
    if (mat->normal_texture) {
        DLB_ASSERT(mat->normal_factor[0] == 1.0f);
        DLB_ASSERT(mat->normal_factor[1] == 1.0f);
        DLB_ASSERT(mat->normal_factor[2] == 1.0f);
    } else {
        DLB_ASSERT(mat->normal_factor[0] == 0.0f);
        DLB_ASSERT(mat->normal_factor[1] == 0.0f);
        DLB_ASSERT(mat->normal_factor[2] == 0.0f);
    }
    material->normal_texture     = mat->normal_texture;
    material->roughness_texture  = mat->roughness_texture;
    material->roughness_factor   = mat->roughness_factor;

    ta_material_init(material);
}

static void ta_ogx_load_mesh(ogx_mesh *msh)
{
    UNUSED(msh);
    //mesh->
    //ta_log_write(&tg_debug_log, SRC_OGX, "ta_game_alloc MESH %s\n", msh->name);
    //ta_model *model = ta_game_alloc(RES_COMP_MODEL, SYM(msh->name));
    //dlb_vec_each(ogx_mesh *, mesh, geo->meshes) {
    //
    //}
}

static void ta_ogx_load_geometry(ogx_geometry *geo)
{
    UNUSED(geo);
    //ta_log_write(&tg_debug_log, SRC_OGX, "ta_game_alloc MODEL %s\n", geo->name);
    //ta_model *model = ta_game_alloc(RES_COMP_MODEL, SYM(geo->name));
    //dlb_vec_each(ogx_mesh *, mesh, geo->meshes) {
    //
    //}
}

void ta_ogx_load(ogx_scene *scene)
{
    dlb_vec_each(ogx_texture *, tex, scene->textures) {
        ta_ogx_load_texture(tex);
    }

    dlb_vec_each(ogx_material *, mat, scene->materials) {
        ta_ogx_load_material(mat);
    }

    dlb_vec_each(ogx_geometry *, geo, scene->geometry) {
        ta_ogx_load_geometry(geo);
    }

    //ogx_camera *cameras;
    //ogx_geometry *geometry;
    //ogx_light *lights;
}