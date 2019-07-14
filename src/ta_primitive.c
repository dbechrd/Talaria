#include "ta_primitive.h"
#include "ta_log.h"
#include "ta_window.h"
#include "ta_shader.h"
#include "ta_symbol.h"
#include "dlb_vector.h"
#include "misc/gl3w.h"
#include <math.h>


typedef struct ta_shader_lines_vertex {
    ta_vec3 position;
    ta_rgba color;
} ta_shader_lines_vertex;

typedef struct ta_vert_line {
    ta_shader_lines_vertex verts[2];
} ta_vert_line;

typedef struct ta_shader_quads_vertex {
    ta_vec3 position;
    ta_rgba color;
    ta_uv uv;
} ta_shader_quads_vertex;

typedef struct ta_vert_quad {
    ta_shader_quads_vertex verts[6];
} ta_vert_quad;

static ta_vert_line *lines_queue;
static GLuint lines_vao;
static GLuint lines_buffer;
static GLint lines_buffer_size;

static ta_vert_quad *quads_queue;
static GLuint quads_vao;
static GLuint quads_buffer;
static GLint quads_buffer_size;

static void ta_primitive_push_line(ta_vert_line *line);
static void ta_primitive_push_quad(ta_vert_quad *quad);
static void ta_primitive_line2d_to_line(ta_vert_line *line, ta_line_2d line2d,
	ta_rgba color0, ta_rgba color1);

