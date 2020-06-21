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

void ta_lighting_init(ta_lighting *state)
{
    glGenBuffers(1, &state->gl_ubo_lights);
    glBindBufferBase(GL_UNIFORM_BUFFER, TA_GL_UNIFORM_BLOCK_BINDING_LIGHTS, state->gl_ubo_lights);
    glBindBuffer(GL_UNIFORM_BUFFER, state->gl_ubo_lights);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(ta_lighting_record) * TA_LIGHTING_MAX_ACTIVE_LIGHTS, 0, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void ta_lighting_bind_lights(ta_lighting *state)
{
    ta_light *lights = ta_game_resource_pool(RES_COMP_LIGHT);
    int light_idx = 0;
    for (int i = 0; i < dlb_vec_len(lights) && i < TA_LIGHTING_MAX_ACTIVE_LIGHTS; ++i) {
        ta_light *light = &lights[i];
        if (light->disabled) {
            continue;
        }

        // Set default values (some are overridden for specific light types below)
        state->light_records[light_idx].type            = light->type;
        state->light_records[light_idx].position        = ta_light_position(light);
        state->light_records[light_idx].color           = *(ta_vec3 *)&light->color;
        state->light_records[light_idx].intensity       = light->intensity;
        state->light_records[light_idx].cast_shadows    = light->cast_shadows;
        state->light_records[light_idx].direction       = VEC3_ZERO;
        state->light_records[light_idx].light_pv        = MAT4_IDENT;
        state->light_records[light_idx].shadowmap_zfar  = 0.0f;
        //u_shadowmap2d->value.sampler_2d    = 0;
        //u_shadowmap3d->value.sampler_cube = 0;

        // Light type-dependent properties
        switch (light->type) {
            case TA_LIGHT_AMBIENT:
                break;
            case TA_LIGHT_DIRECTIONAL:
                state->light_records[light_idx].direction = ta_light_direction(light);
                state->light_records[light_idx].light_pv = ta_light_pv(light);
                //u_shadowmap2d->value.sampler_2d = light->shadowmap.texture.gl_id;
                break;
            case TA_LIGHT_POINT:
                state->light_records[light_idx].shadowmap_zfar  = light->shadowmap.zfar;
                //u_shadowmap3d->value.sampler_cube = light->shadowmap.texture.gl_id;
                break;
            case TA_LIGHT_SPOT:
                state->light_records[light_idx].direction = ta_light_direction(light);
                //u_shadowmap2d->value.sampler_2d = light->shadowmap.texture.gl_id;
                DLB_ASSERT(!"Don't handle spot lights yet");
                break;
            default:
                DLB_ASSERT(!"Don't know how to initialize this type of light");
        }

        // HACK: xform_world isn't set yet, so we need to fix light positions until this code runs every frame
        ta_transform *transform = ta_game_component(light->entity, RES_COMP_TRANSFORM);
        state->light_records[light_idx].position = transform->xform.position;

        light_idx++;
    }

    glBindBuffer(GL_UNIFORM_BUFFER, state->gl_ubo_lights);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(ta_lighting_record) * TA_LIGHTING_MAX_ACTIVE_LIGHTS, state->light_records,
        GL_DYNAMIC_DRAW);
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
    if (!light->intensity) {
        light->intensity = DEFAULT_LIGHT_INTENSITY;
    }
    if (light->type != TA_LIGHT_AMBIENT) {
        if (!light->shadowmap.resolution) {
            light->shadowmap.resolution = DEFAULT_SHADOWMAP_RESOLUTION;
        }
        if (!light->shadowmap.znear) {
            light->shadowmap.znear = DEFAULT_SHADOWMAP_ZNEAR;
        }
        if (!light->shadowmap.zfar) {
            light->shadowmap.zfar = DEFAULT_SHADOWMAP_ZFAR;
        }
    }
    switch (light->type) {
        case TA_LIGHT_AMBIENT: {
            DLB_ASSERT(!light->cast_shadows);
            break;
        } case TA_LIGHT_DIRECTIONAL: {
            const float ortho = 50.0f;
            light->shadowmap.projection = mat4_ortho(
                -ortho, ortho, -ortho, ortho,
                light->shadowmap.znear, light->shadowmap.zfar
            );
            shadowmap_directional_create(light);
            break;
        } case TA_LIGHT_POINT: {
            light->shadowmap.projection = mat4_perspective(
                90.0f, 1.0f, light->shadowmap.znear, light->shadowmap.zfar
            );
            shadowmap_point_create(light);
            break;
        } case TA_LIGHT_SPOT: {
            light->shadowmap.projection = mat4_perspective(
                45.0f, 1.0f, light->shadowmap.znear, light->shadowmap.zfar
            );
            break;
        } default: {
            DLB_ASSERT(!"<UNKNOWN_TA_LIGHT_TYPE>");
        }
    }
}

