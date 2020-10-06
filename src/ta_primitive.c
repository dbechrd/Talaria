#include "ta_primitive.h"
#include "ta_game.h"
#include "ta_log.h"
#include "ta_mesh.h"
#include "ta_shader.h"
#include "ta_symbol.h"
#include "ta_window.h"
#include "dlb/dlb_vector.h"
#include "misc/glad.h"
#include <math.h>

ta_mesh primitive_lines;
ta_mesh primitive_lines_perma;

ta_mesh primitive_quads;
// TODO: This is getting messy, optionally sort by layer before rendering, maybe
//       on insert?
ta_mesh primitive_quads_tooltip_bg;
ta_mesh primitive_quads_tooltip_fg;

void ta_primitive_init()
{
    ta_log_write(&tg_debug_log, SRC_PRIMITIVE, "Initializing lines...\n");
    ta_mesh_create(&primitive_lines);
    ta_mesh_create(&primitive_lines_perma);

    ta_log_write(&tg_debug_log, SRC_PRIMITIVE, "Initializing quads...\n");
    ta_mesh_create(&primitive_quads);

    ta_log_write(&tg_debug_log, SRC_PRIMITIVE, "Initializing tooltips...\n");
    ta_mesh_create(&primitive_quads_tooltip_bg);
    ta_mesh_create(&primitive_quads_tooltip_fg);
}

