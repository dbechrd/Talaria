#include "ta_support.h"
#include "ta_primitive.h"

typedef struct ta_gjk_simplex {
    ta_vec3 vertices[4];
    size_t count;
} ta_gjk_simplex;

ta_vec3 ta_support_obb(ta_obb *obb, ta_vec3 d)
{
    ta_vec3 d_local = quat_mul_vec3(quat_inverse(obb->orientation), d);

    ta_vec3 support = { 0 };
    support.x = obb->extents.x * (d_local.x > 0 ? 1 : -1);
    support.y = obb->extents.y * (d_local.y > 0 ? 1 : -1);
    support.z = obb->extents.z * (d_local.z > 0 ? 1 : -1);

    support = vec3_add(obb->center, quat_mul_vec3(obb->orientation, support));
    return support;
}

static bool ta_gjk_do_simplex(ta_gjk_simplex *simplex, ta_vec3 *dir)
{
    DLB_ASSERT(simplex);
    DLB_ASSERT(simplex->count);
    DLB_ASSERT(simplex->count >= 2);
    DLB_ASSERT(simplex->count <= 4);
    DLB_ASSERT(dir);

    bool simplex_contains_origin = false;

    switch (simplex->count) {
        case 2: {
            // Voronoi regions for a 1-simplex (line)

            // |- = voronoi region separating lines
            // x  = excluded voronoi region
            // ?  = space where origin could exist

            // xxxx|   ?   |xxxx
            // xxxx|       |xxxx
            // xxxx|B-----A|xxxx
            // xxxx|       |xxxx
            // xxxx|   ?   |xxxx

            const ta_vec3 b = simplex->vertices[0];
            const ta_vec3 a = simplex->vertices[1];

            // Case 1: Origin in voronoi region B
            // Optimized out because if A exists, then the origin cannot be in region B

            // Case 2: Origin in voronoi region AB (on either side of line AB)
            ta_vec3 ab = vec3_sub(b, a);
            ta_vec3 ao = vec3_neg(a);
            *dir = vec3_cross(vec3_cross(ab, ao), ab);

            // Case 3: Origin in voronoi region A
            // Optimized out because if A exists (i.e. a point beyond the origin), then
            // the origin obviously cannot be beyond the point A.
            break;
        } case 3: {
            // Voronoi regions for a 2-simplex (triangle)

            // |-/\ = voronoi region separating lines
            // x    = excluded voronoi region
            // .    = space where origin cannot exist within a valid voronoi region
            // ?    = space where origin could exist

            // NOTE: Origin can exist on either side of triangle ABC

            // xxxxxxxxxxxxxx/........
            // xxxxxxxxxx/............
            // xxxxxx/................
            // ----C..................
            // xxxx|\  ? ........../xx
            // xxxx|  \  ....../xxxxxx
            // xxxx| ?  \A./xxxxxxxxxx
            // xxxx|    /..\xxxxxxxxxx
            // xxxx|  /  ......\xxxxxx
            // xxxx|/  ? ..........\xx
            // ----B..................
            // xxxxxx\................
            // xxxxxxxxxx\............
            // xxxxxxxxxxxxxx\........

            const ta_vec3 c = simplex->vertices[0];
            const ta_vec3 b = simplex->vertices[1];
            const ta_vec3 a = simplex->vertices[2];

            ta_vec3 ab = vec3_sub(b, a);
            ta_vec3 ac = vec3_sub(c, a);
            ta_vec3 ao = vec3_neg(a);
            ta_vec3 abc = vec3_cross(ab, ac);  // triangle normal (CCW, into screen in diagram above)

            // Test edge AB
            ta_vec3 ab_perp = vec3_cross(ab, abc);
            if (vec3_dot(ab_perp, ao) > 0.0f) {
                // New direction is from AB -> O
                *dir = vec3_cross(vec3_cross(ab, ao), ab);
                // New simplex is AB: [C, B, A] -> [B, A]
                simplex->vertices[0] = b;
                simplex->vertices[1] = a;
                simplex->vertices[2] = VEC3_ZERO;  // TODO(perf): Could omit zeroing, but makes it harder to debug
                simplex->count = 2;
                break;
            }

            // Test edge AC
            ta_vec3 ac_perp = vec3_cross(abc, ac);
            if (vec3_dot(ac_perp, ao) > 0.0f) {
                // New direction is from AC -> O
                *dir = vec3_cross(vec3_cross(ac, ao), ac);
                // New simplex is AC: [C, B, A] -> [C, A]
              /*simplex->vertices[0] = c;*/
                simplex->vertices[1] = a;
                simplex->vertices[2] = VEC3_ZERO;  // TODO(perf): Could omit zeroing, but makes it harder to debug
                simplex->count = 2;
                break;
            }

            // Origin somewhere in ABC (above or below)
            if (vec3_dot(abc, ao) > 0.0f) {
                // New direction is abc
                *dir = abc;
                // Simplex stays the same
            } else {
                // New direction is -abc
                *dir = vec3_neg(abc);
#if 1
                // Simplex stays the same
#else
                // NOTE: This might be useful but I'm not sure.. was just a thought. It wasn't useful
                // for case 1 -> case 2, so maybe it doesn't matter for case 3 -> case 4 either.

                // Simplex is the same but flip winding order for consistency in case 4
                ta_vec3 tmp = simplex->vertices[0];
                simplex->vertices[0] = simplex->vertices[1];
                simplex->vertices[1] = tmp;
#endif
            }

            break;
        } case 4: {

            // D _________________ C
            //   \ .           . /
            //    \  .   ?   .  /
            //     \   .   .   /
            //      \    A    /
            //       \ ? . ? /
            //        \  .  /
            //         \ . /
            //          \./
            //           B

            const ta_vec3 d = simplex->vertices[0];
            const ta_vec3 c = simplex->vertices[1];
            const ta_vec3 b = simplex->vertices[2];
            const ta_vec3 a = simplex->vertices[3];

            ta_vec3 ab = vec3_sub(b, a);
            ta_vec3 ac = vec3_sub(c, a);
            ta_vec3 ad = vec3_sub(d, a);
            ta_vec3 ao = vec3_neg(a);

            // Test triangle ABC
            ta_vec3 abc = vec3_cross(ab, ac);
            if (vec3_dot(abc, ao) > 0.0f) {
                // New direction is from ABC -> O
                *dir = vec3_cross(vec3_cross(abc, ao), abc);
                // New simplex is AB: [D, C, B, A] -> [C, B, A]
                simplex->vertices[0] = c;
                simplex->vertices[1] = b;
                simplex->vertices[2] = a;
                simplex->vertices[3] = VEC3_ZERO;  // TODO(perf): Could omit zeroing, but makes it harder to debug
                simplex->count = 3;
                break;
            }

            // Test triangle ABD
            ta_vec3 abd = vec3_cross(ad, ab);
            if (vec3_dot(abd, ao) > 0.0f) {
                // New direction is from ABD -> O
                *dir = vec3_cross(vec3_cross(abd, ao), abd);
                // New simplex is AB: [D, C, B, A] -> [D, B, A]
              /*simplex->vertices[0] = d;*/
                simplex->vertices[1] = b;
                simplex->vertices[2] = a;
                simplex->vertices[3] = VEC3_ZERO;  // TODO(perf): Could omit zeroing, but makes it harder to debug
                simplex->count = 3;
                break;
            }

            // Test triangle ACD
            ta_vec3 acd = vec3_cross(ac, ad);
            if (vec3_dot(acd, ao) > 0.0f) {
                // New direction is from acd -> O
                *dir = vec3_cross(vec3_cross(acd, ao), acd);
                // New simplex is AB: [D, C, B, A] -> [D, C, A]
              /*simplex->vertices[0] = d;*/
              /*simplex->vertices[1] = c;*/
                simplex->vertices[2] = a;
                simplex->vertices[3] = VEC3_ZERO;  // TODO(perf): Could omit zeroing, but makes it harder to debug
                simplex->count = 3;
                break;
            }

            // If origin is not outside of the tetrahedron, it must be inside!
            simplex_contains_origin = true;
            break;
        }
    }
    return simplex_contains_origin;
}

