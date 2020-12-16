#include "ta_game.h"
#include "ta_light.h"
#include "ta_log.h"
#include "ta_model.h"
#include "ta_primitive.h"
#include "ta_schema.h"
#include "ta_shader.h"
#include "ta_symbol.h"
#include "ta_texture.h"
#include "ta_transform.h"
#include "ta_window.h"
#include "dlb/dlb_vector.h"
#include "dlb/dlb_index.h"
#include "misc/glad.h"

#define DEFAULT_LIGHT_INTENSITY         1.0f
#define DEFAULT_SHADOWMAP_RESOLUTION    1024
#define DEFAULT_SHADOWMAP_ZNEAR         0.1f
#define DEFAULT_SHADOWMAP_ZFAR          100.0f

void ta_lighting_init(ta_lighting *lighting)
{
    // NOTE: This is necessary for std140 packing of ta_lighting_records in ubo_lights
    DLB_ASSERT(sizeof(bool) == sizeof(int));

    glGenBuffers(1, &lighting->gl_ubo_lights);
    glBindBufferBase(GL_UNIFORM_BUFFER, TA_GLSL_UBO_LIGHTS, lighting->gl_ubo_lights);
    glBindBuffer(GL_UNIFORM_BUFFER, lighting->gl_ubo_lights);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(ta_lighting_record) * TA_LIGHTING_MAX_ACTIVE_LIGHTS, 0, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    // TODO: If we want to use binding point 0 for other things in other shaders, then this mapping needs to be a bit
    // more abstract.
    glBindBufferBase(GL_UNIFORM_BUFFER, TA_GLSL_UBO_LIGHTS, lighting->gl_ubo_lights);
}

