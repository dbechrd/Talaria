#include "ta_gltf.h"
#include "ta_game.h"
#include "ta_json.h"
#include "ta_log.h"
#include "ta_mesh.h"
#include "ta_schema.h"
#include "ta_symbol.h"
#include "dlb/dlb_types.h"
#include "dlb/dlb_vector.h"
#include "misc/cgltf.h"
#include <stdio.h>
#include <math.h>

//#define gltf_each(t, i, s) for (t (i) = (s), *(i##e) = (s + s##_count); (i) != (i##e); (i)++)

void gltf_dump(cgltf_data *data)
{
#define PRNTID(label, vec, ptr) printf(label"[%zu @ %p]\n", (ptr) - (vec), (ptr))
    printf("Type: %s\n", data->file_type == cgltf_file_type_gltf ? "GLTF" : "GLB");

    printf("json:\n");
    printf("  size: %zu bytes\n", data->json_size);
    printf("  data: <json blob>\n");
    //printf("  data: %.*s\n", data->json_size, data->json);

    printf("bin:\n");
    printf("  size: %zu bytes\n", data->bin_size);
    printf("  data: <binary blob>\n");

    printf("asset:\n");
    if (data->asset.copyright)   printf("  copyright: %s\n",   data->asset.copyright);
    if (data->asset.generator)   printf("  generator: %s\n",   data->asset.generator);
    if (data->asset.version)     printf("  version: %s\n",     data->asset.version);
    if (data->asset.min_version) printf("  min_version: %s\n", data->asset.min_version);

    if (data->extensions_used_v) {
        printf("extensions_used:\n");
        dlb_vec_each(char **, extension, data->extensions_used_v) {
            printf("  %s\n", *extension);
        }
    }
    if (data->extensions_required_v) {
        printf("extensions_required:\n");
        dlb_vec_each(const char **, extension, data->extensions_required_v) {
            printf("  %s\n", *extension);
        }
    }

    PRNTID("scene: ", data->scenes_v, data->scene);
    if (data->scenes_v) {
        printf("scenes:\n");
        dlb_vec_each(cgltf_scene *, scene, data->scenes_v) {
            printf("  %s", scene->name);
            PRNTID(" ", data->scenes_v, scene);
            printf("    nodes:\n");
            dlb_vec_each(cgltf_node **, node_p, scene->nodes_v) {
                cgltf_node *node = *node_p;
                printf("      %s", node->name);
                PRNTID(" ", *scene->nodes_v, node);
                //if (node->parent) PRNTID("        parent: ", *scene->nodes_v, node->parent);
                //if (node->children_v) {
                //    printf("        children\n");
                //    dlb_vec_each(cgltf_node **, child_p, node->children_v) {
                //        cgltf_node *child = *child_p;
                //        PRNTID("          child: ", *scene->nodes_v, child);
                //    }
                //}
                //if (node->skin)   PRNTID("        skin: ", data->skins_v, node->skin);
                //if (node->mesh)   PRNTID("        mesh: ", data->meshes_v, node->mesh);
                //if (node->camera) PRNTID("        camera: ", data->cameras_v, node->camera);
                //if (node->light)  PRNTID("        light: ", data->lights_v, node->light);
                //
                //cgltf_float* weights_v;
                //cgltf_bool has_translation;
                //cgltf_bool has_rotation;
                //cgltf_bool has_scale;
                //cgltf_bool has_matrix;
                //cgltf_float translation[3];
                //cgltf_float rotation[4];
                //cgltf_float scale[3];
                //cgltf_float matrix[16];
                //cgltf_extras extras;

            }
        }
    }

    printf("accessors: %zu\n", dlb_vec_len(data->accessors_v));
    dlb_vec_each(cgltf_accessor *, accessor, data->accessors_v) {
        printf("  type: %d\n", accessor->type);
        //printf("  type: %d\n", accessor->buffer_view.);
    }
    printf("animations: %zu\n", dlb_vec_len(data->animations_v));
    dlb_vec_each(cgltf_animation *, animation, data->animations_v) {
        printf("  name: %s\n", animation->name);
        dlb_vec_each(cgltf_animation_channel *, channel, animation->channels_v) {
            printf("    sampler\n");
            printf("      interpolation: %d\n", channel->sampler->interpolation);
            printf("    target node: %s\n", channel->target_node->name);
            printf("    target path: %d\n", channel->target_path);
        }
    }
    printf("buffer_views: %zu\n", dlb_vec_len(data->buffer_views_v));
    dlb_vec_each(cgltf_buffer_view *, buffer_view, data->buffer_views_v) {
        printf("  type: %d\n", buffer_view->type);
    }
    printf("buffers: %zu\n", dlb_vec_len(data->buffers_v));
    dlb_vec_each(cgltf_buffer *, buffer, data->buffers_v) {
        printf("  size: %zu\n", buffer->size);
    }
    printf("cameras: %zu\n", dlb_vec_len(data->cameras_v));
    dlb_vec_each(cgltf_camera *, camera, data->cameras_v) {
        printf("  name: %s\n", camera->name);
    }
    printf("images: %zu\n", dlb_vec_len(data->images_v));
    dlb_vec_each(cgltf_image *, image, data->images_v) {
        printf("  name: %s\n", image->name);
        printf("  mime_type: %s\n", image->mime_type);
        printf("  uri: %s\n", image->uri);
        printf("  buffer_view type: %d\n", image->buffer_view->type);
    }
    printf("lights: %zu\n", dlb_vec_len(data->lights_v));
    dlb_vec_each(cgltf_light *, light, data->lights_v) {
        printf("  name: %s\n", light->name);
    }
    printf("materials: %zu\n", dlb_vec_len(data->materials_v));
    dlb_vec_each(cgltf_material *, material, data->materials_v) {
        printf("  name: %s\n", material->name);
    }
    printf("meshes: %zu\n", dlb_vec_len(data->meshes_v));
    dlb_vec_each(cgltf_mesh *, mesh, data->meshes_v) {
        printf("  name: %s\n", mesh->name);
        printf("  primitives count: %zu\n",   dlb_vec_len(mesh->primitives_v));
        printf("  target names count: %zu\n", dlb_vec_len(mesh->target_names_v));
        printf("  weights count: %zu\n",      dlb_vec_len(mesh->weights_v));
        if (mesh->extras.start_offset) {
            const char *extras = data->json + mesh->extras.start_offset;
            size_t extras_len = mesh->extras.end_offset - mesh->extras.start_offset;

            printf("  extras: (%zu bytes)\n", extras_len);
            printf("    json: %*.s\n", (int)extras_len, extras);
            printf("    dump:\n");

            jsmntok_t *tokens = 0;
            ta_json_parse(extras, extras_len, &tokens);
            DLB_ASSERT(tokens);
            ta_json_dump(extras, tokens, dlb_vec_len(tokens), 3);
            dlb_vec_free(tokens);
        }
    }
    printf("nodes: %zu\n", dlb_vec_len(data->nodes_v));
    dlb_vec_each(cgltf_node *, node, data->nodes_v) {
        printf("  name: %s\n", node->name);
        if (node->camera) {
            printf("    camera: %s\n", node->camera->name);
        }
        if (node->light) {
            printf("    light: %s\n", node->light->name);
        }
        if (node->mesh) {
            printf("    mesh: %s\n", node->mesh->name);
        }
        if (node->skin) {
            printf("    skin: %s\n", node->skin->name);
        }
        if (node->children_v) {
            printf("    children count: %zu\n", dlb_vec_len(node->children_v));
        }
    }
    printf("samplers: %zu\n", dlb_vec_len(data->samplers_v));
    dlb_vec_each(cgltf_sampler *, sampler, data->samplers_v) {
        printf("  min_filter: %d\n", sampler->min_filter);
    }

    printf("skins: %zu\n", dlb_vec_len(data->skins_v));
    dlb_vec_each(cgltf_skin *, skin, data->skins_v) {
        printf("  name: %s\n", skin->name);
    }
    printf("textures: %zu\n", dlb_vec_len(data->textures_v));
    dlb_vec_each(cgltf_texture *, texture, data->textures_v) {
        printf("  name: %s\n", texture->name);
    }

    //cgltf_scene* scene;
    //cgltf_extras extras;

    printf("\n");
#undef VEC_IDX
}