void gjk_debug_draw(ta_vec3 sa, ta_vec3 sb, ta_vec3 dir, ta_gjk_simplex *simplex)
{
    // Origin
    ta_primitive_push_axes_arrow(0, VEC3_ZERO, QUAT_IDENT, 0.04f);

    ta_sphere dbg_sphere = { 0 };
    dbg_sphere.center = sa;
    dbg_sphere.radius = 0.04f;
    ta_primitive_push_sphere(0, dbg_sphere, TA_COLOR_RED);
    dbg_sphere.center = sb;
    ta_primitive_push_sphere(0, dbg_sphere, TA_COLOR_GREEN);
    ta_primitive_push_arrow(0, VEC3_ZERO, vec3_normalize(dir), TA_COLOR_BLUE);

    if (simplex->count == 1) {
        ta_sphere simplex_sphere = { 0 };
        simplex_sphere.center = vec3_add(VEC3_ZERO, simplex->vertices[0]);
        simplex_sphere.radius = 0.04f;
        ta_primitive_push_sphere(0, simplex_sphere, TA_COLOR_PINK);
    } else {
        for (int i = 0; i < simplex->count; ++i) {
            for (int j = i + 1; j < simplex->count; ++j) {
                ta_line_3d simplex_line = { 0 };
                simplex_line.p0 = vec3_add(VEC3_ZERO, simplex->vertices[i]);
                simplex_line.p1 = vec3_add(VEC3_ZERO, simplex->vertices[j]);
                ta_primitive_push_line_3d(0, simplex_line, TA_COLOR_PINK, TA_COLOR_PINK);
            }
        }
    }
}

