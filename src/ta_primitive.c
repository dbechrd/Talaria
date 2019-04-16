#include "ta_primitive.h"
#include "ta_log.h"
#include "ta_window.h"
#include "ta_shader_lines.h"
#include "ta_shader_quads.h"
#include "dlb_vector.h"
#include "misc/gl3w.h"
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
const ta_color TA_COLOR_INVIS = { 0.0f, 0.0f, 0.0f, 0.0f };
const ta_color TA_COLOR_RED   = { 1.0f, 0.1f, 0.1f, 1.0f };
const ta_color TA_COLOR_GREEN = { 0.1f, 1.0f, 0.1f, 1.0f };
const ta_color TA_COLOR_BLUE  = { 0.1f, 0.1f, 1.0f, 1.0f };
const ta_color TA_COLOR_GRAY1 = { 0.1f, 0.1f, 0.1f, 1.0f };
const ta_color TA_COLOR_GRAY2 = { 0.2f, 0.2f, 0.2f, 1.0f };
const ta_color TA_COLOR_GRAY3 = { 0.3f, 0.3f, 0.3f, 1.0f };
const ta_color TA_COLOR_GRAY4 = { 0.4f, 0.4f, 0.4f, 1.0f };
const ta_color TA_COLOR_GRAY5 = { 0.5f, 0.5f, 0.5f, 1.0f };
const ta_color TA_COLOR_GRAY6 = { 0.6f, 0.6f, 0.6f, 1.0f };
const ta_color TA_COLOR_GRAY7 = { 0.7f, 0.7f, 0.7f, 1.0f };
const ta_color TA_COLOR_GRAY8 = { 0.8f, 0.8f, 0.8f, 1.0f };
const ta_color TA_COLOR_GRAY9 = { 0.9f, 0.9f, 0.9f, 1.0f };

static ta_vert_line *lines_queue;
static GLuint lines_vao;
static GLuint lines_buffer;
static GLint lines_buffer_size;

static ta_vert_quad *quads_queue;
static GLuint quads_vao;
static GLuint quads_buffer;
static GLint quads_buffer_size;
static GLuint quads_program;

static void ta_primitive_push_line(ta_vert_line *line);
static void ta_primitive_push_quad(ta_vert_quad *quad);
static void ta_primitive_line2d_to_line(ta_vert_line *line, ta_line_2d *line2d,
	const ta_color *color0, const ta_color *color1);
static void ta_primitive_rect_to_quad(ta_vert_quad *quad, int x, int y,
	const ta_rect *rect, const ta_color *color);