static void* ta_cgltf_alloc(void* user, cgltf_size size)
{
    (void)user;
    void *ptr = 0;
    dlb_vec_alloc_count_size(ptr, 1, size);
    return ptr;
}

static void* ta_cgltf_calloc(void* user, cgltf_size count, size_t size)
{
    (void)user;
    void *ptr = 0;
    // NOTE: Assuming cgltf always fills up the buffers it requests
    dlb_vec_alloc_count_size(ptr, count, size);
    return ptr;
}

static void ta_cgltf_free(void* user, void* ptr)
{
    (void)user;
    dlb_vec_free(ptr);
}

cgltf_result ta_gltf_parse_file(ta_gltf *gltf, const char *filename)
{
    static const char *ta_cgltf_error_str[] = {
        [cgltf_result_success] = "cgltf_result_success",
        [cgltf_result_data_too_short] = "cgltf_result_data_too_short",
        [cgltf_result_unknown_format] = "cgltf_result_unknown_format",
        [cgltf_result_invalid_json] = "cgltf_result_invalid_json",
        [cgltf_result_invalid_gltf] = "cgltf_result_invalid_gltf",
        [cgltf_result_invalid_options] = "cgltf_result_invalid_options",
        [cgltf_result_file_not_found] = "cgltf_result_file_not_found",
        [cgltf_result_io_error] = "cgltf_result_io_error",
        [cgltf_result_out_of_memory] = "cgltf_result_out_of_memory",
        [cgltf_result_legacy_gltf] = "cgltf_result_legacy_gltf",
    };

    cgltf_options options = { 0 };
    options.memory_alloc = &ta_cgltf_alloc;
    options.memory_calloc = &ta_cgltf_calloc;
    options.memory_free = &ta_cgltf_free;
    cgltf_result err;

    ta_log_write(&tg_debug_log, SRC_GLTF, "parsing %s\n", filename);
    err = cgltf_parse_file(&options, filename, &gltf->data);
    if (err) {
        printf("cgltf_parse_file error: %s", ta_cgltf_error_str[err]);
        return err;
    }

    ta_log_write(&tg_debug_log, SRC_GLTF, "loading buffers\n");
    err = cgltf_load_buffers(&options, gltf->data, filename);
    if (err) {
        printf("cgltf_load_buffers error: %s", ta_cgltf_error_str[err]);
        return err;
    }

    ta_log_write(&tg_debug_log, SRC_GLTF, "validating\n");
    err = cgltf_validate(gltf->data);
    if (err) {
        printf("cgltf_validate error: %s", ta_cgltf_error_str[err]);
        return err;
    }

    ta_log_write(&tg_debug_log, SRC_GLTF, "successfully loaded %s\n", filename);
    gltf_dump(gltf->data);

    return err;
}

