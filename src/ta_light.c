#include "ta_entity.h"
#include "ta_game.h"
#include "ta_light.h"
#include "ta_model.h"
#include "ta_primitive.h"
#include "ta_shader.h"
#include "ta_symbol.h"
#include "ta_texture.h"
#include "ta_transform.h"
#include "ta_window.h"
#include "dlb/dlb_vector.h"
#include "dlb/dlb_index.h"
#include "misc/gl3w.h"

#define DEFAULT_LIGHT_INTENSITY         1.0f
#define DEFAULT_SHADOWMAP_RESOLUTION    1024
#define DEFAULT_SHADOWMAP_ZNEAR         0.1f
#define DEFAULT_SHADOWMAP_ZFAR          100.0f

static void shadowmap_directional_create(ta_light *light);
static void shadowmap_point_create(ta_light *light);

const char *ta_light_type_str(int type)
{
    switch(type) {
        case TA_LIGHT_AMBIENT:     return "TA_LIGHT_AMBIENT";
        case TA_LIGHT_DIRECTIONAL: return "TA_LIGHT_DIRECTIONAL";
        case TA_LIGHT_POINT:       return "TA_LIGHT_POINT";
        case TA_LIGHT_SPOT:        return "TA_LIGHT_SPOT";
        default:
            DLB_ASSERT(!"<UNKNOWN_TA_LIGHT_TYPE>");
            return 0;
    }
}

void ta_light_init(ta_light *light)
{
    if (!light->data.common.intensity) {
        light->data.common.intensity = DEFAULT_LIGHT_INTENSITY;
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
    light->shadowmap.texture.width = light->shadowmap.resolution;
    light->shadowmap.texture.height = light->shadowmap.resolution;
    light->shadowmap.texture.cubemap = false;

    glGenTextures(1, &light->shadowmap.texture.gl_id);
    glBindTexture(GL_TEXTURE_2D, light->shadowmap.texture.gl_id);

    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // https://learnopengl.com/Advanced-Lighting/Shadows/Shadow-Mapping
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[4] = { 0 };
    //float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // TODO: Should internalformat be GL_DEPTH_COMPONENT16 instead?
    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, light->shadowmap.resolution,
        light->shadowmap.resolution, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0
    );

    glGenFramebuffers(1, &light->shadowmap.framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, light->shadowmap.framebuffer);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
        light->shadowmap.texture.gl_id, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        DLB_ASSERT(!"Failed to set up framebuffer for some reason.");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

static void shadowmap_point_create(ta_light *light)
{
    light->shadowmap.texture.width = light->shadowmap.resolution;
    light->shadowmap.texture.height = light->shadowmap.resolution;
    light->shadowmap.texture.cubemap = true;

    glGenTextures(1, &light->shadowmap.texture.gl_id);
    glBindTexture(GL_TEXTURE_CUBE_MAP, light->shadowmap.texture.gl_id);
    //glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    //glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    for (int i = 0; i < 6; ++i) {
        glTexImage2D(
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT24,
            light->shadowmap.resolution, light->shadowmap.resolution, 0,
            GL_DEPTH_COMPONENT, GL_FLOAT, NULL
        );
    }

    glGenFramebuffers(1, &light->shadowmap.framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, light->shadowmap.framebuffer);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
        light->shadowmap.texture.gl_id, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        DLB_ASSERT(!"Failed to set up framebuffer for some reason.");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

ta_vec3 ta_light_position(ta_light *light)
{
    ta_transform *transform = ta_game_component(light->entity_name, RES_COMP_TRANSFORM);
    return transform->xform.position;
}
ta_vec3 ta_light_direction(ta_light *light)
{
    ta_transform *transform = ta_game_component(light->entity_name, RES_COMP_TRANSFORM);

    // Lights with identity orientation don't cast shadows; give them a nudge
    if (quat_equal(transform->xform.orientation, QUAT_IDENT)) {
        transform->xform.orientation.x += TA_EPSILON;
        transform->xform.orientation = quat_normalize(transform->xform.orientation);
    }
    // Default light direction is directly down
    ta_vec3 direction = vec3_rotate_quat(VEC3_NY, transform->xform.orientation);
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
static void shadowpass_render_directional(ta_light *light, ta_model *models)
{
    DLB_ASSERT(light->shadowmap.framebuffer);

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
    dlb_vec_each(ta_model *, model, models) {
        ta_model_shadow_pass(model, shader, &light_pv);
    }

    ta_shader_unbind(shader);
}

// Draw into shadowmap from light perspective
// http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-16-shadow-mapping/#rendering-the-shadow-map
// https://www.khronos.org/opengl/wiki/GLAPI/glBindFragDataLocation
// https://gamedev.stackexchange.com/questions/19461/opengl-glsl-render-to-cube-map
static void shadowpass_render_point(ta_light *light, ta_model *models)
{
    DLB_ASSERT(light->shadowmap.framebuffer);

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
        dlb_vec_each(ta_model *, model, models) {
            ta_model_shadow_pass(model, shader, &light_pv);
        }
    }

    ta_shader_unbind(shader);
}

void ta_light_shadowpass_render(ta_light *light, ta_model *models)
{
    if (light->disabled && !light->cast_shadows) {
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
    ta_shader_set_sampler2d(tg_shader_quads, SYM_U_TEX,
        light->shadowmap.texture.gl_id);

    s32 resolution = light->shadowmap.resolution / 10;
    ta_rect rect = { 0 };
    rect.x = x;
    rect.y = y;
    rect.w = resolution;
    rect.h = resolution;
    ta_primitive_push_rect(rect, TA_COLOR_INVIS, UI_LAYER_EDIT_1);
    ta_primitive_render_quads(quads_queue, tg_shader_quads, true, true);

    ta_shader_set_sampler2d(tg_shader_quads, SYM_U_TEX, 0);
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
        ta_primitive_push_rect(rect, TA_COLOR_INVIS, UI_LAYER_EDIT_1);
        ta_primitive_render_quads(quads_queue, tg_shader_cubemap, true, true);
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