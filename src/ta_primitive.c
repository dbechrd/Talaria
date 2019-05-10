#include "ta_primitive.h"
#include "ta_log.h"
#include "ta_window.h"
#include "ta_shader.h"
#include "dlb_vector.h"
#include "misc/gl3w.h"

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
static void ta_primitive_rect_to_quad(ta_vert_quad *quad, int x, int y,
	ta_rect rect, ta_rgba color);
static void ta_primitive_bbox_to_quad(ta_vert_quad *quad, ta_bbox_2d bbox,
	ta_rgba color);

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
static void ta_primitive_rect_to_quad(ta_vert_quad *quad, int x, int y,
	ta_rect rect, ta_rgba color)
{
	// v3 *----* v2
	//    |    |
	// v0 *----* v1
	// v0, v1, v2, v0, v2, v3

	float x0 = X_TO_NDC(x + rect.x);
	float x1 = X_TO_NDC(x + rect.x + rect.w);
	float y0 = Y_TO_NDC(y + rect.y + rect.h);
	float y1 = Y_TO_NDC(y + rect.y);

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
		quad->verts[i].color = color;
	}
}
void ta_primitive_push_rect(int x, int y, ta_rect rect, ta_rgba color)
{
	ta_vert_quad quad = { 0 };
	ta_primitive_rect_to_quad(&quad, x, y, rect, color);
	ta_primitive_push_quad(&quad);
}

static void ta_primitive_bbox_to_quad(ta_vert_quad *quad, ta_bbox_2d bbox,
	ta_rgba color)
{
	// v3 *----* v2
	//    |    |
	// v0 *----* v1
	// v0, v1, v2, v0, v2, v3

	float x0 = bbox.center.x - bbox.half_axes.x;
	float x1 = bbox.center.x + bbox.half_axes.x;
	float y0 = bbox.center.y - bbox.half_axes.y;
	float y1 = bbox.center.y + bbox.half_axes.y;

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
		quad->verts[i].color = color;
	}
}

void ta_primitive_render_lines()
{
	u32 queue_len = dlb_vec_len(lines_queue);
	if (!queue_len) {
		return;
	}

	ta_shader_bind(tg_shader_lines);
    ta_shader_prerender(tg_shader_lines);
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
    ta_shader_unbind(tg_shader_lines);
}
void ta_primitive_render_quads()
{
	u32 queue_len = dlb_vec_len(quads_queue);
	if (!queue_len) {
		return;
	}

    ta_shader_bind(tg_shader_quads);
	ta_shader_prerender(tg_shader_quads);

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
    ta_shader_unbind(tg_shader_quads);
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