void gltf_mesh_accessor(ta_mesh *mesh, cgltf_accessor *accessor, cgltf_attribute_type attr_type)
{
    static const ta_mesh_buffer_type mesh_buffer_idx[] = {
        [cgltf_attribute_type_position] = TA_MESH_BUFFER_POSITION,
        [cgltf_attribute_type_normal  ] = TA_MESH_BUFFER_NORMAL,
        [cgltf_attribute_type_tangent ] = TA_MESH_BUFFER_TANGENT,
        [cgltf_attribute_type_texcoord] = TA_MESH_BUFFER_UV,
        [cgltf_attribute_type_color   ] = TA_MESH_BUFFER_COLOR,
        [cgltf_attribute_type_joints  ] = TA_MESH_BUFFER_JOINTS,
        [cgltf_attribute_type_weights ] = TA_MESH_BUFFER_WEIGHTS,
    };

    cgltf_size data_size = accessor->buffer_view->size;
    void *data = (char *)accessor->buffer_view->buffer->data + accessor->buffer_view->offset;

    // Ensure buffers have been loaded
    DLB_ASSERT(data_size);
    DLB_ASSERT(data);

    // TODO: Return error and load placeholder asset gracefully
    DLB_ASSERT(accessor->type != cgltf_attribute_type_invalid);

    // NOTE: We don't support interleaved vertex attributes for now
    DLB_ASSERT(accessor->buffer_view->stride == 0);

    // TODO: Make a size lookup table for each TA_MESH_BUFFER type and use dlb_vec_reserve_size()
    switch (attr_type) {
        case cgltf_attribute_type_position: {
            DLB_ASSERT(accessor->type == cgltf_type_vec3);
            DLB_ASSERT(data_size == sizeof(*mesh->positions) * accessor->count);
            dlb_vec_reserve(mesh->positions, accessor->count);
            break;
        } case cgltf_attribute_type_normal: {
            DLB_ASSERT(accessor->type == cgltf_type_vec3);
            DLB_ASSERT(data_size == sizeof(*mesh->normals) * accessor->count);
            dlb_vec_reserve(mesh->normals, accessor->count);
            break;
        } case cgltf_attribute_type_tangent: {
            DLB_ASSERT(accessor->type == cgltf_type_vec4);
            DLB_ASSERT(data_size == sizeof(*mesh->tangents) * accessor->count);
            dlb_vec_reserve(mesh->tangents, accessor->count);
            break;
        } case cgltf_attribute_type_texcoord: {
            DLB_ASSERT(accessor->type == cgltf_type_vec2);
            DLB_ASSERT(data_size == sizeof(*mesh->uvs) * accessor->count);
            dlb_vec_reserve(mesh->uvs, accessor->count);
            break;
        } case cgltf_attribute_type_color: {
            DLB_ASSERT(accessor->type == cgltf_type_vec4);
            DLB_ASSERT(data_size == sizeof(*mesh->colors) * accessor->count);
            dlb_vec_reserve(mesh->colors, accessor->count);
            break;
        } case cgltf_attribute_type_joints: {
            DLB_ASSERT(accessor->type == cgltf_type_vec4);
            DLB_ASSERT(data_size == sizeof(*mesh->joints) * accessor->count);
            dlb_vec_reserve(mesh->joints, accessor->count);
            break;
        } case cgltf_attribute_type_weights: {
            DLB_ASSERT(accessor->type == cgltf_type_vec4);
            DLB_ASSERT(data_size == sizeof(*mesh->weights) * accessor->count);
            dlb_vec_reserve(mesh->weights, accessor->count);
            break;
        } default: {
            DLB_ASSERT(!"Unexpected gltf attribute type");
            break;
        }
    }

    ta_mesh_buffer_type mesh_buffer_type = mesh_buffer_idx[attr_type];
    void *buffer = mesh->buffers[mesh_buffer_type];
    dlb_vec_hdr(buffer)->len = accessor->count;
    dlb_memcpy(buffer, data, data_size);
}

