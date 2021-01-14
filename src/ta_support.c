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

bool ta_gjk_intersect_obb(ta_obb *a, ta_obb *b)
{
    // NOTE: The last vertex in the simplex array is always called "A" and is the most recently
    // added support point (see Casey's explanation: https://www.youtube.com/watch?v=Qupqu1xe7Io)
    ta_gjk_simplex simplex = { 0 };

    // Seed with some direction (difference between center points chosen arbitrarily)
    ta_vec3 d = vec3_sub(b->center, a->center);

    // Find support point on "Minkowski difference"
    ta_vec3 sa = ta_support_obb(a, d);
    ta_vec3 sb = ta_support_obb(b, vec3_neg(d));
    ta_vec3 s = vec3_sub(sa, sb);
    simplex.vertices[simplex.count++] = s;

    //--------------------------------------------
    const float hue_inc = 360.0f / 8.0f;
    //--------------------------------------------

    // Start looking in opposite direction of first support point
    d = vec3_neg(s);

    int i = 1;
    for (;;) {
        sa = ta_support_obb(a, d);

        //--------------------------------------------
        // DEBUG(cleanup): Debug rendering
        ta_mat3 hue_rot = mat3_hue_rotation(i * 360.0f / 8.0f);
        ta_rgb red = { 1.0f, 0.0f, 0.0f };
        ta_rgb red_shift = mat3_mul_rgb(&hue_rot, red);
        ta_rgba color = rgba_init(red_shift.r, red_shift.g, red_shift.b, 1.0f);
        ta_sphere dbg_sphere = { 0 };
        dbg_sphere.center = sa;
        dbg_sphere.radius = 0.02f * i;
        ta_primitive_push_sphere(0, dbg_sphere, color);
        dbg_sphere.center = sb;
        ta_primitive_push_sphere(0, dbg_sphere, color);
        ta_line_3d line = { 0 };
        line.p0 = a->center;
        line.p1 = vec3_add(a->center, vec3_scalef(d, 0.2f * i));
        ta_primitive_push_line_3d(0, line, TA_COLOR_WHITE, color);
        //--------------------------------------------

        sb = ta_support_obb(b, vec3_neg(d));
        s = vec3_sub(sa, sb);
        if (vec3_dot(s, d) < 0) {
            // Failed to find support point closer to origin
            return false;
        }
        // Add new support point to simplex and update simplex
        DLB_ASSERT(simplex.count < ARRAY_SIZE(simplex.vertices));
        simplex.vertices[simplex.count++] = s;
        if (ta_gjk_do_simplex(&simplex, &d)) {
            // Simplex contains origin, shapes are intersecting
            return true;
        }
        i++;
    }
}