void ta_lighting_bind_lights(ta_lighting *lighting)
{
    ta_light *lights = (ta_light *)ta_game_resource_pool(RES_COMP_LIGHT);
    int light_idx = 0;
    for (size_t i = 0; i < dlb_vec_len(lights) && i < TA_LIGHTING_MAX_ACTIVE_LIGHTS; ++i) {
        ta_light *light = &lights[i];
        if (!light->enabled) {
            continue;
        }

        // Set default values (some are overridden for specific light types below)
        lighting->light_records[light_idx].type            = light->type;
        lighting->light_records[light_idx].intensity       = light->intensity;
        lighting->light_records[light_idx].cast_shadows    = false;
        lighting->light_records[light_idx].position        = ta_light_position(light);
        lighting->light_records[light_idx].color           = *(ta_vec3 *)&light->color;
        lighting->light_records[light_idx].direction       = VEC3_ZERO;
        lighting->light_records[light_idx].light_pv        = MAT4_IDENT;
        lighting->light_records[light_idx].shadowmap_zfar  = 0.0f;
        //u_shadowmap2d->value.sampler_2d   = 0;
        //u_shadowmap3d->value.sampler_cube = 0;

        // Light type-dependent properties
        switch (light->type) {
            case TA_LIGHT_AMBIENT: {
                break;
            } case TA_LIGHT_DIRECTIONAL: {
                lighting->light_records[light_idx].direction = ta_light_direction(light);
                ta_mat4 light_pv = ta_light_pv(light);
                lighting->light_records[light_idx].light_pv = mat4_transpose(&light_pv);
                lighting->light_records[light_idx].cast_shadows = light->data.directional.cast_shadows;

                ta_texture *tex = (ta_texture *)ta_game_by_sym(RES_TEXTURE, light->data.directional.shadow_map);
                lighting->light_records[light_idx].shadowmap_texture_pool_index = tex->gl_texture_pool_index;
                lighting->light_records[light_idx].shadowmap_texture_array_layer[0][0] = tex->gl_texture_pool_layer;
                break;
            } case TA_LIGHT_POINT: {
                lighting->light_records[light_idx].cast_shadows = light->data.point.cast_shadows;
                lighting->light_records[light_idx].shadowmap_zfar  = light->data.point.shadow_properties.zfar;

                // NOTE: Assume all textures are in the same pool (asserts)
                ta_texture *first_tex = (ta_texture *)ta_game_by_sym(RES_TEXTURE, light->data.point.shadow_map.textures[0]);
                lighting->light_records[light_idx].shadowmap_texture_pool_index = first_tex->gl_texture_pool_index;
                for (int layer = 0; layer < 6; layer++) {
                    ta_texture *tex = (ta_texture *)ta_game_by_sym(RES_TEXTURE, light->data.point.shadow_map.textures[layer]);
                    lighting->light_records[light_idx].shadowmap_texture_array_layer[layer][0] = tex->gl_texture_pool_layer;
                    DLB_ASSERT(tex->gl_texture_pool_index == lighting->light_records[light_idx].shadowmap_texture_pool_index);
                }
                break;
            } case TA_LIGHT_SPOT: {
                lighting->light_records[light_idx].direction = ta_light_direction(light);
                lighting->light_records[light_idx].cast_shadows = light->data.spot.cast_shadows;

                ta_texture *tex = (ta_texture *)ta_game_by_sym(RES_TEXTURE, light->data.spot.shadow_map);
                lighting->light_records[light_idx].shadowmap_texture_pool_index = tex->gl_texture_pool_index;
                lighting->light_records[light_idx].shadowmap_texture_array_layer[0][0] = tex->gl_texture_pool_layer;

                DLB_ASSERT(!"Don't handle spot lights yet");
                break;
            } default: {
                DLB_ASSERT(!"Don't know how to initialize this type of light");
            }
        }

        // HACK: xform_world isn't set yet, so we need to fix light positions until this code runs every frame
        ta_transform *transform = (ta_transform *)ta_game_component(light->entity, RES_COMP_TRANSFORM);
        lighting->light_records[light_idx].position = transform->xform.position;

        light_idx++;
    }

    glBindBuffer(GL_UNIFORM_BUFFER, lighting->gl_ubo_lights);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(lighting->light_records), lighting->light_records, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

static void shadowmap_directional_create(ta_light *light);
static void shadowmap_point_create(ta_light *light);

const char *ta_light_type_str(int type)
{
    switch(type) {
        case TA_LIGHT_AMBIENT:     return "TA_LIGHT_AMBIENT";
        case TA_LIGHT_DIRECTIONAL: return "TA_LIGHT_DIRECTIONAL";
        case TA_LIGHT_POINT:       return "TA_LIGHT_POINT";
        case TA_LIGHT_SPOT:        return "TA_LIGHT_SPOT";
        default: DLB_ASSERT(0);    return "TA_LIGHT_???";
    }
}

void ta_light_init(ta_light *light)
{
    // TODO: Branchless because I felt like it.. should actually look at generated code :D
    light->intensity += !light->intensity * DEFAULT_LIGHT_INTENSITY;

    switch (light->type) {
        case TA_LIGHT_AMBIENT: {
            break;
        }
        case TA_LIGHT_DIRECTIONAL: {
            // TODO: Branchless because I felt like it.. should actually look at generated code :D
            light->data.directional.shadow_properties.resolution +=
                !light->data.directional.shadow_properties.resolution * DEFAULT_SHADOWMAP_RESOLUTION;
            light->data.directional.shadow_properties.znear +=
                !light->data.directional.shadow_properties.znear * DEFAULT_SHADOWMAP_ZNEAR;
            light->data.directional.shadow_properties.zfar +=
                !light->data.directional.shadow_properties.zfar * DEFAULT_SHADOWMAP_ZFAR;

            const float ortho = 50.0f;
            light->data.directional.shadow_properties.projection = mat4_ortho(
                -ortho, ortho, -ortho, ortho,
                light->data.directional.shadow_properties.znear,
                light->data.directional.shadow_properties.zfar
            );
            shadowmap_directional_create(light);
            break;
        } case TA_LIGHT_POINT: {
            // TODO: Branchless because I felt like it.. should actually look at generated code :D
            light->data.point.shadow_properties.resolution +=
                !light->data.point.shadow_properties.resolution * DEFAULT_SHADOWMAP_RESOLUTION;
            light->data.point.shadow_properties.znear +=
                !light->data.point.shadow_properties.znear * DEFAULT_SHADOWMAP_ZNEAR;
            light->data.point.shadow_properties.zfar +=
                !light->data.point.shadow_properties.zfar * DEFAULT_SHADOWMAP_ZFAR;

            light->data.point.shadow_properties.projection = mat4_perspective(
                90.0f, 1.0f,
                light->data.point.shadow_properties.znear,
                light->data.point.shadow_properties.zfar
            );
            shadowmap_point_create(light);
            break;
        } case TA_LIGHT_SPOT: {
            // TODO: Branchless because I felt like it.. should actually look at generated code :D
            light->data.spot.shadow_properties.resolution +=
                !light->data.spot.shadow_properties.resolution * DEFAULT_SHADOWMAP_RESOLUTION;
            light->data.spot.shadow_properties.znear +=
                !light->data.spot.shadow_properties.znear * DEFAULT_SHADOWMAP_ZNEAR;
            light->data.spot.shadow_properties.zfar +=
                !light->data.spot.shadow_properties.zfar * DEFAULT_SHADOWMAP_ZFAR;

            light->data.spot.shadow_properties.projection = mat4_perspective(
                45.0f, 1.0f,
                light->data.spot.shadow_properties.znear,
                light->data.spot.shadow_properties.zfar
            );
            break;
        } default: {
            DLB_ASSERT(!"<UNKNOWN_TA_LIGHT_TYPE>");
        }
    }
}
void ta_light_init_void(void *light)
{
    ta_light_init(light);
}
static void shadowmap_directional_create(ta_light *light)
{
    ta_log_write(&tg_debug_log, SRC_LIGHT, "shadowmap_directional_create\n");

    char tex_name[256] = { 0 };
    size_t tex_name_len = 0;
    tex_name_len = snprintf(tex_name, sizeof(tex_name), "#%s_shadowmap", light->name);

    ta_texture *tex = (ta_texture *)ta_game_alloc(RES_TEXTURE, tex_name, tex_name_len);
    tex->res_type = RES_TEXTURE;
    tex->type = TA_TEXTURE_2D_ARRAY;
    tex->width = light->data.directional.shadow_properties.resolution;
    tex->height = light->data.directional.shadow_properties.resolution;
    tex->channels = 1;
    tex->pixels_format = GL_DEPTH_COMPONENT;
    tex->pixels_type = GL_FLOAT;
    tex->gl_internal_format = GL_DEPTH_COMPONENT;
    tex->gl_filter_min = GL_LINEAR; // GL_NEAREST;
    tex->gl_filter_mag = GL_LINEAR; // GL_NEAREST;
    ta_texture_init(tex);

    light->data.directional.shadow_map = tex->name;

    GLenum target = ta_texture_target(tex);
    ta_texture_pool *pool = ta_texture_texture_pool(tex);

    ta_texture_pool_bind(pool);

    glTexParameteri(target, GL_TEXTURE_MIN_FILTER, tex->gl_filter_min);
    glTexParameteri(target, GL_TEXTURE_MAG_FILTER, tex->gl_filter_mag);
    // TODO: Specify wrap mode as part of texture params, not sure if border
    // mode/color is worth refactoring out though.
    // https://learnopengl.com/Advanced-Lighting/Shadows/Shadow-Mapping
    glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[4] = { 0 };
    //float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(target, GL_TEXTURE_BORDER_COLOR, borderColor);

    // TODO: Possibly use shadow samplers and depth comparison mode? (Faster PCF, but only 2x2?)
    // https://www.khronos.org/opengl/wiki/Sampler_Object#Comparison_mode

    ta_log_write(&tg_debug_log, SRC_LIGHT, "glGenFramebuffers\n");
    glGenFramebuffers(1, &light->data.directional.framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, light->data.directional.framebuffer);
    glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, pool->gl_id, 0, tex->gl_texture_pool_layer);
    // For reflection maps
    //glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, pool->gl_id, 0, tex->gl_texture_pool_layer);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    ta_log_write(&tg_debug_log, SRC_LIGHT, "glCheckFramebufferStatus\n");
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        DLB_ASSERT(!"Failed to set up framebuffer for some reason.");
    }

    ta_log_write(&tg_debug_log, SRC_LIGHT, "unbind buffer and texture\n");
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    ta_texture_pool_unbind(pool);
}