void ta_gltf_load(ta_gltf *gltf)
{
    dlb_vec_each(cgltf_mesh *, gltf_mesh, gltf->data->meshes_v) {
        const char *name = gltf_mesh->name;
        ta_mesh *mesh = ta_game_alloc(RES_MESH, name, strlen(name));

        dlb_vec_each(cgltf_attribute *, attr, gltf_mesh->primitives_v->attributes_v) {
            gltf_mesh_accessor(mesh, attr->data, attr->type);
        }
        if (gltf_mesh->primitives_v->indices->count) {
            cgltf_accessor *accessor = gltf_mesh->primitives_v->indices;
            //cgltf_size data_size = accessor->buffer_view->size;
            void *data = (char *)accessor->buffer_view->buffer->data + accessor->buffer_view->offset;
            DLB_ASSERT(data);

            DLB_ASSERT(accessor->type == cgltf_type_scalar);
            dlb_vec_reserve(mesh->indexes, accessor->count);
            dlb_vec_hdr(mesh->indexes)->len = accessor->count;

            // TODO(perf): Make sure all input data is already the correct size
            // Convert indices to u32
            switch (accessor->component_type)
            {
                case cgltf_component_type_r_8:
                case cgltf_component_type_r_8u:
                    for (cgltf_size i = 0; i < accessor->count; ++i) {
                        mesh->indexes[i] = ((u8 *)data)[i];
                    }
                    break;
                case cgltf_component_type_r_16:
                case cgltf_component_type_r_16u:
                    for (cgltf_size i = 0; i < accessor->count; ++i) {
                        mesh->indexes[i] = ((u16 *)data)[i];
                    }
                    break;
                case cgltf_component_type_r_32u:
                    for (cgltf_size i = 0; i < accessor->count; ++i) {
                        mesh->indexes[i] = ((u32 *)data)[i];
                    }
                    break;
                case cgltf_component_type_r_32f:
                case cgltf_component_type_invalid:
                default:
                    DLB_ASSERT(!"invalid index component type");
                    break;
            }
        }

        ta_mesh_create(mesh);
        ta_mesh_init_normals(mesh, 0.1f);
    }
}

void ta_gltf_free(ta_gltf *gltf)
{
    cgltf_free(gltf->data);
}