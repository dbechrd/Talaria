#include "ta_math.h"
#include "dlb/dlb_types.h"
#include <math.h>
#include <float.h>
#include <stdio.h>

const ta_vec2 VEC2_ZERO = { 0.0f, 0.0f };
const ta_vec2 VEC2_ONE = { 1.0f, 1.0f };
const ta_vec2 VEC2_X = { 1.0f, 0.0f };
const ta_vec2 VEC2_Y = { 0.0f, 1.0f };
const ta_vec2 VEC2_NX = { -1.0f, 0.0f };
const ta_vec2 VEC2_NY = { 0.0f, -1.0f };
const ta_vec2 VEC2_MIN = { FLT_MIN, FLT_MIN };
const ta_vec2 VEC2_MAX = { FLT_MAX, FLT_MAX };
const ta_vec2 VEC2_EPSILON = { TA_EPSILON, TA_EPSILON };

const ta_vec3 VEC3_ZERO = { 0.0f, 0.0f, 0.0f };
const ta_vec3 VEC3_ONE = { 1.0f, 1.0f, 1.0f };
const ta_vec3 VEC3_X = { 1.0f, 0.0f, 0.0f };
const ta_vec3 VEC3_Y = { 0.0f, 1.0f, 0.0f };
const ta_vec3 VEC3_Z = { 0.0f, 0.0f, 1.0f };
const ta_vec3 VEC3_NX = { -1.0f, 0.0f, 0.0f };
const ta_vec3 VEC3_NY = { 0.0f, -1.0f, 0.0f };
const ta_vec3 VEC3_NZ = { 0.0f, 0.0f, -1.0f };
const ta_vec3 VEC3_MIN = { FLT_MIN, FLT_MIN, FLT_MIN };
const ta_vec3 VEC3_MAX = { FLT_MAX, FLT_MAX, FLT_MAX };
const ta_vec3 VEC3_EPSILON = { TA_EPSILON, TA_EPSILON, TA_EPSILON };

const ta_vec4 VEC4_ZERO = { 0.0f, 0.0f, 0.0f, 0.0f };

const ta_vec4 QUAT_IDENT = { 0.0f, 0.0f, 0.0f, 1.0f };

const ta_mat3 MAT3_IDENT = {
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 1.0f,
};
const ta_mat3 MAT3_ZERO = {
    0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f,
};

const ta_mat4 MAT4_IDENT = {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f
};

const ta_rgb TA_COLOR3_BLACK = { 0.0f, 0.0f, 0.0f };
const ta_rgb TA_COLOR3_WHITE = { 1.0f, 1.0f, 1.0f };

const ta_rgba TA_COLOR_INVIS       = { 0.0f, 0.0f, 0.0f, 0.0f };
const ta_rgba TA_COLOR_BLACK       = { 0.0f, 0.0f, 0.0f, 1.0f };
const ta_rgba TA_COLOR_RED         = { 1.0f, 0.1f, 0.1f, 1.0f };
const ta_rgba TA_COLOR_DARK_RED    = { 0.7f, 0.0f, 0.0f, 1.0f };
const ta_rgba TA_COLOR_DARK_REDA   = { 0.7f, 0.0f, 0.0f, 0.7f };
const ta_rgba TA_COLOR_GREEN       = { 0.1f, 1.0f, 0.1f, 1.0f };
const ta_rgba TA_COLOR_DARK_GREEN  = { 0.0f, 0.7f, 0.0f, 1.0f };
const ta_rgba TA_COLOR_DARK_GREENA = { 0.0f, 0.7f, 0.0f, 0.7f };
const ta_rgba TA_COLOR_BLUE        = { 0.1f, 0.1f, 1.0f, 1.0f };
const ta_rgba TA_COLOR_DARK_BLUE   = { 0.0f, 0.0f, 0.7f, 1.0f };
const ta_rgba TA_COLOR_DARK_BLUEA  = { 0.0f, 0.0f, 0.7f, 0.7f };
const ta_rgba TA_COLOR_BLUE1       = { 0.1f, 0.1f, 1.0f, 1.0f };
const ta_rgba TA_COLOR_BLUE2       = { 0.2f, 0.2f, 1.0f, 1.0f };
const ta_rgba TA_COLOR_BLUE3       = { 0.3f, 0.3f, 1.0f, 1.0f };
const ta_rgba TA_COLOR_BLUE4       = { 0.4f, 0.4f, 1.0f, 1.0f };
const ta_rgba TA_COLOR_BLUE5       = { 0.5f, 0.5f, 1.0f, 1.0f };
const ta_rgba TA_COLOR_BLUE5A      = { 0.5f, 0.5f, 1.0f, 0.5f };
const ta_rgba TA_COLOR_BLUE6       = { 0.6f, 0.6f, 1.0f, 1.0f };
const ta_rgba TA_COLOR_BLUE7       = { 0.7f, 0.7f, 1.0f, 1.0f };
const ta_rgba TA_COLOR_BLUE8       = { 0.8f, 0.8f, 1.0f, 1.0f };
const ta_rgba TA_COLOR_BLUE9       = { 0.9f, 0.9f, 1.0f, 1.0f };
const ta_rgba TA_COLOR_CYAN        = { 0.1f, 1.0f, 1.0f, 1.0f };
const ta_rgba TA_COLOR_MAGENTA     = { 1.0f, 0.1f, 1.0f, 1.0f };
const ta_rgba TA_COLOR_YELLOW      = { 1.0f, 1.0f, 0.1f, 1.0f };
const ta_rgba TA_COLOR_ORANGE      = { 1.0f, 0.7f, 0.1f, 1.0f };
const ta_rgba TA_COLOR_PINK        = { 1.0f, 0.6f, 1.0f, 1.0f };
const ta_rgba TA_COLOR_GRAY1       = { 0.1f, 0.1f, 0.1f, 1.0f };
const ta_rgba TA_COLOR_GRAY2       = { 0.2f, 0.2f, 0.2f, 1.0f };
const ta_rgba TA_COLOR_GRAY3       = { 0.3f, 0.3f, 0.3f, 1.0f };
const ta_rgba TA_COLOR_GRAY3A      = { 0.3f, 0.3f, 0.3f, 0.7f };
const ta_rgba TA_COLOR_GRAY4       = { 0.4f, 0.4f, 0.4f, 1.0f };
const ta_rgba TA_COLOR_GRAY5       = { 0.5f, 0.5f, 0.5f, 1.0f };
const ta_rgba TA_COLOR_GRAY6       = { 0.6f, 0.6f, 0.6f, 1.0f };
const ta_rgba TA_COLOR_GRAY7       = { 0.7f, 0.7f, 0.7f, 1.0f };
const ta_rgba TA_COLOR_GRAY8       = { 0.8f, 0.8f, 0.8f, 1.0f };
const ta_rgba TA_COLOR_GRAY9       = { 0.9f, 0.9f, 0.9f, 1.0f };
const ta_rgba TA_COLOR_WHITE       = { 1.0f, 1.0f, 1.0f, 1.0f };
const ta_rgba TA_COLOR_WHITE_ALPHA = { 1.0f, 1.0f, 1.0f, 0.5f };
const ta_rgba TA_COLOR_SHADOW      = { 0.0f, 0.0f, 0.0f, 0.8f };

