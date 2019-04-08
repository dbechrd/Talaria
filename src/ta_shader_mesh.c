#include "ta_shader_mesh.h"
#include "ta_shader.h"
#include "ta_log.h"

static GLuint program;

static GLint u_proj;
static GLint u_view;
static GLint u_model;
static GLint u_tex[1];

static ta_mat4 proj;
static ta_mat4 view;
static ta_mat4 model;
static GLuint tex[1];

void ta_shader_mesh_init()
{
	// Compile shaders
	GLuint vshader = ta_shader_compile_file(GL_VERTEX_SHADER, "data/shader/mesh_v.glsl");
	if (!vshader) {
		DLB_ASSERT(!"ta_shader_mesh_init: failed to compile vertex shader");
	}

	GLuint fshader = ta_shader_compile_file(GL_FRAGMENT_SHADER, "data/shader/mesh_f.glsl");
	if (!fshader) {
		ta_shader_free(vshader);
		DLB_ASSERT(!"ta_shader_mesh_init: failed to compile fragment shader");
	}

	// Link program
	program = ta_shader_program_init();
	glAttachShader(program, vshader);
	glAttachShader(program, fshader);
	glBindAttribLocation(program, TA_SHADER_ATTR_POSITION, "attr_position");
	glBindAttribLocation(program, TA_SHADER_ATTR_COLOR, "attr_color");
	glBindAttribLocation(program, TA_SHADER_ATTR_UV, "attr_uv");
	glBindAttribLocation(program, TA_SHADER_ATTR_NORMAL, "attr_normal");
	ta_shader_program_link(program);

	// Vertex uniforms
	u_proj = ta_shader_uniform_location(program, "u_proj");
	u_view = ta_shader_uniform_location(program, "u_view");
	u_model = ta_shader_uniform_location(program, "u_model");
	u_tex[0] = ta_shader_uniform_location(program, "u_tex0");
	DLB_ASSERT(u_proj >= 0);
	DLB_ASSERT(u_view >= 0);
	DLB_ASSERT(u_model >= 0);
	DLB_ASSERT(u_tex[0] >= 0);

	// Vertex attributes
	//GLint attr_position = ta_shader_attribute_location(program, "attr_position");
	//GLint attr_color = ta_shader_attribute_location(program, "attr_color");
	//GLint attr_uv = ta_shader_attribute_location(program, "attr_uv");
	//GLint attr_normal = ta_shader_attribute_location(program, "attr_normal");
	//DLB_ASSERT(attr_position == ATTR_POSITION);
	//DLB_ASSERT(attr_color == ATTR_COLOR);
	//DLB_ASSERT(attr_uv == ATTR_UV);
	//DLB_ASSERT(attr_normal == ATTR_NORMAL);

	glDetachShader(program, vshader);
	glDetachShader(program, fshader);
	ta_shader_free(vshader);
	ta_shader_free(fshader);
}

void ta_shader_mesh_free()
{
	ta_shader_program_free(program);
}

void ta_shader_mesh_bind()
{
	glUseProgram(program);
}

void ta_shader_mesh_unbind()
{
	glUseProgram(0);
}

void ta_shader_mesh_set_projection(const ta_mat4 *mat)
{
	proj = *mat;
}

void ta_shader_mesh_set_view(const ta_mat4 *mat)
{
	view = *mat;
}

void ta_shader_mesh_set_model(const ta_mat4 *mat)
{
	model = *mat;
}

void ta_shader_mesh_set_texture(int index, GLuint texture)
{
	tex[index] = texture;
}

void ta_shader_mesh_prerender()
{
	if (u_proj >= 0) {
		glUniformMatrix4fv(u_proj, 1, GL_TRUE, (GLfloat *)&proj);
	}
	if (u_view >= 0) {
		glUniformMatrix4fv(u_view, 1, GL_TRUE, (GLfloat *)&view);
	}
	if (u_model >= 0) {
		glUniformMatrix4fv(u_model, 1, GL_TRUE, (GLfloat *)&model);
	}
	for (int i = 0; i < ARRAY_COUNT(tex); i++) {
		if (tex[i] >= 0) {
			glActiveTexture(GL_TEXTURE0 + i);
			glBindTexture(GL_TEXTURE_2D, tex[i]);
			glUniform1i(u_tex[i], i);
		}
	}
}

void ta_shader_mesh_render(ta_mesh *mesh)
{
	glBindVertexArray(mesh->vao);
	if (mesh->index_count) {
		glDrawElements(GL_TRIANGLES, mesh->index_count, GL_UNSIGNED_INT, 0);
	} else {
		glDrawArrays(GL_TRIANGLES, 0, mesh->vertex_count);
	}
	glBindVertexArray(0);
}