static void shadowmap_directional_create(ta_light *light)
{
    ta_log_write(&tg_debug_log, SRC_LIGHT, "shadowmap_directional_create\n");
    ta_log_write(&tg_debug_log, SRC_LIGHT, "texture_create_and_bind\n");
    // HACK: Should probably just make this as a regular texture? But then it shows up in texture selector... could
    // push a '#' in front of the name to make it as a system resource?
    light->shadowmap.texture.res_type = RES_TEXTURE;
    light->shadowmap.texture.type = TA_TEXTURE_2D;
    light->shadowmap.texture.width = light->shadowmap.resolution;
    light->shadowmap.texture.height = light->shadowmap.resolution;
    light->shadowmap.texture.gl_filter_min = GL_LINEAR; // GL_NEAREST;
    light->shadowmap.texture.gl_filter_mag = GL_LINEAR; // GL_NEAREST;
    ta_texture_init(&light->shadowmap.texture);

    ta_texture_create_and_bind(&light->shadowmap.texture);

    // TODO: Specify wrap mode as part of texture params, not sure if border
    // mode/color is worth refactoring out though.
    // https://learnopengl.com/Advanced-Lighting/Shadows/Shadow-Mapping
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[4] = { 0 };
    //float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    // TODO: Possibly use shadow samplers and depth comparison mode? (Faster PCF, but only 2x2?)
    // https://www.khronos.org/opengl/wiki/Sampler_Object#Comparison_mode

    ta_log_write(&tg_debug_log, SRC_LIGHT, "glTexImage2D\n");
    // TOOD: Not sure if this if these formats are worth refactoring out either
    // TODO: Should internalformat be GL_DEPTH_COMPONENT16 instead?
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, light->shadowmap.resolution, light->shadowmap.resolution, 0,
        GL_DEPTH_COMPONENT, GL_FLOAT, 0);

    ta_log_write(&tg_debug_log, SRC_LIGHT, "glGenFramebuffers\n");
    glGenFramebuffers(1, &light->shadowmap.framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, light->shadowmap.framebuffer);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, light->shadowmap.texture.gl_id, 0);
    // For reflection maps
    //glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, light->shadowmap.texture.gl_id, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    ta_log_write(&tg_debug_log, SRC_LIGHT, "glCheckFramebufferStatus\n");
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        DLB_ASSERT(!"Failed to set up framebuffer for some reason.");
    }

    ta_log_write(&tg_debug_log, SRC_LIGHT, "unbind buffer and texture\n");
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    ta_texture_unbind(&light->shadowmap.texture);
}

static void shadowmap_point_create(ta_light *light)
{
    ta_log_write(&tg_debug_log, SRC_LIGHT, "shadowmap_point_create\n");
    ta_log_write(&tg_debug_log, SRC_LIGHT, "texture_create_and_bind\n");
    // HACK: Should probably just make this as a regular texture? But then it shows up in texture selector... could
    // push a '#' in front of the name to make it as a system resource?
    light->shadowmap.texture.res_type = RES_TEXTURE;
    light->shadowmap.texture.type = TA_TEXTURE_CUBEMAP;
    light->shadowmap.texture.width = light->shadowmap.resolution;
    light->shadowmap.texture.height = light->shadowmap.resolution;
    light->shadowmap.texture.gl_filter_min = GL_LINEAR; // GL_NEAREST;
    light->shadowmap.texture.gl_filter_mag = GL_LINEAR; // GL_NEAREST;
    ta_texture_init(&light->shadowmap.texture);

    ta_texture_create_and_bind(&light->shadowmap.texture);

    ta_log_write(&tg_debug_log, SRC_LIGHT, "glTexImage2D\n");
    for (int i = 0; i < 6; ++i) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT16, light->shadowmap.resolution,
            light->shadowmap.resolution, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    }

    ta_log_write(&tg_debug_log, SRC_LIGHT, "glGenFramebuffers\n");
    glGenFramebuffers(1, &light->shadowmap.framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, light->shadowmap.framebuffer);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, light->shadowmap.texture.gl_id, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    ta_log_write(&tg_debug_log, SRC_LIGHT, "glCheckFramebufferStatus\n");
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        DLB_ASSERT(!"Failed to set up framebuffer for some reason.");
    }

    ta_log_write(&tg_debug_log, SRC_LIGHT, "unbind buffer and texture\n");
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    ta_texture_unbind(&light->shadowmap.texture);
}