const ta_size TA_SIZE_ZERO = { 0, 0 };
const ta_vec2i TA_VEC2I_ZERO = { 0, 0 };

int clamp(int d, int min, int max)
{
    if (d <= min) {
        return min;
    } else if (d >= max) {
        return max;
    } else {
        return d;
    }
}
float clampf(float f, float min, float max)
{
    if (f <= min) {
        return min;
    } else if (f >= max) {
        return max;
    } else {
        return f;
    }
}

void vec2_print(FILE *file, ta_vec2 v)
{
    fprintf(file, "vec2: %f %f\n", v.x, v.y);
}
int vec2_zero(ta_vec2 v)
{
    return v.x == 0.0f && v.y == 0.0f;
}
int vec2_tiny(ta_vec2 v)
{
    return fabs(v.x) < TA_EPSILON &&
           fabs(v.y) < TA_EPSILON;
}
int vec2_equal(ta_vec2 a, ta_vec2 b)
{
    return fabs((double)a.x - b.x) < TA_EPSILON &&
           fabs((double)a.y - b.y) < TA_EPSILON;
}
ta_vec2 vec2_neg(ta_vec2 v)
{
    ta_vec2 result;
    result.x = -v.x;
    result.y = -v.y;
    return result;
}
ta_vec2 vec2_add(ta_vec2 a, ta_vec2 b)
{
    ta_vec2 result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    return result;
}
ta_vec2 vec2_sub(ta_vec2 a, ta_vec2 b)
{
    ta_vec2 result;
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    return result;
}
ta_vec2 vec2_scalef(ta_vec2 a, float s)
{
    ta_vec2 result;
    result.x = a.x * s;
    result.y = a.y * s;
    return result;
}
float vec2_len(ta_vec2 v)
{
    float len = sqrtf(v.x * v.x + v.y * v.y);
    //if (fabsf(len) < TA_EPSILON) {
    //    len = 0.0f;
    //}
    return len;
}
float vec2_len2(ta_vec2 v)
{
    float len2 = v.x * v.x + v.y * v.y;
    //if (fabsf(len2) < TA_EPSILON) {
    //    len2 = 0.0f;
    //}
    return len2;
}
ta_vec2 vec2_normalize(ta_vec2 v)
{
    float len = vec2_len(v);
    ta_vec2 result = v;
    if (len) {
        result = vec2_scalef(result, 1.0f / len);
    } else {
        DLB_ASSERT(!"WARNING: Normalizing zero vector\n");
        result = VEC2_ZERO;
    }
    return result;
}
void vec3_print(FILE *file, ta_vec3 v)
{
    fprintf(file, "vec3: %f %f %f\n", v.x, v.y, v.z);
}
int vec3_zero(ta_vec3 v)
{
    return v.x == 0.0f && v.y == 0.0f && v.z == 0.0f;
}
int vec3_tiny(ta_vec3 v)
{
    return fabs(v.x) < TA_EPSILON &&
           fabs(v.y) < TA_EPSILON &&
           fabs(v.z) < TA_EPSILON;
}
int vec3_equal(ta_vec3 a, ta_vec3 b)
{
    return fabs((double)a.x - b.x) < TA_EPSILON &&
           fabs((double)a.y - b.y) < TA_EPSILON &&
           fabs((double)a.z - b.z) < TA_EPSILON;
}
int vec3_bad(ta_vec3 v)
{
    return (v.x && !isnormal(v.x)) &&
           (v.y && !isnormal(v.y)) &&
           (v.z && !isnormal(v.z));
}
int vec3_good(ta_vec3 v)
{
    return (!v.x || isnormal(v.x)) &&
           (!v.y || isnormal(v.y)) &&
           (!v.z || isnormal(v.z));
}
ta_vec3 vec3_init(float x, float y, float z)
{
    ta_vec3 v;
    v.x = x;
    v.y = y;
    v.z = z;
    return v;
}
ta_vec3 vec3_neg(ta_vec3 v)
{
    ta_vec3 result;
    result.x = -v.x;
    result.y = -v.y;
    result.z = -v.z;
    return result;
}
ta_vec3 vec3_add(ta_vec3 a, ta_vec3 b)
{
    ta_vec3 result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;
    return result;
}
ta_vec3 vec3_add3(ta_vec3 a, ta_vec3 b, ta_vec3 c)
{
    ta_vec3 result;
    result.x = a.x + b.x + c.x;
    result.y = a.y + b.y + c.y;
    result.z = a.z + b.z + c.z;
    return result;
}
ta_vec3 vec3_sub(ta_vec3 a, ta_vec3 b)
{
    ta_vec3 result;
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    result.z = a.z - b.z;
    return result;
}
ta_vec3 vec3_scalef(ta_vec3 a, float s)
{
    ta_vec3 result;
    result.x = a.x * s;
    result.y = a.y * s;
    result.z = a.z * s;
    return result;
}
float vec3_len(ta_vec3 v)
{
    float len = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
    //if (fabsf(len) < TA_EPSILON) {
    //    len = 0.0f;
    //}
    return len;
}
float vec3_len2(ta_vec3 v)
{
    float len2 = v.x * v.x + v.y * v.y + v.z * v.z;
    //if (fabsf(len2) < TA_EPSILON) {
    //    len2 = 0.0f;
    //}
    return len2;
}
ta_vec3 vec3_normalize(ta_vec3 v)
{
    float len = vec3_len(v);
    ta_vec3 result = v;
    if (len) {
        result = vec3_scalef(result, 1.0f / len);
    } else {
        //DLB_ASSERT(!"WARNING: Normalizing zero vector\n");
        result = VEC3_ZERO;
    }
    return result;
}
float vec3_dot(ta_vec3 a, ta_vec3 b)
{
    float dot = a.x * b.x + a.y * b.y + a.z * b.z;
    //if (fabsf(dot) < TA_EPSILON) {
    //    dot = 0.0f;
    //}
    return dot;
}
ta_vec3 vec3_cross(ta_vec3 a, ta_vec3 b)
{
    ta_vec3 result;
    result.x = a.y * b.z - a.z * b.y;
    result.y = a.z * b.x - a.x * b.z;
    result.z = a.x * b.y - a.y * b.x;
    return result;
}
ta_vec3 vec3_lerp(ta_vec3 a, ta_vec3 b, float w)
{
    ta_vec3 result;
    result.x = a.x + w * (b.x - a.x);
    result.y = a.y + w * (b.y - a.y);
    result.z = a.z + w * (b.z - a.z);
    return result;
}
ta_vec3 vec3_perp(ta_vec3 v)
{
    // Finds an abitrary, reasonable perpendicular vector to v, if possible
    ta_vec3 result = { 0 };
    if (fabs(v.x) >= TA_EPSILON || fabs(v.y) >= TA_EPSILON) {
        result.x = v.y;
        result.y = -v.x;
    } else {
        DLB_ASSERT(fabs(v.z) >= TA_EPSILON);
        result.y = -v.z;
        result.z = v.y;
    }
    return result;
}

