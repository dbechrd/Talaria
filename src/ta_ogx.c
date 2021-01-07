#include "ta_ogx.h"

static void ta_ogx_load_camera(ogx_camera *o_cam)
{
    UNUSED(o_cam);
}

static const char *ta_ogx_load_mesh(ogx_mesh *o_mesh)
{
    // HACK: Let OGX node override DML node
    // TODO: This is potentially a memory leak, need to unify DML and OGEX to prevent resource stomping
    ta_mesh *mesh = (ta_mesh *)ta_game_by_sym_try(RES_MESH, o_mesh->name);
    if (!mesh) {
        mesh = ta_game_alloc(RES_MESH, SYM(o_mesh->name));
    } else {
        ta_log_write(&tg_debug_log, SRC_OGX, "WARNING: Overwriting mesh for %s\n", o_mesh->name);
    }

    mesh->mode = TA_PRIMITIVE_MODE_TRIANGLES;

    dlb_vec_each(ogx_morph_target *, o_morph_target, o_mesh->morph_targets) {
        // TODO: Load morph targets
        UNUSED(o_morph_target);
    }

    dlb_vec_each(ogx_vertex_array *, o_vertex_array, o_mesh->vertex_arrays) {
        // TODO: Store morph index alongside vertex array
        UNUSED(o_vertex_array->morph_index);

        switch (o_vertex_array->attrib_type) {
            case TA_VERTEX_ATTR_UV:
                DLB_ASSERT(sizeof(*mesh->uvs) == sizeof(*o_vertex_array->values.as_vec2));
                mesh->uvs = (ta_vec2 *)o_vertex_array->values.as_vec2;
                break;
            case TA_VERTEX_ATTR_POSITION:
                DLB_ASSERT(sizeof(*mesh->positions) == sizeof(*o_vertex_array->values.as_vec3));
                mesh->positions = (ta_vec3 *)o_vertex_array->values.as_vec3;
                break;
            case TA_VERTEX_ATTR_NORMAL:
                DLB_ASSERT(sizeof(*mesh->normals) == sizeof(*o_vertex_array->values.as_vec3));
                mesh->normals = (ta_vec3 *)o_vertex_array->values.as_vec3;
                break;
            case TA_VERTEX_ATTR_TANGENT:
                DLB_ASSERT(sizeof(*mesh->tangents) == sizeof(*o_vertex_array->values.as_vec3));
                mesh->tangents = (ta_vec3 *)o_vertex_array->values.as_vec3;
                break;
            case TA_VERTEX_ATTR_MORPH1_POSITION:
                DLB_ASSERT(sizeof(*mesh->morph1_positions) == sizeof(*o_vertex_array->values.as_vec3));
                mesh->morph1_positions = (ta_vec3 *)o_vertex_array->values.as_vec3;
                break;
            case TA_VERTEX_ATTR_MORPH1_NORMAL:
                DLB_ASSERT(sizeof(*mesh->morph1_normals) == sizeof(*o_vertex_array->values.as_vec3));
                mesh->morph1_normals = (ta_vec3 *)o_vertex_array->values.as_vec3;
                break;
            case TA_VERTEX_ATTR_MORPH1_TANGENT:
                DLB_ASSERT(sizeof(*mesh->morph1_tangents) == sizeof(*o_vertex_array->values.as_vec3));
                mesh->morph1_tangents = (ta_vec3 *)o_vertex_array->values.as_vec3;
                break;
            default:
                DLB_ASSERT(!"I wanted to know when ogx_vertex_attrib_lookup fails.");
                ta_log_write(&tg_debug_log, SRC_OGX, "ogx_vertex_attrib_lookup failed for mesh '%s', "
                    "ignoring attribute.\n", mesh->name);
        }
    }

    dlb_vec_each(ogx_index_array *, o_index_array, o_mesh->index_arrays) {
        ta_index_array *index_array = dlb_vec_alloc(mesh->index_arrays);
        index_array->mode = TA_PRIMITIVE_MODE_TRIANGLES;
        index_array->material_slot = o_index_array->material_slot;
        index_array->values = o_index_array->values;
    }


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

    ta_mesh_create(mesh);
    ta_mesh_calculate_joints_and_weights(mesh);
    ta_mesh_update_buffers(mesh);
    ta_mesh_update_debug_lines(mesh, 0.1f);

    // TODO: Calculate mesh AABB (or better, precalculate it and store it in the file)

    return mesh->name;
}