#if 0
static void ta_primitive_line2d_to_line(ta_vert_line *line, ta_line_2d line2d,
    ta_rgba color0, ta_rgba color1)
{
    float x0 = NDC_X(line2d.p0.x);
    float x1 = NDC_X(line2d.p1.x);
    float y0 = NDC_Y(line2d.p0.y);
    float y1 = NDC_Y(line2d.p1.y);

    line->verts[0].position.x = x0;
    line->verts[0].position.y = y0;
    line->verts[0].color = color0;
    line->verts[1].position.x = x1;
    line->verts[1].position.y = y1;
    line->verts[1].color = color1;
}
static void ta_primitive_line3d_to_line(ta_vert_line *line, ta_line_3d line3d,
    ta_rgba color0, ta_rgba color1)
{
    line->verts[0].position = line3d.p0;
    line->verts[0].color = color0;
    line->verts[1].position = line3d.p1;
    line->verts[1].color = color1;
}
void ta_primitive_line3d_to_quad(ta_vert_quad *quad, ta_line_3d line3d,
    ta_vec3 normal, ta_rgba color0, ta_rgba color1)
{
    ta_vec3 p0_p1 = vec3_sub(line3d.p1, line3d.p0);
    ta_vec3 p0_cam = vec3_sub(normal, line3d.p0);
    ta_vec3 p0_offset = vec3_scalef(vec3_normalize(vec3_cross(p0_p1, p0_cam)), 0.001f);

    ta_vec3 p1_p0 = vec3_neg(p0_p1);
    ta_vec3 p1_cam = vec3_sub(normal, line3d.p1);
    ta_vec3 p1_offset = vec3_scalef(vec3_normalize(vec3_cross(p1_p0, p1_cam)), 0.001f);

    ta_vec3 v0 = vec3_add(line3d.p0, p0_offset);
    ta_vec3 v1 = vec3_sub(line3d.p0, p0_offset);
    ta_vec3 v2 = vec3_add(line3d.p1, p1_offset);
    ta_vec3 v3 = vec3_sub(line3d.p1, p1_offset);

    quad->verts[0].position = v0;
    quad->verts[0].color = color0;
    quad->verts[0].uv.line = 0.0f;
    quad->verts[0].uv.y = 0.0f;
    quad->verts[1].position = v1;
    quad->verts[1].color = color0;
    quad->verts[1].uv.line = 1.0f;
    quad->verts[1].uv.y = 0.0f;
    quad->verts[2].position = v2;
    quad->verts[2].color = color1;
    quad->verts[2].uv.line = 1.0f;
    quad->verts[2].uv.y = 1.0f;
    quad->verts[3].position = v0;
    quad->verts[3].color = color0;
    quad->verts[3].uv.line = 0.0f;
    quad->verts[3].uv.y = 0.0f;
    quad->verts[4].position = v2;
    quad->verts[4].color = color1;
    quad->verts[4].uv.line = 1.0f;
    quad->verts[4].uv.y = 1.0f;
    quad->verts[5].position = v3;
    quad->verts[5].color = color1;
    quad->verts[5].uv.line = 0.0f;
    quad->verts[5].uv.y = 1.0f;
}
#endif
// TODO: Take p0 and p1
void ta_primitive_push_line_2d(ta_mesh *mesh, ta_line_2d line2d, ta_rgba color0,
    ta_rgba color1)
{
    ta_vec3 p0 = { 0 };
    p0.x = NDC_X(line2d.p0.x);
    p0.y = NDC_Y(line2d.p0.y);
    ta_vec3 p1 = { 0 };
    p1.x = NDC_X(line2d.p1.x);
    p1.y = NDC_Y(line2d.p1.y);

    if (!mesh) mesh = &primitive_lines;
    dlb_vec_push(mesh->positions, p0);
    dlb_vec_push(mesh->positions, p1);
    dlb_vec_push(mesh->colors, color0);
    dlb_vec_push(mesh->colors, color1);
}
// TODO: Take p0 and p1
void ta_primitive_push_line_3d(ta_mesh *mesh, ta_line_3d line3d, ta_rgba color0, ta_rgba color1)
{
    if (!mesh) mesh = &primitive_lines;
    dlb_vec_push(mesh->positions, line3d.p0);
    dlb_vec_push(mesh->positions, line3d.p1);
    dlb_vec_push(mesh->colors, color0);
    dlb_vec_push(mesh->colors, color1);
}
void ta_primitive_push_quad(ta_mesh *mesh, ta_quad quad, ta_rgba color)
{
    if (!mesh) mesh = &primitive_quads;
    // v3 _______ v2
    //    |    /|
    //    |  /  |
    //    |/____|
    // v0         v1

    // {v0, v1, v2}, {v0, v2, v3}

    // u,v are +x,+y in quad space
    ta_vec3 u = quat_mul_vec3(quad.orientation, VEC3_X);
    u = vec3_normalize(u);
    ta_vec3 v = quat_mul_vec3(quad.orientation, VEC3_Y);
    //ta_vec3 v = vec3_cross(vec3_normalize(quad.normal), u);
    v = vec3_normalize(v);

    u = vec3_scalef(u, quad.extents.x);
    v = vec3_scalef(v, quad.extents.y);

    ta_vec3 v0 = vec3_sub(vec3_sub(quad.center, u), v);
    ta_vec3 v1 = vec3_add(vec3_sub(quad.center, u), v);
    ta_vec3 v2 = vec3_sub(vec3_add(quad.center, u), v);
    ta_vec3 v3 = vec3_add(vec3_add(quad.center, u), v);

    dlb_vec_push(primitive_quads.positions, v0);
    dlb_vec_push(primitive_quads.positions, v1);
    dlb_vec_push(primitive_quads.positions, v2);
    dlb_vec_push(primitive_quads.positions, v2);
    dlb_vec_push(primitive_quads.positions, v1);
    dlb_vec_push(primitive_quads.positions, v3);

    static const ta_vec2 uv[6] = {
        { 0.0f, 0.0f },
        { 1.0f, 0.0f },
        { 1.0f, 1.0f },
        { 0.0f, 0.0f },
        { 1.0f, 1.0f },
        { 0.0f, 1.0f },
    };

    for (int i = 0; i < 6; i++) {
        dlb_vec_push(primitive_quads.uvs, uv[i]);
        dlb_vec_push(primitive_quads.colors, color);
    }
}
void ta_primitive_push_rect(ta_mesh *mesh, ta_rect rect, ta_rgba color, float z)
{
    if (!mesh) mesh = &primitive_quads;
    // v3 _______ v2
    //    |    /|
    //    |  /  |
    //    |/____|
    // v0         v1

    // {v0, v1, v2}, {v0, v2, v3}

    float x0 = NDC_X(rect.x);
    float x1 = NDC_X(rect.x + rect.w);
    float y1 = NDC_Y(rect.y);
    float y0 = NDC_Y(rect.y + rect.h);

    ta_vec3 p[6];
    ta_vec2 uv[6];
    p[0].x = x0; p[0].y = y0; p[0].z = z; uv[0].x = 0.0f; uv[0].y = 0.0f; // 0,0
    p[1].x = x1; p[1].y = y0; p[1].z = z; uv[1].x = 1.0f; uv[1].y = 0.0f; // 1,0
    p[2].x = x1; p[2].y = y1; p[2].z = z; uv[2].x = 1.0f; uv[2].y = 1.0f; // 1,1
    p[3].x = x0; p[3].y = y0; p[3].z = z; uv[3].x = 0.0f; uv[3].y = 0.0f; // 0,0
    p[4].x = x1; p[4].y = y1; p[4].z = z; uv[4].x = 1.0f; uv[4].y = 1.0f; // 1,1
    p[5].x = x0; p[5].y = y1; p[5].z = z; uv[5].x = 0.0f; uv[5].y = 1.0f; // 0,1

    for (int i = 0; i < 6; i++) {
        dlb_vec_push(mesh->positions, p[i]);
        dlb_vec_push(mesh->uvs, uv[i]);
        dlb_vec_push(mesh->colors, color);
    }
}
void ta_primitive_push_rect_uv(ta_mesh *mesh, ta_rect_uv rect_uv, ta_rgba color,
    float z, bool screen, bool top_left)
{
    if (!mesh) mesh = &primitive_quads;
    // v3 _______ v2
    //    |    /|
    //    |  /  |
    //    |/____|
    // v0         v1

    // {v0, v1, v2}, {v0, v2, v3}

    float x0, x1, y0, y1;
    if (screen) {
        x0 = NDC_X(rect_uv.rect.x);
        x1 = NDC_X(rect_uv.rect.x + rect_uv.rect.w);
        y0 = NDC_Y(rect_uv.rect.y);
        y1 = NDC_Y(rect_uv.rect.y + rect_uv.rect.h);
        if (top_left) {
            x0 += 1.0f;
            x1 += 1.0f;
            y0 -= 1.0f;
            y1 -= 1.0f;
        }
    } else {
        // NOTE: This is only used for nametags atm.
        x0 = (float)(rect_uv.rect.x);
        x1 = (float)(rect_uv.rect.x + rect_uv.rect.w);
        y0 = (float)(rect_uv.rect.y);
        y1 = (float)(rect_uv.rect.y + rect_uv.rect.h);
    }

    ta_vec3 p[6];
    ta_vec2 uv[6];
    p[0].x = x0; p[0].y = y0; p[0].z = z; uv[0].x = rect_uv.uv0.u; uv[0].y = rect_uv.uv0.v; // 0,0
    p[1].x = x1; p[1].y = y0; p[1].z = z; uv[1].x = rect_uv.uv1.u; uv[1].y = rect_uv.uv0.v; // 1,0
    p[2].x = x1; p[2].y = y1; p[2].z = z; uv[2].x = rect_uv.uv1.u; uv[2].y = rect_uv.uv1.v; // 1,1
    p[3].x = x0; p[3].y = y0; p[3].z = z; uv[3].x = rect_uv.uv0.u; uv[3].y = rect_uv.uv0.v; // 0,0
    p[4].x = x1; p[4].y = y1; p[4].z = z; uv[4].x = rect_uv.uv1.u; uv[4].y = rect_uv.uv1.v; // 1,1
    p[5].x = x0; p[5].y = y1; p[5].z = z; uv[5].x = rect_uv.uv0.u; uv[5].y = rect_uv.uv1.v; // 0,1

    for (int i = 0; i < 6; i++) {
        dlb_vec_push(mesh->positions, p[i]);
        dlb_vec_push(mesh->uvs, uv[i]);
        dlb_vec_push(mesh->colors, color);
    }
}
void ta_primitive_push_plane(ta_mesh *mesh, ta_plane plane, float radius,
    ta_rgba color)
{
    if (!mesh) mesh = &primitive_quads;
#if 1
    // u,v are +x,+y in plane space
    ta_vec3 u = vec3_perp(plane.normal);
    u = vec3_normalize(u);
    ta_vec3 v = vec3_cross(vec3_normalize(plane.normal), u);
    v = vec3_normalize(v);
#else
    // TODO: This seems like a hack that could be replaced with vec3_perp
    plane.center = vec3_add(plane.center, VEC3_EPSILON);
    ta_vec3 u = vec3_normalize(vec3_cross(plane.center, plane.normal));
    ta_vec3 v = vec3_normalize(vec3_cross(x, plane.normal));
#endif

    u = vec3_scalef(u, radius);
    v = vec3_scalef(v, radius);

    ta_vec3 v0 = vec3_sub(vec3_sub(plane.center, u), v);
    ta_vec3 v1 = vec3_add(vec3_sub(plane.center, u), v);
    ta_vec3 v2 = vec3_sub(vec3_add(plane.center, u), v);
    ta_vec3 v3 = vec3_add(vec3_add(plane.center, u), v);

    ta_vec2 uv[6];
    uv[0].x = 0.0f; uv[0].y = 0.0f; // 0,0
    uv[1].x = 1.0f; uv[1].y = 0.0f; // 1,0
    uv[2].x = 1.0f; uv[2].y = 1.0f; // 1,1
    uv[3].x = 0.0f; uv[3].y = 0.0f; // 0,0
    uv[4].x = 1.0f; uv[4].y = 1.0f; // 1,1
    uv[5].x = 0.0f; uv[5].y = 1.0f; // 0,1

    dlb_vec_push(mesh->positions, v0);
    dlb_vec_push(mesh->positions, v1);
    dlb_vec_push(mesh->positions, v2);
    dlb_vec_push(mesh->positions, v2);
    dlb_vec_push(mesh->positions, v1);
    dlb_vec_push(mesh->positions, v3);

    for (int i = 0; i < 6; i++) {
        dlb_vec_push(mesh->uvs, uv[i]);
        dlb_vec_push(mesh->colors, color);
    }
}
void ta_primitive_push_crosshair(ta_mesh *mesh, s32 length, s32 thickness)
{
    ta_rect x = { 0 };
    x.x = WINDOW_W / 2 - length / 2;
    x.y = WINDOW_H / 2 - thickness / 2;
    x.w = length;
    x.h = thickness;

    ta_rect y = { 0 };
    y.x = WINDOW_W / 2 - thickness / 2;
    y.y = WINDOW_H / 2 - length / 2;
    y.w = thickness;
    y.h = length;

    ta_primitive_push_rect(mesh, x, TA_COLOR_WHITE_ALPHA, UI_LAYER_HUD);
    ta_primitive_push_rect(mesh, y, TA_COLOR_WHITE_ALPHA, UI_LAYER_HUD);
}

