#include "ta_shader_quads.h"
#include "ta_shader.h"
#include "ta_log.h"

static GLint u_proj;
static GLint u_view;
static GLint u_model;
static GLint u_tex[1];

static ta_mat4 proj;
static ta_mat4 view;
static ta_mat4 model;
static GLuint tex[1];

GLuint ta_shader_quads_init()
{
    // Compile shaders
    GLuint vshader = ta_shader_compile_file(GL_VERTEX_SHADER, "data/shader/quads_v.glsl");
    if (!vshader) {
        DLB_ASSERT(!"ta_shader_quads_init: failed to compile vertex shader");
    }

    GLuint fshader = ta_shader_compile_file(GL_FRAGMENT_SHADER, "data/shader/quads_f.glsl");
    if (!fshader) {
        ta_shader_free(vshader);
        DLB_ASSERT(!"ta_shader_quads_init: failed to compile fragment shader");
    }

    // Link program
    GLuint program = ta_shader_program_init();
    glAttachShader(program, vshader);
    glAttachShader(program, fshader);
    glBindAttribLocation(program, TA_SHADER_ATTR_POSITION, "attr_position");
	glBindAttribLocation(program, TA_SHADER_ATTR_COLOR, "attr_color");
	glBindAttribLocation(program, TA_SHADER_ATTR_UV, "attr_uv");
    ta_shader_program_link(program);

    // Vertex uniforms
	u_proj = ta_shader_uniform_location(program, "u_proj");
    u_view = ta_shader_uniform_location(program, "u_view");
    u_model = ta_shader_uniform_location(program, "u_model");
	u_tex[0] = ta_shader_uniform_location(program, "u_tex0");
    //DLB_ASSERT(u_proj >= 0);
    //DLB_ASSERT(u_view >= 0);
    //DLB_ASSERT(u_model >= 0);
	DLB_ASSERT(u_tex[0] >= 0);

    // Vertex attributes
    //GLint attr_position = ta_shader_attribute_location(program, "attr_position");
	//GLint attr_color = ta_shader_attribute_location(program, "attr_color");
	//GLint attr_uv = ta_shader_attribute_location(program, "attr_uv");
    //DLB_ASSERT(attr_position == ATTR_POSITION);
	//DLB_ASSERT(attr_color == ATTR_COLOR);
	//DLB_ASSERT(attr_uv == ATTR_UV);

    glDetachShader(program, vshader);
    glDetachShader(program, fshader);
    ta_shader_free(vshader);
    ta_shader_free(fshader);
    return program;
}

void ta_shader_quads_free(GLuint program)
{
    ta_shader_program_free(program);
}

void ta_shader_quads_attribs()
{
    glEnableVertexAttribArray(TA_SHADER_ATTR_POSITION);
	glEnableVertexAttribArray(TA_SHADER_ATTR_COLOR);
	glEnableVertexAttribArray(TA_SHADER_ATTR_UV);

    glVertexAttribPointer(TA_SHADER_ATTR_POSITION, 3, GL_FLOAT, GL_FALSE,
        sizeof(ta_shader_quads_vertex), 0);
    glVertexAttribPointer(TA_SHADER_ATTR_COLOR, 4, GL_FLOAT, GL_FALSE,
        sizeof(ta_shader_quads_vertex), (void *)sizeof(ta_vec3));
	glVertexAttribPointer(TA_SHADER_ATTR_UV, 2, GL_FLOAT, GL_FALSE,
		sizeof(ta_shader_quads_vertex), (void *)(sizeof(ta_vec3) + sizeof(ta_rgba)));
}

void ta_shader_quads_set_projection(const ta_mat4 *mat)
{
	proj = *mat;
}

void ta_shader_quads_set_view(const ta_mat4 *mat)
{
	view = *mat;
}

void ta_shader_quads_set_model(const ta_mat4 *mat)
{
	model = *mat;
}

void ta_shader_quads_set_texture(int index, GLuint texture)
{
	tex[index] = texture;
}

void ta_shader_quads_prerender()
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
		if (u_tex[i] >= 0) {
			glActiveTexture(GL_TEXTURE0 + i);
			glBindTexture(GL_TEXTURE_2D, tex[i]);
			glUniform1i(u_tex[i], 0);
		}
	}
}