ta_vec3 ta_light_position(ta_light *light)
{
    ta_transform *transform = ta_game_component(light->entity, RES_COMP_TRANSFORM);
    return transform->xform_world.position;
}
ta_vec3 ta_light_direction(ta_light *light)
{
    ta_transform *transform = ta_game_component(light->entity, RES_COMP_TRANSFORM);

    // Lights with identity orientation don't cast shadows; give them a nudge
    if (quat_equal(transform->xform_world.orientation, QUAT_IDENT)) {
        transform->xform_world.orientation.x += TA_EPSILON;
        transform->xform_world.orientation = quat_normalize(transform->xform_world.orientation);
    }
    // Default light direction is directly down
    ta_vec3 direction = vec3_rotate_quat(VEC3_NY, transform->xform_world.orientation);
    direction = vec3_normalize(direction);
    return direction;
}
ta_mat4 ta_light_pv(ta_light *light)
{
    ta_vec3 inv_dir = vec3_neg(ta_light_direction(light));
    ta_mat4 view = mat4_lookat(inv_dir, VEC3_ZERO, VEC3_Y);
    ta_mat4 light_pv = mat4_mul(&light->shadowmap.projection, &view);
    return light_pv;
}

// http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-16-shadow-mapping/#spot-lights
// Use texture2Dproj to account for perspective-divide
static void shadowpass_render_directional(ta_light *light, ta_transform *transforms)
{
    DLB_ASSERT(light->shadowmap.framebuffer);

    //ta_texture_pool *texture_pool = ta_game_texture_pool(light->shadowmap.texture.gl_texture_pool_index);
    //GLint prev_min = texture_pool->gl_filter_min;
    //GLint prev_mag = texture_pool->gl_filter_mag;
    //ta_texture_pool_bind(texture_pool);
    //ta_texture_pool_set_filter_mode(texture_pool, light->shadowmap.texture.gl_filter_min,
    //    light->shadowmap.texture.gl_filter_mag);

    ta_shader *shader = ta_game_by_sym(RES_SHADER, light->shadowmap.shader);
    ta_shader_bind(shader);

    // TODO: Draw into shadowmap from ortho big enough to cover camera
    // http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-16-shadow-mapping/#rendering-the-shadow-map
    // https://www.khronos.org/opengl/wiki/GLAPI/glBindFragDataLocation

    glViewport(0, 0, light->shadowmap.resolution, light->shadowmap.resolution);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, light->shadowmap.framebuffer);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        DLB_ASSERT(!"Failed to set up framebuffer for some reason.");
    }

    glClear(GL_DEPTH_BUFFER_BIT);

    ta_mat4 light_pv = ta_light_pv(light);
    dlb_vec_each(ta_transform *, transform, transforms) {
        ta_model *model = ta_game_component_try(transform->entity, RES_COMP_MODEL);
        if (model) {
            ta_model_shadow_pass(model, shader, &light_pv);
        }
    }

    ta_shader_unbind();
    //ta_texture_pool_set_filter_mode(texture_pool, prev_min, prev_mag);
    //ta_texture_pool_unbind(texture_pool);
}

