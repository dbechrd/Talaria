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

static void ta_ogx_load_camera(ogx_camera *o_cam)
{
    UNUSED(o_cam);
}

static const char *ta_ogx_load_mesh(ogx_mesh *o_mesh)
{
    static int mesh_id = 1;
    char buf[32] = { 0 };
    size_t buf_len = snprintf(buf, sizeof(buf), "ogx_mesh_%04d", mesh_id++);
    ta_mesh *mesh = ta_game_alloc(RES_MESH, buf, buf_len);

    dlb_vec_each(ogx_vertex_array *, o_vertex_array, o_mesh->vertex_arrays) {
        // TODO: Store morph index alongside vertex array
        UNUSED(o_vertex_array->morph);

        switch (o_vertex_array->attrib) {
            case OGX_VERTEX_ATTRIB_POSIITON:
                DLB_ASSERT(sizeof(*mesh->positions) == sizeof(*o_vertex_array->values.as_vec3));
                mesh->positions = (ta_vec3 *)o_vertex_array->values.as_vec3;
                break;
            case OGX_VERTEX_ATTRIB_NORMAL:
                DLB_ASSERT(sizeof(*mesh->normals) == sizeof(*o_vertex_array->values.as_vec3));
                mesh->normals = (ta_vec3 *)o_vertex_array->values.as_vec3;
                break;
            case OGX_VERTEX_ATTRIB_TANGENT:
                DLB_ASSERT(sizeof(*mesh->tangents) == sizeof(*o_vertex_array->values.as_vec3));
                mesh->tangents = (ta_vec3 *)o_vertex_array->values.as_vec3;
                break;
            case OGX_VERTEX_ATTRIB_TEXCOORD0:
                DLB_ASSERT(sizeof(*mesh->uvs) == sizeof(*o_vertex_array->values.as_vec2));
                mesh->uvs = (ta_vec2 *)o_vertex_array->values.as_vec2;
                break;
            default:
                DLB_ASSERT(!"I wanted to know when ogx_vertex_attrib_lookup fails.");
                ta_log_write(&tg_debug_log, SRC_OGX, "ogx_vertex_attrib_lookup failed for mesh '%s', "
                    "ignoring attribute.\n", buf);
        }
    }

    dlb_vec_each(ogx_index_array *, o_index_array, o_mesh->index_arrays) {
        // TODO: Allow u16 in DML or ogx_parser
        ta_mesh_index_array *index_array = dlb_vec_alloc(mesh->index_arrays);
        size_t index_count = dlb_vec_len(o_index_array->values.as_float);
        dlb_vec_reserve(index_array->values, index_count);
        for (size_t i = 0; i < index_count; ++i) {
            dlb_vec_push(index_array->values, (u16)o_index_array->values.as_float[i]);
        }

        index_array->material_slot = (u16)o_index_array->material_slot;
    }

    // TODO: Calculate mesh AABB (or better, precalculate it and store it in the file)

    ta_mesh_create(mesh);

    if (o_mesh->skin.bone_count_array) {
        // TODO: Load skin (if present)
        UNUSED(o_mesh->skin);
        //ta_skin skin = { 0 };
    }

    return mesh->name;
}

static void ta_ogx_load_geometry(ogx_geometry *o_geo)
{
    // TODO: We shouldn't just be overwriting stuff with the same name if it exists.. remove this once we no longer have
    // 3 different loaders running in parallel, heh.
    ta_model *model = ta_game_component_try(o_geo->name, RES_COMP_MODEL);
    if (!model) {
        model = ta_game_component_add(o_geo->name, RES_COMP_MODEL, SYM(o_geo->name));
    } else {
        DLB_ASSERT(1);
    }

    dlb_vec_each(ogx_mesh *, o_mesh, o_geo->meshes) {
        const char *mesh_name = ta_ogx_load_mesh(o_mesh);
        ta_piece *piece = dlb_vec_alloc(model->pieces);
        piece->mesh = mesh_name;

        // TODO: Figure out materials..
        //piece->material = o_mesh->index_arrays[0].material_slot;
        UNUSED(piece->material);

        // TODO: Bind all needed materials at once via a UBO? Pass material indices as uniform/attrib?
        // https://www.khronos.org/opengl/wiki/Uniform_Buffer_Object
    }

    dlb_vec_each(ogx_morph *, o_morph, o_geo->morphs) {
        // TODO: Load morphs
        UNUSED(o_morph);
    }
}

static void ta_ogx_load_light(ogx_light *o_light)
{
    UNUSED(o_light);
}

static void ta_ogx_load_material(ogx_material *o_mat)
{
    ta_material *mat = ta_game_alloc(RES_MATERIAL, SYM(o_mat->name));
    mat->albedo_texture     = o_mat->albedo_texture;
    mat->albedo_factor.r    = o_mat->albedo_factor[0];
    mat->albedo_factor.g    = o_mat->albedo_factor[1];
    mat->albedo_factor.b    = o_mat->albedo_factor[2];
    mat->albedo_factor.a    = o_mat->alpha_factor;
    // NOTE: ta_material doesn't support alpha_texture, so ensure we're not discarding anything
    DLB_ASSERT(!o_mat->alpha_texture);
    mat->emission_texture   = o_mat->emissive_texture;
    mat->emission_factor.r  = o_mat->emissive_factor[0];
    mat->emission_factor.g  = o_mat->emissive_factor[1];
    mat->emission_factor.g  = o_mat->emissive_factor[2];
    mat->metallic_texture   = o_mat->metallic_texture;
    mat->metallic_factor    = o_mat->metallic_factor;
    mat->normal_texture     = o_mat->normal_texture;
    // NOTE: ta_material doesn't support normal_factor for now, so ensure we're not discarding anything
    if (o_mat->normal_texture) {
        DLB_ASSERT(o_mat->normal_factor[0] == 1.0f);
        DLB_ASSERT(o_mat->normal_factor[1] == 1.0f);
        DLB_ASSERT(o_mat->normal_factor[2] == 1.0f);
    } else {
        DLB_ASSERT(o_mat->normal_factor[0] == 0.0f);
        DLB_ASSERT(o_mat->normal_factor[1] == 0.0f);
        DLB_ASSERT(o_mat->normal_factor[2] == 0.0f);
    }
    mat->normal_texture     = o_mat->normal_texture;
    mat->roughness_texture  = o_mat->roughness_texture;
    mat->roughness_factor   = o_mat->roughness_factor;

    ta_material_init(mat);
}

