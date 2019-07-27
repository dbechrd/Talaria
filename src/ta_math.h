#pragma once
#include <stdio.h>

#define TA_EPSILON 0.0001f
#define M_PI 3.14159265358979323846264338327950288
#define M_2PI 6.28318530717958647692528676655900576
#define DEG_TO_RAD(deg) deg * M_PI / 180.0
#define RAD_TO_DEG(rad) rad * 180.0 / M_PI
#define DEG_TO_RADF(deg) deg * (float)M_PI / 180.0f
#define RAD_TO_DEGF(rad) rad * 180.0f / (float)M_PI

#define TA_SIZE(w, h) (ta_size){ w, h }
#define TA_POSITION(x, y) (ta_vec2i){ x, y }

typedef struct ta_vec2 {
    float x;
    float y;
} ta_vec2;

typedef struct ta_uv {
    float u;
    float v;
} ta_uv;

typedef struct ta_vec3 {
    float x;
    float y;
    float z;
} ta_vec3;

typedef struct ta_vec4 {
    float x;
    float y;
    float z;
    float w;
} ta_vec4;

typedef struct ta_quat {
    float x;
    float y;
    float z;
    float w;
} ta_quat;

typedef struct ta_mat3 {
    union {
        ta_vec3 v[3];
        float f[3][3];
        float arr[9];
    } data;
} ta_mat3;

typedef struct ta_mat4 {
    union {
        ta_vec4 v[4];
        float f[4][4];
        float arr[16];
    } data;
} ta_mat4;

typedef struct ta_transform {
    ta_vec3 position;
    ta_quat orientation;
    ta_vec3 scale;
} ta_transform;

typedef struct ta_hsl {
    float h;
    float s;
    float l;
} ta_hsl;

typedef struct ta_rgb {
    float r;
    float g;
    float b;
} ta_rgb;

typedef struct ta_rgba {
    float r;
    float g;
    float b;
    float a;
} ta_rgba;

typedef struct ta_vec2i {
	int x;
	int y;
} ta_vec2i;

typedef struct ta_size {
    int w;
    int h;
} ta_size;

extern const ta_vec3 VEC3_ZERO;
extern const ta_vec3 VEC3_ONE;
extern const ta_vec3 VEC3_X;
extern const ta_vec3 VEC3_Y;
extern const ta_vec3 VEC3_Z;
extern const ta_vec3 VEC3_NX;
extern const ta_vec3 VEC3_NY;
extern const ta_vec3 VEC3_NZ;
extern const ta_vec3 VEC3_MIN;
extern const ta_vec3 VEC3_MAX;
extern const ta_vec3 VEC3_EPSILON;

extern const ta_quat QUAT_IDENT;

extern const ta_mat3 MAT3_IDENT;

extern const ta_mat4 MAT4_IDENT;

extern const ta_rgba TA_COLOR_INVIS;
extern const ta_rgba TA_COLOR_RED;
extern const ta_rgba TA_COLOR_GREEN;
extern const ta_rgba TA_COLOR_BLUE;
extern const ta_rgba TA_COLOR_CYAN;
extern const ta_rgba TA_COLOR_MAGENTA;
extern const ta_rgba TA_COLOR_YELLOW;
extern const ta_rgba TA_COLOR_GRAY1;
extern const ta_rgba TA_COLOR_GRAY2;
extern const ta_rgba TA_COLOR_GRAY3;
extern const ta_rgba TA_COLOR_GRAY4;
extern const ta_rgba TA_COLOR_GRAY5;
extern const ta_rgba TA_COLOR_GRAY6;
extern const ta_rgba TA_COLOR_GRAY7;
extern const ta_rgba TA_COLOR_GRAY8;
extern const ta_rgba TA_COLOR_GRAY9;
extern const ta_rgba TA_COLOR_WHITE;
extern const ta_rgba TA_COLOR_WHITE_ALPHA;

extern const ta_size TA_SIZE_ZERO;

extern const ta_vec2i TA_VEC2I_ZERO;

//int clamp(int d, int min, int max);
float clampf(float f, float min, float max);