// Draw into shadowmap from light perspective
// http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-16-shadow-mapping/#rendering-the-shadow-map
// https://www.khronos.org/opengl/wiki/GLAPI/glBindFragDataLocation
// https://gamedev.stackexchange.com/questions/19461/opengl-glsl-render-to-cube-map
static void shadowpass_render_point(ta_light *light, ta_transform *transforms)
{
    DLB_ASSERT(light->shadowmap.framebuffer);

    //ta_texture_pool *texture_pool = ta_game_texture_pool(light->shadowmap.texture.gl_texture_pool_index);
    //GLint prev_min = texture_pool->gl_filter_min;
    //GLint prev_mag = texture_pool->gl_filter_mag;
    //ta_texture_pool_bind(texture_pool);
    //ta_texture_pool_set_filter_mode(texture_pool, light->shadowmap.texture.gl_filter_min,
    //    light->shadowmap.texture.gl_filter_mag);

    ta_vec3 position = ta_light_position(light);
    ta_shader *shader = ta_game_by_sym(RES_SHADER, light->shadowmap.shader);
    ta_shader_set_vec3(shader, SYM_U_LIGHT_POS, &position);
    ta_shader_set_float(shader, SYM_U_LIGHT_ZFAR, light->shadowmap.zfar);

    // TODO: Cache lookat matrices in dlb_vec, store light_pos as lookat_pos and
    //       update if light_pos != lookat_pos (i.e. position has changed)
    // PERF: Cache may be slower than just recalculating every frame.. profile!
    ta_mat4 view[6];
    view[0] = mat4_lookat(position, vec3_add(position, VEC3_X),  VEC3_NY);
    view[1] = mat4_lookat(position, vec3_add(position, VEC3_NX), VEC3_NY);
    view[2] = mat4_lookat(position, vec3_add(position, VEC3_Y),  VEC3_Z);
    view[3] = mat4_lookat(position, vec3_add(position, VEC3_NY), VEC3_NZ);
    view[4] = mat4_lookat(position, vec3_add(position, VEC3_Z),  VEC3_NY);
    view[5] = mat4_lookat(position, vec3_add(position, VEC3_NZ), VEC3_NY);

    glViewport(0, 0, light->shadowmap.resolution, light->shadowmap.resolution);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, light->shadowmap.framebuffer);

    for (int face = 0; face < 6; ++face) {
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + face,
            light->shadowmap.texture.gl_id, 0);

        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            DLB_ASSERT(!"Failed to set up framebuffer for some reason.");
        }

        glClear(GL_DEPTH_BUFFER_BIT);

        ta_mat4 light_pv = mat4_mul(&light->shadowmap.projection, &view[face]);
        dlb_vec_each(ta_transform *, transform, transforms) {
            ta_model *model = ta_game_component_try(transform->entity, RES_COMP_MODEL);
            if (model) {
                ta_model_shadow_pass(model, shader, &light_pv);
            }
        }
    }

    ta_shader_unbind();
    //ta_texture_pool_set_filter_mode(texture_pool, prev_min, prev_mag);
    //ta_texture_pool_unbind(texture_pool);
}

void ta_light_shadowpass_render(ta_light *light, ta_transform *transforms)
{
    if (light->disabled && !light->cast_shadows) {
        return;
    }

    typedef void (* shadowpass_render)(ta_light *light, ta_transform *transforms);

    static shadowpass_render shadowpass_renderers[TA_LIGHT_COUNT] = {
        [TA_LIGHT_DIRECTIONAL] = shadowpass_render_directional,
        [TA_LIGHT_POINT]       = shadowpass_render_point,
    };

    if (shadowpass_renderers[light->type]) {
        shadowpass_renderers[light->type](light, transforms);
    } else {
        DLB_ASSERT(!"No shadowpass renderer for this light type");
    }
}

void render_shadowmap_debug_directional(ta_light *light, int x, int y)
{
    ta_shader_set_sampler_2d(tg_shader_quads, SYM_U_TEX,
        light->shadowmap.texture.gl_id);

    s32 resolution = light->shadowmap.resolution / 10;
    ta_rect rect = { 0 };
    rect.x = x;
    rect.y = y;
    rect.w = resolution;
    rect.h = resolution;
    ta_primitive_push_rect(0, rect, TA_COLOR_INVIS, UI_LAYER_EDIT_1);
    ta_primitive_render_mesh(&primitive_quads, tg_shader_quads, TA_TRIANGLES,
        true, true);

    ta_shader_set_sampler_2d(tg_shader_quads, SYM_U_TEX, 0);
}
void render_shadowmap_debug_point(ta_light *light, int x, int y)
{
    ta_shader_set_sampler_cube(tg_shader_cubemap, SYM_U_TEX,
        light->shadowmap.texture.gl_id);

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

    s32 resolution = light->shadowmap.resolution / 10;
    for (int face = 0; face < 6; face++) {
        ta_rect rect = { 0 };
        rect.x = x + resolution * face_grid[face].x;
        rect.y = y + resolution * face_grid[face].y;
        rect.w = resolution;
        rect.h = resolution;
        ta_shader_set_int(tg_shader_cubemap, SYM_U_FACE, face);
        ta_primitive_push_rect(0, rect, TA_COLOR_INVIS, UI_LAYER_EDIT_1);
        ta_primitive_render_mesh(&primitive_quads, tg_shader_cubemap,
            TA_TRIANGLES, true, true);
    }

    ta_shader_set_sampler_cube(tg_shader_cubemap, SYM_U_TEX, 0);
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

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        DLB_ASSERT("Failed to set up framebuffer for some reason.");
    }
}
#endif