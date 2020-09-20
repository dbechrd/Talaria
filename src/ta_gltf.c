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
#include "misc/stb_image.h"
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
        dlb_vec_each(char **, extension, data->extensions_required_v) {
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
                dlb_vec_each(char **, target_name, mesh->target_names_v) {
                    printf("      [%zu]: %s\n", target_name - mesh->target_names_v, *target_name);
                }
            }
            printf("    weights: float[%zu]\n", dlb_vec_len(mesh->weights_v));
#if 0
            if (mesh->extras.start_offset) {
                const char *extras = data->json + mesh->extras.start_offset;
                size_t extras_len = mesh->extras.end_offset - mesh->extras.start_offset;

                printf("    extras: (%zu bytes)\n", extras_len);
                printf("      json: %.*s\n", (int)extras_len, extras);
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

#if 0
    ta_log_write(&tg_debug_log, SRC_GLTF, "dumping\n");
    gltf_dump(gltf->data);
#endif

    ta_log_write(&tg_debug_log, SRC_GLTF, "successfully parsed %s\n", gltf->filename);
    return err;
}

static void gltf_mesh_accessor(ta_mesh *mesh, cgltf_accessor *accessor, cgltf_attribute_type cgltf_attr_type,
    size_t target)
{
    static const ta_vertex_attrib_type attr_type_lookup[] = {
        [cgltf_attribute_type_position] = TA_VERTEX_ATTR_POSITION,
        [cgltf_attribute_type_color   ] = TA_VERTEX_ATTR_COLOR,
        [cgltf_attribute_type_texcoord] = TA_VERTEX_ATTR_UV,
        [cgltf_attribute_type_normal  ] = TA_VERTEX_ATTR_NORMAL,
        [cgltf_attribute_type_tangent ] = TA_VERTEX_ATTR_TANGENT,
        [cgltf_attribute_type_joints  ] = TA_VERTEX_ATTR_BONE_INDICES,
        [cgltf_attribute_type_weights ] = TA_VERTEX_ATTR_BONE_WEIGHTS,
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

    // Ensure attribute is a morphable attribute
    ta_vertex_attrib_type attr_type = attr_type_lookup[cgltf_attr_type];
    if (target) {
        // NOTE: We don't support morph targets on top of joints/weight for now
        DLB_ASSERT(
            attr_type == TA_VERTEX_ATTR_COLOR    ||
            attr_type == TA_VERTEX_ATTR_UV       ||
            attr_type == TA_VERTEX_ATTR_POSITION ||
            attr_type == TA_VERTEX_ATTR_NORMAL   ||
            attr_type == TA_VERTEX_ATTR_TANGENT
        );
    }
    attr_type += MORPH_OFFSET * target;
    if (target == 1) {
        // NOTE: Make sure first morph mapped to the correct attr_type (assume that this means others will too)
        DLB_ASSERT(
            attr_type == TA_VERTEX_ATTR_MORPH1_POSITION ||
            attr_type == TA_VERTEX_ATTR_MORPH1_NORMAL   ||
            attr_type == TA_VERTEX_ATTR_MORPH1_TANGENT
        );
    }

    // TODO: Could make a size lookup table for each TA_MESH_BUFFER type and use dlb_vec_reserve_size()
    switch (attr_type) {
        case TA_VERTEX_ATTR_POSITION:
        case TA_VERTEX_ATTR_MORPH1_POSITION: {
            DLB_ASSERT(accessor->type == cgltf_type_vec3);
            DLB_ASSERT(accessor->component_type == cgltf_component_type_r_32f);
            DLB_ASSERT(data_size == sizeof(*mesh->positions) * accessor->count);
            DLB_ASSERT(sizeof(*mesh->positions) == sizeof(*mesh->morph1_positions));
            dlb_vec_reserve_size(mesh->buffers[attr_type], accessor->count, sizeof(*mesh->positions));
            break;
        }
        case TA_VERTEX_ATTR_COLOR: {
            DLB_ASSERT(accessor->type == cgltf_type_vec4);
            DLB_ASSERT(accessor->component_type == cgltf_component_type_r_32f);
            DLB_ASSERT(data_size == sizeof(*mesh->colors) * accessor->count);
            dlb_vec_reserve_size(mesh->buffers[attr_type], accessor->count, sizeof(*mesh->colors));
            break;
        }
        case TA_VERTEX_ATTR_UV: {
            DLB_ASSERT(accessor->type == cgltf_type_vec2);
            DLB_ASSERT(accessor->component_type == cgltf_component_type_r_32f);
            DLB_ASSERT(data_size == sizeof(*mesh->uvs) * accessor->count);
            dlb_vec_reserve_size(mesh->buffers[attr_type], accessor->count, sizeof(*mesh->uvs));
            break;
        }
        case TA_VERTEX_ATTR_NORMAL:
        case TA_VERTEX_ATTR_MORPH1_NORMAL: {
            DLB_ASSERT(accessor->type == cgltf_type_vec3);
            DLB_ASSERT(accessor->component_type == cgltf_component_type_r_32f);
            DLB_ASSERT(data_size == sizeof(*mesh->normals) * accessor->count);
            DLB_ASSERT(sizeof(*mesh->normals) == sizeof(*mesh->morph1_normals));
            dlb_vec_reserve_size(mesh->buffers[attr_type], accessor->count, sizeof(*mesh->normals));
            break;
        }
        case TA_VERTEX_ATTR_TANGENT:
        case TA_VERTEX_ATTR_MORPH1_TANGENT: {
            DLB_ASSERT(accessor->component_type == cgltf_component_type_r_32f);
            if (accessor->type == cgltf_type_vec3) {
                DLB_ASSERT(data_size == sizeof(*mesh->tangents) * accessor->count);
                DLB_ASSERT(sizeof(*mesh->tangents) == sizeof(*mesh->morph1_tangents));
                dlb_vec_reserve_size(mesh->buffers[attr_type], accessor->count, sizeof(*mesh->tangents));
            } else if (accessor->type == cgltf_type_vec4) {
                // NOTE: gltf decided tangents should be vec4.. but only sometimes. Fix that dumb shit. -.-
                DLB_ASSERT(sizeof(*mesh->tangents) == 12);
                DLB_ASSERT(sizeof(*mesh->tangents) == sizeof(*mesh->morph1_tangents));
                DLB_ASSERT(data_size / accessor->count == 16);
                dlb_vec_alloc_count_size(mesh->buffers[attr_type], accessor->count, sizeof(*mesh->tangents));

                // Calculate how many extra bytes to skip per element
                size_t extra_bytes = (data_size / accessor->count) - sizeof(*mesh->tangents);

                // Copy array of vec4 to array of vec3
                const u8 *src = data;
                u8 *dst = (void *)mesh->tangents;
                DLB_ASSERT(dst);
                float w = 1.0f;
                for (size_t i = 0; i < accessor->count; ++i) {
                    w = ((ta_vec4 *)src)->w;
                    memcpy(dst, src, sizeof(*mesh->tangents));
                    ((ta_vec3 *)dst)->x *= w;
                    ((ta_vec3 *)dst)->y *= w;
                    ((ta_vec3 *)dst)->z *= w;
                    src += sizeof(*mesh->tangents) + extra_bytes;
                    dst += sizeof(*mesh->tangents);
                }
                return;
            } else {
                DLB_ASSERT(!"Unexpected accessor type for tangents");
            }
            break;
        } case TA_VERTEX_ATTR_BONE_INDICES: {
            DLB_ASSERT(!target);
            DLB_ASSERT(accessor->type == cgltf_type_vec4);
            if (accessor->component_type == cgltf_component_type_r_16u) {
                DLB_ASSERT(data_size == sizeof(*mesh->joints) * accessor->count);
                //DLB_ASSERT(sizeof(*mesh->joints) == sizeof(*mesh->morph1_joints));
                dlb_vec_reserve(mesh->joints, accessor->count);
                //dlb_vec_reserve_size(mesh->buffers[attr_type], accessor->count, sizeof(*mesh->joints));
            } else if (accessor->component_type == cgltf_component_type_r_8u) {
                // NOTE: gltf decided joints should be 8u.. but only sometimes. Fix that dumb shit. -.-
                DLB_ASSERT(sizeof(*mesh->joints) / VERTEX_MAX_JOINTS == sizeof(u16));
                //DLB_ASSERT(sizeof(*mesh->joints) == sizeof(*mesh->morph1_joints));
                DLB_ASSERT(data_size / accessor->count / VERTEX_MAX_JOINTS == sizeof(u8));
                //dlb_vec_reserve_size(mesh->buffers[attr_type], accessor->count, sizeof(*mesh->joints));
                dlb_vec_alloc_count(mesh->joints, accessor->count);

                // Copy array of vec4 to array of vec3
                const u8 *src = data;
                u16 *dst = (void *)mesh->joints;
                DLB_ASSERT(dst);
                for (size_t i = 0; i < accessor->count; ++i) {
                    *dst = (u16)*src;
                    src++;
                    dst++;
                }
                return;
            } else {
                DLB_ASSERT(!"Unexpected component type for joints");
            }
            break;
        } case TA_VERTEX_ATTR_BONE_WEIGHTS: {
            // NOTE: We don't support morph targets on top of joints/weight for now
            DLB_ASSERT(!target);
            DLB_ASSERT(accessor->type == cgltf_type_vec4);
            DLB_ASSERT(accessor->component_type == cgltf_component_type_r_32f);
            DLB_ASSERT(data_size == sizeof(*mesh->weights) * accessor->count);
            //DLB_ASSERT(sizeof(*mesh->weights) == sizeof(*mesh->morph0_weights));
            //dlb_vec_reserve_size(mesh->buffers[attr_type], accessor->count, sizeof(*mesh->weights));
            dlb_vec_reserve(mesh->weights, accessor->count);
            break;
        } default: {
            DLB_ASSERT(!"Unexpected gltf attribute type");
            break;
        }
    }

    void *buffer = mesh->buffers[attr_type];
    dlb_vec_hdr(buffer)->len = accessor->count;
    dlb_memcpy(buffer, data, data_size);
}

static void gltf_texture(const char **out_texture_name, cgltf_texture_view *view)
{
    if (!view->texture) {
        return;
    }

    DLB_ASSERT(view->texcoord == 0);  // TODO: What is this?
    DLB_ASSERT(view->scale == 1.0f);
    DLB_ASSERT(!view->has_transform);

    DLB_ASSERT(view->texture->image);
    //DLB_ASSERT(!view->texture->sampler);  // TODO: Handle sampler options

    // TODO: Give it a placeholder name if it doesn't have one (e.g. "bee_occlusion")
    const char *temp_name = 0;
    if (!view->texture->image->name) {
        static int placeholder_tex_id = 1;
        // Load meshes (ta_model -> ta_piece) and primitives (ta_mesh)
        char name_buf[256] = { 0 };

        const size_t name_size = ARRAY_SIZE(name_buf);
        size_t name_len = snprintf(name_buf, name_size, "gltf_texture_%04d", placeholder_tex_id++);
        DLB_ASSERT(name_len < name_size);
        temp_name = ta_symbol_intern(name_buf, name_len);
    } else {
        temp_name = view->texture->image->name;
    }
    DLB_ASSERT(temp_name);

    DLB_ASSERT(strcmp(view->texture->image->mime_type, "image/png") == 0);  // TODO: Handle other types?
    DLB_ASSERT(!view->texture->image->uri);  // TODO: We could fix external URIs.. do I really care?

    DLB_ASSERT(view->texture->image->buffer_view);
    DLB_ASSERT(view->texture->image->buffer_view->size);
    DLB_ASSERT(!view->texture->image->buffer_view->stride);

    DLB_ASSERT(view->texture->image->buffer_view->buffer->size);
    DLB_ASSERT(view->texture->image->buffer_view->buffer->data);

    char texture_name[256] = { 0 };
    const size_t texture_name_size = ARRAY_SIZE(texture_name);
    size_t texture_name_len = snprintf(texture_name, texture_name_size, "#%s", temp_name);
    DLB_ASSERT(texture_name_len < texture_name_size);

    ta_resource *exists = ta_game_by_name_try(RES_TEXTURE, texture_name, texture_name_len);
    if (exists) {
        *out_texture_name = exists->name;
        return;
    }

    u8 *buffer = view->texture->image->buffer_view->buffer->data;
    buffer += view->texture->image->buffer_view->offset;
    size_t buffer_len = view->texture->image->buffer_view->buffer->size;

    // NOTE: stbi_load takes len as int, make sure we're below the limit (a 16384 x 16384 x 4 texture can fit in an int,
    // so if we're over this.. that's one BF texture).
    DLB_ASSERT(buffer_len <= INT_MAX);

    ta_log_write(&tg_debug_log, SRC_GLTF, "stbi_load_from_memory: %s\n", texture_name);
    //stbi_set_flip_vertically_on_load(true);
    int w = 0;
    int h = 0;
    int channels = 0;  // components/channels
    u8 *pixels = stbi_load_from_memory(buffer, (int)buffer_len, &w, &h, &channels, 0);
    if (!pixels) {
        const char *reason = stbi_failure_reason();
        ta_log_write(&tg_debug_log, SRC_GLTF, "Failed to load tex: %s\nSTBI Reason: %s\n", texture_name, reason);
#if _DEBUG
        // PNG should start with 137,80,78,71,13,10,26,10
        printf("Buffer 0-8: ");
        for (int i = 0; i < 8; ++i) {
            printf("%u ", buffer[i]);
        }
        printf("\n");
#endif
        DLB_ASSERT(!"ta_texture_init: Failed to load tex");
    }

    DLB_ASSERT(w);
    DLB_ASSERT(h);
    DLB_ASSERT(channels);

    ta_log_write(&tg_debug_log, SRC_GLTF, "ta_game_alloc TEXTURE %s\n", texture_name);
    ta_texture *texture = ta_game_alloc(RES_TEXTURE, texture_name, texture_name_len);
    texture->type = TA_TEXTURE_2D_ARRAY;
    texture->width = w;
    texture->height = h;
    texture->channels = (u8)channels;
    texture->pixels = pixels;
    ta_texture_init(texture);
    texture->pixels = 0; // HACK: Don't want ta_texture_free to try to call dlb_vec_free() on this buffer
    stbi_image_free(pixels);

    *out_texture_name = texture->name;
}

static void gltf_metallic_roughness(const char **out_metallic, const char **out_roughness, cgltf_texture_view *view)
{
    if (!view->texture) {
        return;
    }

    DLB_ASSERT(view->texcoord == 0);  // TODO: What is this?
    DLB_ASSERT(view->scale == 1.0f);
    DLB_ASSERT(!view->has_transform);

    DLB_ASSERT(view->texture->image);
    //DLB_ASSERT(!view->texture->sampler);  // TODO: Handle sampler options

    /// TODO: Give it a placeholder name if it doesn't have one (e.g. "bee_occlusion")
    const char *temp_name = 0;
    if (!view->texture->image->name) {
        static int placeholder_tex_id = 1;
        // Load meshes (ta_model -> ta_piece) and primitives (ta_mesh)
        char name_buf[256] = { 0 };

        const size_t name_size = ARRAY_SIZE(name_buf);
        size_t name_len = snprintf(name_buf, name_size, "gltf_texture_%04d", placeholder_tex_id++);
        DLB_ASSERT(name_len < name_size);
        temp_name = ta_symbol_intern(name_buf, name_len);
    } else {
        temp_name = view->texture->image->name;
    }
    DLB_ASSERT(temp_name);

    DLB_ASSERT(strcmp(view->texture->image->mime_type, "image/png") == 0);  // TODO: Handle other types?
    DLB_ASSERT(!view->texture->image->uri);

    DLB_ASSERT(view->texture->image->buffer_view);
    DLB_ASSERT(view->texture->image->buffer_view->size);
    DLB_ASSERT(!view->texture->image->buffer_view->stride);

    DLB_ASSERT(view->texture->image->buffer_view->buffer->size);
    DLB_ASSERT(view->texture->image->buffer_view->buffer->data);

    char metallic_name_buf[256] = { 0 };
    char roughness_name_buf[256] = { 0 };
    const size_t metallic_name_size = ARRAY_SIZE(metallic_name_buf);
    const size_t roughness_name_size = ARRAY_SIZE(roughness_name_buf);
    size_t metallic_name_len = snprintf(metallic_name_buf, metallic_name_size, "#%s.r", temp_name);
    size_t roughness_name_len = snprintf(roughness_name_buf, roughness_name_size, "#%s.g", temp_name);
    DLB_ASSERT(metallic_name_len < metallic_name_size);
    DLB_ASSERT(roughness_name_len < roughness_name_size);

    ta_resource *metallic_exists = ta_game_by_name_try(RES_TEXTURE, metallic_name_buf, metallic_name_len);
    ta_resource *roughness_exists = ta_game_by_name_try(RES_TEXTURE, roughness_name_buf, roughness_name_len);
    if (metallic_exists || roughness_exists) {
        DLB_ASSERT(metallic_exists && roughness_exists);  // Wtf.. how did we create one but not the other last time?
        *out_metallic = metallic_exists->name;
        *out_roughness = roughness_exists->name;
        return;
    }

    u8 *buffer = view->texture->image->buffer_view->buffer->data;
    buffer += view->texture->image->buffer_view->offset;
    size_t buffer_len = view->texture->image->buffer_view->buffer->size;

    // NOTE: stbi_load takes len as int, make sure we're below the limit (a 16384 x 16384 x 4 texture can fit in an int,
    // so if we're over this.. that's one BF texture).
    DLB_ASSERT(buffer_len <= INT_MAX);

    ta_log_write(&tg_debug_log, SRC_GLTF, "stbi_load_from_memory: %s\n", temp_name);
    //stbi_set_flip_vertically_on_load(true);
    int w = 0;
    int h = 0;
    int channels = 0;  // components/channels
    u8 *pixels = stbi_load_from_memory(buffer, (int)buffer_len, &w, &h, &channels, 0);
    if (!pixels) {
        const char *reason = stbi_failure_reason();
        ta_log_write(&tg_debug_log, SRC_GLTF, "Failed to load tex: %s\nSTBI Reason: %s\n", temp_name, reason);
#if _DEBUG
        // PNG should start with 137,80,78,71,13,10,26,10
        printf("Buffer 0-8: ");
        for (int i = 0; i < 8; ++i) {
            printf("%u ", buffer[i]);
        }
        printf("\n");
#endif
        DLB_ASSERT(!"ta_texture_init: Failed to load tex");
    }

    DLB_ASSERT(w);
    DLB_ASSERT(h);
    DLB_ASSERT(channels >= 2);

    const char *metallic_name = ta_symbol_intern(metallic_name_buf, metallic_name_len);
    const char *roughness_name = ta_symbol_intern(roughness_name_buf, roughness_name_len);

    ta_log_write(&tg_debug_log, SRC_GLTF, "ta_game_alloc TEXTURE %s\n", metallic_name);
    ta_game_alloc(RES_TEXTURE, SYM(metallic_name));
    ta_log_write(&tg_debug_log, SRC_GLTF, "ta_game_alloc TEXTURE %s\n", roughness_name);
    ta_game_alloc(RES_TEXTURE, SYM(roughness_name));

    // NOTE: Re-lookup textures in case pool resized
    ta_texture *metallic = ta_game_by_sym(RES_TEXTURE, metallic_name);
    ta_texture *roughness = ta_game_by_sym(RES_TEXTURE, roughness_name);

    size_t pixels_len = w * h * channels;
    metallic->width = w;
    metallic->height = h;
    metallic->channels = 1;
    dlb_vec_reserve(metallic->pixels, pixels_len);
    roughness->width = w;
    roughness->height = h;
    roughness->channels = 1;
    dlb_vec_reserve(roughness->pixels, pixels_len);

    // Split channels into two separate textures
    // NOTE: glTF spec states that metallic is in B channel and roughness is in G channel, other channels are ignored
    // https://github.com/KhronosGroup/glTF/blob/master/specification/2.0/schema/material.pbrMetallicRoughness.schema.json
    for (size_t i = 0; i < pixels_len; i += channels) {
        dlb_vec_push(metallic->pixels, pixels[i+2]);  // G channel
        dlb_vec_push(roughness->pixels, pixels[i+1]); // B channel
    }

    DLB_ASSERT(dlb_vec_len(metallic->pixels) == pixels_len);
    DLB_ASSERT(dlb_vec_len(roughness->pixels) == pixels_len);

    ta_texture_init(metallic);
    ta_texture_init(roughness);
    stbi_image_free(pixels);

    *out_metallic = metallic->name;
    *out_roughness = roughness->name;
}

void ta_gltf_load(ta_gltf *gltf)
{
    ta_log_write(&tg_debug_log, SRC_GLTF, "ta_gltf_load %s\n", gltf->filename);

    size_t scene_count = dlb_vec_len(gltf->data->scenes_v);
    DLB_ASSERT(scene_count == 1);

    static const char *dude = 0;
    if (!dude) {
        dude = ta_symbol_intern(CSTR("dude_armature"));
    }
    dlb_vec_each(cgltf_node *, gltf_node, gltf->data->nodes_v) {
        const char *node_name = ta_symbol_intern(gltf_node->name, strlen(gltf_node->name));
        if (node_name == dude) {
            DLB_ASSERT(1);
        }
    }

    // Load animations
    dlb_vec_each(cgltf_animation *, gltf_animation, gltf->data->animations_v) {
        char animation_name[256] = { 0 };
        const size_t animation_name_size = ARRAY_SIZE(animation_name);
        size_t animation_name_len = snprintf(animation_name, animation_name_size, "#%s", gltf_animation->name);
        DLB_ASSERT(animation_name_len < animation_name_size);

        ta_log_write(&tg_debug_log, SRC_GLTF, "ta_game_alloc ANIMATION %s\n", animation_name);
        ta_animation *animation = ta_game_alloc(RES_ANIMATION, animation_name, animation_name_len);

        dlb_vec_each(cgltf_animation_sampler *, gltf_sampler, gltf_animation->samplers_v) {
            ta_animation_sampler sampler = { 0 };

            cgltf_accessor *in = gltf_sampler->input;
            cgltf_accessor *out = gltf_sampler->output;

            cgltf_size in_size = in->buffer_view->size;
            float *in_data = (float *)((char *)in->buffer_view->buffer->data + in->buffer_view->offset);
            cgltf_size out_size = out->buffer_view->size;
            float *out_data = (float *)((char *)out->buffer_view->buffer->data + out->buffer_view->offset);

            // Ensure buffers have been loaded and are the expected type
            DLB_ASSERT(in_size);
            DLB_ASSERT(in_data);
            DLB_ASSERT(in->component_type == cgltf_component_type_r_32f);
            DLB_ASSERT(in->type != cgltf_type_invalid);
            DLB_ASSERT(in->buffer_view->stride == 0);  // NOTE: We don't support interleaved vertex attributes for now

            DLB_ASSERT(out_size);
            DLB_ASSERT(out_data);
            DLB_ASSERT(out->component_type == cgltf_component_type_r_32f);
            DLB_ASSERT(out->type != cgltf_type_invalid);
            DLB_ASSERT(out->buffer_view->stride == 0);  // NOTE: We don't support interleaved vertex attributes for now

            static const ta_animation_interpolation_type interp_type_lookup[] = {
                [cgltf_interpolation_type_linear]       = TA_ANIMATION_INTERP_LINEAR,
                [cgltf_interpolation_type_step]         = TA_ANIMATION_INTERP_STEP,
                [cgltf_interpolation_type_cubic_spline] = TA_ANIMATION_INTERP_CUBICSPLINE,
            };

            sampler.interpolation_mode = interp_type_lookup[gltf_sampler->interpolation];
            dlb_vec_alloc_count(sampler.input, in_size / sizeof(float));
            dlb_vec_alloc_count(sampler.output, out_size / sizeof(float));
            dlb_memcpy(sampler.input, in_data, in_size);
            dlb_memcpy(sampler.output, out_data, out_size);

            dlb_vec_push(animation->samplers, sampler);
        }

        dlb_vec_each(cgltf_animation_channel *, gltf_channel, gltf_animation->channels_v) {
            ta_animation_channel channel = { 0 };

            size_t sampler_idx = gltf_channel->sampler - gltf_animation->samplers_v;
            channel.sampler_idx = sampler_idx;
            const char *target_name = gltf_channel->target_node->name;
            target_name = ta_symbol_intern(target_name, strlen(target_name));

            // TODO: Prepend `bone_` to all bones to easily identify them? Is there a better, deterministic way to know
            // if a node is a bone vs. model? Hmm..
            channel.target_bone = target_name;

            cgltf_accessor *in = gltf_channel->sampler->input;
            cgltf_accessor *out = gltf_channel->sampler->output;

            static const ta_animation_path_type animation_path_type_lookup[] = {
                [cgltf_animation_path_type_translation] = TA_ANIMATION_PATH_TRANSLATION,
                [cgltf_animation_path_type_rotation]    = TA_ANIMATION_PATH_ROTATION,
                [cgltf_animation_path_type_scale]       = TA_ANIMATION_PATH_SCALE,
                [cgltf_animation_path_type_weights]     = TA_ANIMATION_PATH_WEIGHTS,
            };

            channel.target_path = animation_path_type_lookup[gltf_channel->target_path];
            switch (channel.target_path) {
                case TA_ANIMATION_PATH_TRANSLATION:
                    DLB_ASSERT(in->type == cgltf_type_scalar);
                    DLB_ASSERT(out->type == cgltf_type_vec3);
                    break;
                case TA_ANIMATION_PATH_ROTATION:
                    DLB_ASSERT(in->type == cgltf_type_scalar);
                    DLB_ASSERT(out->type == cgltf_type_vec4);
                    break;
                case TA_ANIMATION_PATH_SCALE:
                    DLB_ASSERT(in->type == cgltf_type_scalar);
                    DLB_ASSERT(out->type == cgltf_type_vec3);
                    break;
                case TA_ANIMATION_PATH_WEIGHTS:
                    DLB_ASSERT(in->type == cgltf_type_scalar);
                    DLB_ASSERT(out->type == cgltf_type_scalar);
                    break;
                default:
                    DLB_ASSERT(!"Invalid animation path type");
            }

            dlb_vec_push(animation->channels, channel);
        }
    }

    // Load skins
    dlb_vec_each(cgltf_skin *, gltf_skin, gltf->data->skins_v) {
        DLB_ASSERT(1 || gltf_skin);
        //char* name;
        //cgltf_node** joints_v;
        //cgltf_node* skeleton;
        //cgltf_accessor* inverse_bind_matrices;
        //cgltf_extras extras;
    }

    // TODO(cleanup): Load texture samplers (already sort of asserted in gltf_texture()
    DLB_ASSERT(1 || gltf->data->samplers_v);

    // Load materials
    dlb_vec_each(cgltf_material *, gltf_material, gltf->data->materials_v) {
        const char *temp_material_name = 0;
        if (!gltf_material->name) {
            static int placeholder_material_id = 1;
            // Load meshes (ta_model -> ta_piece) and primitives (ta_mesh)
            char name_buf[256] = { 0 };

            const size_t name_size = ARRAY_SIZE(name_buf);
            size_t name_len = snprintf(name_buf, name_size, "gltf_material_%04d", placeholder_material_id++);
            DLB_ASSERT(name_len < name_size);
            temp_material_name = ta_symbol_intern(name_buf, name_len);
        } else {
            temp_material_name = gltf_material->name;
        }
        DLB_ASSERT(temp_material_name);

        char material_name[256] = { 0 };
        const size_t material_name_size = ARRAY_SIZE(material_name);
        size_t material_name_len = snprintf(material_name, material_name_size, "#%s", temp_material_name);
        DLB_ASSERT(material_name_len < material_name_size);

        ta_log_write(&tg_debug_log, SRC_GLTF, "ta_game_alloc MATERIAL %s\n", material_name);
        ta_material *material = ta_game_alloc(RES_MATERIAL, material_name, material_name_len);

        DLB_ASSERT(gltf_material->has_pbr_metallic_roughness);
        //DLB_ASSERT(!gltf_material->has_pbr_specular_glossiness);  // NOTE: Some files have both?? (e.g. bee.glb)

        gltf_texture(&material->albedo_texture, &gltf_material->pbr_metallic_roughness.base_color_texture);
        material->albedo_factor.r = gltf_material->pbr_metallic_roughness.base_color_factor[0];
        material->albedo_factor.g = gltf_material->pbr_metallic_roughness.base_color_factor[1];
        material->albedo_factor.b = gltf_material->pbr_metallic_roughness.base_color_factor[2];
        material->albedo_factor.a = gltf_material->pbr_metallic_roughness.base_color_factor[3];

        // Split 2-channel metallic/roughness into separate textures
        gltf_metallic_roughness(&material->metallic_texture, &material->roughness_texture,
            &gltf_material->pbr_metallic_roughness.metallic_roughness_texture);
        material->metallic_factor = gltf_material->pbr_metallic_roughness.metallic_factor;
        material->roughness_factor = gltf_material->pbr_metallic_roughness.roughness_factor;

        gltf_texture(&material->normal_texture, &gltf_material->normal_texture);
        gltf_texture(&material->occlusion_texture, &gltf_material->occlusion_texture);
        gltf_texture(&material->emission_texture, &gltf_material->emissive_texture);
        material->emission_factor.r = gltf_material->emissive_factor[0];
        material->emission_factor.g = gltf_material->emissive_factor[1];
        material->emission_factor.b = gltf_material->emissive_factor[2];

#if 0
        // TODO: Handle this stuff..
        cgltf_alpha_mode alpha_mode;
        cgltf_float alpha_cutoff;
        cgltf_bool double_sided;
        cgltf_bool unlit;
#endif

        ta_material_init(material);
    }

    // Load meshes (ta_model -> ta_piece) and primitives (ta_mesh)
    char model_name_buf[256] = { 0 };
    char mesh_name_buf[256] = { 0 };
    dlb_vec_each(cgltf_mesh *, gltf_mesh, gltf->data->meshes_v) {
        const char *entity_name = 0;
        if (!gltf_mesh->name) {
            static int placeholder_mesh_id = 1;
            // Load meshes (ta_model -> ta_piece) and primitives (ta_mesh)
            char name_buf[256] = { 0 };

            const size_t name_size = ARRAY_SIZE(name_buf);
            size_t name_len = snprintf(name_buf, name_size, "gltf_mesh_%04d", placeholder_mesh_id++);
            DLB_ASSERT(name_len < name_size);
            entity_name = ta_symbol_intern(name_buf, name_len);
        } else {
            entity_name = ta_symbol_intern(gltf_mesh->name, strlen(gltf_mesh->name));
        }
        DLB_ASSERT(entity_name);

        const size_t model_name_size = ARRAY_SIZE(model_name_buf);
        size_t model_name_len = snprintf(model_name_buf, model_name_size, "%s", entity_name);
        DLB_ASSERT(model_name_len < model_name_size);

        ta_log_write(&tg_debug_log, SRC_GLTF, "ta_game_alloc MODEL %s\n", model_name_buf);
        ta_model *model = ta_game_component_try(entity_name, RES_COMP_MODEL);
        if (!model) {
            ta_log_write(&tg_debug_log, SRC_GLTF, "Couldn't find model '%.*s', letting GLTF loader create one\n",
                model_name_len, model_name_buf);
            model = ta_game_component_add(entity_name, RES_COMP_MODEL, model_name_buf, model_name_len);
        }
        DLB_ASSERT(model);

        model->cast_shadows = true;
        model->receive_shadows = true;

        // HACK: Clear existing stuff that may have been serialized (e.g. #references)
        dlb_vec_zero((char **)model->anim_targets);
        dlb_vec_zero((char **)model->anim_target_weights);
        dlb_vec_zero(model->pieces);

        dlb_vec_each(char **, gltf_target, gltf_mesh->target_names_v) {
            const char *target = ta_symbol_intern(*gltf_target, strlen(*gltf_target));
            dlb_vec_push(model->anim_targets, target);
            dlb_vec_push(model->anim_target_weights, 0.0f);
        }

        int prim_idx = 0;
        dlb_vec_each(cgltf_primitive *, gltf_prim, gltf_mesh->primitives_v) {
            const size_t mesh_name_size = ARRAY_SIZE(mesh_name_buf);
            size_t mesh_name_len = snprintf(mesh_name_buf, mesh_name_size, "#%s.prim%03d", model->name, prim_idx);
            DLB_ASSERT(mesh_name_len < mesh_name_size);

            char material_name_buf[256] = { 0 };
            const size_t material_name_size = ARRAY_SIZE(material_name_buf);
            size_t material_name_len = snprintf(material_name_buf, material_name_size, "#%s", gltf_prim->material->name);
            DLB_ASSERT(material_name_len < material_name_size);
            const char *material_name = ta_symbol_intern(material_name_buf, material_name_len);

            ta_log_write(&tg_debug_log, SRC_GLTF, "ta_game_alloc MESH %s\n", mesh_name_buf);
            ta_mesh *mesh = ta_game_alloc(RES_MESH, mesh_name_buf, mesh_name_len);

            DLB_ASSERT(gltf_prim->type == cgltf_primitive_type_triangles);

            ta_log_write(&tg_debug_log, SRC_GLTF, "copying attributes\n");
            dlb_vec_each(cgltf_attribute *, attr, gltf_prim->attributes_v) {
                gltf_mesh_accessor(mesh, attr->data, attr->type, 0);
            }
            if (gltf_prim->indices->count) {
                cgltf_accessor *accessor = gltf_prim->indices;
                //cgltf_size data_size = accessor->buffer_view->size;
                void *data = (char *)accessor->buffer_view->buffer->data + accessor->buffer_view->offset;
                DLB_ASSERT(data);

                DLB_ASSERT(accessor->type == cgltf_type_scalar);
                ta_index_array *index_array = dlb_vec_alloc(mesh->index_arrays);
                dlb_vec_alloc_count(index_array->values, accessor->count);

                // TODO(perf): Make sure all input data is already the correct size
                // Convert indices to u32
                switch (accessor->component_type)
                {
                    case cgltf_component_type_r_8:
                    case cgltf_component_type_r_8u:
                        for (cgltf_size i = 0; i < accessor->count; ++i) {
                            index_array->values[i] = ((u8 *)data)[i];
                        }
                        break;
                    case cgltf_component_type_r_16:
                    case cgltf_component_type_r_16u:
                        for (cgltf_size i = 0; i < accessor->count; ++i) {
                            index_array->values[i] = ((u16 *)data)[i];
                        }
                        break;
                    case cgltf_component_type_r_32u:
                        //for (cgltf_size i = 0; i < accessor->count; ++i) {
                        //    index_array->values[i] = ((u32 *)data)[i];
                        //}
                        //break;
                    case cgltf_component_type_r_32f:
                    case cgltf_component_type_invalid:
                    default:
                        DLB_ASSERT(!"invalid index component type");
                        break;
                }

                index_array->material_slot = (u32)dlb_vec_len(model->materials);
            }

            dlb_vec_push(model->materials, material_name);

            ta_piece piece = { 0 };
            piece.mesh = ta_symbol_intern(mesh_name_buf, mesh_name_len);
            piece.material = ta_symbol_intern(material_name_buf, material_name_len);

            // Load animation target meshes
            char target_name_buf[256] = { 0 };
            const size_t target_name_size = ARRAY_SIZE(target_name_buf);
            size_t target_idx = 0;
            dlb_vec_each(cgltf_morph_target *, target, gltf_prim->targets_v) {
#if 1
                size_t target_name_len = 0;
                if (target_idx < dlb_vec_len(model->anim_targets)) {
                    target_name_len = snprintf(target_name_buf, target_name_size, "%s", model->anim_targets[target_idx]);
                } else {
                    DLB_ASSERT(!"Target names missing from gltf_mesh, this will break finding targets later");
                    target_name_len = snprintf(target_name_buf, target_name_size, "target_%03zu", target_idx);
                }
                DLB_ASSERT(target_name_len < target_name_size);
                const char *target_name = ta_symbol_intern(target_name_buf, target_name_len);

                ta_log_write(&tg_debug_log, SRC_GLTF, "copying target attributes\n");
                dlb_vec_each(cgltf_attribute *, attr, target->attributes_v) {
                    gltf_mesh_accessor(mesh, attr->data, attr->type, target_idx + 1);
                }
                dlb_vec_push(piece.anim_targets, target_name);
                target_idx++;
#else
                size_t target_name_len = 0;
                if (target_idx < dlb_vec_len(model->anim_targets)) {
                    target_name_len = snprintf(target_name, target_name_size, "%s.%s", mesh->name, model->anim_targets[target_idx]);
                } else {
                    DLB_ASSERT(!"Target names missing from gltf_mesh, this will break finding targets later");
                    target_name_len = snprintf(target_name, target_name_size, "%s.%03d", mesh->name, target_idx);
                }
                DLB_ASSERT(target_name_len < target_name_size);

                ta_log_write(&tg_debug_log, SRC_GLTF, "ta_game_alloc MESH %s\n", target_name);
                ta_mesh *target_mesh = ta_game_alloc(RES_MESH, target_name, target_name_len);

                ta_log_write(&tg_debug_log, SRC_GLTF, "copying attributes\n");
                dlb_vec_each(cgltf_attribute *, attr, target->attributes_v) {
                    gltf_mesh_accessor(target_mesh, attr->data, attr->type);
                }

                ta_log_write(&tg_debug_log, SRC_GLTF, "ta_mesh_create (target)\n");
                ta_mesh_create(target_mesh);
                dlb_vec_push(piece.anim_targets, target_mesh->name);
                target_idx++;
#endif
            }

            dlb_vec_push(model->pieces, piece);

            ta_log_write(&tg_debug_log, SRC_GLTF, "ta_mesh_create\n");
            ta_mesh_create(mesh);
            ta_log_write(&tg_debug_log, SRC_GLTF, "ta_mesh_init_normals\n");
            ta_mesh_init_normals(mesh, 0.1f);

            prim_idx++;
        }
    }
    ta_log_write(&tg_debug_log, SRC_GLTF, "successfully loaded meshes for %s\n", gltf->filename);
}

void ta_gltf_free(ta_gltf *gltf)
{
    cgltf_free(gltf->data);
}