bool ta_gjk_intersect_obb(ta_obb *a, ta_obb *b, int *gjk_step, int *gjk_steps)
{
    DLB_ASSERT(gjk_step);
    DLB_ASSERT(gjk_steps);

    ta_vec3 verts_a[8] = { 0 };
    verts_a[0].x = -a->extents.x;
    verts_a[0].y = -a->extents.y;
    verts_a[0].z = -a->extents.z;
    verts_a[1].x = -a->extents.x;
    verts_a[1].y = -a->extents.y;
    verts_a[1].z = +a->extents.z;
    verts_a[2].x = -a->extents.x;
    verts_a[2].y = +a->extents.y;
    verts_a[2].z = -a->extents.z;
    verts_a[3].x = -a->extents.x;
    verts_a[3].y = +a->extents.y;
    verts_a[3].z = +a->extents.z;
    verts_a[4].x = +a->extents.x;
    verts_a[4].y = -a->extents.y;
    verts_a[4].z = -a->extents.z;
    verts_a[5].x = +a->extents.x;
    verts_a[5].y = -a->extents.y;
    verts_a[5].z = +a->extents.z;
    verts_a[6].x = +a->extents.x;
    verts_a[6].y = +a->extents.y;
    verts_a[6].z = -a->extents.z;
    verts_a[7].x = +a->extents.x;
    verts_a[7].y = +a->extents.y;
    verts_a[7].z = +a->extents.z;
    for (int i = 0; i < 8; ++i) {
        verts_a[i] = quat_mul_vec3(a->orientation, verts_a[i]);
        verts_a[i] = vec3_add(verts_a[i], a->center);
    }

    ta_vec3 verts_b[8] = { 0 };
    verts_b[0].x = -b->extents.x;
    verts_b[0].y = -b->extents.y;
    verts_b[0].z = -b->extents.z;
    verts_b[1].x = -b->extents.x;
    verts_b[1].y = -b->extents.y;
    verts_b[1].z = +b->extents.z;
    verts_b[2].x = -b->extents.x;
    verts_b[2].y = +b->extents.y;
    verts_b[2].z = -b->extents.z;
    verts_b[3].x = -b->extents.x;
    verts_b[3].y = +b->extents.y;
    verts_b[3].z = +b->extents.z;
    verts_b[4].x = +b->extents.x;
    verts_b[4].y = -b->extents.y;
    verts_b[4].z = -b->extents.z;
    verts_b[5].x = +b->extents.x;
    verts_b[5].y = -b->extents.y;
    verts_b[5].z = +b->extents.z;
    verts_b[6].x = +b->extents.x;
    verts_b[6].y = +b->extents.y;
    verts_b[6].z = -b->extents.z;
    verts_b[7].x = +b->extents.x;
    verts_b[7].y = +b->extents.y;
    verts_b[7].z = +b->extents.z;
    for (int i = 0; i < 8; ++i) {
        verts_b[i] = quat_mul_vec3(b->orientation, verts_b[i]);
        verts_b[i] = vec3_add(verts_b[i], b->center);
    }



    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) {
            ta_sphere simplex_sphere = { 0 };
            simplex_sphere.center = vec3_sub(verts_a[i], verts_b[j]);
            simplex_sphere.radius = 0.01f;
            ta_primitive_push_sphere(0, simplex_sphere, TA_COLOR_WHITE);
        }
    }

    // NOTE: The last vertex in the simplex array is always called "A" and is the most recently
    // added support point (see Casey's explanation: https://www.youtube.com/watch?v=Qupqu1xe7Io)
    ta_gjk_simplex simplex = { 0 };

    // Seed with some direction (difference between center points, or +Y if center points are the same)
    ta_vec3 d = vec3_sub(b->center, a->center);
    if (vec3_zero(d)) {
        d = VEC3_Y;
    }

    int step = 0;

    // Find support point on "Minkowski difference"
    ta_vec3 sa = ta_support_obb(a, d);
    ta_vec3 sb = ta_support_obb(b, vec3_neg(d));
    ta_vec3 s = vec3_sub(sa, sb);
    simplex.vertices[simplex.count++] = s;
    step++;

    //--------------------------------------------
    const float hue_inc = 360.0f / 8.0f;
    //--------------------------------------------

    // Start looking in opposite direction of first support point
    d = vec3_neg(s);