void vec4_print(FILE *file, ta_vec4 v)
{
    fprintf(file, "vec4: %f %f %f %f\n", v.x, v.y, v.z, v.w);
}
int vec4_zero(ta_vec4 v)
{
    return v.x == 0.0f && v.y == 0.0f && v.z == 0.0f && v.w == 0.0f;
}
int vec4_tiny(ta_vec4 v)
{
    return fabs(v.x) < TA_EPSILON &&
           fabs(v.y) < TA_EPSILON &&
           fabs(v.z) < TA_EPSILON &&
           fabs(v.w) < TA_EPSILON;
}
int vec4_equal(ta_vec4 a, ta_vec4 b)
{
    return fabs((double)a.x - b.x) < TA_EPSILON &&
           fabs((double)a.y - b.y) < TA_EPSILON &&
           fabs((double)a.z - b.z) < TA_EPSILON &&
           fabs((double)a.w - b.w) < TA_EPSILON;
}
ta_vec4 vec4_init(float x, float y, float z, float w)
{
    ta_vec4 v;
    v.x = x;
    v.y = y;
    v.z = z;
    v.w = w;
    return v;
}
ta_vec4 vec4_init_vw(ta_vec3 v, float w)
{
    ta_vec4 result;
    result.x = v.x;
    result.y = v.y;
    result.z = v.z;
    result.w = w;
    return result;
}

void quat_print(FILE *file, ta_vec4 q)
{
    fprintf(file, "v: %f %f %f %f\n", q.x, q.y, q.z, q.w);
}
int quat_zero(ta_vec4 v)
{
    return v.x == 0.0f && v.y == 0.0f && v.z == 0.0f && v.w == 0.0f;
}
int quat_ident(ta_vec4 q)
{
    return q.x == 0.0f && q.y == 0.0f && q.z == 0.0f && q.w == 1.0f;
}
int quat_equal(ta_vec4 a, ta_vec4 b)
{
    return
        fabs((double)a.x - b.x) < TA_EPSILON &&
        fabs((double)a.y - b.y) < TA_EPSILON &&
        fabs((double)a.z - b.z) < TA_EPSILON &&
        fabs((double)a.w - b.w) < TA_EPSILON;
}
ta_vec4 quat_init(float x, float y, float z, float w)
{
    ta_vec4 q;
    q.x = x;
    q.y = y;
    q.z = z;
    q.w = w;
    return q;
}
ta_vec4 quat_from_axis_angle(ta_vec3 axis, float deg)
{
    DLB_ASSERT(vec3_equal(axis, vec3_normalize(axis)));

    ta_vec4 result;
    float s = sinf(DEG_TO_RADF(deg) / 2.0f);
    result.w = cosf(DEG_TO_RADF(deg) / 2.0f);
    result.x = axis.x * s;
    result.y = axis.y * s;
    result.z = axis.z * s;
    quat_normalize(result);
    return result;
}
ta_vec4 quat_from_vec_vec(ta_vec3 from, ta_vec3 to)
{
    // http://physicsforgames.blogspot.com/2010/03/quaternion-tricks.html
    ta_vec3 h = vec3_add(from, to);
    h = vec3_normalize(h);

    ta_vec4 result;
    result.w = vec3_dot(from, h);
    result.x = from.y*h.z - from.z*h.y;
    result.y = from.z*h.x - from.x*h.z;
    result.z = from.x*h.y - from.y*h.x;
    result = quat_normalize(result);
    return result;
}
float quat_norm_sq(ta_vec4 q)
{
    return q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w;
}
float quat_norm(ta_vec4 q)
{
    return sqrtf(quat_norm_sq(q));
}
ta_vec4 quat_normalize(ta_vec4 q)
{
    float norm = quat_norm(q);

    // Quaternions of norm 1.0f are already normalized
    if (norm == 1.0f)
        return q;

    ta_vec4 result = { 0 };
    if (norm > TA_EPSILON) {
        float inv_norm = 1.0f / norm;
        result.x = q.x * inv_norm;
        result.y = q.y * inv_norm;
        result.z = q.z * inv_norm;
        result.w = q.w * inv_norm;
    } else {
        //DLB_ASSERT(!"WARNING: Normalizing bad quaternion\n");
        result = QUAT_IDENT;
    }

    return result;
}
ta_vec4 quat_conjugate(ta_vec4 q)
{
    ta_vec4 result;
    result.x = -q.x;
    result.y = -q.y;
    result.z = -q.z;
    result.w = q.w;
    return result;
}
ta_vec4 quat_inverse(ta_vec4 q)
{
    ta_vec4 result = quat_conjugate(q);
    float norm_sq = quat_norm_sq(result);

    // Inverse == conjugate for normalized ("unit-norm") quaternions
    if (fabs(norm_sq - 1.0f) < TA_EPSILON)
        return result;

    assert(norm_sq != 0.0f);
    float inv_norm_sq = 1.0f / norm_sq;
    result.w *= inv_norm_sq;
    result.x *= inv_norm_sq;
    result.y *= inv_norm_sq;
    result.z *= inv_norm_sq;
    return result;
}
ta_vec4 quat_scale(ta_vec4 q, float s)
{
    ta_vec4 result;
    result.x = q.x *= s;
    result.y = q.y *= s;
    result.z = q.z *= s;
    result.w = q.w *= s;
    return result;
}
ta_vec4 quat_mul(ta_vec4 a, ta_vec4 b)
{
    ta_vec4 result;
    result.w = a.w*b.w - a.x*b.x - a.y*b.y - a.z*b.z;
    result.x = a.w*b.x + a.x*b.w + a.y*b.z - a.z*b.y;
    result.y = a.w*b.y - a.x*b.z + a.y*b.w + a.z*b.x;
    result.z = a.w*b.z + a.x*b.y - a.y*b.x + a.z*b.w;
    return result;
}
ta_vec4 quat_add(ta_vec4 a, ta_vec4 b)
{
    ta_vec4 result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;
    result.w = a.w + b.w;
    return result;
}
ta_vec4 quat_sub(ta_vec4 a, ta_vec4 b)
{
    ta_vec4 result;
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    result.z = a.z - b.z;
    result.w = a.w - b.w;
    return result;
}
float quat_dot(ta_vec4 a, ta_vec4 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}
static ta_vec4 quat_negate(ta_vec4 q)
{
    ta_vec4 result;
    result.x = -q.x;
    result.y = -q.y;
    result.z = -q.z;
    result.w = -q.w;
    return result;
}
ta_vec4 quat_nlerp(ta_vec4 a, ta_vec4 b, float w)
{
    float dot = quat_dot(a, b);
    float inv_w = 1.0f - w;
    if (dot < 0.0f) {
        b = quat_negate(b);
    }

    ta_vec4 result;
    result.w = inv_w*a.w + w*b.w;
    result.x = inv_w*a.x + w*b.x;
    result.y = inv_w*a.y + w*b.y;
    result.z = inv_w*a.z + w*b.z;
    result = quat_normalize(result);
    return result;
}
ta_vec4 quat_slerp(ta_vec4 a, ta_vec4 b, float w)
{
    // http://number-none.com/product/Understanding%20Slerp,%20Then%20Not%20Using%20It/
    DLB_ASSERT(!"Not yet implemented");

    UNUSED(a);
    UNUSED(b);
    UNUSED(w);
    return QUAT_IDENT;
}
// NOTE: This is the same as vec3_rotate_quat, use this version
ta_vec3 quat_mul_vec3(ta_vec4 q, ta_vec3 v)
{
    // http://physicsforgames.blogspot.com/2010/03/quaternion-tricks.html
    float x1 = q.y*v.z - q.z*v.y;
    float y1 = q.z*v.x - q.x*v.z;
    float z1 = q.x*v.y - q.y*v.x;

    float x2 = q.w*x1 + q.y*z1 - q.z*y1;
    float y2 = q.w*y1 + q.z*x1 - q.x*z1;
    float z2 = q.w*z1 + q.x*y1 - q.y*x1;

    ta_vec3 result;
    result.x = v.x + 2.0f*x2;
    result.y = v.y + 2.0f*y2;
    result.z = v.z + 2.0f*z2;
    return result;
}

