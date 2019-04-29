#pragma once

#define TA_EPSILON 0.0001f
#define M_PI 3.14159265358979323846264338327950288
#define M_2PI 6.28318530717958647692528676655900576
#define DEG_TO_RAD(deg) deg * M_PI / 180.0
#define RAD_TO_DEG(rad) rad * 180.0 / M_PI
#define DEG_TO_RADF(deg) deg * (float)M_PI / 180.0f
#define RAD_TO_DEGF(rad) rad * 180.0f / (float)M_PI

typedef struct {
    float x;
    float y;
} ta_vec2;

typedef struct {
    float u;
    float v;
} ta_uv;

typedef struct {
    float x;
    float y;
    float z;
} ta_vec3;

typedef struct {
    float x;
    float y;
    float z;
    float w;
} ta_vec4;

typedef struct {
    union {
        ta_vec3 v[3];
        float f[3][3];
    } rows;
} ta_mat3;

typedef struct {
    union {
        ta_vec4 v[4];
        float f[4][4];
    } rows;
} ta_mat4;

typedef struct {
    ta_vec3 position;
    ta_vec4 rotation;
    ta_vec3 scale;
} ta_transform;

typedef struct {
    float h;
    float s;
    float l;
} ta_hsl;

typedef struct {
    float r;
    float g;
    float b;
} ta_rgb;

typedef struct {
    float r;
    float g;
    float b;
    float a;
} ta_rgba;

typedef struct {
    int w;
    int h;
} ta_size;

typedef struct {
    ta_vec2 p0;
    ta_vec2 p1;
} ta_line_2d;

typedef struct {
    ta_vec3 p0;
    ta_vec3 p1;
} ta_line_3d;

typedef struct {
    int x;
    int y;
    int w;
    int h;
} ta_rect;

typedef struct {
    ta_vec2 center;
    ta_vec2 half_axes;
} ta_bbox_2d;

typedef struct {
    ta_vec3 center;
    ta_vec3 half_axes;
} ta_bbox_3d;

extern const ta_vec3 VEC3_ZERO;
extern const ta_vec3 VEC3_X;
extern const ta_vec3 VEC3_Y;
extern const ta_vec3 VEC3_Z;
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

//int clamp(int d, int min, int max);
float clampf(float f, float min, float max);

void ta_vec3_print(ta_vec3 *vec);
ta_vec3 vec3_negate(const ta_vec3 v);
ta_vec3 vec3_add(const ta_vec3 a, const ta_vec3 b);
ta_vec3 vec3_sub(const ta_vec3 a, const ta_vec3 b);
ta_vec3 vec3_scalef(const ta_vec3 a, float s);
float vec3_len(const ta_vec3 v);
ta_vec3 vec3_normalize(const ta_vec3 v);
float vec3_dot(const ta_vec3 a, const ta_vec3 b);
ta_vec3 vec3_cross(const ta_vec3 a, const ta_vec3 b);

void ta_vec4_print(ta_vec4 *vec);

void ta_mat3_print(ta_mat3 *mat);
ta_vec3 mat3_mul_vec3(const ta_mat3 m, const ta_vec3 v);
ta_rgb mat3_mul_rgb(const ta_mat3 m, const ta_rgb v);
ta_mat3 mat3_hue_rotation(float degrees);

void ta_mat4_print(ta_mat4 *mat);
ta_mat4 mat4_mul(const ta_mat4 a, const ta_mat4 b);
ta_mat4 mat4_init(
    float m00, float m01, float m02, float m03,
    float m10, float m11, float m12, float m13,
    float m20, float m21, float m22, float m23,
    float m30, float m31, float m32, float m33);
ta_mat4 mat4_transpose(const ta_mat4 *m);
ta_mat4 mat4_translate(const ta_vec3 *v);
ta_mat4 mat4_scale(const ta_vec3 *s);
ta_mat4 mat4_scalef(float s);
ta_mat4 mat4_rotate_x(float deg);
ta_mat4 mat4_rotate_y(float deg);
ta_mat4 mat4_rotate_z(float deg);
ta_mat4 mat4_perspective(float fov_deg, float aspect, float nearz, float farz);
ta_mat4 mat4_perspective_inf(float fov_deg, float aspect, float nearz);
ta_mat4 mat4_ortho(float left, float right, float bottom, float top,
    float nearz, float farz);