static void ta_primitive_init_lines()
{
	glCreateVertexArrays(1, &lines_vao);
	glCreateBuffers(1, &lines_buffer);
	lines_buffer_size = 0;

	glBindVertexArray(lines_vao);
	glBindBuffer(GL_ARRAY_BUFFER, lines_buffer);

    glEnableVertexAttribArray(TA_SHADER_ATTR_POSITION);
    glEnableVertexAttribArray(TA_SHADER_ATTR_COLOR);

    glVertexAttribPointer(TA_SHADER_ATTR_POSITION, 3, GL_FLOAT, GL_FALSE,
        sizeof(ta_shader_lines_vertex), 0);
    glVertexAttribPointer(TA_SHADER_ATTR_COLOR, 4, GL_FLOAT, GL_FALSE,
        sizeof(ta_shader_lines_vertex), (void *)sizeof(ta_vec3));

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}
static void ta_primitive_init_quads()
{
	glCreateVertexArrays(1, &quads_vao);
	glCreateBuffers(1, &quads_buffer);
	quads_buffer_size = 0;

	glBindVertexArray(quads_vao);
	glBindBuffer(GL_ARRAY_BUFFER, quads_buffer);

    glEnableVertexAttribArray(TA_SHADER_ATTR_POSITION);
    glEnableVertexAttribArray(TA_SHADER_ATTR_COLOR);
    glEnableVertexAttribArray(TA_SHADER_ATTR_UV);

    glVertexAttribPointer(TA_SHADER_ATTR_POSITION, 3, GL_FLOAT, GL_FALSE,
        sizeof(ta_shader_quads_vertex), 0);
    glVertexAttribPointer(TA_SHADER_ATTR_COLOR, 4, GL_FLOAT, GL_FALSE,
        sizeof(ta_shader_quads_vertex), (void *)sizeof(ta_vec3));
    glVertexAttribPointer(TA_SHADER_ATTR_UV, 2, GL_FLOAT, GL_FALSE,
        sizeof(ta_shader_quads_vertex), (void *)(sizeof(ta_vec3) + sizeof(ta_rgba)));

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
static void ta_primitive_line2d_to_line(ta_vert_line *line, ta_line_2d line2d,
	ta_rgba color0, ta_rgba color1)
{
	float x0 = X_TO_NDC(line2d.p0.x);
	float x1 = X_TO_NDC(line2d.p1.x);
	float y0 = Y_TO_NDC(line2d.p0.y);
	float y1 = Y_TO_NDC(line2d.p1.y);

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
void ta_primitive_push_line_2d(ta_line_2d line_2d, ta_rgba color0,
	ta_rgba color1)
{
	ta_vert_line line = { 0 };
	ta_primitive_line2d_to_line(&line, line_2d, color0, color1);
	ta_primitive_push_line(&line);
}
void ta_primitive_push_line_3d(ta_line_3d line_3d, ta_rgba color0,
    ta_rgba color1)
{
    ta_vert_line line = { 0 };
    ta_primitive_line3d_to_line(&line, line_3d, color0, color1);
    ta_primitive_push_line(&line);
}

static void ta_primitive_push_quad(ta_vert_quad *quad)
{
	dlb_vec_push(quads_queue, *quad);
}
void ta_primitive_push_rect(ta_rect parent, ta_rect rect, ta_rgba color)
{
    // v3 *----* v2
    //    |    |
    // v0 *----* v1
    // v0, v1, v2, v0, v2, v3

#define X_TO_NDC_RECT(x, r) ((float)(x) / (r.w / 2.0f) - 1.0f)
#define Y_TO_NDC_RECT(y, r) (-(float)(y) / (r.h / 2.0f) + 1.0f)
    float x0 = X_TO_NDC_RECT(parent.x + rect.x, parent);
    float x1 = X_TO_NDC_RECT(parent.x + rect.x + rect.w, parent);
    float y0 = Y_TO_NDC_RECT(parent.y + rect.y + rect.h, parent);
    float y1 = Y_TO_NDC_RECT(parent.y + rect.y, parent);
#undef X_TO_NDC_RECT
#undef Y_TO_NDC_RECT

    ta_vert_quad quad = { 0 };
    quad.verts[0].position.x = x0;  // v0 (0,0)
    quad.verts[0].position.y = y0;
    quad.verts[0].uv.u = 0.0f;
    quad.verts[0].uv.v = 0.0f;
    quad.verts[1].position.x = x1;  // v1 (1,0)
    quad.verts[1].position.y = y0;
    quad.verts[1].uv.u = 1.0f;
    quad.verts[1].uv.v = 0.0f;
    quad.verts[2].position.x = x1;  // v2 (1,1)
    quad.verts[2].position.y = y1;
    quad.verts[2].uv.u = 1.0f;
    quad.verts[2].uv.v = 1.0f;
    quad.verts[3].position.x = x0;  // v0 (0,0)
    quad.verts[3].position.y = y0;
    quad.verts[3].uv.u = 0.0f;
    quad.verts[3].uv.v = 0.0f;
    quad.verts[4].position.x = x1;  // v2 (1,1)
    quad.verts[4].position.y = y1;
    quad.verts[4].uv.u = 1.0f;
    quad.verts[4].uv.v = 1.0f;
    quad.verts[5].position.x = x0;  // v3 (0,1)
    quad.verts[5].position.y = y1;
    quad.verts[5].uv.u = 0.0f;
    quad.verts[5].uv.v = 1.0f;
    for (int i = 0; i < 6; i++) {
        quad.verts[i].position.z = 0.1f;
        quad.verts[i].color = color;
    }

	ta_primitive_push_quad(&quad);
}
void ta_primitive_push_plane(ta_plane plane, float radius, ta_rgba color)
{
    plane.center = vec3_add(plane.center, VEC3_EPSILON);
    ta_vec3 x = vec3_normalize(vec3_cross(plane.center, plane.normal));
    ta_vec3 y = vec3_normalize(vec3_cross(x, plane.normal));
    x = vec3_scalef(x, radius);
    y = vec3_scalef(y, radius);

    ta_vec3 v0 = vec3_sub(vec3_sub(plane.center, x), y);
    ta_vec3 v1 = vec3_add(vec3_sub(plane.center, x), y);
    ta_vec3 v2 = vec3_sub(vec3_add(plane.center, x), y);
    ta_vec3 v3 = vec3_add(vec3_add(plane.center, x), y);

    ta_vert_quad quad = { 0 };
    quad.verts[0].position = v0;
    quad.verts[0].uv.u = 0.0f;
    quad.verts[0].uv.v = 0.0f;
    quad.verts[1].position = v1;
    quad.verts[1].uv.u = 1.0f;
    quad.verts[1].uv.v = 0.0f;
    quad.verts[2].position = v2;
    quad.verts[2].uv.u = 0.0f;
    quad.verts[2].uv.v = 1.0f;
    quad.verts[3].position = v2;
    quad.verts[3].uv.u = 0.0f;
    quad.verts[3].uv.v = 1.0f;
    quad.verts[4].position = v1;
    quad.verts[4].uv.u = 1.0f;
    quad.verts[4].uv.v = 0.0f;
    quad.verts[5].position = v3;
    quad.verts[5].uv.u = 1.0f;
    quad.verts[5].uv.v = 1.0f;
    for (int i = 0; i < 6; i++) {
        quad.verts[i].color = color;
    }
    ta_primitive_push_quad(&quad);
}

void ta_primitive_push_crosshair(s32 length, s32 thickness)
{
	float ndc_x = (2.0f / tg_window.rect.w);
	float ndc_y = (2.0f / tg_window.rect.h);

	ta_rect x = { 0 };
	x.x = tg_window.rect.w / 2 - length / 2;
	x.y = tg_window.rect.h / 2 - thickness / 2;
	x.w = length;
	x.h = thickness;

	ta_rect y = { 0 };
	y.x = tg_window.rect.w / 2 - thickness / 2;
	y.y = tg_window.rect.h / 2 - length / 2;
	y.w = thickness;
	y.h = length;

	ta_primitive_push_rect(tg_window.rect, x, TA_COLOR_WHITE_ALPHA);
	ta_primitive_push_rect(tg_window.rect, y, TA_COLOR_WHITE_ALPHA);
}

void ta_primitive_push_axes(float scale)
{
    ta_line_3d X_AXIS = { 0 };
    ta_line_3d Y_AXIS = { 0 };
    ta_line_3d Z_AXIS = { 0 };
    X_AXIS.p1 = vec3_scalef(VEC3_X, scale);
    Y_AXIS.p1 = vec3_scalef(VEC3_Y, scale);
    Z_AXIS.p1 = vec3_scalef(VEC3_Z, scale);

    ta_primitive_push_line_3d(X_AXIS, TA_COLOR_RED,   TA_COLOR_RED);
    ta_primitive_push_line_3d(Y_AXIS, TA_COLOR_GREEN, TA_COLOR_GREEN);
    ta_primitive_push_line_3d(Z_AXIS, TA_COLOR_BLUE,  TA_COLOR_BLUE);
}

// TODO: Make this a setting somewhere?
#define SPHERE_SEGMENTS 32
#define SPHERE_SEG_RAD (DEG_TO_RADF(360.f / SPHERE_SEGMENTS))
void ta_primitive_push_sphere(ta_sphere sphere, ta_rgba color)
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
        ta_primitive_push_line_3d(line_yz, color, color);
        line_yz.p0 = line_yz.p1;

        line_xz.p1 = sphere.center;
        line_xz.p1.x += cosr;
        line_xz.p1.z += sinr;
        ta_primitive_push_line_3d(line_xz, color, color);
        line_xz.p0 = line_xz.p1;

        line_xy.p1 = sphere.center;
        line_xy.p1.x += cosr;
        line_xy.p1.y += sinr;
        ta_primitive_push_line_3d(line_xy, color, color);
        line_xy.p0 = line_xy.p1;
    }

    line_yz.p1 = sphere.center;
    line_yz.p1.y += sphere.radius;

    line_xz.p1 = sphere.center;
    line_xz.p1.x += sphere.radius;

    line_xy.p1 = sphere.center;
    line_xy.p1.x += sphere.radius;

    ta_primitive_push_line_3d(line_yz, color, color);
    ta_primitive_push_line_3d(line_xz, color, color);
    ta_primitive_push_line_3d(line_xy, color, color);
}