void mat3_print(FILE *file, const ta_mat3 *m)
{
    for (int i = 0; i < 3; i++) {
        fprintf(file, "mat3[%d]: %f %f %f\n", i,
            m->data.v[i].x,
            m->data.v[i].y,
            m->data.v[i].z
        );
    }
}
ta_mat3 mat3_init(
    float m00, float m01, float m02,
    float m10, float m11, float m12,
    float m20, float m21, float m22)
{
    ta_mat3 result;
    result.data.f[0][0] = m00;
    result.data.f[0][1] = m01;
    result.data.f[0][2] = m02;
    result.data.f[1][0] = m10;
    result.data.f[1][1] = m11;
    result.data.f[1][2] = m12;
    result.data.f[2][0] = m20;
    result.data.f[2][1] = m21;
    result.data.f[2][2] = m22;
    return result;
}
int mat3_equal(const ta_mat3 *a, const ta_mat3 *b)
{
    for (int i = 0; i < 9; ++i) {
        if (a->data.arr[i] != b->data.arr[i]) {
            return 0;
        }
    }
    return 1;
}
ta_mat3 mat3_transpose(const ta_mat3 *m)
{
    ta_mat3 result = mat3_init(
        m->data.f[0][0], m->data.f[1][0], m->data.f[2][0],
        m->data.f[0][1], m->data.f[1][1], m->data.f[2][1],
        m->data.f[0][2], m->data.f[1][2], m->data.f[2][2]
    );
    return result;
}
ta_mat3 mat3_rotate_x(float deg)
{
    float rad = DEG_TO_RADF(deg);
    float s = sinf(rad);
    float c = cosf(rad);
    ta_mat3 result = mat3_init(
        1, 0, 0,
        0, c,-s,
        0, s, c
    );
    return result;
}
ta_mat3 mat3_rotate_y(float deg)
{
    float rad = DEG_TO_RADF(deg);
    float s = sinf(rad);
    float c = cosf(rad);
    ta_mat3 result = mat3_init(
        c, 0, s,
        0, 1, 0,
       -s, 0, c
    );
    return result;
}
ta_mat3 mat3_rotate_z(float deg)
{
    float rad = DEG_TO_RADF(deg);
    float s = sinf(rad);
    float c = cosf(rad);
    ta_mat3 result = mat3_init(
        c,-s, 0,
        s, c, 0,
        0, 0, 1
    );
    return result;
}
ta_mat3 mat3_rotate_quat(ta_vec4 q)
{
    DLB_ASSERT(quat_equal(q, quat_normalize(q)));

    float xx = q.x * q.x;
    float yy = q.y * q.y;
    float zz = q.z * q.z;
    float xy = q.x * q.y;
    float xw = q.x * q.w;
    float xz = q.x * q.z;
    float yz = q.y * q.z;
    float yw = q.y * q.w;
    float zw = q.z * q.w;

    ta_mat3 m = { 0 };
    m.data.f[0][0] = 1 - 2*yy - 2*zz;
    m.data.f[0][1] = 2*xy - 2*zw;
    m.data.f[0][2] = 2*xz + 2*yw;
    m.data.f[1][0] = 2*xy + 2*zw;
    m.data.f[1][1] = 1 - 2*xx - 2*zz;
    m.data.f[1][2] = 2*yz - 2*xw;
    m.data.f[2][0] = 2*xz - 2*yw;
    m.data.f[2][1] = 2*yz + 2*xw;
    m.data.f[2][2] = 1 - 2*xx - 2*yy;
    return m;
}
ta_mat3 mat3_mul(const ta_mat3 *a, const ta_mat3 *b)
{
    ta_mat3 result = { 0 };
    for (int j = 0; j < 3; ++j) {
        for (int i = 0; i < 3; ++i) {
            for (int n = 0; n < 3; ++n) {
                result.data.f[j][i] += a->data.f[j][n] * b->data.f[n][i];
            }
        }
    }
    return result;
}
// matrix3 * column vector
ta_vec3 mat3_mul_vec3(const ta_mat3 *m, ta_vec3 v)
{
    ta_vec3 result;
    result.x =
        m->data.f[0][0] * v.x +
        m->data.f[0][1] * v.y +
        m->data.f[0][2] * v.z;
    result.y =
        m->data.f[1][0] * v.x +
        m->data.f[1][1] * v.y +
        m->data.f[1][2] * v.z;
    result.z =
        m->data.f[2][0] * v.x +
        m->data.f[2][1] * v.y +
        m->data.f[2][2] * v.z;

    if (fabsf(result.x) < TA_EPSILON) result.x = 0.0f;
    if (fabsf(result.y) < TA_EPSILON) result.y = 0.0f;
    if (fabsf(result.z) < TA_EPSILON) result.z = 0.0f;

    return result;
}
//// row vector * matrix3
//ta_vec3 vec3_mul_mat3(ta_vec3 v, const ta_mat3 *m)
//{
//    ta_vec3 result;
//    result.x =
//        v.x * m->data.f[0][0] +
//        v.y * m->data.f[1][0] +
//        v.z * m->data.f[2][0];
//    result.y =
//        v.x * m->data.f[0][1] +
//        v.y * m->data.f[1][1] +
//        v.z * m->data.f[2][1];
//    result.z =
//        v.x * m->data.f[0][2] +
//        v.y * m->data.f[1][2] +
//        v.z * m->data.f[2][2];
//
//    if (fabsf(result.x) < TA_EPSILON) result.x = 0.0f;
//    if (fabsf(result.y) < TA_EPSILON) result.y = 0.0f;
//    if (fabsf(result.z) < TA_EPSILON) result.z = 0.0f;
//
//    return result;
//}
ta_rgb mat3_mul_rgb(const ta_mat3 *m, ta_rgb v)
{
    ta_rgb result;
    result.r =
        m->data.f[0][0] * v.r +
        m->data.f[0][1] * v.g +
        m->data.f[0][2] * v.b;
    result.g =
        m->data.f[1][0] * v.r +
        m->data.f[1][1] * v.g +
        m->data.f[1][2] * v.b;
    result.b =
        m->data.f[2][0] * v.r +
        m->data.f[2][1] * v.g +
        m->data.f[2][2] * v.b;

    if (result.r < TA_EPSILON) {
        result.r = 0.0f;
    } else if (result.r > 1.0f) {
        result.r = 1.0f;
    }

    if (result.g < TA_EPSILON) {
        result.g = 0.0f;
    } else if (result.g > 1.0f) {
        result.g = 1.0f;
    }

    if (result.b < TA_EPSILON) {
        result.b = 0.0f;
    } else if (result.b > 1.0f) {
        result.b = 1.0f;
    }

    return result;
}
ta_mat3 mat3_hue_rotation(float degrees)
{
    // Note: This doesn't preserve luminance. Eventually, do this instead:
    //       http://www.graficaobscura.com/matrix/index.html
    float cosa = cosf(DEG_TO_RADF(degrees));
    float sina = sinf(DEG_TO_RADF(degrees));
    float onecos = 1.0f - cosa;
    float sqthird = sqrtf(1.0f / 3.0f);
    ta_mat3 m;
    m.data.f[0][0] = cosa + onecos / 3.0f;
    m.data.f[0][1] = onecos / 3.0f - sqthird * sina;
    m.data.f[0][2] = onecos / 3.0f + sqthird * sina;
    m.data.f[1][0] = m.data.f[0][2];
    m.data.f[1][1] = m.data.f[0][0];
    m.data.f[1][2] = m.data.f[0][1];
    m.data.f[2][0] = m.data.f[0][1];
    m.data.f[2][1] = m.data.f[0][2];
    m.data.f[2][2] = m.data.f[0][0];
    return m;
}
float mat3_deter(const ta_mat3 *m)
{
    float a = m->data.f[0][0];
    float b = m->data.f[0][1];
    float c = m->data.f[0][2];
    float d = m->data.f[1][0];
    float e = m->data.f[1][1];
    float f = m->data.f[1][2];
    float g = m->data.f[2][0];
    float h = m->data.f[2][1];
    float i = m->data.f[2][2];

    float result =
        a * (e*i - f*h) +
        b * (f*g - d*i) +
        c * (d*h - e*g);
    return result;
}