static void shadowmap_point_create(ta_light *light)
{
    ta_log_write(&tg_debug_log, SRC_LIGHT, "shadowmap_point_create\n");

    // Create 6 textures of the same size/type
    char tex_name[256] = { 0 };
    size_t tex_name_len = 0;
    const char *face_names[6] = { "+X", "-X", "+Y", "-Y", "+Z", "-Z" };

    for (int face = 0; face < 6; ++face) {
        tex_name_len = snprintf(tex_name, sizeof(tex_name), "#%s_shadowmap%s", light->name, face_names[face]);

        ta_texture *tex = (ta_texture *)ta_game_alloc(RES_TEXTURE, tex_name, tex_name_len);
        tex->res_type = RES_TEXTURE;
        tex->type = TA_TEXTURE_2D_ARRAY;
        tex->width = light->data.point.shadow_properties.resolution;
        tex->height = light->data.point.shadow_properties.resolution;
        tex->channels = 1;
        tex->pixels_format = GL_DEPTH_COMPONENT;
        tex->pixels_type = GL_FLOAT;
        tex->gl_internal_format = GL_DEPTH_COMPONENT;
        tex->gl_filter_min = GL_LINEAR; // GL_NEAREST;
        tex->gl_filter_mag = GL_LINEAR; // GL_NEAREST;
        ta_texture_init(tex);

        light->data.point.shadow_map.textures[face] = tex->name;
    }

    ta_texture *tex = (ta_texture *)ta_game_by_sym(RES_TEXTURE, light->data.point.shadow_map.textures[0]);
    GLenum target = ta_texture_target(tex);

    ta_texture_pool *pool = ta_texture_texture_pool(tex);
    ta_texture_pool_bind(pool);

    glTexParameteri(target, GL_TEXTURE_MIN_FILTER, tex->gl_filter_min);
    glTexParameteri(target, GL_TEXTURE_MAG_FILTER, tex->gl_filter_mag);
    glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    //ta_log_write(&tg_debug_log, SRC_LIGHT, "glTexImage2D\n");
    //for (int i = 0; i < 6; ++i) {
    //    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT, tex->width, tex->height, 0,
    //        GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    //}

    ta_log_write(&tg_debug_log, SRC_LIGHT, "glGenFramebuffers\n");
    glGenFramebuffers(1, &light->data.point.framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, light->data.point.framebuffer);
    // TODO: Set frame buffer texture 6 times during render pass, or have 6 framebuffers
    //glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, tex->gl_id, 0);
    glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, pool->gl_id, 0, tex->gl_texture_pool_layer);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    ta_log_write(&tg_debug_log, SRC_LIGHT, "glCheckFramebufferStatus\n");
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        DLB_ASSERT(!"Failed to set up framebuffer for some reason.");
    }

    ta_log_write(&tg_debug_log, SRC_LIGHT, "unbind buffer and texture\n");
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    ta_texture_pool_unbind(pool);
}