#define SPHERE_SEGMENTS 32
#define SPHERE_SEG_RAD (DEG_TO_RADF(360.f / SPHERE_SEGMENTS))
void ta_primitive_push_sphere(ta_mesh *mesh, ta_sphere sphere, ta_rgba color)
{
    // TODO: Could save some bandwidth with line strips
    ta_line_3d line_yz = { 0 };
    ta_line_3d line_xz = { 0 };
    ta_line_3d line_xy = { 0 };

    line_yz.p0 = sphere.center;
    line_yz.p0.y += sphere.radius;

    line_xz.p0 = sphere.center;
    line_xz.p0.x += sphere.radius;

    line_xy.p0 = sphere.center;
    line_xy.p0.x += sphere.radius;

    for (int i = 1; i < SPHERE_SEGMENTS; i++) {
        float cosr = cosf(SPHERE_SEG_RAD * i) * sphere.radius;
        float sinr = sinf(SPHERE_SEG_RAD * i) * sphere.radius;

        line_yz.p1 = sphere.center;
        line_yz.p1.y += cosr;
        line_yz.p1.z += sinr;
        ta_primitive_push_line_3d(mesh, line_yz, color, color);
        line_yz.p0 = line_yz.p1;

        line_xz.p1 = sphere.center;
        line_xz.p1.x += cosr;
        line_xz.p1.z += sinr;
        ta_primitive_push_line_3d(mesh, line_xz, color, color);
        line_xz.p0 = line_xz.p1;

        line_xy.p1 = sphere.center;
        line_xy.p1.x += cosr;
        line_xy.p1.y += sinr;
        ta_primitive_push_line_3d(mesh, line_xy, color, color);
        line_xy.p0 = line_xy.p1;
    }

    line_yz.p1 = sphere.center;
    line_yz.p1.y += sphere.radius;

    line_xz.p1 = sphere.center;
    line_xz.p1.x += sphere.radius;

    line_xy.p1 = sphere.center;
    line_xy.p1.x += sphere.radius;

    ta_primitive_push_line_3d(mesh, line_yz, color, color);
    ta_primitive_push_line_3d(mesh, line_xz, color, color);
    ta_primitive_push_line_3d(mesh, line_xy, color, color);
}
void ta_primitive_push_rgb_sphere(ta_mesh *mesh, ta_sphere sphere)
{
    // TODO: Could save some bandwidth with line strips
    ta_line_3d line_yz = { 0 };
    ta_line_3d line_xz = { 0 };
    ta_line_3d line_xy = { 0 };

    line_yz.p0 = sphere.center;
    line_yz.p0.y += sphere.radius;

    line_xz.p0 = sphere.center;
    line_xz.p0.x += sphere.radius;

    line_xy.p0 = sphere.center;
    line_xy.p0.x += sphere.radius;

    for (int i = 1; i < SPHERE_SEGMENTS; i++) {
        float cosr = cosf(SPHERE_SEG_RAD * i) * sphere.radius;
        float sinr = sinf(SPHERE_SEG_RAD * i) * sphere.radius;

        line_yz.p1 = sphere.center;
        line_yz.p1.y += cosr;
        line_yz.p1.z += sinr;
        ta_primitive_push_line_3d(mesh, line_yz, TA_COLOR_RED, TA_COLOR_RED);
        line_yz.p0 = line_yz.p1;

        line_xz.p1 = sphere.center;
        line_xz.p1.x += cosr;
        line_xz.p1.z += sinr;
        ta_primitive_push_line_3d(mesh, line_xz, TA_COLOR_GREEN, TA_COLOR_GREEN);
        line_xz.p0 = line_xz.p1;

        line_xy.p1 = sphere.center;
        line_xy.p1.x += cosr;
        line_xy.p1.y += sinr;
        ta_primitive_push_line_3d(mesh, line_xy, TA_COLOR_BLUE, TA_COLOR_BLUE);
        line_xy.p0 = line_xy.p1;
    }

    line_yz.p1 = sphere.center;
    line_yz.p1.y += sphere.radius;

    line_xz.p1 = sphere.center;
    line_xz.p1.x += sphere.radius;

    line_xy.p1 = sphere.center;
    line_xy.p1.x += sphere.radius;

    ta_primitive_push_line_3d(mesh, line_yz, TA_COLOR_RED, TA_COLOR_RED);
    ta_primitive_push_line_3d(mesh, line_xz, TA_COLOR_GREEN, TA_COLOR_GREEN);
    ta_primitive_push_line_3d(mesh, line_xy, TA_COLOR_BLUE, TA_COLOR_BLUE);
}
#undef SPHERE_SEG_RAD
#undef SPHERE_SEGMENTS