void mat4_print(FILE *file, const ta_mat4 *m)
{
    for (int i = 0; i < 4; i++) {
        fprintf(file, "mat4[%d]: %f %f %f %f\n", i,
            m->data.v[i].x,
            m->data.v[i].y,
            m->data.v[i].z,
            m->data.v[i].w
        );
    }
}
ta_mat4 mat4_init(
    float m00, float m01, float m02, float m03,
    float m10, float m11, float m12, float m13,
    float m20, float m21, float m22, float m23,
    float m30, float m31, float m32, float m33)
{
    ta_mat4 result;
    result.data.f[0][0] = m00;
    result.data.f[0][1] = m01;
    result.data.f[0][2] = m02;
    result.data.f[0][3] = m03;
    result.data.f[1][0] = m10;
    result.data.f[1][1] = m11;
    result.data.f[1][2] = m12;
    result.data.f[1][3] = m13;
    result.data.f[2][0] = m20;
    result.data.f[2][1] = m21;
    result.data.f[2][2] = m22;
    result.data.f[2][3] = m23;
    result.data.f[3][0] = m30;
    result.data.f[3][1] = m31;
    result.data.f[3][2] = m32;
    result.data.f[3][3] = m33;
    return result;
}
int mat4_equal(const ta_mat4 *a, const ta_mat4 *b)
{
    for (int i = 0; i < 16; ++i) {
        if (fabs((double)a->data.arr[i] - b->data.arr[i]) > TA_EPSILON) {
            return 0;
        }
    }
    return 1;
}
ta_mat4 mat4_transpose(const ta_mat4 *m)
{
    ta_mat4 result = mat4_init(
        m->data.f[0][0], m->data.f[1][0], m->data.f[2][0], m->data.f[3][0],
        m->data.f[0][1], m->data.f[1][1], m->data.f[2][1], m->data.f[3][1],
        m->data.f[0][2], m->data.f[1][2], m->data.f[2][2], m->data.f[3][2],
        m->data.f[0][3], m->data.f[1][3], m->data.f[2][3], m->data.f[3][3]
    );
    return result;
}
ta_mat4 mat4_translate(ta_vec3 v)
{
    ta_mat4 result = mat4_init(
        1, 0, 0, v.x,
        0, 1, 0, v.y,
        0, 0, 1, v.z,
        0, 0, 0, 1
    );
    return result;
}
ta_mat4 mat4_scale(ta_vec3 s)
{
    ta_mat4 result = mat4_init(
        s.x, 0, 0, 0,
        0, s.y, 0, 0,
        0, 0, s.z, 0,
        0, 0, 0, 1
    );
    return result;
}
ta_mat4 mat4_scalef(float s)
{
    ta_mat4 result = mat4_init(
        s, 0, 0, 0,
        0, s, 0, 0,
        0, 0, s, 0,
        0, 0, 0, 1
    );
    return result;
}
ta_mat4 mat4_rotate_x(float deg)
{
    float rad = DEG_TO_RADF(deg);
    float s = sinf(rad);
    float c = cosf(rad);
    ta_mat4 result = mat4_init(
        1, 0, 0, 0,
        0, c,-s, 0,
        0, s, c, 0,
        0, 0, 0, 1
    );
    return result;
}
ta_mat4 mat4_rotate_y(float deg)
{
    float rad = DEG_TO_RADF(deg);
    float s = sinf(rad);
    float c = cosf(rad);
    ta_mat4 result = mat4_init(
        c, 0, s, 0,
        0, 1, 0, 0,
        -s, 0, c, 0,
        0, 0, 0, 1
    );
    return result;
}
ta_mat4 mat4_rotate_z(float deg)
{
    float rad = DEG_TO_RADF(deg);
    float s = sinf(rad);
    float c = cosf(rad);
    ta_mat4 result = mat4_init(
        c,-s, 0, 0,
        s, c, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    );
    return result;
}
ta_mat4 mat4_rotate_quat(ta_vec4 q)
{
    DLB_ASSERT(quat_equal(q, quat_normalize(q)));

    float xx = q.x * q.x;
    float yy = q.y * q.y;
    float zz = q.z * q.z;
    float xy = q.x * q.y;
    float xw = q.x * q.w;
    float xz = q.x * q.z;
    float yz = q.y * q.z;
    float yw = q.y * q.w;
    float zw = q.z * q.w;

    ta_mat4 m = { 0 };
    m.data.f[0][0] = 1 - 2*yy - 2*zz;
    m.data.f[0][1] = 2*xy - 2*zw;
    m.data.f[0][2] = 2*xz + 2*yw;
    m.data.f[0][3] = 0;
    m.data.f[1][0] = 2*xy + 2*zw;
    m.data.f[1][1] = 1 - 2*xx - 2*zz;
    m.data.f[1][2] = 2*yz - 2*xw;
    m.data.f[1][3] = 0;
    m.data.f[2][0] = 2*xz - 2*yw;
    m.data.f[2][1] = 2*yz + 2*xw;
    m.data.f[2][2] = 1 - 2*xx - 2*yy;
    m.data.f[2][3] = 0;
    m.data.f[3][0] = 0;
    m.data.f[3][1] = 0;
    m.data.f[3][2] = 0;
    m.data.f[3][3] = 1;
    return m;
}
ta_vec3 mat4_to_location(const ta_mat4 *m)
{
    ta_vec3 loc;
    loc.x = m->data.f[0][3];
    loc.y = m->data.f[1][3];
    loc.z = m->data.f[2][3];
    return loc;
}
ta_vec4 mat4_to_quaternion(const ta_mat4 *m)
{
// NOTE: These defines also transpose the matrix (makes it easier to check both transpositions when debugging)
#define m00 (m->data.f[0][0])
#define m01 (m->data.f[1][0])
#define m02 (m->data.f[2][0])
#define m10 (m->data.f[0][1])
#define m11 (m->data.f[1][1])
#define m12 (m->data.f[2][1])
#define m20 (m->data.f[0][2])
#define m21 (m->data.f[1][2])
#define m22 (m->data.f[2][2])

    ta_vec4 q;
    float t;
    if (m22 < 0) {
        if (m00 > m11) {
            t = 1 + m00 - m11 - m22;
            q = quat_init(t, m01+m10, m20+m02, m12-m21);
        } else {
            t = 1 - m00 + m11 - m22;
            q = quat_init(m01+m10, t, m12+m21, m20-m02);
        }
    } else {
        if (m00 < -m11) {
            t = 1 - m00 - m11 + m22;
            q = quat_init(m20+m02, m12+m21, t, m01-m10);
        }
        else
        {
            t = 1 + m00 + m11 + m22;
            q = quat_init(m12-m21, m20-m02, m01-m10, t);
        }
    }
    q = quat_scale(q, 0.5f / sqrtf(t));
    return q;

#undef m00
#undef m01
#undef m02
#undef m10
#undef m11
#undef m12
#undef m20
#undef m21
#undef m22
}
ta_mat4 mat4_mul(const ta_mat4 *a, const ta_mat4 *b)
{
    ta_mat4 result = { 0 };
    for (int j = 0; j < 4; ++j) {
        for (int i = 0; i < 4; ++i) {
            for (int n = 0; n < 4; ++n) {
                result.data.f[j][i] += a->data.f[j][n] * b->data.f[n][i];
            }
        }
    }
    return result;
}
ta_vec4 mat4_mul_vec4(const ta_mat4 *m, ta_vec4 v)
{
    ta_vec4 result;
    result.x =
        m->data.f[0][0] * v.x +
        m->data.f[0][1] * v.y +
        m->data.f[0][2] * v.z +
        m->data.f[0][3] * v.w;
    result.y =
        m->data.f[1][0] * v.x +
        m->data.f[1][1] * v.y +
        m->data.f[1][2] * v.z +
        m->data.f[1][3] * v.w;
    result.z =
        m->data.f[2][0] * v.x +
        m->data.f[2][1] * v.y +
        m->data.f[2][2] * v.z +
        m->data.f[2][3] * v.w;
    result.z =
        m->data.f[3][0] * v.x +
        m->data.f[3][1] * v.y +
        m->data.f[3][2] * v.z +
        m->data.f[3][3] * v.w;

    if (fabsf(result.x) < TA_EPSILON) result.x = 0.0f;
    if (fabsf(result.y) < TA_EPSILON) result.y = 0.0f;
    if (fabsf(result.z) < TA_EPSILON) result.z = 0.0f;
    if (fabsf(result.w) < TA_EPSILON) result.w = 0.0f;

    return result;
}
float mat4_det(const ta_mat4 *mat)
{
    float a = mat->data.f[0][0];
    float b = mat->data.f[0][1];
    float c = mat->data.f[0][2];
    float d = mat->data.f[0][3];
    float e = mat->data.f[1][0];
    float f = mat->data.f[1][1];
    float g = mat->data.f[1][2];
    float h = mat->data.f[1][3];
    float i = mat->data.f[2][0];
    float j = mat->data.f[2][1];
    float k = mat->data.f[2][2];
    float l = mat->data.f[2][3];
    float m = mat->data.f[3][0];
    float n = mat->data.f[3][1];
    float o = mat->data.f[3][2];
    float p = mat->data.f[3][3];

    float kp = k*p;
    float lo = l*o;
    float ln = l*n;
    float jp = j*p;
    float jo = j*o;
    float kn = k*n;
    float lm = l*m;
    float ip = i*p;
    float io = i*o;
    float km = k*m;
    float in = i*n;
    float jm = j*m;

    float result =
        a * (f * (kp - lo) + g * (ln - jp) + h * (jo - kn)) -
        b * (e * (kp - lo) + g * (lm - ip) + h * (io - km)) +
        c * (e * (jp - ln) + f * (lm - ip) + h * (in - jm)) -
        d * (e * (jo - kn) + f * (km - io) + g * (in - jm));
    return result;
}
// from gluInvertMatrix:
//     https://stackoverflow.com/a/1148405/770230
// see also:
//     http://www-graphics.stanford.edu/courses/cs248-98-fall/Final/q4.html
int mat4_inverse(const ta_mat4 *m, ta_mat4 *result)
{
#define m0  m->data.f[0][0]
#define m1  m->data.f[0][1]
#define m2  m->data.f[0][2]
#define m3  m->data.f[0][3]
#define m4  m->data.f[1][0]
#define m5  m->data.f[1][1]
#define m6  m->data.f[1][2]
#define m7  m->data.f[1][3]
#define m8  m->data.f[2][0]
#define m9  m->data.f[2][1]
#define m10 m->data.f[2][2]
#define m11 m->data.f[2][3]
#define m12 m->data.f[3][0]
#define m13 m->data.f[3][1]
#define m14 m->data.f[3][2]
#define m15 m->data.f[3][3]

    float inv[16];
    inv[0] =
        m5  * m10 * m15 -
        m5  * m11 * m14 -
        m9  * m6  * m15 +
        m9  * m7  * m14 +
        m13 * m6  * m11 -
        m13 * m7  * m10;
    inv[4] =
       -m4  * m10 * m15 +
        m4  * m11 * m14 +
        m8  * m6  * m15 -
        m8  * m7  * m14 -
        m12 * m6  * m11 +
        m12 * m7  * m10;
    inv[8] =
        m4  * m9  * m15 -
        m4  * m11 * m13 -
        m8  * m5  * m15 +
        m8  * m7  * m13 +
        m12 * m5  * m11 -
        m12 * m7  * m9;
    inv[12] =
       -m4  * m9  * m14 +
        m4  * m10 * m13 +
        m8  * m5  * m14 -
        m8  * m6  * m13 -
        m12 * m5  * m10 +
        m12 * m6  * m9;

    float det = m0 * inv[0] + m1 * inv[4] + m2 * inv[8] + m3 * inv[12];
    if (fabsf(det) < TA_EPSILON) {
        return false;
    }

    inv[1] =
       -m1  * m10 * m15 +
        m1  * m11 * m14 +
        m9  * m2  * m15 -
        m9  * m3  * m14 -
        m13 * m2  * m11 +
        m13 * m3  * m10;
    inv[5] =
        m0  * m10 * m15 -
        m0  * m11 * m14 -
        m8  * m2  * m15 +
        m8  * m3  * m14 +
        m12 * m2  * m11 -
        m12 * m3  * m10;
    inv[9] =
       -m0  * m9  * m15 +
        m0  * m11 * m13 +
        m8  * m1  * m15 -
        m8  * m3  * m13 -
        m12 * m1  * m11 +
        m12 * m3  * m9;
    inv[13] =
        m0  * m9  * m14 -
        m0  * m10 * m13 -
        m8  * m1  * m14 +
        m8  * m2  * m13 +
        m12 * m1  * m10 -
        m12 * m2  * m9;
    inv[2] =
        m1  * m6 * m15 -
        m1  * m7 * m14 -
        m5  * m2 * m15 +
        m5  * m3 * m14 +
        m13 * m2 * m7 -
        m13 * m3 * m6;
    inv[6] =
       -m0  * m6 * m15 +
        m0  * m7 * m14 +
        m4  * m2 * m15 -
        m4  * m3 * m14 -
        m12 * m2 * m7 +
        m12 * m3 * m6;
    inv[10] =
        m0  * m5 * m15 -
        m0  * m7 * m13 -
        m4  * m1 * m15 +
        m4  * m3 * m13 +
        m12 * m1 * m7 -
        m12 * m3 * m5;
    inv[14] =
       -m0  * m5 * m14 +
        m0  * m6 * m13 +
        m4  * m1 * m14 -
        m4  * m2 * m13 -
        m12 * m1 * m6 +
        m12 * m2 * m5;
    inv[3] =
       -m1 * m6 * m11 +
        m1 * m7 * m10 +
        m5 * m2 * m11 -
        m5 * m3 * m10 -
        m9 * m2 * m7 +
        m9 * m3 * m6;
    inv[7] =
        m0 * m6 * m11 -
        m0 * m7 * m10 -
        m4 * m2 * m11 +
        m4 * m3 * m10 +
        m8 * m2 * m7 -
        m8 * m3 * m6;
    inv[11] =
       -m0 * m5 * m11 +
        m0 * m7 * m9 +
        m4 * m1 * m11 -
        m4 * m3 * m9 -
        m8 * m1 * m7 +
        m8 * m3 * m5;
    inv[15] =
        m0 * m5 * m10 -
        m0 * m6 * m9 -
        m4 * m1 * m10 +
        m4 * m2 * m9 +
        m8 * m1 * m6 -
        m8 * m2 * m5;

#undef m0
#undef m1
#undef m2
#undef m3
#undef m4
#undef m5
#undef m6
#undef m7
#undef m8
#undef m9
#undef m10
#undef m11
#undef m12
#undef m13
#undef m14
#undef m15

    det = 1.0f / det;
    for (int i = 0; i < 16; i++) {
        ((float *)result)[i] = inv[i] * det;
    }
    return true;
}
ta_mat4 mat4_perspective(float fov_deg, float aspect, float znear, float zfar)
{
    float f = 1.0f / tanf(DEG_TO_RADF(fov_deg) / 2.0f);
    float nf = 1.0f / (znear - zfar);
    ta_mat4 result = { 0 };
    result.data.f[0][0] = f / aspect;
    result.data.f[1][1] = f;
    result.data.f[2][2] = (zfar + znear) * nf;
    result.data.f[2][3] = (2.0f * zfar * znear) * nf;
    result.data.f[3][2] = -1.0f;
    return result;
}
ta_mat4 mat4_perspective_inf(float fov_deg, float aspect, float znear)
{
    float f = 1.0f / tanf(DEG_TO_RADF(fov_deg) / 2.0f);
    ta_mat4 result = { 0 };
    result.data.f[0][0] = f / aspect;
    result.data.f[1][1] = f;
    result.data.f[2][3] = -znear;
    result.data.f[3][2] = -1.0f;
    return result;
}
ta_mat4 mat4_ortho(float left, float right, float top, float bottom,
    float znear, float zfar)
{
    ta_mat4 mat = { 0 };
    mat.data.f[0][0] = 2.0f / (right - left);
    mat.data.f[1][1] = 2.0f / (top - bottom);
    mat.data.f[2][2] = -2.0f / (zfar - znear);
    mat.data.f[0][3] = -(right + left) / (right - left);
    mat.data.f[1][3] = -(top + bottom) / (top - bottom);
    mat.data.f[2][3] = -(zfar + znear) / (zfar - znear);
    mat.data.f[3][3] = 1.0f;
    return mat;
}