ta_vec3 ta_light_position(ta_light *light)
{
    ta_transform *transform = (ta_transform *)ta_game_component(light->entity, RES_COMP_TRANSFORM);
    return transform->xform_world.position;
}
ta_vec3 ta_light_direction(ta_light *light)
{
    ta_transform *transform = (ta_transform *)ta_game_component(light->entity, RES_COMP_TRANSFORM);

    // Lights with identity orientation don't cast shadows; give them a nudge
    if (quat_equal(transform->xform_world.orientation, QUAT_IDENT)) {
        transform->xform_world.orientation.x += TA_EPSILON;
        transform->xform_world.orientation = quat_normalize(transform->xform_world.orientation);
    }
    // Default light direction is directly down
    ta_vec3 direction = quat_mul_vec3(transform->xform_world.orientation, VEC3_NY);
    direction = vec3_normalize(direction);
    return direction;
}
ta_mat4 ta_light_pv(ta_light *light)
{
    ta_vec3 inv_dir = vec3_neg(ta_light_direction(light));
    ta_mat4 view = mat4_lookat(inv_dir, VEC3_ZERO, VEC3_Y);
    const ta_mat4 *proj = 0;
    switch (light->type) {
        case TA_LIGHT_DIRECTIONAL:
            proj = &light->data.directional.shadow_properties.projection;
            break;
        case TA_LIGHT_POINT:
            // NOTE: This isn't used for point lights.. it would need to take face into account for the view matrix
            proj = &light->data.point.shadow_properties.projection;
            break;
        case TA_LIGHT_SPOT:
            proj = &light->data.spot.shadow_properties.projection;
            break;
        default:
            proj = &MAT4_IDENT;
            break;
    }
    ta_mat4 light_pv = mat4_mul(proj, &view);
    return light_pv;
}