void vec3_print(FILE *file, ta_vec3 v);
int vec3_zero(ta_vec3 v);
int vec3_tiny(ta_vec3 v);
int vec3_equal(ta_vec3 a, ta_vec3 b);
ta_vec3 vec3_neg(ta_vec3 v);
ta_vec3 vec3_add(ta_vec3 a, ta_vec3 b);
ta_vec3 vec3_sub(ta_vec3 a, ta_vec3 b);
ta_vec3 vec3_scalef(ta_vec3 a, float s);
float vec3_len(ta_vec3 v);
float vec3_len2(ta_vec3 v);
ta_vec3 vec3_normalize(ta_vec3 v);
float vec3_dot(ta_vec3 a, ta_vec3 b);
ta_vec3 vec3_cross(ta_vec3 a, ta_vec3 b);
ta_vec3 vec3_lerp(ta_vec3 a, ta_vec3 b, float w);
ta_vec3 vec3_rotate_quat(ta_vec3 v, ta_quat q);

void vec4_print(FILE *file, ta_vec4 v);
int vec4_zero(ta_vec4 v);

void quat_print(FILE *file, ta_quat q);
int quat_zero(ta_quat v);
int quat_ident(ta_quat q);
int quat_equals(ta_quat a, ta_quat b);
ta_quat quat_from_axis_angle(ta_vec3 axis, float deg);
ta_quat quat_from_vec_vec(ta_vec3 from, ta_vec3 to);
ta_quat quat_normalize(ta_quat q);
ta_quat quat_conjugate(ta_quat q);
ta_quat quat_inverse(ta_quat q);
ta_quat quat_mul(ta_quat a, ta_quat b);
float quat_dot(ta_quat a, ta_quat b);
ta_quat quat_nlerp(ta_quat a, ta_quat b, float w);
ta_quat quat_slerp(ta_quat a, ta_quat b, float w);

void mat3_print(FILE *file, const ta_mat3 *m);
ta_mat3 mat3_init(
    float m00, float m01, float m02,
    float m10, float m11, float m12,
    float m20, float m21, float m22);
ta_mat3 mat3_transpose(const ta_mat3 *m);
ta_mat3 mat3_rotate_x(float deg);
ta_mat3 mat3_rotate_y(float deg);
ta_mat3 mat3_rotate_z(float deg);
ta_mat3 mat3_rotate_quat(ta_quat q);
ta_mat3 mat3_mul(const ta_mat3 *a, const ta_mat3 *b);
ta_vec3 mat3_mul_vec3(const ta_mat3 *m, ta_vec3 v);
ta_rgb mat3_mul_rgb(const ta_mat3 *m, ta_rgb v);
ta_mat3 mat3_hue_rotation(float degrees);
float mat3_deter(const ta_mat3 *m);

void mat4_print(FILE *file, const ta_mat4 *m);
ta_mat4 mat4_mul(const ta_mat4 *a, const ta_mat4 *b);
ta_mat4 mat4_init(
    float m00, float m01, float m02, float m03,
    float m10, float m11, float m12, float m13,
    float m20, float m21, float m22, float m23,
    float m30, float m31, float m32, float m33);
ta_mat4 mat4_transpose(const ta_mat4 *m);
ta_mat4 mat4_translate(ta_vec3 v);
ta_mat4 mat4_scale(ta_vec3 s);
ta_mat4 mat4_scalef(float s);
ta_mat4 mat4_rotate_x(float deg);
ta_mat4 mat4_rotate_y(float deg);
ta_mat4 mat4_rotate_z(float deg);
ta_mat4 mat4_rotate_quat(ta_quat q);
float mat4_det(const ta_mat4 *mat);
int mat4_inverse(const ta_mat4 *m, ta_mat4 *result);
ta_mat4 mat4_perspective(float fov_deg, float aspect, float znear, float zfar);
ta_mat4 mat4_perspective_inf(float fov_deg, float aspect, float znear);
ta_mat4 mat4_ortho(float left, float right, float bottom, float top,
    float znear, float zfar);
ta_mat4 mat4_lookat_fru(ta_vec3 position, ta_vec3 front, ta_vec3 right,
    ta_vec3 up);
ta_mat4 mat4_lookat(ta_vec3 position, ta_vec3 target, ta_vec3 world_up);

void ta_math_test();