#define CONE_SEGMENTS 32
#define CONE_SEG_RAD (DEG_TO_RADF(360.f / CONE_SEGMENTS))
void ta_primitive_push_cone(ta_mesh *mesh, ta_cone cone, ta_rgba color)
{
    // This is to prevent vec3_perp() from failing; just don't draw tiny cones
    if (vec3_tiny(cone.apex)) {
        return;
    }

    // u,v are +x,+y in circle space
    ta_vec3 u = vec3_perp(cone.apex);
    u = vec3_normalize(u);
    ta_vec3 v = vec3_cross(vec3_normalize(cone.apex), u);
    v = vec3_normalize(v);

    // Start at (1, 0) on the circle (simplified because cos(0) is 1.0, i.e.
    // just the radius, and sin(0) is 0, so v does not contribute).
    ta_line_3d directrix = { 0 };
    directrix.p0 = vec3_add(cone.center, vec3_scalef(u, cone.radius));

    ta_line_3d generatrix = { 0 };
    generatrix.p1 = vec3_add(cone.center, cone.apex);

    // Calculate directrix segments counter-clockwise, and a generatrix from the
    // base to the apex for each one.
    for (int i = 1; i <= CONE_SEGMENTS; i++) {
        float rcos = cone.radius * cosf(CONE_SEG_RAD * i);
        float rsin = cone.radius * sinf(CONE_SEG_RAD * i);

        generatrix.p0 = directrix.p0;
        ta_primitive_push_line_3d(mesh, generatrix, color, color);

        directrix.p1 = vec3_scalef(u, rcos);
        directrix.p1 = vec3_add(directrix.p1, vec3_scalef(v, rsin));
        directrix.p1 = vec3_add(directrix.p1, cone.center);
        ta_primitive_push_line_3d(mesh, directrix, color, color);

        directrix.p0 = directrix.p1;
    }
}
#undef CONE_SEG_RAD
#undef CONE_SEGMENTS