ta_mat4 mat4_frustum(ta_vec3 front, ta_vec3 right, ta_vec3 up)
{
    // [ rx, ry, rz, 0 ]
    // [ ux, uy, uz, 0 ]
    // [ dx, dy, dz, 0 ]
    // [  0,  0,  0, 1 ]
    ta_mat4 transform = { 0 };
    transform.data.v[0].x = right.x;
    transform.data.v[0].y = right.y;
    transform.data.v[0].z = right.z;
    transform.data.v[1].x = up.x;
    transform.data.v[1].y = up.y;
    transform.data.v[1].z = up.z;
    transform.data.v[2].x = -front.x;
    transform.data.v[2].y = -front.y;
    transform.data.v[2].z = -front.z;
    transform.data.v[3].w = 1.0f;
    return transform;
}
ta_mat4 mat4_lookat_fru(ta_vec3 position, ta_vec3 front, ta_vec3 right, ta_vec3 up)
{
    // [ rx, ry, rz, 0 ]
    // [ ux, uy, uz, 0 ]
    // [ dx, dy, dz, 0 ]
    // [  0,  0,  0, 1 ]
    ta_mat4 transform = { 0 };
    transform.data.v[0].x = right.x;
    transform.data.v[0].y = right.y;
    transform.data.v[0].z = right.z;
    transform.data.v[1].x = up.x;
    transform.data.v[1].y = up.y;
    transform.data.v[1].z = up.z;
    transform.data.v[2].x = -front.x;
    transform.data.v[2].y = -front.y;
    transform.data.v[2].z = -front.z;
    transform.data.v[3].w = 1.0f;

    // [ 1, 0, 0, -px ]
    // [ 0, 1, 0, -py ]
    // [ 0, 0, 1, -pz ]
    // [ 0, 0, 0,   1 ]
    ta_mat4 translate = { 0 };
    translate.data.v[0].x = 1.0f;
    translate.data.v[0].w = -position.x;
    translate.data.v[1].y = 1.0f;
    translate.data.v[1].w = -position.y;
    translate.data.v[2].z = 1.0f;
    translate.data.v[2].w = -position.z;
    translate.data.v[3].w = 1.0f;

    ta_mat4 look_at = mat4_mul(&transform, &translate);
    return look_at;
}
ta_mat4 mat4_lookat(ta_vec3 position, ta_vec3 target, ta_vec3 world_up)
{
    ta_vec3 front = vec3_normalize(vec3_sub(target, position));
    ta_vec3 right = vec3_normalize(vec3_cross(front, world_up));
    ta_vec3 up = vec3_cross(right, front);

    ta_mat4 look_at = mat4_lookat_fru(position, front, right, up);
    return look_at;
}