static void ta_ogx_load_light(ogx_light *o_light)
{
    UNUSED(o_light);
}

static void ta_ogx_load_material(ogx_material *o_mat)
{
    // HACK: Let OGX node override DML node
    // TODO: This is potentially a memory leak, need to unify DML and OGEX to prevent resource stomping
    ta_material *mat = (ta_material *)ta_game_by_sym_try(RES_MATERIAL, o_mat->name);
    if (!mat) {
        mat = ta_game_alloc(RES_MATERIAL, SYM(o_mat->name));
    } else {
        ta_log_write(&tg_debug_log, SRC_OGX, "WARNING: Overwriting material for %s\n", o_mat->name);
    }

    mat->albedo_texture     = o_mat->albedo_texture;
    mat->albedo_factor.r    = o_mat->albedo_factor.x;
    mat->albedo_factor.g    = o_mat->albedo_factor.y;
    mat->albedo_factor.b    = o_mat->albedo_factor.z;
    mat->albedo_factor.a    = o_mat->alpha_factor;
    // NOTE: ta_material doesn't support alpha_texture, so ensure we're not discarding anything
    //DLB_ASSERT(!o_mat->alpha_texture);
    mat->emission_texture   = o_mat->emissive_texture;
    mat->emission_factor.r  = o_mat->emissive_factor.x;
    mat->emission_factor.g  = o_mat->emissive_factor.y;
    mat->emission_factor.g  = o_mat->emissive_factor.z;
    mat->metallic_texture   = o_mat->metallic_texture;
    mat->metallic_factor    = o_mat->metallic_factor;
    mat->normal_texture     = o_mat->normal_texture;
    // NOTE: ta_material doesn't support normal_factor for now, so ensure we're not discarding anything
    //if (o_mat->normal_texture) {
    //    DLB_ASSERT(o_mat->normal_factor.x == 1.0f);
    //    DLB_ASSERT(o_mat->normal_factor.y == 1.0f);
    //    DLB_ASSERT(o_mat->normal_factor.z == 1.0f);
    //} else {
    //    DLB_ASSERT(o_mat->normal_factor.x == 0.0f);
    //    DLB_ASSERT(o_mat->normal_factor.y == 0.0f);
    //    DLB_ASSERT(o_mat->normal_factor.z == 0.0f);
    //}
    mat->normal_texture     = o_mat->normal_texture;
    mat->roughness_texture  = o_mat->roughness_texture;
    mat->roughness_factor   = o_mat->roughness_factor;

    ta_material_init(mat);
}