static void ta_primitive_bbox_to_quad(ta_vert_quad *quad, ta_bbox_2d *bbox,
	const ta_color *color);

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
ta_rgbf mat3_mul_rgbf(const ta_mat3 m, const ta_rgbf v)
{
	ta_rgbf result;
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

static void ta_primitive_init_lines()
{
	ta_shader_lines_init();

	glCreateVertexArrays(1, &lines_vao);
	glCreateBuffers(1, &lines_buffer);
	lines_buffer_size = 0;

	glBindVertexArray(lines_vao);
	glBindBuffer(GL_ARRAY_BUFFER, lines_buffer);
	ta_shader_lines_attribs();
	glBindVertexArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
}
static void ta_primitive_init_quads()
{
	quads_program = ta_shader_quads_init();

	glCreateVertexArrays(1, &quads_vao);
	glCreateBuffers(1, &quads_buffer);
	quads_buffer_size = 0;

	glBindVertexArray(quads_vao);
	glBindBuffer(GL_ARRAY_BUFFER, quads_buffer);
	ta_shader_quads_attribs();
	glBindVertexArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
}
void ta_primitive_init()
{
	ta_primitive_init_lines();
	ta_primitive_init_quads();
}

static void ta_primitive_push_line(ta_vert_line *line)
{
	dlb_vec_push(lines_queue, *line);
}
static void ta_primitive_line2d_to_line(ta_vert_line *line, ta_line_2d *line2d,
	const ta_color *color0, const ta_color *color1)
{
	float x0 = X_TO_NDC(line2d->p0.x);
	float x1 = X_TO_NDC(line2d->p1.x);
	float y0 = Y_TO_NDC(line2d->p0.y);
	float y1 = Y_TO_NDC(line2d->p1.y);

	line->verts[0].position.x = x0;
	line->verts[0].position.y = y0;
	line->verts[0].color = *color0;
	line->verts[1].position.x = x1;
	line->verts[1].position.y = y1;
	line->verts[1].color = *color1;
}
static void ta_primitive_line3d_to_line(ta_vert_line *line, ta_line_3d *line3d,
    const ta_color *color0, const ta_color *color1)
{
    line->verts[0].position = line3d->p0;
    line->verts[0].color = *color0;
    line->verts[1].position = line3d->p1;
    line->verts[1].color = *color1;
}
void ta_primitive_push_line_2d(ta_line_2d *line_2d, const ta_color *color0,
	const ta_color * color1)
{
	ta_vert_line line = { 0 };
	ta_primitive_line2d_to_line(&line, line_2d, color0, color1);
	ta_primitive_push_line(&line);
}
void ta_primitive_push_line_3d(ta_line_3d *line_3d, const ta_color *color0,
    const ta_color * color1)
{
    ta_vert_line line = { 0 };
    ta_primitive_line3d_to_line(&line, line_3d, color0, color1);
    ta_primitive_push_line(&line);
}

static void ta_primitive_push_quad(ta_vert_quad *quad)
{
	dlb_vec_push(quads_queue, *quad);
}
static void ta_primitive_rect_to_quad(ta_vert_quad *quad, int x, int y,
	const ta_rect *rect, const ta_color *color)
{
	// v3 *----* v2
	//    |    |
	// v0 *----* v1
	// v0, v1, v2, v0, v2, v3

	float x0 = X_TO_NDC(x + rect->x);
	float x1 = X_TO_NDC(x + rect->x + rect->w);
	float y0 = Y_TO_NDC(y + rect->y + rect->h);
	float y1 = Y_TO_NDC(y + rect->y);

	quad->verts[0].position.x = x0;  // v0 (0,0)
	quad->verts[0].position.y = y0;
	quad->verts[0].uv.u = 0.0f;
	quad->verts[0].uv.v = 0.0f;
	quad->verts[1].position.x = x1;  // v1 (1,0)
	quad->verts[1].position.y = y0;
	quad->verts[1].uv.u = 1.0f;
	quad->verts[1].uv.v = 0.0f;
	quad->verts[2].position.x = x1;  // v2 (1,1)
	quad->verts[2].position.y = y1;
	quad->verts[2].uv.u = 1.0f;
	quad->verts[2].uv.v = 1.0f;
	quad->verts[3].position.x = x0;  // v0 (0,0)
	quad->verts[3].position.y = y0;
	quad->verts[3].uv.u = 0.0f;
	quad->verts[3].uv.v = 0.0f;
	quad->verts[4].position.x = x1;  // v2 (1,1)
	quad->verts[4].position.y = y1;
	quad->verts[4].uv.u = 1.0f;
	quad->verts[4].uv.v = 1.0f;
	quad->verts[5].position.x = x0;  // v3 (0,1)
	quad->verts[5].position.y = y1;
	quad->verts[5].uv.u = 0.0f;
	quad->verts[5].uv.v = 1.0f;
	for (int i = 0; i < 6; i++) {
		quad->verts[i].position.z = 0.1f;
		quad->verts[i].color = *color;
	}
}
void ta_primitive_push_rect(int x, int y, ta_rect *rect, const ta_color *color)
{
	ta_vert_quad quad = { 0 };
	ta_primitive_rect_to_quad(&quad, x, y, rect, color);
	ta_primitive_push_quad(&quad);
}

static void ta_primitive_bbox_to_quad(ta_vert_quad *quad, ta_bbox_2d *bbox,
	const ta_color *color)
{
	// v3 *----* v2
	//    |    |
	// v0 *----* v1
	// v0, v1, v2, v0, v2, v3

	float x0 = bbox->center.x - bbox->half_axes.x;
	float x1 = bbox->center.x + bbox->half_axes.x;
	float y0 = bbox->center.y - bbox->half_axes.y;
	float y1 = bbox->center.y + bbox->half_axes.y;

	quad->verts[0].position.x = x0;  // v0 (0,0)
	quad->verts[0].position.y = y0;
	quad->verts[1].position.x = x1;  // v1 (1,0)
	quad->verts[1].position.y = y0;
	quad->verts[2].position.x = x1;  // v2 (1,1)
	quad->verts[2].position.y = y1;
	quad->verts[3].position.x = x0;  // v0 (0,0)
	quad->verts[3].position.y = y0;
	quad->verts[4].position.x = x1;  // v2 (1,1)
	quad->verts[4].position.y = y1;
	quad->verts[5].position.x = x0;  // v3 (0,1)
	quad->verts[5].position.y = y1;
	for (int i = 0; i < 6; i++) {
		quad->verts[i].position.z = 0.1f;
		quad->verts[i].color = *color;
	}
}

void ta_primitive_render_lines()
{
	u32 queue_len = dlb_vec_len(lines_queue);
	if (!queue_len) {
		return;
	}

	ta_shader_lines_bind();
    ta_shader_lines_prerender();

    glBindVertexArray(lines_vao);
	glBindBuffer(GL_ARRAY_BUFFER, lines_buffer);

	// Update buffer (resize if necessary)
	int queue_size = dlb_vec_size(lines_queue);
	if (queue_size > lines_buffer_size) {
		glNamedBufferData(lines_buffer, queue_size, lines_queue, GL_DYNAMIC_DRAW);
		lines_buffer_size = queue_size;
	} else {
		glNamedBufferSubData(lines_buffer, 0, queue_size, lines_queue);
	}

	// Draw lines
    ta_shader_lines_render(lines_queue);

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
    ta_shader_lines_unbind();
}
void ta_primitive_render_quads()
{
	u32 queue_len = dlb_vec_len(quads_queue);
	if (!queue_len) {
		return;
	}

	glUseProgram(quads_program);

	ta_shader_quads_prerender();

	glBindVertexArray(quads_vao);
	glBindBuffer(GL_ARRAY_BUFFER, quads_buffer);

	// Update buffer (resize if necessary)
	int queue_size = dlb_vec_size(quads_queue);
	if (queue_size > quads_buffer_size) {
		glNamedBufferData(quads_buffer, queue_size, quads_queue, GL_DYNAMIC_DRAW);
		quads_buffer_size = queue_size;
	} else {
		glNamedBufferSubData(quads_buffer, 0, queue_size, quads_queue);
	}

	// Draw quads
	glDrawArrays(GL_TRIANGLES, 0, 6 * queue_len);

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glUseProgram(0);
}
void ta_primitive_render()
{
	ta_primitive_render_lines();
	ta_primitive_render_quads();
}
void ta_primitive_clear()
{
	dlb_vec_clear(lines_queue);
	dlb_vec_clear(quads_queue);
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