// http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-16-shadow-mapping/#spot-lights
// Use texture2Dproj to account for perspective-divide
static void shadowpass_render_directional(ta_light *light, ta_model *models)
{
    if (!light->data.directional.cast_shadows) {
        return;
    }

    DLB_ASSERT(light->data.directional.framebuffer);

    //ta_texture_pool *texture_pool = (ta_texture_pool *)ta_game_texture_pool(light->shadowmap.texture.gl_texture_pool_index);
    //GLint prev_min = texture_pool->gl_filter_min;
    //GLint prev_mag = texture_pool->gl_filter_mag;
    //ta_texture_pool_bind(texture_pool);
    //ta_texture_pool_set_filter_mode(texture_pool, light->shadowmap.texture.gl_filter_min,
    //    light->shadowmap.texture.gl_filter_mag);

    ta_shader *shader = (ta_shader *)ta_game_by_sym(RES_SHADER, light->data.directional.shadow_properties.shader);
    ta_shader_bind(shader);

    // TODO: Draw into shadowmap from ortho big enough to cover camera
    // http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-16-shadow-mapping/#rendering-the-shadow-map
    // https://www.khronos.org/opengl/wiki/GLAPI/glBindFragDataLocation

    glViewport(0, 0, light->data.directional.shadow_properties.resolution,
        light->data.directional.shadow_properties.resolution);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, light->data.directional.framebuffer);

    // NOTE: This is extremely expensive, only do a sanity check on the first frame
    static int fb_check = 0;
    if (!fb_check) {
        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            DLB_ASSERT(!"Failed to set up framebuffer for some reason.");
        }
        fb_check = 1;
    }

    glClear(GL_DEPTH_BUFFER_BIT);

#if 1
    ta_mat4 light_pv = ta_light_pv(light);
#else
    ta_vec3 inv_dir = vec3_neg(ta_light_direction(light));
    ta_mat4 view = mat4_lookat(inv_dir, VEC3_ZERO, VEC3_Y);
    ta_mat4 light_pv = mat4_mul(&light->data.directional.shadow_properties.projection, &view);
#endif
    dlb_vec_each(ta_model *, model, models) {
        ta_model_shadow_pass(model, shader, &light_pv);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    ta_shader_unbind();
    //ta_texture_pool_set_filter_mode(texture_pool, prev_min, prev_mag);
    //ta_texture_pool_unbind(texture_pool);
}

