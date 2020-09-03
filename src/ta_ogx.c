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
    ta_mesh *mesh = ta_game_alloc(RES_MESH, SYM(o_mesh->name));

    dlb_vec_each(ogx_morph_target *, o_morph_target, o_mesh->morph_targets) {
        // TODO: Load morph targets
        UNUSED(o_morph_target);
    }

    dlb_vec_each(ogx_vertex_array *, o_vertex_array, o_mesh->vertex_arrays) {
        // TODO: Store morph index alongside vertex array
        UNUSED(o_vertex_array->morph_index);

        switch (o_vertex_array->attrib) {
            case OGX_VERTEX_ATTRIB_TEXCOORD0:
                DLB_ASSERT(sizeof(*mesh->uvs) == sizeof(*o_vertex_array->values.as_vec2));
                mesh->uvs = (ta_vec2 *)o_vertex_array->values.as_vec2;
                break;
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
            default:
                DLB_ASSERT(!"I wanted to know when ogx_vertex_attrib_lookup fails.");
                ta_log_write(&tg_debug_log, SRC_OGX, "ogx_vertex_attrib_lookup failed for mesh '%s', "
                    "ignoring attribute.\n", mesh->name);
        }
    }


    dlb_vec_each(ogx_index_array *, o_index_array, o_mesh->index_arrays) {
        const size_t blah = dlb_vec_len(o_index_array->values);
        ta_index_array *index_array = dlb_vec_alloc(mesh->index_arrays);
        index_array->material_slot = o_index_array->material_slot;
        index_array->values = o_index_array->values;
    }

    // TODO: Calculate mesh AABB (or better, precalculate it and store it in the file)

    ta_mesh_create(mesh);
    ta_mesh_update_buffers(mesh);

    // Load skin (if present)
    if (o_mesh->skin.bone_count_array) {
        mesh->skin.transform = o_mesh->skin.transform;
        mesh->skin.bone_count_array = o_mesh->skin.bone_count_array;
        mesh->skin.bone_index_array = o_mesh->skin.bone_index_array;
        mesh->skin.bone_weight_array = o_mesh->skin.bone_weight_array;
        mesh->skin.skeleton.bones = o_mesh->skin.skeleton.bones;
        mesh->skin.skeleton.bind_pose_positions = (ta_vec3 *)o_mesh->skin.skeleton.bind_pose_positions;
        mesh->skin.skeleton.bind_pose_orientations = (ta_vec4 *)o_mesh->skin.skeleton.bind_pose_orientations;
    }

    return mesh->name;
}

static void ta_ogx_load_light(ogx_light *o_light)
{
    UNUSED(o_light);
}

static void ta_ogx_load_material(ogx_material *o_mat)
{
    ta_material *mat = ta_game_alloc(RES_MATERIAL, SYM(o_mat->name));
    mat->albedo_texture     = o_mat->albedo_texture;
    mat->albedo_factor.r    = o_mat->albedo_factor.x;
    mat->albedo_factor.g    = o_mat->albedo_factor.y;
    mat->albedo_factor.b    = o_mat->albedo_factor.z;
    mat->albedo_factor.a    = o_mat->alpha_factor;
    // NOTE: ta_material doesn't support alpha_texture, so ensure we're not discarding anything
    DLB_ASSERT(!o_mat->alpha_texture);
    mat->emission_texture   = o_mat->emissive_texture;
    mat->emission_factor.r  = o_mat->emissive_factor.x;
    mat->emission_factor.g  = o_mat->emissive_factor.y;
    mat->emission_factor.g  = o_mat->emissive_factor.z;
    mat->metallic_texture   = o_mat->metallic_texture;
    mat->metallic_factor    = o_mat->metallic_factor;
    mat->normal_texture     = o_mat->normal_texture;
    // NOTE: ta_material doesn't support normal_factor for now, so ensure we're not discarding anything
    if (o_mat->normal_texture) {
        DLB_ASSERT(o_mat->normal_factor.x == 1.0f);
        DLB_ASSERT(o_mat->normal_factor.y == 1.0f);
        DLB_ASSERT(o_mat->normal_factor.z == 1.0f);
    } else {
        DLB_ASSERT(o_mat->normal_factor.x == 0.0f);
        DLB_ASSERT(o_mat->normal_factor.y == 0.0f);
        DLB_ASSERT(o_mat->normal_factor.z == 0.0f);
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
    // TODO: Add RES_COMP_BONE with same name as transform
    // These components will then be searchable via the names skin->skeleton->bones
    UNUSED(o_node);
}

static void ta_ogx_load_camera_node(ogx_node *o_node)
{
    UNUSED(o_node);
}

static void ta_ogx_load_geometry_node(ogx_node *o_node)
{
    ogx_geometry_node *o_geom = &o_node->properties.geometry;

    // HACK: Let OGX geometry node override DML model
    ta_model *model = ta_game_by_sym_try(RES_COMP_MODEL, o_node->name);
    if (!model) {
        model = ta_game_component_add(o_node->name, RES_COMP_MODEL, SYM(o_node->name));
    } else {
        ta_log_write(&tg_debug_log, SRC_OGX, "WARNING: Overwriting model for %s\n", o_node->name);
    }

    model->mesh = o_geom->mesh;
    dlb_vec_each(const char **, material, o_geom->materials) {
        dlb_vec_push(model->materials, *material);
    }
    dlb_vec_each(float *, weight, o_geom->morph_weights) {
        dlb_vec_push(model->morph_target_weights, *weight);
    }
}

static void ta_ogx_load_light_node(ogx_node *o_node)
{
    UNUSED(o_node);
}

static void ta_ogx_load_node(ogx_node *o_node, ogx_scene *o_scene)
{
    // HACK: Let OGX transform override DML transform
    ta_transform *transform = ta_game_by_sym_try(RES_COMP_TRANSFORM, o_node->name);
    if (!transform) {
        transform = ta_game_component_add(o_node->name, RES_COMP_TRANSFORM, SYM(o_node->name));
        ta_transform_init(transform);
    } else {
        ta_log_write(&tg_debug_log, SRC_OGX, "WARNING: Overwriting transform for %s\n", o_node->name);
    }
    transform->xform.position = o_node->transform.position;
    transform->xform.orientation = quat_normalize(o_node->transform.orientation);

    if (o_node->parent != OGX_INDEX_NULL) {
        transform->parent = o_scene->nodes[o_node->parent].name;
    }
    size_t children_count = dlb_vec_len(o_node->children);
    if (children_count) {
        dlb_vec_reserve(transform->children, children_count);
        dlb_vec_each(s32 *, node_idx, o_node->children) {
            dlb_vec_push(transform->children, o_scene->nodes[*node_idx].name);
        }
    }

    // TODO: Load animation data
    UNUSED(o_node->animations);

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

    dlb_vec_each(ogx_mesh *, o_mesh, scene->meshes) {
        ta_ogx_load_mesh(o_mesh);
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
        ta_ogx_load_node(o_node, scene);
    }
}