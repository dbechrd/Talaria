#include "ta_audio.h"
#include "ta_buffer.h"
#include "ta_button.h"
#include "ta_camera.h"
#include "ta_editor.h"
#include "ta_entity.h"
#include "ta_file.h"
#include "ta_font.h"
#include "ta_game.h"
#include "ta_light.h"
#include "ta_log.h"
#include "ta_material.h"
#include "ta_mesh_group.h"
#include "ta_model.h"
#include "ta_parse.h"
#include "ta_position.h"
#include "ta_primitive.h"
#include "ta_rigid_body.h"
#include "ta_scene.h"
#include "ta_shader.h"
#include "ta_symbol.h"
#include "ta_texture.h"
#include "ta_token.h"
#include "ta_window.h"
#include "dlb/dlb_vector.h"
#include "dlb/dlb_index.h"
#include <stdlib.h>
#include <float.h>

static void scene_load_placeholders(ta_scene *scene)
{
    UNUSED(scene);
    // TODO: Fix fallback resources
#if 0
    // Fallback resources
    ta_texture *tex_albedo = ta_scene_alloc(scene, COMP_TEXTURE,
        INTERN("DEFAULT_TEXTURE_ALBEDO"));
    {
#if 0
        tex_albedo->path = INTERN("data/texture/default_1024_1024.png");
#else
        // Generate magenta/white grid pattern
        tex_albedo->width = 64;
        tex_albedo->height = 64;
        tex_albedo->channels = 3;
        tex_albedo->linear = true;
        u8 *albedo_pixels = 0;
        u32 bytes = tex_albedo->width * tex_albedo->height * tex_albedo->channels;
        dlb_vec_reserve(albedo_pixels, bytes);
        u8 toggle = 0;
        u8 toggle_width = 4;
        for (int y = 0; y < tex_albedo->height; y++) {
            if (y % toggle_width == 0) toggle = !toggle;
            for (int x = 0; x < tex_albedo->width; x++) {
                if (x % toggle_width == 0) toggle = !toggle;
                dlb_vec_push(albedo_pixels, 255);
                dlb_vec_push(albedo_pixels, toggle * 255);
                dlb_vec_push(albedo_pixels, 255);
            }
        }
        DLB_ASSERT(dlb_vec_len(albedo_pixels) == bytes);
        tex_albedo->pixels = albedo_pixels;
#endif
    }

    ta_texture *tex_metallic = ta_scene_alloc(scene, COMP_TEXTURE,
        INTERN("DEFAULT_TEXTURE_METALLIC"));
    {
#if 0
        tex_metallic->path = INTERN("data/texture/default_1024_1024.png");
#else
        tex_metallic->width = 1;
        tex_metallic->height = 1;
        tex_metallic->channels = 1;
        tex_metallic->linear = true;
        u8 *metallic = 0;
        dlb_vec_alloc(metallic);
        tex_metallic->pixels = metallic;
#endif
    }

    ta_material *material = ta_scene_alloc(scene, COMP_MATERIAL,
        INTERN("DEFAULT_MATERIAL"));
    // TODO: Hard-code default shader instead of hoping it's in the scene file
    material->shader_uid = INTERN("shader_mesh");
    material->texture_albedo_uid = tex_albedo->hnd.uid;
    material->texture_metallic_uid = tex_metallic->hnd.uid;

    ta_mesh_group *mesh_group = ta_scene_alloc(scene, COMP_MESH_GROUP,
        INTERN("DEFAULT_MESH_GROUP"));
    mesh_group->path = INTERN("data/mesh/default.obj");

    scene->components[COMP_MATERIAL][0] = material->hnd.uid;
    scene->components[COMP_MESH_GROUP][0] = mesh_group->hnd.uid;
#endif
}