// Draw into shadowmap from light perspective
// http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-16-shadow-mapping/#rendering-the-shadow-map
// https://www.khronos.org/opengl/wiki/GLAPI/glBindFragDataLocation
// https://gamedev.stackexchange.com/questions/19461/opengl-glsl-render-to-cube-map
static void shadowpass_render_point(ta_light *light, ta_model *models)
{
    if (!light->data.point.cast_shadows) {
        return;
    }

    DLB_ASSERT(light->data.point.framebuffer);

    //ta_texture_pool *texture_pool = (ta_texture_pool *)ta_game_texture_pool(light->shadowmap.texture.gl_texture_pool_index);
    //GLint prev_min = texture_pool->gl_filter_min;
    //GLint prev_mag = texture_pool->gl_filter_mag;
    //ta_texture_pool_bind(texture_pool);
    //ta_texture_pool_set_filter_mode(texture_pool, light->shadowmap.texture.gl_filter_min,
    //    light->shadowmap.texture.gl_filter_mag);

    ta_vec3 position = ta_light_position(light);
    ta_shader *shader = (ta_shader *)ta_game_by_sym(RES_SHADER, light->data.point.shadow_properties.shader);
    ta_shader_set_vec3(shader, SYM_U_LIGHT_POS, &position);
    ta_shader_set_float(shader, SYM_U_LIGHT_ZFAR, light->data.point.shadow_properties.zfar);

    ta_mat4 view[6] = { 0 };
    view[0] = mat4_lookat(position, vec3_add(position, VEC3_X),  VEC3_NY);
    view[1] = mat4_lookat(position, vec3_add(position, VEC3_NX), VEC3_NY);
    view[2] = mat4_lookat(position, vec3_add(position, VEC3_Y),  VEC3_Z);
    view[3] = mat4_lookat(position, vec3_add(position, VEC3_NY), VEC3_NZ);
    view[4] = mat4_lookat(position, vec3_add(position, VEC3_Z),  VEC3_NY);
    view[5] = mat4_lookat(position, vec3_add(position, VEC3_NZ), VEC3_NY);

    glViewport(0, 0, light->data.point.shadow_properties.resolution, light->data.point.shadow_properties.resolution);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, light->data.point.framebuffer);

    static int fb_check = 0;
    for (int face = 0; face < 6; ++face) {
        ta_texture *tex = (ta_texture *)ta_game_by_sym(RES_TEXTURE, light->data.point.shadow_map.textures[face]);
        ta_texture_pool *tex_pool = ta_texture_texture_pool(tex);
        glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, tex_pool->gl_id, 0, tex->gl_texture_pool_layer);

        // NOTE: This is extremely expensive, only do a sanity check on the first frame
        if (!fb_check) {
            GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
            if (status != GL_FRAMEBUFFER_COMPLETE) {
                DLB_ASSERT(!"Failed to set up framebuffer for some reason.");
            }
        }

        glClear(GL_DEPTH_BUFFER_BIT);

        ta_mat4 light_pv = mat4_mul(&light->data.point.shadow_properties.projection, &view[face]);
        dlb_vec_each(ta_model *, model, models) {
            ta_model_shadow_pass(model, shader, &light_pv);
        }
    }
    fb_check = 1;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    ta_shader_unbind();
    //ta_texture_pool_set_filter_mode(texture_pool, prev_min, prev_mag);
    //ta_texture_pool_unbind(texture_pool);
}

void ta_light_shadowpass_render(ta_light *light, ta_model *models)
{
    if (!light->enabled) {
        return;
    }

    typedef void (* shadowpass_render)(ta_light *light, ta_model *models);

    static shadowpass_render shadowpass_renderers[TA_LIGHT_COUNT] = {
        [TA_LIGHT_DIRECTIONAL] = shadowpass_render_directional,
        [TA_LIGHT_POINT]       = shadowpass_render_point,
    };

    if (shadowpass_renderers[light->type]) {
        shadowpass_renderers[light->type](light, models);
    } else {
        DLB_ASSERT(!"No shadowpass renderer for this light type");
    }
}