#if 0
// Another (probably slower) way to calculate lookat matrix
ta_mat4 ric_mat4_lookat(ta_vec3 pos, ta_vec3 view, ta_vec3 up)
{
    ta_vec3 z = pos;
    z = vec3_sub(z, view);
    z = vec3_normalize(z);

    ta_vec3 x = vec3_cross(up, z);
    x = vec3_normalize(x);

    ta_vec3 y = vec3_cross(z, x);
    y = vec3_normalize(y);

    ta_mat4 look = mat4_init(
        x.x,  x.y,  x.z, -vec3_dot(x, pos),
        y.x,  y.y,  y.z, -vec3_dot(y, pos),
        z.x,  z.y,  z.z, -vec3_dot(z, pos),
        0.0f, 0.0f, 0.0f, 1.0f
    );
    return look;
}

float hue_to_rgb(float p, float q, float t) {
    if (t < 0.0f) t += 1.0f;
    if (t > 1.0f) t -= 1.0f;
    if (t < 1.0f / 6.0f) return p + (q - p) * 6.0f * t;
    if (t < 1.0f / 2.0f) return q;
    if (t < 2.0f / 3.0f) return p + (q - p) * 6.0f * (2.0f / 3.0f - t);
    return p;
}

// All components are in range 0.0f - 1.0f
// ta_rgb rgb = hsl_to_rbg((ta_hsl) { 115.f / 360.f, .83f, .38f });
ta_rgb hsl_to_rbg(ta_hsl hsl)
{
    float r, g, b;
    if (hsl.s == 0.0f) {
        r = hsl.l;
        g = hsl.l;
        b = hsl.l;
    } else {
        float q;
        if (hsl.l < 0.5f) {
            q = hsl.l * (1.0f + hsl.s);
        } else {
            q = hsl.l + hsl.s - hsl.l * hsl.s;
        }
        float p = 2 * hsl.l - q;
        r = hue_to_rgb(p, q, hsl.h + 1.0f / 3.0f);
        g = hue_to_rgb(p, q, hsl.h);
        b = hue_to_rgb(p, q, hsl.h - 1.0f / 3.0f);
    }
    ta_rgb rgb;
    rgb.r = (int)roundf(255.f * r);
    rgb.g = (int)roundf(255.f * g);
    rgb.b = (int)roundf(255.f * b);
    return rgb;
}
#endif