void ta_scene_init(ta_scene *scene)
{
    DLB_ASSERT(scene->filename);
    if (!scene->name) {
        scene->name = scene->filename;  // TODO: Load name from scene file
    }

    // TODO(perf): Fine-tune reservations (e.g. scene header)
    // TODO(perf): This is a lot of back-to-back allocations, can we avoid?
    for (ta_resource_type type = 0; type < RES_COUNT; type++) {
        dlb_index_init(&scene->index_by_name[type], 128, 128);
    }
    scene_load_placeholders(scene);
}
// TODO: This should take a ta_buffer pointer. Load entire file into memory
//       and refactor all of the e.g. read_char and expect_char logic out from
//       ta_file into ta_buffer.
void ta_scene_load(ta_scene *scene, ta_file *file)
{
    ta_log_write(&tg_debug_log, SRC_SCENE, "Loading %s\n", file->filename);
    scene->filename = file->filename;
    scene->name = file->filename;  // TODO: Load name from scene file
    ta_scene_init(scene);

    // TODO: Reserve arrays based on scene header (which doesn't exist yet)
    //dlb_vec_reserve(scene->entities, 2);
    token *tokens = tokenize(file);

    //tokens_print(tg_debug_log->stream, tokens);
    //tokens_print_debug(tg_debug_log.stream, tokens);
    tokens_parse(scene, tokens);
    dlb_vec_free(tokens);

    // Initialize resources
    // NOTE: Iterate backward to ensure resources are initialized before any
    // components that might depend on them.
    for (ta_resource_type res_type = RES_COUNT - 1; res_type >= 0; --res_type) {
        ta_schema_field_type schema_type = res_to_typ(res_type);
        if (tg_schemas[schema_type].init) {
            ta_log_write(&tg_debug_log, SRC_SCENE, "Initializing %s\n",
                ta_schema_field_type_str(schema_type));
            u32 size = tg_schemas[schema_type].size;
            void *pool = scene->resource_data[res_type];
            u8 *end = dlb_vec_end_size(pool, size);
            for (u8 *ptr = pool; ptr != end; ptr += size) {
                if (res_type == RES_SHADER) {
                    DLB_ASSERT(1);
                }
                tg_schemas[schema_type].init(ptr);
            }
        }
    }
}
void ta_scene_load_file(ta_scene *scene, const char *filename)
{
    //ta_buffer *buffer = ta_file_read_all(filename);
    //ta_scene *scene = ta_scene_load(buffer);
    ta_file *file = ta_file_open(filename, FILE_READ);
    ta_scene_load(scene, file);
}
void ta_scene_free(ta_scene *scene)
{
    // Free resources
    for (ta_resource_type res_type = 0; res_type < RES_COUNT; ++res_type) {
        ta_schema_field_type schema_type = res_to_typ(res_type);
        if (tg_schemas[schema_type].free) {
            u32 size = tg_schemas[schema_type].size;
            void *pool = scene->resource_data[res_type];
            u8 *end = dlb_vec_end_size(pool, size);
            for (u8 *ptr = pool; ptr != end; ptr += size) {
                tg_schemas[schema_type].free(ptr);
            }
        }
        dlb_vec_free(scene->resource_data[res_type]);
        dlb_index_free(&scene->index_by_name[res_type]);
    }
}
void ta_scene_save(ta_buffer *buffer)
{
    UNUSED(buffer);
    // TODO: Write scene to memory buffer
    DLB_ASSERT(0);
}
void ta_scene_save_file(ta_scene *scene, const char *filename)
{
    // TODO: Alloc dynamic buffer to write arbitrary data to
    //ta_buffer *buffer = ??
    ta_file *file = ta_file_open(filename, FILE_WRITE);
    ta_scene_print(scene, file->hnd);
    ta_file_close(file);
}
void ta_scene_print(ta_scene *scene, FILE *hnd)
{
    fprintf(hnd, "#-------------------------------------------------------------------------------\n");
    fprintf(hnd, "# [SCENE] %s\n", scene->name);
    for (ta_resource_type res_type = 0; res_type < RES_COUNT; ++res_type) {
        ta_schema_field_type schema_type = res_to_typ(res_type);
        fprintf(hnd, "#-------------------------------------------------------------------------------\n");
        fprintf(hnd, "# %s\n", ta_schema_field_type_str(schema_type));
        fprintf(hnd, "#-------------------------------------------------------------------------------\n");

        if (res_type == RES_MESH) {
            fprintf(hnd,
                "#\n"
                "# NOTE: We may have mesh properties at some point, but, for now, meshes are\n"
                "# created at run-time via mesh_groups.\n"
                "#\n"
            );
            continue;
        }

        u32 size = tg_schemas[schema_type].size;
        void *pool = scene->resource_data[res_type];
        u8 *end = dlb_vec_end_size(pool, size);
        for (u8 *ptr = pool; ptr != end; ptr += size) {
            ta_schema_print(hnd, schema_type, ptr, 0, 0);
        }
    }
    fflush(hnd);
}
void ta_scene_save_file_json(ta_scene *scene, const char *filename)
{
    // TODO: Alloc dynamic buffer to write arbitrary data to
    //ta_buffer *buffer = ??
    ta_file *file = ta_file_open(filename, FILE_WRITE);
    ta_scene_print_json(scene, file->hnd);
    ta_file_close(file);
}
void ta_scene_print_json(ta_scene *scene, FILE *f)
{
    fprintf(f, "{\n");
    for (ta_resource_type res_type = 0; res_type < RES_COUNT; ++res_type) {
        ta_schema_field_type schema_type = res_to_typ(res_type);
        // NOTE: We may have mesh properties at some point, but, for now, meshes are
        // created at run-time via mesh_groups.
        if (res_type == RES_MESH) {
            continue;
        }

        ta_schema *schema = &tg_schemas[schema_type];

        fprintf(f, "  \"%s\": [\n", schema->name);

        void *pool = scene->resource_data[res_type];
        u32 pool_len = dlb_vec_len(pool);
        u8 *ptr = pool;
        for (u32 i = 0; i < pool_len; ++i) {
            fprintf(f, "    {\n");
            ta_schema_print_json(f, schema_type, ptr, 2, 0);
            fprintf(f, "    }");
            if (pool_len && i < pool_len - 1) {
                fprintf(f, ",");
            }
            fprintf(f, "\n");
            ptr += schema->size;
        }

        fprintf(f, "  ]");
        if (RES_COUNT && res_type < RES_COUNT - 1) {
            fprintf(f, ",");
        }
        fprintf(f, "\n");
    }
    fprintf(f, "}\n");
    fflush(f);
}
void *ta_scene_alloc(ta_scene *scene, ta_resource_type type, const char *name)
{
    DLB_ASSERT(scene);
    DLB_ASSERT(type < RES_COUNT);
    DLB_ASSERT(name);

    ta_schema_field_type schema_type = res_to_typ(type);
    u32 size = tg_schemas[schema_type].size;

    ta_resource *res = dlb_vec_alloc_size(scene->resource_data[type], size);
    res->index = dlb_vec_len(scene->resource_data[type]) - 1;
    res->name = name;

    dlb_index *store = &scene->index_by_name[type];
    u32 hash = dlb_murmur3(SYM(name));
    dlb_index_insert(store, hash, res->index);

    if (tg_schemas[schema_type].init) {
        tg_schemas[schema_type].init(res);
    }

    return res;
}
void ta_scene_destroy(ta_scene *scene, ta_resource_type type, const char *name)
{
    DLB_ASSERT(scene);
    // TODO: if type is a component type, find and update parent entity:
    // entity->components[type] = 0
    DLB_ASSERT(type >= RES_COMP_COUNT && type < RES_COUNT);

    ta_schema_field_type schema_type = res_to_typ(type);
    u32 size = tg_schemas[schema_type].size;

    // TODO: Find resource
    DLB_ASSERT(0);
    ta_resource *res = 0000000;
    if (tg_schemas[schema_type].free) {
        tg_schemas[schema_type].free(res);
    }

    // Remove name from index
    dlb_index *store = &scene->index_by_name[type];
    u32 hash = dlb_murmur3(SYM(name));
    // TODO: Find index
    DLB_ASSERT(0);
    u32 index = 0000000;
    dlb_index_delete(store, hash, index);

    // TODO: Remove data from pool (swap last element into empty slot, then
    // update index for the moved element)
    DLB_ASSERT(0);
    //dlb_vec_delete(scene->resource_data[type], index);
}
void *ta_scene_find_at(ta_scene *scene, ta_resource_type type, u32 index)
{
    DLB_ASSERT(scene);
    DLB_ASSERT(type >= 0 && type < RES_COUNT);

    ta_schema_field_type schema_type = res_to_typ(type);
    u32 size = tg_schemas[schema_type].size;
    void *resource = dlb_vec_index_size(scene->resource_data[type], index, size);
    return resource;
}
// If not found, returns NULL
void *ta_scene_find_by_name_try(ta_scene *scene, ta_resource_type type,
    const char *name)
{
    DLB_ASSERT(scene);
    DLB_ASSERT(name);

    ta_schema_field_type schema_type = res_to_typ(type);
    u32 size = tg_schemas[schema_type].size;

    u32 hash = dlb_murmur3(SYM(name));
    dlb_index *store = &scene->index_by_name[type];
    for (u32 i = dlb_index_first(store, hash); i != DLB_INDEX_EMPTY; i = dlb_index_next(store, i)) {
        ta_resource *res = dlb_vec_index_size(scene->resource_data[type], i, size);
        if (res->name == name) {
            DLB_ASSERT(res->index == i);
            return res;
        }
    }
    return 0;
}
// If not found, ASSERT
void *ta_scene_find_by_name(ta_scene *scene, ta_resource_type type,
    const char *name)
{
    void *resource = ta_scene_find_by_name_try(scene, type, name);
    DLB_ASSERT(resource);
    return resource;
}
// If not found, returns the first resource of the given type
void *ta_scene_find_by_name_or_default(ta_scene *scene, ta_resource_type type,
    const char *name)
{
    ta_schema_field_type schema_type = res_to_typ(type);
    u32 size = tg_schemas[schema_type].size;

    void *resource = ta_scene_find_by_name_try(scene, type, name);
    if (!resource) {
        resource = dlb_vec_index_size(scene->resource_data[type], 0, size);
    }
    return resource;
}
void *ta_scene_component_add(ta_scene *scene, ta_resource_type type,
    const char *entity)
{
    DLB_ASSERT(scene);
    DLB_ASSERT(type >= 0 && type < RES_COMP_COUNT);

    // Prevent duplicates
    ta_component *component = ta_scene_component(scene, type, entity);
    DLB_ASSERT(!component);

    // TODO: Build better component name (or guarantee name == entity_name)
    component = ta_scene_alloc(scene, type, entity);
    DLB_ASSERT(component);
    component->entity_name = entity;
    return component;
}
void *ta_scene_component_try(ta_scene *scene, ta_resource_type type,
    const char *entity)
{
    DLB_ASSERT(scene);
    DLB_ASSERT(entity);
    DLB_ASSERT(type >= 0 && type < RES_COMP_COUNT);

    void *component = ta_scene_find_by_name_try(scene, type, entity);
    return component;
}
void *ta_scene_component(ta_scene *scene, ta_resource_type type,
    const char *entity)
{
    DLB_ASSERT(scene);
    DLB_ASSERT(entity);
    DLB_ASSERT(type >= 0 && type < RES_COMP_COUNT);

    void *component = ta_scene_component_try(scene, type, entity);
    DLB_ASSERT(component);
    return component;
}

