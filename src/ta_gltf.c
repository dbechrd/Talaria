#include "ta_gltf.h"
#include "ta_game.h"
#include "ta_json.h"
#include "ta_log.h"
#include "ta_mesh.h"
#include "ta_model.h"
#include "ta_schema.h"
#include "ta_symbol.h"
#include "dlb/dlb_types.h"
#include "dlb/dlb_vector.h"
#include "misc/cgltf.h"
#include <stdio.h>
#include <math.h>

static const char *ta_cgltf_file_type_str[] =
{
    [cgltf_file_type_invalid] = "cgltf_file_type_invalid",
    [cgltf_file_type_gltf] = "cgltf_file_type_gltf",
    [cgltf_file_type_glb] = "cgltf_file_type_glb",
};

static const char *ta_cgltf_result_str[] = {
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

static const char *ta_cgltf_buffer_view_type_str[] =
{
    [cgltf_buffer_view_type_invalid] = "cgltf_buffer_view_type_invalid",
    [cgltf_buffer_view_type_indices] = "cgltf_buffer_view_type_indices",
    [cgltf_buffer_view_type_vertices] = "cgltf_buffer_view_type_vertices",
};

static const char *ta_cgltf_attribute_type_str[] =
{
    [cgltf_attribute_type_invalid] = "cgltf_attribute_type_invalid",
    [cgltf_attribute_type_position] = "cgltf_attribute_type_position",
    [cgltf_attribute_type_normal] = "cgltf_attribute_type_normal",
    [cgltf_attribute_type_tangent] = "cgltf_attribute_type_tangent",
    [cgltf_attribute_type_texcoord] = "cgltf_attribute_type_texcoord",
    [cgltf_attribute_type_color] = "cgltf_attribute_type_color",
    [cgltf_attribute_type_joints] = "cgltf_attribute_type_joints",
    [cgltf_attribute_type_weights] = "cgltf_attribute_type_weights",
};

static const char *ta_cgltf_component_type_str[] =
{
    [cgltf_component_type_invalid] = "cgltf_component_type_invalid",
    [cgltf_component_type_r_8] = "cgltf_component_type_r_8", /* BYTE */
    [cgltf_component_type_r_8u] = "cgltf_component_type_r_8u", /* UNSIGNED_BYTE */
    [cgltf_component_type_r_16] = "cgltf_component_type_r_16", /* SHORT */
    [cgltf_component_type_r_16u] = "cgltf_component_type_r_16u", /* UNSIGNED_SHORT */
    [cgltf_component_type_r_32u] = "cgltf_component_type_r_32u", /* UNSIGNED_INT */
    [cgltf_component_type_r_32f] = "cgltf_component_type_r_32f", /* FLOAT */
};

static const char *ta_cgltf_type_str[] =
{
    [cgltf_type_invalid] = "cgltf_type_invalid",
    [cgltf_type_scalar] = "cgltf_type_scalar",
    [cgltf_type_vec2] = "cgltf_type_vec2",
    [cgltf_type_vec3] = "cgltf_type_vec3",
    [cgltf_type_vec4] = "cgltf_type_vec4",
    [cgltf_type_mat2] = "cgltf_type_mat2",
    [cgltf_type_mat3] = "cgltf_type_mat3",
    [cgltf_type_mat4] = "cgltf_type_mat4",
};

static const char *ta_cgltf_primitive_type_str[] =
{
    [cgltf_primitive_type_points] = "cgltf_primitive_type_points",
    [cgltf_primitive_type_lines] = "cgltf_primitive_type_lines",
    [cgltf_primitive_type_line_loop] = "cgltf_primitive_type_line_loop",
    [cgltf_primitive_type_line_strip] = "cgltf_primitive_type_line_strip",
    [cgltf_primitive_type_triangles] = "cgltf_primitive_type_triangles",
    [cgltf_primitive_type_triangle_strip] = "cgltf_primitive_type_triangle_strip",
    [cgltf_primitive_type_triangle_fan] = "cgltf_primitive_type_triangle_fan",
};

static const char *ta_cgltf_alpha_mode_str[] =
{
    [cgltf_alpha_mode_opaque] = "cgltf_alpha_mode_opaque",
    [cgltf_alpha_mode_mask] = "cgltf_alpha_mode_mask",
    [cgltf_alpha_mode_blend] = "cgltf_alpha_mode_blend",
};

static const char *ta_cgltf_animation_path_type_str[] = {
    [cgltf_animation_path_type_invalid] = "cgltf_animation_path_type_invalid",
    [cgltf_animation_path_type_translation] = "cgltf_animation_path_type_translation",
    [cgltf_animation_path_type_rotation] = "cgltf_animation_path_type_rotation",
    [cgltf_animation_path_type_scale] = "cgltf_animation_path_type_scale",
    [cgltf_animation_path_type_weights] = "cgltf_animation_path_type_weights",
};

static const char *ta_cgltf_interpolation_type_str[] = {
    [cgltf_interpolation_type_linear] = "cgltf_interpolation_type_linear",
    [cgltf_interpolation_type_step] = "cgltf_interpolation_type_step",
    [cgltf_interpolation_type_cubic_spline] = "cgltf_interpolation_type_cubic_spline",
};

static const char *ta_cgltf_camera_type_str[] = {
    [cgltf_camera_type_invalid] = "cgltf_camera_type_invalid",
    [cgltf_camera_type_perspective] = "cgltf_camera_type_perspective",
    [cgltf_camera_type_orthographic] = "cgltf_camera_type_orthographic",
};

static const char *ta_cgltf_light_type_str[] = {
    [cgltf_light_type_invalid] = "cgltf_light_type_invalid",
    [cgltf_light_type_directional] = "cgltf_light_type_directional",
    [cgltf_light_type_point] = "cgltf_light_type_point",
    [cgltf_light_type_spot] = "cgltf_light_type_spot",
};

//#define gltf_each(t, i, s) for (t (i) = (s), *(i##e) = (s + s##_count); (i) != (i##e); (i)++)

void gltf_dump(cgltf_data *data)
{
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

    if (data->lights_v) {
        printf("lights:\n");
        dlb_vec_each(cgltf_light *, light, data->lights_v) {
            printf("  [%zu]:\n", light - data->lights_v);
            printf("    name: %s\n", light->name);
            printf("    type: %s\n", ta_cgltf_light_type_str[light->type]);
            printf("    intensity: %f\n", light->intensity);
            printf("    color: %f %f %f\n", light->color[0], light->color[1], light->color[2]);
        }
    }

    if (data->scenes_v) {
        printf("scenes:\n");
        dlb_vec_each(cgltf_scene *, scene, data->scenes_v) {
            printf("  [%zu]:\n", scene - data->scenes_v);
            printf("    name: %s\n", scene->name);
            printf("    nodes:\n");
            dlb_vec_each(cgltf_node **, node, scene->nodes_v) {
                printf("      %s\n", (*node)->name);
            }
        }
    }

    printf("nodes:\n");
    dlb_vec_each(cgltf_node *, node, data->nodes_v) {
        printf("  [%zu]:\n", node - data->nodes_v);
        printf("    name: %s\n", node->name);
        if (node->parent) {
            printf("    parent: %s\n", node->parent->name);
        }
        if (node->children_v) {
            printf("    children:\n");
            dlb_vec_each(cgltf_node **, child, node->children_v) {
                printf("      %s\n", (*child)->name);
            }
        }
        if (node->skin)   printf("    skin: %s\n", node->skin->name);
        if (node->mesh)   printf("    mesh: %s\n", node->mesh->name);
        if (node->camera) printf("    camera: %s\n", node->camera->name);
        if (node->light)  printf("    light: %s\n", node->light->name);
        if (node->has_rotation) {
            printf("    rotation: %f %f %f %f\n", node->rotation[0], node->rotation[1], node->rotation[2], node->rotation[3]);
        }
        if (node->has_scale) {
            ta_vec3 scale = { 0 };
            scale.x = node->scale[0];
            scale.y = node->scale[1];
            scale.z = node->scale[2];
            DLB_ASSERT(vec3_equal(scale, VEC3_ONE) && "Non-uniform scale not supported");
        }
        if (node->has_translation) {
            printf("    translation: %f %f %f\n", node->translation[0], node->translation[1], node->translation[2]);
        }
        //
        //cgltf_float* weights_v;
        //cgltf_bool has_matrix;
        //cgltf_float matrix[16];
    }

    if (data->cameras_v) {
        printf("cameras:\n");
        dlb_vec_each(cgltf_camera *, camera, data->cameras_v) {
            printf("  [%zu]:\n", camera - data->cameras_v);
            printf("    name: %s\n", camera->name);
            printf("    type: %s\n", ta_cgltf_camera_type_str[camera->type]);
            if (camera->type == cgltf_camera_type_perspective) {
                printf("    aspect_ratio: %f\n", camera->data.perspective.aspect_ratio);
                printf("    yfov: %f\n", camera->data.perspective.yfov);
                printf("    znear: %f\n", camera->data.perspective.znear);
                printf("    zfar: %f\n", camera->data.perspective.zfar);
            } else if (camera->type == cgltf_camera_type_orthographic) {
                printf("    xmag: %f\n", camera->data.orthographic.xmag);
                printf("    ymag: %f\n", camera->data.orthographic.ymag);
                printf("    znear: %f\n", camera->data.orthographic.znear);
                printf("    zfar: %f\n", camera->data.orthographic.zfar);
            }
        }
    }

    if (data->animations_v) {
        printf("animations:\n");
        dlb_vec_each(cgltf_animation *, animation, data->animations_v) {
            printf("  [%zu]:\n", animation - data->animations_v);
            printf("    name: %s\n", animation->name);
            if (animation->samplers_v) {
                printf("    samplers:\n");
                dlb_vec_each(cgltf_animation_sampler *, sampler, animation->samplers_v) {
                    printf("      [%zu]:\n", sampler - animation->samplers_v);
                    printf("        input: [%zu]\n", sampler->input - data->accessors_v);
                    printf("        output: [%zu]\n", sampler->output - data->accessors_v);
                    printf("        interpolation: %s\n", ta_cgltf_interpolation_type_str[sampler->interpolation]);
                }
            }
            if (animation->channels_v) {
                printf("    channels:\n");
                dlb_vec_each(cgltf_animation_channel *, channel, animation->channels_v) {
                    printf("      [%zu]:\n", channel - animation->channels_v);
                    printf("        anim_sampler: [%zu]\n", channel->sampler - animation->samplers_v);
                    printf("        target node: %s\n", channel->target_node->name);
                    printf("        target path: %s\n", ta_cgltf_animation_path_type_str[channel->target_path]);
                }
            }
        }
    }

    if (data->materials_v) {
        printf("materials:\n");
        dlb_vec_each(cgltf_material *, material, data->materials_v) {
            printf("  [%zu]:\n", material - data->materials_v);
            printf("    name: %s\n", material->name);
            if (material->has_pbr_metallic_roughness) {
                cgltf_pbr_metallic_roughness *pbr = &material->pbr_metallic_roughness;
                printf("    pbr_metallic_roughness:\n");
                if (pbr->base_color_texture.texture &&
                    pbr->base_color_texture.texture->image &&
                    pbr->base_color_texture.texture->image->name)
                {
                    printf("      base_color_texture: %s\n", pbr->base_color_texture.texture->image->name);
                }
                if (pbr->metallic_roughness_texture.texture &&
                    pbr->metallic_roughness_texture.texture->image &&
                    pbr->metallic_roughness_texture.texture->image->name)
                {
                    printf("      metallic_roughness_texture: %s\n", pbr->metallic_roughness_texture.texture->image->name);
                }
                printf("      base_color_factor: %f %f %f %f\n", pbr->base_color_factor[0], pbr->base_color_factor[1], pbr->base_color_factor[2], pbr->base_color_factor[3]);
                printf("      metallic_factor: %f\n", pbr->metallic_factor);
                printf("      roughness_factor: %f\n", pbr->roughness_factor);
            }
            if (material->has_pbr_specular_glossiness) {
                cgltf_pbr_specular_glossiness *spec = &material->pbr_specular_glossiness;
                printf("    pbr_specular_glossiness:\n");
                if (spec->diffuse_texture.texture &&
                    spec->diffuse_texture.texture->image &&
                    spec->diffuse_texture.texture->image->name)
                {
                    printf("      diffuse_texture: %s\n", spec->diffuse_texture.texture->image->name);
                }
                if (spec->specular_glossiness_texture.texture &&
                    spec->specular_glossiness_texture.texture->image &&
                    spec->specular_glossiness_texture.texture->image->name) {
                    printf("      metallic_roughness_texture: %s\n", spec->specular_glossiness_texture.texture->image->name);
                }
                printf("      diffuse_factor: %f %f %f %f\n", spec->diffuse_factor[0], spec->diffuse_factor[1], spec->diffuse_factor[2], spec->diffuse_factor[3]);
                printf("      specular_factor: %f %f %f\n", spec->specular_factor[0], spec->specular_factor[1], spec->specular_factor[2]);
                printf("      glossiness_factor: %f\n", spec->glossiness_factor);
            }
            if (material->normal_texture.texture &&
                material->normal_texture.texture->image &&
                material->normal_texture.texture->image->name)
            {
                printf("    normal_texture: %s\n", material->normal_texture.texture->image->name);
            }
            if (material->occlusion_texture.texture &&
                material->occlusion_texture.texture->image &&
                material->occlusion_texture.texture->image->name)
            {
                printf("    occlusion_texture: %s\n", material->occlusion_texture.texture->image->name);
            }
            if (material->emissive_texture.texture &&
                material->emissive_texture.texture->image &&
                material->emissive_texture.texture->image->name)
            {
                printf("    emissive_texture: %s\n", material->emissive_texture.texture->image->name);
            }
            printf("    emissive_factor: %f %f %f\n", material->emissive_factor[0], material->emissive_factor[1], material->emissive_factor[2]);
            printf("    alpha_mode: %s\n", ta_cgltf_alpha_mode_str[material->alpha_mode]);
            printf("    alpha_cutoff: %f\n", material->alpha_cutoff);
            printf("    double_sided: %s\n", material->double_sided ? "True" : "False");
            printf("    unlit: %s\n", material->unlit ? "True" : "False");
        }
    }

    if (data->meshes_v) {
        printf("meshes:\n");
        dlb_vec_each(cgltf_mesh *, mesh, data->meshes_v) {
            printf("  [%zu]:\n", mesh - data->meshes_v);
            printf("    name: %s\n", mesh->name);
            if (mesh->primitives_v) {
                printf("    primitives:\n");
                dlb_vec_each(cgltf_primitive *, prim, mesh->primitives_v) {
                    printf("      %s\n", ta_cgltf_primitive_type_str[prim->type]);
                    if (prim->attributes_v) {
                        printf("        attributes:\n");
                        dlb_vec_each(cgltf_attribute *, attr, prim->attributes_v) {
                            printf("          %s\n", attr->name);
                            printf("            type: %s\n", ta_cgltf_attribute_type_str[attr->type]);
                            printf("            index: %d\n", attr->index);
                            printf("            accessor: [%zu]\n", attr->data - data->accessors_v);
                        }
                    }
                    if (prim->indices) {
                        printf("          indices:\n");
                        printf("            offset: %zu\n", prim->indices->offset);
                        printf("            count: %zu\n", prim->indices->count);
                    }
                    printf("        material: %s\n", prim->material->name);
                    if (prim->targets_v) {
                        printf("        targets:\n");
                        dlb_vec_each(cgltf_morph_target *, target, prim->targets_v) {
                            printf("          [%zu]:\n", target - prim->targets_v);
                            if (target->attributes_v) {
                                printf("            attributes:\n");
                                dlb_vec_each(cgltf_attribute *, attr, target->attributes_v) {
                                    printf("              %s:\n", attr->name);
                                    printf("                type: %s\n", ta_cgltf_attribute_type_str[attr->type]);
                                    printf("                index: %d\n", attr->index);
                                    printf("                data: [%zu]\n", attr->data - data->accessors_v);
                                }
                            }
                        }
                    }
                }
            }
            if (mesh->target_names_v) {
                printf("    target_names:\n");
                dlb_vec_each(const char **, target_name, mesh->target_names_v) {
                    printf("      [%zu]: %s\n", target_name - mesh->target_names_v, *target_name);
                }
            }
            printf("    weights: float[%zu]\n", dlb_vec_len(mesh->weights_v));
#if 0
            if (mesh->extras.start_offset) {
                const char *extras = data->json + mesh->extras.start_offset;
                size_t extras_len = mesh->extras.end_offset - mesh->extras.start_offset;

                printf("    extras: (%zu bytes)\n", extras_len);
                printf("      json: %*.s\n", (int)extras_len, extras);
                printf("      dump:\n");

                jsmntok_t *tokens = 0;
                ta_json_parse(extras, extras_len, &tokens);
                DLB_ASSERT(tokens);
                ta_json_dump(extras, tokens, dlb_vec_len(tokens), 3);
                dlb_vec_free(tokens);
            }
#endif
        }
    }

    if (data->textures_v) {
        printf("textures:\n");
        dlb_vec_each(cgltf_texture *, texture, data->textures_v) {
            printf("  [%zu]:\n", texture - data->textures_v);
            if (texture->name)    printf("    name: %s\n", texture->name);
            if (texture->image)   printf("    image: %s\n", texture->image->name);
            if (texture->sampler) printf("    sampler: [%zu]\n", texture->sampler - data->samplers_v);
        }
    }

    if (data->images_v) {
        printf("images:\n");
        dlb_vec_each(cgltf_image *, image, data->images_v) {
            printf("  [%zu]:\n", image - data->images_v);
            if (image->name)        printf("    name: %s\n", image->name);
            if (image->mime_type)   printf("    mime_type: %s\n", image->mime_type);
            if (image->uri)         printf("    uri: %s\n", image->uri);
            if (image->buffer_view) printf("    buffer_view: [%zu]\n", image->buffer_view - data->buffer_views_v);
        }
    }

    if (data->accessors_v) {
        printf("accessors:\n");
        dlb_vec_each(cgltf_accessor *, accessor, data->accessors_v) {
            printf("  [%zu]:\n", accessor - data->accessors_v);
            printf("    component_type: %s\n", ta_cgltf_component_type_str[accessor->component_type]);
            printf("    normalized: %s\n", accessor->normalized ? "True" : "False");
            printf("    type: %s\n", ta_cgltf_type_str[accessor->type]);
            printf("    offset: %zu\n", accessor->offset);
            printf("    count: %zu\n", accessor->count);
            printf("    stride: %zu\n", accessor->stride);
            printf("    buffer_view: [%zu]\n", accessor->buffer_view - data->buffer_views_v);
            if (accessor->has_min && accessor->type == cgltf_attribute_type_position) {
                printf("    min: %f %f %f\n",
                    accessor->min[0], accessor->min[1], accessor->min[2]);
            }
            if (accessor->has_max && accessor->type == cgltf_attribute_type_position) {
                printf("    max: %f %f %f\n",
                    accessor->max[0], accessor->max[1], accessor->max[2]);
            }
            if (accessor->is_sparse) {
                printf("    sparse: %zu\n", accessor->sparse.count);
                // TODO: Bunch of other sparse properties.. jeebus too many props
            }
        }
    }

    if (data->buffer_views_v) {
        printf("buffer_views:\n");
        dlb_vec_each(cgltf_buffer_view *, buffer_view, data->buffer_views_v) {
            printf("  [%zu]:\n", buffer_view - data->buffer_views_v);
            printf("    buffer: [%zu]\n", buffer_view->buffer - data->buffers_v);
            printf("    offset: %zu\n", buffer_view->offset);
            printf("    size: %zu\n", buffer_view->size);
            printf("    stride: %zu\n", buffer_view->stride);
            printf("    type: %s\n", ta_cgltf_buffer_view_type_str[buffer_view->type]);
        }
    }

    if (data->buffers_v) {
        printf("buffers:\n");
        dlb_vec_each(cgltf_buffer *, buffer, data->buffers_v) {
            printf("  [%zu]:\n", buffer - data->buffers_v);
            printf("    size: %zu\n", buffer->size);
            if (buffer->uri) {
                printf("    uri: ");
                char *c = buffer->uri;
                while (*c && *c != ',') {
                    fputc(*c, stdout);
                    c++;
                }
                printf("\n");
            }
            printf("    data: %p\n", buffer->data);
        }
    }







    if (data->samplers_v) {
        printf("samplers:\n");
        dlb_vec_each(cgltf_sampler *, sampler, data->samplers_v) {
            printf("  [%zu]:\n", sampler - data->samplers_v);
            printf("    min_filter: %d\n", sampler->min_filter);
        }
    }

    if (data->skins_v) {
        printf("skins:\n");
        dlb_vec_each(cgltf_skin *, skin, data->skins_v) {
            printf("  [%zu]:\n", skin - data->skins_v);
            printf("    name: %s\n", skin->name);
        }
    }


    //cgltf_scene* scene;
    //cgltf_extras extras;

    printf("\n");
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

cgltf_result ta_gltf_parse_file(ta_gltf *gltf)
{
    cgltf_options options = { 0 };
    options.memory_alloc = &ta_cgltf_alloc;
    options.memory_calloc = &ta_cgltf_calloc;
    options.memory_free = &ta_cgltf_free;
    cgltf_result err;

    ta_log_write(&tg_debug_log, SRC_GLTF, "parsing %s\n", gltf->filename);
    err = cgltf_parse_file(&options, gltf->filename, &gltf->data);
    if (err) {
        printf("cgltf_parse_file error: %s", ta_cgltf_result_str[err]);
        return err;
    }

    ta_log_write(&tg_debug_log, SRC_GLTF, "loading buffers\n");
    err = cgltf_load_buffers(&options, gltf->data, gltf->filename);
    if (err) {
        printf("cgltf_load_buffers error: %s", ta_cgltf_result_str[err]);
        return err;
    }

    ta_log_write(&tg_debug_log, SRC_GLTF, "validating\n");
    err = cgltf_validate(gltf->data);
    if (err) {
        printf("cgltf_validate error: %s", ta_cgltf_result_str[err]);
        return err;
    }

    ta_log_write(&tg_debug_log, SRC_GLTF, "dumping\n");
    gltf_dump(gltf->data);

    ta_log_write(&tg_debug_log, SRC_GLTF, "successfully parsed %s\n", gltf->filename);
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
    ta_log_write(&tg_debug_log, SRC_GLTF, "ta_gltf_load %s\n", gltf->filename);

    size_t scene_count = dlb_vec_len(gltf->data->scenes_v);
    DLB_ASSERT(scene_count == 1);

    dlb_vec_each(cgltf_node *, gltf_node, gltf->data->nodes_v) {
        UNUSED(gltf_node);
    }

    // TODO: Load materials
    dlb_vec_each(cgltf_material *, gltf_material, gltf->data->materials_v) {
        UNUSED(gltf_material);
    }

    dlb_vec_each(cgltf_mesh *, gltf_mesh, gltf->data->meshes_v) {
        ta_log_write(&tg_debug_log, SRC_GLTF, "ta_game_alloc MESH %s\n", gltf_mesh->name);
        ta_mesh *mesh = ta_game_alloc(RES_MESH, gltf_mesh->name, strlen(gltf_mesh->name));

        dlb_vec_each(cgltf_primitive *, gltf_prim, gltf_mesh->primitives_v) {
            if (gltf_prim != gltf_mesh->primitives_v)
                break;  // TODO: Handle loading multiple primitives

            DLB_ASSERT(gltf_prim->type == cgltf_primitive_type_triangles);
            cgltf_material* material;
            cgltf_morph_target* targets_v;

            //ta_piece piece = { 0 };
            //piece.mesh = "mesh0";
            //piece.material = ta_symbol_intern(material->name, strlen(material->name));

            ta_log_write(&tg_debug_log, SRC_GLTF, "copying attributes\n");
            dlb_vec_each(cgltf_attribute *, attr, gltf_prim->attributes_v) {
                gltf_mesh_accessor(mesh, attr->data, attr->type);
            }
            if (gltf_prim->indices->count) {
                cgltf_accessor *accessor = gltf_prim->indices;
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
        }
        //dlb_vec_each(const char **, gltf_target, gltf_mesh->target_names_v) {
        //    const char *target = *gltf_target;
        //    UNUSED(target);
        //}

        ta_log_write(&tg_debug_log, SRC_GLTF, "ta_mesh_create\n");
        ta_mesh_create(mesh);
        ta_log_write(&tg_debug_log, SRC_GLTF, "ta_mesh_init_normals\n");
        ta_mesh_init_normals(mesh, 0.1f);
    }
    ta_log_write(&tg_debug_log, SRC_GLTF, "successfully loaded meshes for %s\n", gltf->filename);
}

void ta_gltf_free(ta_gltf *gltf)
{
    cgltf_free(gltf->data);
}