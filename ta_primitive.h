#pragma once

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
		ta_vec4 v[4];
		float f[4][4];
	} rows;
} ta_mat4;

typedef struct {
    float r;
    float g;
    float b;
    float a;
} ta_color4;

typedef struct {
	int x;
	int y;
} ta_position;

typedef struct {
	int w;
	int h;
} ta_size;

typedef struct {
	ta_position p0;
	ta_position p1;
} ta_line_2d;

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

extern const ta_vec3 vec3_up;
extern const ta_mat4 mat4_ident;

void ta_vec3_print(ta_vec3 *vec);
void ta_vec4_print(ta_vec4 *vec);
void ta_mat4_print(ta_mat4 *mat);

ta_vec3 vec3_negate(const ta_vec3 v);
ta_vec3 vec3_add(const ta_vec3 a, const ta_vec3 b);
ta_vec3 vec3_sub(const ta_vec3 a, const ta_vec3 b);
float vec3_len(const ta_vec3 v);
ta_vec3 vec3_normalize(const ta_vec3 v);
float vec3_dot(const ta_vec3 a, const ta_vec3 b);
ta_vec3 vec3_cross(const ta_vec3 a, const ta_vec3 b);

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

void ta_primitive_init();
void ta_primitive_push_line_2d(ta_line_2d *line_2d, ta_color4 *color0, ta_color4 *color1);
void ta_primitive_push_rect(ta_rect *rect, ta_color4 *color);
void ta_primitive_render();
void ta_primitive_clear();