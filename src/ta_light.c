#include "ta_light.h"
#include "ta_window.h"
#include "ta_node.h"
#include "ta_symbol.h"
#include "ta_primitive.h"
#include "dlb/dlb_vector.h"
#include "misc/gl3w.h"

#define DEFAULT_LIGHT_INTENSITY         1.0f
#define DEFAULT_SHADOWMAP_RESOLUTION    1024
#define DEFAULT_SHADOWMAP_ZNEAR         0.1f
#define DEFAULT_SHADOWMAP_ZFAR          50.0f

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
            light->data.directional.direction =
                vec3_normalize(light->data.directional.direction);
            light->shadowmap.projection = mat4_ortho(
                -10.0f, 10.0f, -10.0f, 10.0f, -10.0f, 50.0f);
            shadowmap_directional_create(light);
            break;
        } case TA_LIGHT_POINT: {
            light->shadowmap.projection = mat4_perspective(
                90.0f, 1.0f, light->shadowmap.znear, light->shadowmap.zfar
            );
            shadowmap_point_create(light);
            break;
        } case TA_LIGHT_SPOT: {
            light->data.directional.direction =
                vec3_normalize(light->data.directional.direction);
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
    glGenFramebuffers(1, &light->shadowmap.framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, light->shadowmap.framebuffer);

    glGenTextures(1, &light->shadowmap.texture);
    glBindTexture(GL_TEXTURE_2D, light->shadowmap.texture);
    // TODO: Should internalformat be GL_DEPTH_COMPONENT16 instead?
    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, light->shadowmap.resolution,
        light->shadowmap.resolution, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0
    );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
        light->shadowmap.texture, 0);
    glDrawBuffer(GL_NONE);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        DLB_ASSERT(!"Failed to set up framebuffer for some reason.");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

static void shadowmap_point_create(ta_light *light)
{
    glGenFramebuffers(1, &light->shadowmap.framebuffer);

    glGenTextures(1, &light->shadowmap.texture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, light->shadowmap.texture);
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

    glBindFramebuffer(GL_FRAMEBUFFER, light->shadowmap.framebuffer);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, light->shadowmap.texture, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        DLB_ASSERT(!"Failed to set up framebuffer for some reason.");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

// http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-16-shadow-mapping/#spot-lights
// Use texture2Dproj to account for perspective-divide

static void shadowpass_render_directional(ta_light *light, ta_shader *shader,
    float alpha, ta_node *entities)
{
    DLB_ASSERT(light->shadowmap.framebuffer);

    // TODO: Draw into shadowmap from ortho big enough to cover camera
    // http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-16-shadow-mapping/#rendering-the-shadow-map
    // https://www.khronos.org/opengl/wiki/GLAPI/glBindFragDataLocation

    glBindFramebuffer(GL_FRAMEBUFFER, light->shadowmap.framebuffer);
    glViewport(0, 0, light->shadowmap.resolution, light->shadowmap.resolution);

    ta_vec3 inv_dir = vec3_neg(light->data.directional.direction);
    ta_mat4 view = mat4_lookat(inv_dir, VEC3_ZERO, VEC3_Y);
    ta_mat4 light_pv = mat4_mul(&light->shadowmap.projection, &view);
    dlb_vec_each(ta_node *, entity, entities) {
        ta_node_shadow_pass(entity, shader, &light_pv, alpha);
    }
}

static void shadowpass_render_point(ta_light *light, ta_shader *shader,
    float alpha, ta_node *entities)
{
    DLB_ASSERT(light->shadowmap.framebuffer);

    // TODO: Draw into shadowmap from light perspective
    // http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-16-shadow-mapping/#rendering-the-shadow-map
    // https://www.khronos.org/opengl/wiki/GLAPI/glBindFragDataLocation

    // https://gamedev.stackexchange.com/questions/19461/opengl-glsl-render-to-cube-map

    // TODO: Cache lookat matrices in dlb_vec, store light_pos as lookat_pos and
    //       update if light_pos != lookat_pos (i.e. position has changed)
    // PERF: Cache may be slower than just recalculating every frame.. profile!
    ta_mat4 view[6];
    view[0] = mat4_lookat(light->position, vec3_add(light->position, VEC3_X),  VEC3_NY);
    view[1] = mat4_lookat(light->position, vec3_add(light->position, VEC3_NX), VEC3_NY);
    view[2] = mat4_lookat(light->position, vec3_add(light->position, VEC3_Y),  VEC3_Z);
    view[3] = mat4_lookat(light->position, vec3_add(light->position, VEC3_NY), VEC3_NZ);
    view[4] = mat4_lookat(light->position, vec3_add(light->position, VEC3_Z),  VEC3_NY);
    view[5] = mat4_lookat(light->position, vec3_add(light->position, VEC3_NZ), VEC3_NY);

    //glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, light->shadowmap.texture, 0);

    //GLenum draw_buffers[] = { GL_DEPTH_ATTACHMENT };
    //glDrawBuffers(1, draw_buffers);

    glViewport(0, 0, light->shadowmap.resolution, light->shadowmap.resolution);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, light->shadowmap.framebuffer);

    for (int i = 0; i < 6; ++i ) {
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, light->shadowmap.texture, 0);

        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            DLB_ASSERT(!"Failed to set up framebuffer for some reason.");
        }

        glClear(GL_DEPTH_BUFFER_BIT);

        ta_mat4 light_pv = mat4_mul(&light->shadowmap.projection, &view[i]);
        dlb_vec_each(ta_node *, entity, entities) {
            ta_node_shadow_pass(entity, shader, &light_pv, alpha);
        }
    }
}

typedef void (* shadowpass_render)(ta_light *light, ta_shader *shader,
    float alpha, ta_node *entities);

static shadowpass_render shadowpass_renderers[TA_LIGHT_COUNT] = {
    [TA_LIGHT_DIRECTIONAL] = shadowpass_render_directional,
    [TA_LIGHT_POINT]       = shadowpass_render_point,
};

void ta_light_shadowpass_render(ta_light *light, ta_shader *shader,
    float alpha, ta_node *entities)
{
    if (shadowpass_renderers[light->type]) {
        shadowpass_renderers[light->type](light, shader, alpha, entities);
    } else {
        DLB_ASSERT(!"No shadowpass renderer for this light type");
    }
}

void ta_light_render_shadowmap_debug(ta_light *light)
{
    ta_shader_set_sampler_cube(tg_shader_cubemap, SYM_U_TEX,
        light->shadowmap.texture);

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
        rect.x = resolution * face_grid[face].x;
        rect.y = resolution * face_grid[face].y;
        rect.w = resolution;
        rect.h = resolution;
        ta_shader_set_int(tg_shader_cubemap, SYM_U_FACE, face);
        ta_primitive_push_rect(rect, TA_COLOR_INVIS, UI_LAYER_HUD_BG);
        ta_primitive_render_quads(quads_queue, tg_shader_cubemap, true, true);
    }

    ta_shader_set_sampler_cube(tg_shader_cubemap, SYM_U_TEX, 0);
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