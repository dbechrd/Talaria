#include "ta_math.h"
#include "ta_log.h"
#include <math.h>

const ta_vec3 VEC3_ZERO = { 0.0f, 0.0f, 0.0f };
const ta_vec3 VEC3_X = { 1.0f, 0.0f, 0.0f };
const ta_vec3 VEC3_Y = { 0.0f, 1.0f, 0.0f };
const ta_vec3 VEC3_Z = { 0.0f, 0.0f, 1.0f };
const ta_mat4 MAT4_IDENT = {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f
};
const ta_rgba TA_COLOR_INVIS   = { 0.0f, 0.0f, 0.0f, 0.0f };
const ta_rgba TA_COLOR_RED     = { 1.0f, 0.1f, 0.1f, 1.0f };
const ta_rgba TA_COLOR_GREEN   = { 0.1f, 1.0f, 0.1f, 1.0f };
const ta_rgba TA_COLOR_BLUE    = { 0.1f, 0.1f, 1.0f, 1.0f };
const ta_rgba TA_COLOR_YELLOW  = { 1.0f, 1.0f, 0.1f, 1.0f };
const ta_rgba TA_COLOR_MAGENTA = { 1.0f, 0.1f, 1.0f, 1.0f };
const ta_rgba TA_COLOR_CYAN    = { 0.1f, 1.0f, 1.0f, 1.0f };
const ta_rgba TA_COLOR_GRAY1   = { 0.1f, 0.1f, 0.1f, 1.0f };
const ta_rgba TA_COLOR_GRAY2   = { 0.2f, 0.2f, 0.2f, 1.0f };
const ta_rgba TA_COLOR_GRAY3   = { 0.3f, 0.3f, 0.3f, 1.0f };
const ta_rgba TA_COLOR_GRAY4   = { 0.4f, 0.4f, 0.4f, 1.0f };
const ta_rgba TA_COLOR_GRAY5   = { 0.5f, 0.5f, 0.5f, 1.0f };
const ta_rgba TA_COLOR_GRAY6   = { 0.6f, 0.6f, 0.6f, 1.0f };
const ta_rgba TA_COLOR_GRAY7   = { 0.7f, 0.7f, 0.7f, 1.0f };
const ta_rgba TA_COLOR_GRAY8   = { 0.8f, 0.8f, 0.8f, 1.0f };
const ta_rgba TA_COLOR_GRAY9   = { 0.9f, 0.9f, 0.9f, 1.0f };

#if 0
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
#endif
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

void ta_vec3_print(ta_vec3 *vec)
{
    ta_log_write(tg_debug_log, "vec3: %f %f %f\n",
        vec->x, vec->y, vec->z);
}
ta_vec3 vec3_negate(const ta_vec3 v)
{
    ta_vec3 result;
    result.x = -v.x;
    result.y = -v.y;
    result.z = -v.z;
    return result;
}
ta_vec3 vec3_add(const ta_vec3 a, const ta_vec3 b)
{
    ta_vec3 result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;
    return result;
}
ta_vec3 vec3_sub(const ta_vec3 a, const ta_vec3 b)
{
    ta_vec3 result;
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    result.z = a.z - b.z;
    return result;
}
ta_vec3 vec3_scalef(const ta_vec3 a, float s)
{
    ta_vec3 result;
    result.x = a.x * s;
    result.y = a.y * s;
    result.z = a.z * s;
    return result;
}
float vec3_len(const ta_vec3 v)
{
    float len = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
    return len;
}
ta_vec3 vec3_normalize(const ta_vec3 v)
{
    float len = vec3_len(v);
    ta_vec3 result = v;
    result.x /= len;
    result.y /= len;
    result.z /= len;
    return result;
}
float vec3_dot(const ta_vec3 a, const ta_vec3 b)
{
    float dot = a.x * b.x + a.y * b.y + a.z * b.z;
    if (fabsf(dot) < TA_EPSILON) {
        dot = 0.0f;
    }
    return dot;
}
ta_vec3 vec3_cross(const ta_vec3 a, const ta_vec3 b)
{
    ta_vec3 result;
    result.x = a.y * b.z - a.z * b.y;
    result.y = a.z * b.x - a.x * b.z;
    result.z = a.x * b.y - a.y * b.x;
    return result;
}

void ta_vec4_print(ta_vec4 *vec)
{
    ta_log_write(tg_debug_log, "vec4: %f %f %f %f\n",
        vec->x, vec->y, vec->z, vec->w);
}