#if 1
    // DEBUG(cleanup): Debug rendering
    if (step == *gjk_step) {
        gjk_debug_draw(sa, sb, d, &simplex);
    }
#endif

    bool colliding = false;
    for (;;) {
        sa = ta_support_obb(a, d);
        sb = ta_support_obb(b, vec3_neg(d));
        s = vec3_sub(sa, sb);
        DLB_ASSERT(simplex.count < ARRAY_SIZE(simplex.vertices));
        simplex.vertices[simplex.count++] = s;
        step++;
#if 0
        //--------------------------------------------
        // DEBUG(cleanup): Debug rendering
        ta_mat3 hue_rot = mat3_hue_rotation(step * 360.0f / 8.0f);
        ta_rgb red = { 1.0f, 0.0f, 0.0f };
        ta_rgb red_shift = mat3_mul_rgb(&hue_rot, red);
        ta_rgba color = rgba_init(red_shift.r, red_shift.g, red_shift.b, 1.0f);
        ta_sphere dbg_sphere = { 0 };
        dbg_sphere.center = sa;
        dbg_sphere.radius = 0.02f * step;
        ta_primitive_push_sphere(0, dbg_sphere, color);
        dbg_sphere.center = sb;
        ta_primitive_push_sphere(0, dbg_sphere, color);
        ta_line_3d line = { 0 };
        line.p0 = a->center;
        line.p1 = vec3_add(a->center, vec3_scalef(d, 0.2f * step));
        ta_primitive_push_line_3d(0, line, TA_COLOR_WHITE, color);
        //--------------------------------------------
#endif

        if (vec3_dot(s, d) < 0) {
            // Failed to find support point closer to origin
            break;
        }

        colliding = ta_gjk_do_simplex(&simplex, &d);

#if 1
        // DEBUG(cleanup): Debug rendering
        if (step == *gjk_step) {
            gjk_debug_draw(sa, sb, d, &simplex);
        }
#endif

        if (vec3_zero(d)) {
            // Origin lies on the simplex exactly, not sure how to pick a new search direction, so let's
            // just return true. May need to handle this differently when we start generating manifolds.
            colliding = true;
        }

        if (colliding) {
            // Simplex contains origin, shapes are intersecting
            break;
        }
    }

    *gjk_steps = step;

#if 1
    // DEBUG(cleanup): Debug rendering
    if (step == *gjk_step) {
        gjk_debug_draw(sa, sb, d, &simplex);
    }
#endif

    return colliding;
}