void ta_primitive_push_arrow(ta_mesh *mesh, ta_vec3 origin, ta_vec3 direction, ta_rgba color)
{
    ta_vec3 tip = vec3_add(origin, direction);

    ta_line_3d line;
    line.p0 = origin;
    line.p1 = tip;
    ta_primitive_push_line_3d(mesh, line, color, color);

    ta_cone cone = { 0 };
    cone.apex = vec3_scalef(direction, 0.2f);
    cone.center = vec3_sub(tip, cone.apex);
    cone.radius = vec3_len(direction) / TA_PRIMITIVE_CONE_RADIUS_SCALE;
    ta_primitive_push_cone(mesh, cone, color);
}
void ta_primitive_push_aabb(ta_mesh *mesh, ta_aabb aabb, ta_rgba color)
{
#if 1
    ta_obb obb = { 0 };
    obb.center = aabb.center;
    obb.extents = aabb.extents;
    obb.orientation = QUAT_IDENT;
    ta_primitive_push_obb(mesh, obb, color);
#else
    ta_line_3d edge = { 0 };
    ta_vec3 pmin = vec3_sub(aabb.center, aabb.extents);
    ta_vec3 size = vec3_scalef(aabb.extents, 2.0f);

    ta_vec3 p0, p1, p2, p3, p4, p5, p6, p7;

#define VEC3(vec, a, b, c) vec.line = a; vec.y = b; vec.z = c;
    VEC3(p0, pmin.line         , pmin.y         , pmin.z         );
    VEC3(p1, pmin.line         , pmin.y         , pmin.z + size.z);
    VEC3(p2, pmin.line         , pmin.y + size.y, pmin.z         );
    VEC3(p3, pmin.line         , pmin.y + size.y, pmin.z + size.z);
    VEC3(p4, pmin.line + size.line, pmin.y         , pmin.z         );
    VEC3(p5, pmin.line + size.line, pmin.y         , pmin.z + size.z);
    VEC3(p6, pmin.line + size.line, pmin.y + size.y, pmin.z         );
    VEC3(p7, pmin.line + size.line, pmin.y + size.y, pmin.z + size.z);
#undef VEC3

#define PUSH_LINE(a, b) edge.p0 = a; edge.p1 = b; \
                        ta_primitive_push_line_3d(edge, color, color)
    PUSH_LINE(p0, p1);
    PUSH_LINE(p0, p2);
    PUSH_LINE(p0, p4);
    PUSH_LINE(p1, p3);
    PUSH_LINE(p1, p5);
    PUSH_LINE(p2, p3);
    PUSH_LINE(p2, p6);
    PUSH_LINE(p3, p7);
    PUSH_LINE(p4, p5);
    PUSH_LINE(p4, p6);
    PUSH_LINE(p5, p7);
    PUSH_LINE(p6, p7);
#undef PUSH_LINE
#endif
}
void ta_primitive_push_obb(ta_mesh *mesh, ta_obb obb, ta_rgba color)
{
    ta_vec3 p[8] = { 0 };
    p[0].x = -obb.extents.x;
    p[0].y = -obb.extents.y;
    p[0].z = -obb.extents.z;
    p[1].x = -obb.extents.x;
    p[1].y = -obb.extents.y;
    p[1].z = +obb.extents.z;
    p[2].x = -obb.extents.x;
    p[2].y = +obb.extents.y;
    p[2].z = -obb.extents.z;
    p[3].x = -obb.extents.x;
    p[3].y = +obb.extents.y;
    p[3].z = +obb.extents.z;
    p[4].x = +obb.extents.x;
    p[4].y = -obb.extents.y;
    p[4].z = -obb.extents.z;
    p[5].x = +obb.extents.x;
    p[5].y = -obb.extents.y;
    p[5].z = +obb.extents.z;
    p[6].x = +obb.extents.x;
    p[6].y = +obb.extents.y;
    p[6].z = -obb.extents.z;
    p[7].x = +obb.extents.x;
    p[7].y = +obb.extents.y;
    p[7].z = +obb.extents.z;

    for (int i = 0; i < 8; ++i) {
        p[i] = quat_mul_vec3(obb.orientation, p[i]);
        p[i] = vec3_add(p[i], obb.center);
    }

    ta_line_3d line = { 0 };
#define PUSH_LINE(a, b) line.p0 = a; line.p1 = b; \
                        ta_primitive_push_line_3d(mesh, line, color, color)
    PUSH_LINE(p[0], p[1]);
    PUSH_LINE(p[0], p[2]);
    PUSH_LINE(p[0], p[4]);
    PUSH_LINE(p[1], p[3]);
    PUSH_LINE(p[1], p[5]);
    PUSH_LINE(p[2], p[3]);
    PUSH_LINE(p[2], p[6]);
    PUSH_LINE(p[3], p[7]);
    PUSH_LINE(p[4], p[5]);
    PUSH_LINE(p[4], p[6]);
    PUSH_LINE(p[5], p[7]);
    PUSH_LINE(p[6], p[7]);
#undef PUSH_LINE

#if 0
    ta_vec3 p[8] = { 0 };
    p[0] = vec3_sub(vec3_sub(vec3_sub(obb.center, obb.axes[0]), obb.axes[1]), obb.axes[2]);
    p[1] = vec3_sub(vec3_sub(vec3_add(obb.center, obb.axes[0]), obb.axes[1]), obb.axes[2]);
    p[2] = vec3_sub(vec3_add(vec3_sub(obb.center, obb.axes[0]), obb.axes[1]), obb.axes[2]);
    p[3] = vec3_sub(vec3_add(vec3_add(obb.center, obb.axes[0]), obb.axes[1]), obb.axes[2]);
    p[4] = vec3_add(vec3_sub(vec3_sub(obb.center, obb.axes[0]), obb.axes[1]), obb.axes[2]);
    p[5] = vec3_add(vec3_sub(vec3_add(obb.center, obb.axes[0]), obb.axes[1]), obb.axes[2]);
    p[6] = vec3_add(vec3_add(vec3_sub(obb.center, obb.axes[0]), obb.axes[1]), obb.axes[2]);
    p[7] = vec3_add(vec3_add(vec3_add(obb.center, obb.axes[0]), obb.axes[1]), obb.axes[2]);
#endif
}
void ta_primitive_push_cube(ta_mesh *mesh, ta_vec3 center, float radius,
    ta_rgba color)
{
    ta_vec3 pmin = center;
    pmin.x -= radius;
    pmin.y -= radius;
    pmin.z -= radius;

    float size = radius * 2.0f;

    ta_vec3 p0, p1, p2, p3, p4, p5, p6, p7;

#define VEC3(vec, a, b, c) vec.x = a; vec.y = b; vec.z = c;
    VEC3(p0, pmin.x       , pmin.y       , pmin.z       );
    VEC3(p1, pmin.x       , pmin.y       , pmin.z + size);
    VEC3(p2, pmin.x       , pmin.y + size, pmin.z       );
    VEC3(p3, pmin.x       , pmin.y + size, pmin.z + size);
    VEC3(p4, pmin.x + size, pmin.y       , pmin.z       );
    VEC3(p5, pmin.x + size, pmin.y       , pmin.z + size);
    VEC3(p6, pmin.x + size, pmin.y + size, pmin.z       );
    VEC3(p7, pmin.x + size, pmin.y + size, pmin.z + size);
#undef VEC3

    ta_line_3d edge = { 0 };
#define PUSH_LINE(a, b) edge.p0 = a; edge.p1 = b; \
                        ta_primitive_push_line_3d(mesh, edge, color, color)
    PUSH_LINE(p0, p1);
    PUSH_LINE(p0, p2);
    PUSH_LINE(p0, p4);
    PUSH_LINE(p1, p3);
    PUSH_LINE(p1, p5);
    PUSH_LINE(p2, p3);
    PUSH_LINE(p2, p6);
    PUSH_LINE(p3, p7);
    PUSH_LINE(p4, p5);
    PUSH_LINE(p4, p6);
    PUSH_LINE(p5, p7);
    PUSH_LINE(p6, p7);
#undef PUSH_LINE
}
// NOTE: "scale" is extent of grid from origin to edge (radius), "frequency" is
// how often to draw a line (spacing). center, normal and color should be pretty
// self-explanatory.
void ta_primitive_push_grid(ta_mesh *mesh, ta_vec3 center, ta_vec3 normal, float radius, float frequency, ta_rgba color)
{
    DLB_ASSERT(frequency < radius);
    ta_vec3 u = vec3_normalize(vec3_perp(normal));
    ta_vec3 v = vec3_normalize(vec3_cross(normal, u));
    ta_vec3 u_inc = vec3_scalef(u, frequency);
    ta_vec3 v_inc = vec3_scalef(v, frequency);

    ta_vec3 half_u = vec3_scalef(u, radius);
    ta_vec3 u_min = vec3_sub(center, half_u);
    ta_vec3 half_v = vec3_scalef(v, radius);
    ta_vec3 v_min = vec3_sub(center, half_v);

    float lines = radius * 2.0f / frequency;
    ta_line_3d line;

    // U lines from -v_min to +v_min
    line.p0 = vec3_sub(v_min, half_u);
    line.p1 = vec3_add(v_min, half_u);
    ta_primitive_push_line_3d(mesh, line, color, color);
    for (float i = 0; i < lines; ++i) {
        line.p0 = vec3_add(line.p0, v_inc);
        line.p1 = vec3_add(line.p1, v_inc);
        ta_primitive_push_line_3d(mesh, line, color, color);
    }

    // V lines from -u_min to +u_min
    line.p0 = vec3_sub(u_min, half_v);
    line.p1 = vec3_add(u_min, half_v);
    ta_primitive_push_line_3d(mesh, line, color, color);
    for (float i = 0; i < lines; ++i) {
        line.p0 = vec3_add(line.p0, u_inc);
        line.p1 = vec3_add(line.p1, u_inc);
        ta_primitive_push_line_3d(mesh, line, color, color);
    }
}
void ta_primitive_push_axes_arrow_color(ta_mesh *mesh, ta_vec3 position, ta_vec4 orientation, float scale, ta_rgba cx,
    ta_rgba cy, ta_rgba cz)
{
    ta_vec3 x = VEC3_X;
    ta_vec3 y = VEC3_Y;
    ta_vec3 z = VEC3_Z;
    if (!quat_ident(orientation)) {
        x = quat_mul_vec3(orientation, x);
        y = quat_mul_vec3(orientation, y);
        z = quat_mul_vec3(orientation, z);
    }
    ta_primitive_push_arrow(mesh, position, vec3_scalef(x, scale), cx);
    ta_primitive_push_arrow(mesh, position, vec3_scalef(y, scale), cy);
    ta_primitive_push_arrow(mesh, position, vec3_scalef(z, scale), cz);
}
void ta_primitive_push_axes_arrow(ta_mesh *mesh, ta_vec3 position, ta_vec4 orientation, float scale)
{
    ta_primitive_push_axes_arrow_color(mesh, position, orientation, scale, TA_COLOR_RED, TA_COLOR_GREEN, TA_COLOR_BLUE);
}
void ta_primitive_push_axes_cube(ta_mesh *mesh, ta_vec3 position, float scale)
{
    // Render lines
    ta_line_3d line;
    line.p0 = position;

    float cube_radius = scale / 20.0f;
    float cube_extent = scale - cube_radius;
    ta_vec3 cube_center;

    line.p1 = vec3_add(position, vec3_scalef(VEC3_X, scale));
    ta_primitive_push_line_3d(mesh, line, TA_COLOR_RED, TA_COLOR_RED);
    cube_center = vec3_add(line.p0, vec3_scalef(VEC3_X, cube_extent));
    ta_primitive_push_cube(mesh, cube_center, cube_radius, TA_COLOR_RED);

    line.p1 = vec3_add(position, vec3_scalef(VEC3_Y, scale));
    ta_primitive_push_line_3d(mesh, line, TA_COLOR_GREEN, TA_COLOR_GREEN);
    cube_center = vec3_add(line.p0, vec3_scalef(VEC3_Y, cube_extent));
    ta_primitive_push_cube(mesh, cube_center, cube_radius, TA_COLOR_GREEN);

    line.p1 = vec3_add(position, vec3_scalef(VEC3_Z, scale));
    ta_primitive_push_line_3d(mesh, line, TA_COLOR_BLUE, TA_COLOR_BLUE);
    cube_center = vec3_add(line.p0, vec3_scalef(VEC3_Z, cube_extent));
    ta_primitive_push_cube(mesh, cube_center, cube_radius, TA_COLOR_BLUE);
}