void ta_primitive_push_rgb_sphere(ta_sphere sphere)
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
        ta_primitive_push_line_3d(line_yz, TA_COLOR_WHITE, TA_COLOR_RED);
        line_yz.p0 = line_yz.p1;

        line_xz.p1 = sphere.center;
        line_xz.p1.x += cosr;
        line_xz.p1.z += sinr;
        ta_primitive_push_line_3d(line_xz, TA_COLOR_WHITE, TA_COLOR_GREEN);
        line_xz.p0 = line_xz.p1;

        line_xy.p1 = sphere.center;
        line_xy.p1.x += cosr;
        line_xy.p1.y += sinr;
        ta_primitive_push_line_3d(line_xy, TA_COLOR_WHITE, TA_COLOR_BLUE);
        line_xy.p0 = line_xy.p1;
    }

    line_yz.p1 = sphere.center;
    line_yz.p1.y += sphere.radius;

    line_xz.p1 = sphere.center;
    line_xz.p1.x += sphere.radius;

    line_xy.p1 = sphere.center;
    line_xy.p1.x += sphere.radius;

    ta_primitive_push_line_3d(line_yz, TA_COLOR_WHITE, TA_COLOR_RED);
    ta_primitive_push_line_3d(line_xz, TA_COLOR_WHITE, TA_COLOR_GREEN);
    ta_primitive_push_line_3d(line_xy, TA_COLOR_WHITE, TA_COLOR_BLUE);
}
#undef SPHERE_SEGMENTS

