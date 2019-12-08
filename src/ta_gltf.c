#include "ta_gltf.h"
#include "ta_buffer.h"
#include "ta_json.h"
#include "ta_log.h"
#include "dlb/dlb_types.h"
#include "dlb/dlb_vector.h"
#include "misc/jsmn.h"
#define CGLTF_IMPLEMENTATION
#include "misc/cgltf.h"
#include <stdio.h>

#define gltf_each(t, i, s) for (t (i) = (s), *(e) = (s + s##_count); (i) != (e); (i)++)

void gltf_dump(cgltf_data *data)
{
    printf("Type: %s\n", data->file_type == cgltf_file_type_gltf ? "GLTF" : "GLB");
    void* file_data;

    cgltf_asset asset;

    printf("asset:\n");
    printf("  copyright: %s\n", data->asset.copyright);
    printf("  generator: %s\n", data->asset.generator);
    printf("  version: %s\n", data->asset.version);
    printf("  min_version: %s\n", data->asset.min_version);

    printf("extensions_used: %zu\n", data->extensions_used_count);
    if (data->extensions_used_count) {
        for (size_t i = 0; i < data->extensions_used_count; ++i) {
            printf("  name: %s\n", data->extensions_used[i]);
        }
    }
    printf("extensions_required: %zu\n", data->extensions_required_count);
    if (data->extensions_required_count) {
        for (size_t i = 0; i < data->extensions_required_count; ++i) {
            printf("  name: %s\n", data->extensions_required[i]);
        }
    }

    printf("json:\n");
    printf("  size: %zu bytes\n", data->json_size);
    printf("  data: <json blob>\n");
    //printf("  data: %.*s\n", data->json_size, data->json);

    printf("bin:\n");
    printf("  size: %zu bytes\n", data->bin_size);
    printf("  data: <binary blob>\n");

    printf("accessors: %zu\n", data->accessors_count);
    //if (data->accessors_count) {
    //    gltf_each(cgltf_accessor *, accessor, data->accessors) {
    //        printf("  type: %d\n", accessor->type);
    //    }
    //}
    printf("animations: %zu\n", data->animations_count);
    if (data->animations_count) {
        gltf_each(cgltf_animation *, animation, data->animations) {
            printf("  name: %s\n", animation->name);
        }
    }
    printf("buffer_views: %zu\n", data->buffer_views_count);
    //if (data->buffer_views) {
    //    gltf_each(cgltf_buffer_view *, buffer_view, data->buffer_views) {
    //        printf("  type: %d\n", buffer_view->type);
    //    }
    //}
    printf("buffers: %zu\n", data->buffers_count);
    if (data->buffers_count) {
        gltf_each(cgltf_buffer *, buffer, data->buffers) {
            printf("  size: %zu\n", buffer->size);
        }
    }
    printf("cameras: %zu\n", data->cameras_count);
    if (data->cameras_count) {
        gltf_each(cgltf_camera *, camera, data->cameras) {
            printf("  name: %s\n", camera->name);
        }
    }
    printf("images: %zu\n", data->images_count);
    if (data->images_count) {
        gltf_each(cgltf_image *, image, data->images) {
            printf("  name: %s\n", image->name);
        }
    }
    printf("lights: %zu\n", data->lights_count);
    if (data->lights_count) {
        gltf_each(cgltf_light *, light, data->lights) {
            printf("  name: %s\n", light->name);
        }
    }
    printf("meshes: %zu\n", data->meshes_count);
    if (data->meshes_count) {
        gltf_each(cgltf_mesh *, mesh, data->meshes) {
            printf("  name: %s\n", mesh->name);
            if (mesh->extras.start_offset) {
                u8 *extras = (u8 *)data->json + mesh->extras.start_offset;
                size_t extras_len = mesh->extras.end_offset - mesh->extras.start_offset;

                printf("  extras: (%zu bytes)\n", extras_len);
                printf("    json: %*.s\n", extras_len, extras);
                printf("    dump:\n");

                ta_buffer extras_json = { 0 };
                extras_json.data = extras;
                extras_json.length = extras_len;

                jsmntok_t *tokens = 0;
                ta_json_parse(extras, extras_json.length, &tokens);
                DLB_ASSERT(tokens);
                ta_json_dump(extras, tokens, dlb_vec_len(tokens), 3);
                dlb_vec_free(tokens);
            }
        }
    }
    printf("materials: %zu\n", data->materials_count);
    if (data->materials_count) {
        gltf_each(cgltf_material *, material, data->materials) {
            printf("  name: %s\n", material->name);
        }
    }
    printf("nodes: %zu\n", data->nodes_count);
    if (data->nodes_count) {
        gltf_each(cgltf_node *, node, data->nodes) {
            printf("  name: %s\n", node->name);
        }
    }
    printf("samplers: %zu\n", data->samplers_count);
    if (data->samplers_count) {
        gltf_each(cgltf_sampler *, sampler, data->samplers) {
            printf("  min_filter: %d\n", sampler->min_filter);
        }
    }
    printf("scenes: %zu\n", data->scenes_count);
    if (data->scenes_count) {
        gltf_each(cgltf_scene *, scene, data->scenes) {
            printf("  name: %s\n", scene->name);
        }
    }
    printf("skins: %zu\n", data->skins_count);
    if (data->skins_count) {
        gltf_each(cgltf_skin *, skin, data->skins) {
            printf("  name: %s\n", skin->name);
        }
    }
    printf("textures: %zu\n", data->textures_count);
    if (data->textures_count) {
        gltf_each(cgltf_texture *, texture, data->textures) {
            printf("  name: %s\n", texture->name);
        }
    }

    //cgltf_scene* scene;
    //cgltf_extras extras;

    printf("\n");
}

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

cgltf_result ta_gltf_parse(const char *filename)
{
    cgltf_options options = { 0 };
    cgltf_data* data = NULL;
    cgltf_result err;

    ta_log_write(&tg_debug_log, SRC_GLTF, "parsing %s\n", filename);
    err = cgltf_parse_file(&options, filename, &data);
    if (err) {
        printf("cgltf_parse_file error: %s", ta_cgltf_error_str[err]);
        return err;
    }

    ta_log_write(&tg_debug_log, SRC_GLTF, "loading buffers\n");
    err = cgltf_load_buffers(&options, data, filename);
    if (err) {
        printf("cgltf_load_buffers error: %s", ta_cgltf_error_str[err]);
        return err;
    }

    ta_log_write(&tg_debug_log, SRC_GLTF, "validating\n");
    err = cgltf_validate(data);
    if (err) {
        printf("cgltf_validate error: %s", ta_cgltf_error_str[err]);
        return err;
    }

    ta_log_write(&tg_debug_log, SRC_GLTF, "successfully loaded %s\n", filename);
    gltf_dump(data);

    cgltf_free(data);
    return err;
}

void ta_gltf_test()
{
    cgltf_result err = 0;
    //err = ta_gltf_parse("F:/Users/User/Rez/Models/bee.glb");
    DLB_ASSERT(!err);
}