void ta_primitive_render_mesh(ta_mesh *mesh, ta_shader *shader, int mode,
    bool clear_buffers, bool reset_uniforms)
{
    // TODO: Move this out into its own explicit call, it's more confusing here
    if (reset_uniforms) {
        ta_shader_reset_pvm(shader);
    }

    //ta_mesh_render(mesh, shader);
    size_t positions_count = dlb_vec_len(mesh->positions);
    if (positions_count) {
        ta_mesh_update_buffers(mesh);

        // HACK: Some quads are backwards (probably because the texture is flipped), so we have to disable CULLING -_-
        glDisable(GL_CULL_FACE);

        // Draw the primitives
        ta_shader_bind(shader);
        glBindVertexArray(mesh->gl_vao);
        glDrawArrays(mode, 0, (GLsizei)positions_count);
        glBindVertexArray(0);
        ta_shader_unbind();

        glEnable(GL_CULL_FACE);
    }

    // TODO: Move this out into its own explicit call, it's more confusing here
    // ta_mesh_clear_buffers();
    if (clear_buffers) {
        for (int i = 0; i < TA_VERTEX_ATTR_COUNT; ++i) {
            dlb_vec_clear(mesh->buffers[i]);
        }
    }
}
void ta_primitive_render(bool clear_buffers, bool reset_uniforms)
{
    ta_primitive_render_mesh(&primitive_lines, tg_shader_lines, TA_LINES, clear_buffers, reset_uniforms);
    ta_primitive_render_mesh(&primitive_quads, tg_shader_quads, TA_TRIANGLES, clear_buffers, reset_uniforms);
}