static void ta_ogx_load_texture(ogx_texture *o_tex)
{
    ta_texture *tex = ta_game_by_sym_try(RES_TEXTURE, o_tex->name);
    if (tex) {
        return;
    }

    char filepath[1024] = { 0 };
    snprintf(filepath, sizeof(filepath) - 1, "data/mesh/%s", o_tex->path);

    ta_log_write(&tg_debug_log, SRC_OGX, "stbi_load_from_memory: %s\n", o_tex->name);
    stbi_set_flip_vertically_on_load(true);
    int w = 0;
    int h = 0;
    int channels = 0;  // components/channels
    u8 *pixels = stbi_load(filepath, &w, &h, &channels, 0);
    //u8 *pixels = stbi_load_from_memory(buffer, (int)buffer_len, &w, &h, &channels, 0);
    if (!pixels) {
        const char *reason = stbi_failure_reason();
        ta_log_write(&tg_debug_log, SRC_OGX, "Failed to load tex: %s\nSTBI Reason: %s\n", o_tex->name, reason);
        DLB_ASSERT(!"ta_texture_init: Failed to load tex");
    }

    DLB_ASSERT(w);
    DLB_ASSERT(h);
    DLB_ASSERT(channels);

    tex = ta_game_alloc(RES_TEXTURE, SYM(o_tex->name));
    tex->type = TA_TEXTURE_2D_ARRAY;
    tex->width = w;
    tex->height = h;
    tex->channels = (u8)channels;
    tex->pixels = pixels;

    ta_texture_init(tex);

    tex->pixels = 0; // HACK: Don't want ta_texture_free to try to call dlb_vec_free() on this buffer
    stbi_image_free(pixels);
}

static void ta_ogx_load_bone_node(ogx_node *o_node)
{
    UNUSED(o_node);
}

static void ta_ogx_load_camera_node(ogx_node *o_node)
{
    UNUSED(o_node);
}

static void ta_ogx_load_geometry_node(ogx_node *o_node)
{
    UNUSED(o_node);
}

static void ta_ogx_load_light_node(ogx_node *o_node)
{
    UNUSED(o_node);
}

static void ta_ogx_load_node(ogx_node *o_node)
{
    // TODO: Load node
    UNUSED(o_node->type);
    //o_node->

    //ta_transform *transform = ta_game_alloc(RES_COMP_TRANSFORM, SYM(o_node->name));

    // HACK: Let OGX transform override DML transform
    ta_transform *transform = ta_game_by_sym_try(RES_COMP_TRANSFORM, o_node->name);
    if (!transform) {
        transform = ta_game_component_add(o_node->name, RES_COMP_TRANSFORM, SYM(o_node->name));
        ta_transform_init(transform);
    }

    DLB_ASSERT(sizeof(ta_vec3) == sizeof(ogx_vec3));
    DLB_ASSERT(sizeof(ta_vec4) == sizeof(ogx_vec4));
    ta_vec3 ogx_pos = o_node->transform.position;
    ta_vec4 ogx_rot = o_node->transform.orientation;
    transform->xform.position = vec3_add(transform->xform.position, ogx_pos);
    transform->xform.orientation = quat_normalize(quat_mul(transform->xform.orientation, ogx_rot));

    //ogx_transform transform;
    //ogx_animation *animations;
    //struct ogx_node *parent;    // TODO: Use index (or name), not pointer
    //struct ogx_node *children;  // TODO: Use index (or name), not pointer

    switch (o_node->type) {
        case OGX_BONE_NODE:
            ta_ogx_load_bone_node(o_node);
            break;
        case OGX_CAMERA_NODE:
            ta_ogx_load_camera_node(o_node);
            break;
        case OGX_GEOMETRY_NODE:
            ta_ogx_load_geometry_node(o_node);
            break;
        case OGX_LIGHT_NODE:
            ta_ogx_load_light_node(o_node);
            break;
        default:
            break;
    }
}

void ta_ogx_load(ogx_scene *scene)
{
    dlb_vec_each(ogx_camera *, o_camera, scene->cameras) {
        ta_ogx_load_camera(o_camera);
    }

    dlb_vec_each(ogx_geometry *, o_geo, scene->geometry) {
        ta_ogx_load_geometry(o_geo);
    }

    dlb_vec_each(ogx_light *, o_light, scene->lights) {
        ta_ogx_load_light(o_light);
    }

    dlb_vec_each(ogx_material *, o_mat, scene->materials) {
        ta_ogx_load_material(o_mat);
    }

    dlb_vec_each(ogx_texture *, o_tex, scene->textures) {
        ta_ogx_load_texture(o_tex);
    }

    dlb_vec_each(ogx_node *, o_node, scene->nodes) {
        ta_ogx_load_node(o_node);
    }
}