static void ta_ogx_load_texture(ogx_texture *o_tex)
{
    ta_texture *tex = (ta_texture *)ta_game_by_sym_try(RES_TEXTURE, o_tex->name);
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

    // HACK: sponza has 16-bit alpha textures according to stb_image; reload them padded to 32-bits
    if (channels == 2) {
        stbi_image_free(pixels);
        pixels = stbi_load(filepath, &w, &h, &channels, 4);
        channels = 4;
    }

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

static void ta_ogx_load_bone_node(ogx_node *o_node, ogx_scene *o_scene)
{
    ogx_bone_node *o_bone = &o_node->properties.bone;
    UNUSED(o_bone);

    // HACK: Let OGX node override DML node
    // TODO: This is potentially a memory leak, need to unify DML and OGEX to prevent resource stomping
    ta_bone *bone = (ta_bone *)ta_game_by_sym_try(RES_COMP_BONE, o_node->name);
    if (!bone) {
        bone = ta_game_component_add(o_node->name, RES_COMP_BONE, SYM(o_node->name));
    } else {
        ta_log_write(&tg_debug_log, SRC_OGX, "WARNING: Overwriting bone for %s\n", o_node->name);
    }

    // Find the bone's armature (first parent that isn't a bone)
    ogx_node *armature = &o_scene->nodes[o_node->parent];
    while (armature && armature->type == OGX_BONE_NODE) {
        armature = &o_scene->nodes[armature->parent];
    }

    DLB_ASSERT(armature);
    DLB_ASSERT(armature->type == OGX_BASIC_NODE);
    bone->armature = armature->name;
}

static void ta_ogx_load_camera_node(ogx_node *o_node)
{
    UNUSED(o_node);
}

static void ta_ogx_load_geometry_node(ogx_node *o_node)
{
    ogx_geometry_node *o_geom = &o_node->properties.geometry;

    // HACK: Let OGX node override DML node
    // TODO: This is potentially a memory leak, need to unify DML and OGEX to prevent resource stomping
    ta_model *model = (ta_model *)ta_game_by_sym_try(RES_COMP_MODEL, o_node->name);
    if (!model) {
        model = ta_game_component_add(o_node->name, RES_COMP_MODEL, SYM(o_node->name));
        ta_model_init(model);
    } else {
        ta_log_write(&tg_debug_log, SRC_OGX, "WARNING: Overwriting model for %s\n", o_node->name);
        dlb_vec_zero((void *)model->materials);
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
    ta_transform *transform = (ta_transform *)ta_game_by_sym_try(RES_COMP_TRANSFORM, o_node->name);
    if (!transform) {
        transform = ta_game_component_add(o_node->name, RES_COMP_TRANSFORM, SYM(o_node->name));
        ta_transform_init(transform);
        transform->xform.position = o_node->transform.position;
        transform->xform.orientation = quat_normalize(o_node->transform.orientation);
    } else {
        if (!vec3_equal(o_node->transform.position, transform->xform.position)) {
            ta_log_write(&tg_debug_log, SRC_OGX, "WARNING: Transform already exists; ignoring non-zero position for %s\n", o_node->name);
        }
        if (!quat_equal(o_node->transform.orientation, transform->xform.orientation)) {
            ta_log_write(&tg_debug_log, SRC_OGX, "WARNING: Transform already exists; ignoring non-identify orientation for %s\n", o_node->name);
        }
    }

    if (o_node->parent != OGX_INDEX_NULL) {
        transform->parent = o_scene->nodes[o_node->parent].name;
    }
    size_t children_count = dlb_vec_len(o_node->children);
    if (children_count) {
        dlb_vec_zero((void *)transform->children);
        dlb_vec_reserve(transform->children, children_count);
        dlb_vec_each(s32 *, node_idx, o_node->children) {
            dlb_vec_push(transform->children, o_scene->nodes[*node_idx].name);
        }
    }

    //// Load animation data
    //dlb_vec_each(ta_animation *, o_animation, o_node->animations) {
    //    // TODO: Handle non-sampled animations? I think that's the only reason these properties exist.
    //    DLB_ASSERT(!o_animation->begin);
    //    DLB_ASSERT(!o_animation->end);
    //
    //    ta_game_alloc(RES_ANIMATION, SYM(o_animation->clip));
    //    o_animation->tracks
    //}

    switch (o_node->type) {
        case OGX_BONE_NODE:
            ta_ogx_load_bone_node(o_node, o_scene);
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
    TracyCZone(ctxMethod, true);

    TracyCZoneN(ctxCameras, "load cameras", true);
    dlb_vec_each(ogx_camera *, o_camera, scene->cameras) {
        ta_ogx_load_camera(o_camera);
    }
    TracyCZoneEnd(ctxCameras);

    TracyCZoneN(ctxMeshes, "load meshes", true);
    dlb_vec_each(ogx_mesh *, o_mesh, scene->meshes) {
        ta_ogx_load_mesh(o_mesh);
    }
    TracyCZoneEnd(ctxMeshes);

    TracyCZoneN(ctxLights, "load lights", true);
    dlb_vec_each(ogx_light *, o_light, scene->lights) {
        ta_ogx_load_light(o_light);
    }
    TracyCZoneEnd(ctxLights);

    TracyCZoneN(ctxMaterials, "load materials", true);
    dlb_vec_each(ogx_material *, o_mat, scene->materials) {
        ta_ogx_load_material(o_mat);
    }
    TracyCZoneEnd(ctxMaterials);

    TracyCZoneN(ctxTextures, "load textures", true);
    dlb_vec_each(ogx_texture *, o_tex, scene->textures) {
        ta_ogx_load_texture(o_tex);
    }
    TracyCZoneEnd(ctxTextures);

    TracyCZoneN(ctxNodes, "load nodes", true);
    dlb_vec_each(ogx_node *, o_node, scene->nodes) {
        ta_ogx_load_node(o_node, scene);
    }
    TracyCZoneEnd(ctxNodes);

    TracyCZoneEnd(ctxMethod);
}