void render_shadowmap_debug_directional(ta_light *light, int x, int y)
{
    ta_texture *tex = (ta_texture *)ta_game_by_sym(RES_TEXTURE, light->data.directional.shadow_map);

    s32 resolution = (s32)(light->data.directional.shadow_properties.resolution / 10);
    ta_rect rect = { 0 };
    rect.x = x;
    rect.y = y;
    rect.w = resolution;
    rect.h = resolution;

    ta_shader_set_sampler_2d(tg_shader_quads, SYM_U_TEX, tex->gl_id);
    ta_primitive_push_rect(0, rect, TA_COLOR_INVIS, UI_LAYER_EDIT_1);
    ta_primitive_render_mesh(&primitive_quads, tg_shader_quads, TA_TRIANGLES, true, true);
    ta_shader_set_sampler_2d(tg_shader_quads, SYM_U_TEX, 0);
}
void render_shadowmap_debug_point(ta_light *light, int x, int y)
{
    ta_texture *tex[6] = { 0 };

    tex[0] = ta_game_by_sym(RES_TEXTURE, light->data.point.shadow_map.textures[0]);
    tex[1] = ta_game_by_sym(RES_TEXTURE, light->data.point.shadow_map.textures[1]);
    tex[2] = ta_game_by_sym(RES_TEXTURE, light->data.point.shadow_map.textures[2]);
    tex[3] = ta_game_by_sym(RES_TEXTURE, light->data.point.shadow_map.textures[3]);
    tex[4] = ta_game_by_sym(RES_TEXTURE, light->data.point.shadow_map.textures[4]);
    tex[5] = ta_game_by_sym(RES_TEXTURE, light->data.point.shadow_map.textures[5]);

    // Render cubemap with the following layout:
    //      ------
    //      | +Y |
    // -----|----|----------
    // | -X | -Z | +X | +Z |
    // -----|----|----------
    //      | -Y |
    //      ------
    ta_vec2i face_grid[6] = {
        { 2, 1 },  // +X
        { 0, 1 },  // -X
        { 1, 0 },  // +Y
        { 1, 2 },  // -Y
        { 3, 1 },  // +Z
        { 1, 1 },  // -Z
    };

    s32 resolution = (s32)(light->data.point.shadow_properties.resolution / 10);
    for (int face = 0; face < 6; face++) {
        ta_rect rect = { 0 };
        rect.x = x + resolution * face_grid[face].x;
        rect.y = y + resolution * face_grid[face].y;
        rect.w = resolution;
        rect.h = resolution;

        ta_shader_set_sampler_2d(tg_shader_quads, SYM_U_TEX, tex[face]->gl_id);
        ta_primitive_push_rect(0, rect, TA_COLOR_INVIS, UI_LAYER_EDIT_1);
        ta_primitive_render_mesh(&primitive_quads, tg_shader_quads, TA_TRIANGLES, true, true);
    }

    ta_shader_set_sampler_2d(tg_shader_quads, SYM_U_TEX, 0);
}
void ta_light_render_shadowmap_debug(ta_light *light, int x, int y)
{
    typedef void (* shadowmap_render)(ta_light *light, int x, int y);

    static shadowmap_render shadowmap_renderers[TA_LIGHT_COUNT] = {
        [TA_LIGHT_DIRECTIONAL] = render_shadowmap_debug_directional,
        [TA_LIGHT_POINT]       = render_shadowmap_debug_point,
    };

    if (shadowmap_renderers[light->type]) {
        shadowmap_renderers[light->type](light, x, y);
    }
}

#if 0
// TODO: Use a color/depth framebuffer for postfx
void ta_framebuffer_postfx()
{
    glGenFramebuffers(1, &light->shadow_framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, light->shadow_framebuffer);

    GLuint render_texture;
    glGenTextures(1, &render_texture);
    glBindTexture(GL_TEXTURE_2D, render_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tg_window.width, tg_window.height, 0,
        GL_RGB, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    GLuint depth_buffer;
    glGenRenderbuffers(1, &depth_buffer);
    glBindRenderbuffer(GL_RENDERBUFFER, depth_buffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, tg_window.width, tg_window.height);

    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, render_texture, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_buffer);

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    // NOTE: Redundant for a single color attachment; same as default
    GLenum draw_buffers[] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(1, draw_buffers);

    // NOTE: This is extremely expensive, only do a sanity check on the first frame
    static int fb_check = 0;
    if (!fb_check) {
        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            DLB_ASSERT("Failed to set up framebuffer for some reason.");
        }
        fb_check = 1;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
#endif