void ta_math_test()
{
    {
        ta_vec4 q = { 1.0f, 2.0f, 3.0f, 1.0f };
        q = quat_normalize(q);
        ta_mat4 result = mat4_rotate_quat(q);
        DLB_ASSERT(result.data.f[0][0] - -0.733333f < TA_EPSILON);
        DLB_ASSERT(result.data.f[0][1] - -0.133333f < TA_EPSILON);
        DLB_ASSERT(result.data.f[0][2] -  0.666667f < TA_EPSILON);
        DLB_ASSERT(result.data.f[0][3] < TA_EPSILON);
        DLB_ASSERT(result.data.f[1][0] -  0.666667f < TA_EPSILON);
        DLB_ASSERT(result.data.f[1][1] - -0.333333f < TA_EPSILON);
        DLB_ASSERT(result.data.f[1][2] -  0.666667f < TA_EPSILON);
        DLB_ASSERT(result.data.f[1][3] < TA_EPSILON);
        DLB_ASSERT(result.data.f[2][0] -  0.133333f < TA_EPSILON);
        DLB_ASSERT(result.data.f[2][1] -  0.933333f < TA_EPSILON);
        DLB_ASSERT(result.data.f[2][2] -  0.333333f < TA_EPSILON);
        DLB_ASSERT(result.data.f[2][3] < TA_EPSILON);
        DLB_ASSERT(result.data.f[3][0] < TA_EPSILON);
        DLB_ASSERT(result.data.f[3][1] < TA_EPSILON);
        DLB_ASSERT(result.data.f[3][2] < TA_EPSILON);
        DLB_ASSERT(result.data.f[3][3] - 1.0f < TA_EPSILON);
    }

    {
        ta_vec3 orig_loc = { 1.0f, 2.0f, 3.0f };
        ta_vec4 orig_quat = quat_normalize(quat_init(4.0f, 5.0f, 6.0f, 7.0f));

        ta_mat4 mat_trans = mat4_translate(orig_loc);
        ta_mat4 mat_rot = mat4_rotate_quat(orig_quat);

        ta_mat4 mat = MAT4_IDENT;
        mat = mat4_mul(&mat_rot, &mat);
        mat = mat4_mul(&mat_trans, &mat);

        ta_vec3 loc = mat4_to_location(&mat);
        ta_vec4 quat = mat4_to_quaternion(&mat);

        DLB_ASSERT(vec3_equal(loc, orig_loc));
        DLB_ASSERT(quat_equal(quat, orig_quat));
    }

    //quat_print(stdout, q);
    //mat4_print(stdout, &result);
}