static ta_rigid_body_pair *collision_broadphase(ta_scene *scene, double dt)
{
    // Box2D supports 16 collision categories. For each fixture you can
    // specify which category it belongs to. You also specify what other
    // categories this fixture can collide with.
    //
    //   if ((categoryA & maskB) != 0 && (categoryB & maskA) != 0)
    //
    // Collision groups let you specify an integral group index. You can
    // have all fixtures with the same group index always collide
    // (positive index) or never collide (negative index). Group indices
    // are usually used for things that are somehow related, like the
    // parts of a bicycle.
    //
    // Collisions between fixtures of different group indices are
    // filtered according the category and mask bits. In other words,
    // group filtering has higher precedence than category filtering.
    //
    // - A fixture on a static body can only collide with a dynamic
    //   body.
    // - A fixture on a kinematic body can only collide with a dynamic
    //   body.
    // - Fixtures on the same body never collide with each other.
    // - You can optionally enable/disable collision between fixtures on
    //   bodies connected by a joint.
    //
    // Sensor: Fixture which only detects collision, no response.
    // -----------------------------------------------------------------
    // Depth-first traversal of AABB tree to find islands. Put islands
    // to sleep when all objects in island are resting. Wake up when
    // anything interacts or applies a force to any body in the island.

    UNUSED(dt);

    static ta_rigid_body_pair *pairs = 0;

    ta_rigid_body *rigid_bodies = scene->resource_data[RES_COMP_RIGID_BODY];
    dlb_vec_each(ta_rigid_body *, a, rigid_bodies) {
        dlb_vec_range(ta_rigid_body *, b, a + 1, dlb_vec_end(rigid_bodies)) {
            // Don't let entities collide with themselves
            if (a->entity_name == b->entity_name) {
                continue;
            }
            if (ta_aabb_v_aabb(&a->aabb, &b->aabb, 0))
            {
                ta_rigid_body_pair *pair = dlb_vec_alloc(pairs);
                pair->a = a;
                pair->b = b;
            }
        }
    }

    return pairs;
}
static ta_manifold *detect_collisions(ta_rigid_body_pair *pairs, double dt)
{
    UNUSED(dt);

    static ta_manifold *manifolds = 0;

    ta_manifold manifold;
    dlb_vec_each(ta_rigid_body_pair *, pair, pairs) {
        if (ta_rigid_body_intersect(pair->a, pair->b, &manifold)) {
            ta_manifold *m = dlb_vec_alloc(manifolds);
            *m = manifold;
        }
    }

    return manifolds;
}
void ta_scene_update(ta_scene *scene, float dt)
{
    // https://www.toptal.com/game/video-game-physics-part-type-an-introduction-to-rigid-body-dynamics

    // Simulate rigid bodies
    dlb_vec_each(ta_rigid_body *, body, scene->resource_data[RES_COMP_RIGID_BODY]) {
        ta_rigid_body_update(body, dt);
    }

    // Collision broad phase
    ta_rigid_body_pair *pairs = collision_broadphase(scene, dt);
    if (pairs) {
        // Collision narrow phase
        ta_manifold *manifolds = detect_collisions(pairs, dt);
        dlb_vec_each(ta_manifold *, manifold, manifolds) {
            // Collision resolution
            ta_rigid_body_resolve_collision(manifold);
            ta_rigid_body_positional_correction(manifold);
        }
        dlb_vec_zero(manifolds);
        dlb_vec_zero(pairs);
    }

    // Update positions
    dlb_vec_each(ta_position *, position, scene->resource_data[RES_COMP_POSITION]) {
        position->transform_prev = position->transform;
    }
    dlb_vec_each(ta_rigid_body *, body, scene->resource_data[RES_COMP_RIGID_BODY]) {
        ta_position *position = ta_scene_component(scene, RES_COMP_POSITION,
            body->entity_name);
        position->transform.position = body->position;
        position->transform.orientation = body->orientation;
    }

    // Update buttons
    dlb_vec_each(ta_e_button *, button, scene->resource_data[RES_COMP_BUTTON]) {
        e_button_update(button);
    }

#if 0
    dlb_vec_each(ta_entity *, entity, scene->pools[TYP_BUTTON]) {
        ta_node_update(entity);
    }
#endif
}
void ta_scene_shadow_pass(ta_scene *scene, ta_shader *shader, float alpha)
{
    // TODO: Try VSM, then CSM.
    // NOTE: If we switch to back face culling it will prevent light leaks, but
    // cause a lot more jitter on the lit side. :(
    glCullFace(GL_FRONT);
    //glClearColor(FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    ta_shader_bind(shader);
    dlb_vec_each(ta_light *, light, scene->resource_data[RES_COMP_LIGHT]) {
        if (light->disabled) continue;
        if (light->type != TA_LIGHT_POINT) {
            // TODO: Handle shadows for other light types
            continue;
        }
        // TODO: Disable shadows per light (pass cast_shadows as light uniform)
        //if (!light->cast_shadows) continue;

        ta_shader_set_vec3(shader, SYM_U_LIGHT_POS, &light->position);
        ta_shader_set_float(shader, SYM_U_LIGHT_ZFAR, light->shadowmap.zfar);
        ta_light_shadowpass_render(light, shader, alpha,
            scene->resource_data[RES_COMP_MODEL]);

        // TODO: Make button a component that an entity can have (*button_uid)
        //       instead of having it contain entity. It probably needs to have
        //       (*entity_uid) pointer as well in order to find the rigid body?
        //       Alternatively, it can have an explicit rigid body of its own
        //       which defaults to entity->rigid_body on initialization.
        //ta_light_shadowpass_render(light, shader, alpha, scene->pools[TYP_BUTTON]);
    }
    ta_shader_unbind();
    glCullFace(GL_BACK);
    glViewport(0, 0, WINDOW_W, WINDOW_H);
}
void ta_scene_render(ta_scene *scene, ta_camera *render_camera, float alpha)
{
    //glCullFace(GL_BACK);
    //glDisable(GL_CULL_FACE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glStencilMask(0xFF);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glStencilMask(0x00);

    // TODO: This is going to make a zillion extranous calls
    static GLenum tg_polygon_mode = GL_FILL;
    GLenum camera_poly_mode = render_camera->debug_wireframe ? GL_LINE : GL_FILL;
    if (camera_poly_mode != tg_polygon_mode) {
        glPolygonMode(GL_FRONT_AND_BACK, camera_poly_mode);
        tg_polygon_mode = camera_poly_mode;
    }

    ta_shader_set_mat4(tg_shader_lines, SYM_U_PROJ, &render_camera->projection);
    ta_shader_set_mat4(tg_shader_lines, SYM_U_VIEW, &render_camera->look_at);
    ta_shader_set_mat4(tg_shader_quads, SYM_U_PROJ, &render_camera->projection);
    ta_shader_set_mat4(tg_shader_quads, SYM_U_VIEW, &render_camera->look_at);

    // TODO: Group by shader / material to minimize redundant uniform calls
    dlb_vec_each(ta_model *, model, scene->resource_data[RES_COMP_MODEL]) {
        ta_model_render(model, render_camera, alpha);
    }

    ta_shader_set_mat4(tg_shader_lines, SYM_U_MODEL, &MAT4_IDENT);
    ta_shader_set_mat4(tg_shader_quads, SYM_U_MODEL, &MAT4_IDENT);

#if 1
    dlb_vec_each(ta_camera *, camera, scene->resource_data[RES_COMP_CAMERA]) {
        if (camera->name != tg_e_active_camera) {
            ta_sphere sphere = { 0 };
            sphere.center = camera->position;
            sphere.radius = 0.2f;
            ta_primitive_push_rgb_sphere(sphere);
            //ta_primitive_push_sphere(sphere, TA_COLOR_GREEN);
        }
    }
    dlb_vec_each(ta_light *, light, scene->resource_data[RES_COMP_LIGHT]) {
        ta_sphere light_pos = { 0 };
        light_pos.center = light->position;
        light_pos.radius = 0.2f;
        ta_rgba color = { 0 };
        if (light->disabled) {
            color.r = 0.5f;
            color.g = 0.5f;
            color.b = 0.5f;
        } else {
            color.r = light->data.common.color.r;
            color.g = light->data.common.color.g;
            color.b = light->data.common.color.b;
        }
        ta_primitive_push_sphere(light_pos, color);

        //ta_sphere light_aoe = { 0 };
        //light_aoe.center = light->position;
        //light_aoe.radius = light->shadowmap.zfar;
        //ta_primitive_push_rgb_sphere(light_aoe);
    }

    ta_shader_set_mat4(tg_shader_lines, SYM_U_MODEL, &MAT4_IDENT);
    ta_shader_set_mat4(tg_shader_quads, SYM_U_MODEL, &MAT4_IDENT);
    ta_primitive_render(true, false);
#endif
}