void ta_primitive_push_aabb(ta_aabb aabb, ta_rgba color)
{
    ta_line_3d line = { 0 };

    ta_vec3 min = vec3_sub(aabb.center, aabb.extents);
    ta_vec3 size = vec3_scalef(aabb.extents, 2.0f);

    ta_vec3 p0, p1, p2, p3, p4, p5, p6, p7;

#define VEC3(vec, a, b, c) vec.x = a; vec.y = b; vec.z = c;
    VEC3(p0, min.x         , min.y         , min.z         );
    VEC3(p1, min.x         , min.y         , min.z + size.z);
    VEC3(p2, min.x         , min.y + size.y, min.z         );
    VEC3(p3, min.x         , min.y + size.y, min.z + size.z);
    VEC3(p4, min.x + size.x, min.y         , min.z         );
    VEC3(p5, min.x + size.x, min.y         , min.z + size.z);
    VEC3(p6, min.x + size.x, min.y + size.y, min.z         );
    VEC3(p7, min.x + size.x, min.y + size.y, min.z + size.z);
#undef VEC3

#define PUSH_LINE(a, b) line.p0 = a; line.p1 = b; \
                        ta_primitive_push_line_3d(line, color, color)
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

void ta_primitive_render_lines(ta_shader *shader)
{
	u32 queue_len = dlb_vec_len(lines_queue);
	if (!queue_len) {
		return;
	}

	ta_shader_bind(shader);
    ta_shader_prerender(shader);
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
    glDrawArrays(GL_LINES, 0, 2 * queue_len);

    glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
    ta_shader_unbind(shader);
}
void ta_primitive_render_quads(ta_shader *shader)
{
	u32 queue_len = dlb_vec_len(quads_queue);
	if (!queue_len) {
		return;
	}

    ta_shader_bind(shader);
	ta_shader_prerender(shader);

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
    ta_shader_unbind(shader);
}
void ta_primitive_render()
{
	ta_primitive_render_lines(tg_shader_lines);
	ta_primitive_render_quads(tg_shader_quads);
}
void ta_primitive_clear()
{
	dlb_vec_clear(lines_queue);
	dlb_vec_clear(quads_queue);

	ta_shader_set_mat4(tg_shader_lines, SYM_U_PROJ, &MAT4_IDENT);
	ta_shader_set_mat4(tg_shader_lines, SYM_U_VIEW, &MAT4_IDENT);
	ta_shader_set_mat4(tg_shader_lines, SYM_U_MODEL, &MAT4_IDENT);

	ta_shader_set_mat4(tg_shader_quads, SYM_U_PROJ, &MAT4_IDENT);
	ta_shader_set_mat4(tg_shader_quads, SYM_U_VIEW, &MAT4_IDENT);
	ta_shader_set_mat4(tg_shader_quads, SYM_U_MODEL, &MAT4_IDENT);
}