void ta_mat3_print(ta_mat3 *mat)
{
    for (int i = 0; i < 3; i++) {
        ta_log_write(tg_debug_log, "mat[%d]: %f %f %f\n", i,
            mat->rows.v[i].x,
            mat->rows.v[i].y,
            mat->rows.v[i].z
        );
    }
}
ta_vec3 mat3_mul_vec3(const ta_mat3 m, const ta_vec3 v)
{
    ta_vec3 result;
    result.x =
        m.rows.f[0][0] * v.x +
        m.rows.f[0][1] * v.y +
        m.rows.f[0][2] * v.z;
    result.y =
        m.rows.f[1][0] * v.x +
        m.rows.f[1][1] * v.y +
        m.rows.f[1][2] * v.z;
    result.z =
        m.rows.f[2][0] * v.x +
        m.rows.f[2][1] * v.y +
        m.rows.f[2][2] * v.z;

    if (fabsf(result.x) < TA_EPSILON) result.x = 0.0f;
    if (fabsf(result.y) < TA_EPSILON) result.y = 0.0f;
    if (fabsf(result.z) < TA_EPSILON) result.z = 0.0f;

    return result;
}
ta_rgb mat3_mul_rgb(const ta_mat3 m, const ta_rgb v)
{
    ta_rgb result;
    result.r =
        m.rows.f[0][0] * v.r +
        m.rows.f[0][1] * v.g +
        m.rows.f[0][2] * v.b;
    result.g =
        m.rows.f[1][0] * v.r +
        m.rows.f[1][1] * v.g +
        m.rows.f[1][2] * v.b;
    result.b =
        m.rows.f[2][0] * v.r +
        m.rows.f[2][1] * v.g +
        m.rows.f[2][2] * v.b;

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
    m.rows.f[0][0] = cosa + onecos / 3.0f;
    m.rows.f[0][1] = onecos / 3.0f - sqthird * sina;
    m.rows.f[0][2] = onecos / 3.0f + sqthird * sina;
    m.rows.f[1][0] = m.rows.f[0][2];
    m.rows.f[1][1] = m.rows.f[0][0];
    m.rows.f[1][2] = m.rows.f[0][1];
    m.rows.f[2][0] = m.rows.f[0][1];
    m.rows.f[2][1] = m.rows.f[0][2];
    m.rows.f[2][2] = m.rows.f[0][0];
    return m;
}

void ta_mat4_print(ta_mat4 *mat)
{
    for (int i = 0; i < 4; i++) {
        ta_log_write(tg_debug_log, "mat[%d]: %f %f %f %f\n", i,
            mat->rows.v[i].x,
            mat->rows.v[i].y,
            mat->rows.v[i].z,
            mat->rows.v[i].w
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
    result.rows.f[0][0] = m00;
    result.rows.f[0][1] = m01;
    result.rows.f[0][2] = m02;
    result.rows.f[0][3] = m03;
    result.rows.f[1][0] = m10;
    result.rows.f[1][1] = m11;
    result.rows.f[1][2] = m12;
    result.rows.f[1][3] = m13;
    result.rows.f[2][0] = m20;
    result.rows.f[2][1] = m21;
    result.rows.f[2][2] = m22;
    result.rows.f[2][3] = m23;
    result.rows.f[3][0] = m30;
    result.rows.f[3][1] = m31;
    result.rows.f[3][2] = m32;
    result.rows.f[3][3] = m33;
    return result;
}
ta_mat4 mat4_transpose(const ta_mat4 *m)
{
    ta_mat4 result = mat4_init(
        m->rows.f[0][0], m->rows.f[1][0], m->rows.f[2][0], m->rows.f[3][0],
        m->rows.f[0][1], m->rows.f[1][1], m->rows.f[2][1], m->rows.f[3][1],
        m->rows.f[0][2], m->rows.f[1][2], m->rows.f[2][2], m->rows.f[3][2],
        m->rows.f[0][3], m->rows.f[1][3], m->rows.f[2][3], m->rows.f[3][3]
    );
    return result;
}
ta_mat4 mat4_translate(const ta_vec3 *v)
{
    ta_mat4 result = mat4_init(
        1, 0, 0, v->x,
        0, 1, 0, v->y,
        0, 0, 1, v->z,
        0, 0, 0, 1
    );
    return result;
}
ta_mat4 mat4_scale(const ta_vec3 *s)
{
    ta_mat4 result = mat4_init(
        s->x, 0, 0, 0,
        0, s->y, 0, 0,
        0, 0, s->z, 0,
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
ta_mat4 mat4_mul(const ta_mat4 a, const ta_mat4 b)
{
    ta_mat4 result = { 0 };
    for (int j = 0; j < 4; ++j) {
        for (int i = 0; i < 4; ++i) {
            for (int n = 0; n < 4; ++n) {
                result.rows.f[j][i] += a.rows.f[j][n] * b.rows.f[n][i];
            }
        }
    }
    return result;
}
ta_mat4 mat4_perspective(float fov_deg, float aspect, float nearz, float farz)
{
    float f = 1.0f / tanf(DEG_TO_RADF(fov_deg) / 2.0f);
    float nf = 1.0f / (nearz - farz);
    ta_mat4 result = { 0 };
    result.rows.f[0][0] = f / aspect;
    result.rows.f[1][1] = f;
    result.rows.f[2][2] = (farz + nearz) * nf;
    result.rows.f[2][3] = (2.0f * farz * nearz) * nf;
    result.rows.f[3][2] = -1.0f;
    return result;
}
ta_mat4 mat4_perspective_inf(float fov_deg, float aspect, float nearz)
{
    float f = 1.0f / tanf(DEG_TO_RADF(fov_deg) / 2.0f);
    ta_mat4 result = { 0 };
    result.rows.f[0][0] = f / aspect;
    result.rows.f[1][1] = f;
    result.rows.f[2][3] = -nearz;
    result.rows.f[3][2] = -1.0f;
    return result;
}
ta_mat4 mat4_ortho(float left, float right, float bottom, float top,
    float nearz, float farz)
{
    float lr = 1.0f / (left - right);
    float bt = 1.0f / (bottom - top);
    float nf = 1.0f / (nearz - farz);
    ta_mat4 result = { 0 };
    result.rows.f[0][0] = -2.0f * lr;
    result.rows.f[1][1] = -2.0f * bt;
    result.rows.f[2][2] = 2.0f * nf;
    result.rows.f[3][0] = (left + right) * lr;
    result.rows.f[3][1] = (top + bottom) * bt;
    result.rows.f[3][2] = (farz + nearz) * nf;
    result.rows.f[3][3] = 1.0f;
    return result;
}

#if 0
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