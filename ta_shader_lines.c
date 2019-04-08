#include "ta_shader_lines.h"
#include "ta_shader.h"

GLuint ta_shader_lines_init()
{
    // Compile shaders
    GLuint vshader = ta_shader_compile_file(GL_VERTEX_SHADER, "data/shader/lines_v.glsl");
    if (!vshader) {
        DLB_ASSERT(!"ta_shader_lines_init: failed to compile vertex shader");
    }

    GLuint fshader = ta_shader_compile_file(GL_FRAGMENT_SHADER, "data/shader/lines_f.glsl");
    if (!fshader) {
        ta_shader_free(vshader);
        DLB_ASSERT(!"ta_shader_lines_init: failed to compile fragment shader");
    }

    // Link program
    GLuint program = ta_shader_program_init();
    glAttachShader(program, vshader);
    glAttachShader(program, fshader);
    glBindAttribLocation(program, TA_SHADER_ATTR_POSITION, "attr_position");
    glBindAttribLocation(program, TA_SHADER_ATTR_COLOR, "attr_color");
    ta_shader_program_link(program);

    // Vertex uniforms
    //GLint u_model = ta_shader_uniform_location(program, "model");
    //GLint u_view = ta_shader_uniform_location(program, "view");
    //GLint u_proj = ta_shader_uniform_location(program, "proj");
    //DLB_ASSERT(u_model >= 0);
    //DLB_ASSERT(u_view >= 0);
    //DLB_ASSERT(u_proj >= 0);

    // Vertex attributes
    GLint attr_position = ta_shader_attribute_location(program, "attr_position");
    GLint attr_color = ta_shader_attribute_location(program, "attr_color");
    DLB_ASSERT(attr_position == TA_SHADER_ATTR_POSITION);
    DLB_ASSERT(attr_color == TA_SHADER_ATTR_COLOR);

    glDetachShader(program, vshader);
    glDetachShader(program, fshader);
    ta_shader_free(vshader);
    ta_shader_free(fshader);
    return program;
}

void ta_shader_lines_free(GLuint program)
{
    ta_shader_program_free(program);
}

void ta_shader_lines_attribs()
{
    glEnableVertexAttribArray(TA_SHADER_ATTR_POSITION);
    glEnableVertexAttribArray(TA_SHADER_ATTR_COLOR);

    glVertexAttribPointer(TA_SHADER_ATTR_POSITION, 3, GL_FLOAT, GL_FALSE,
        sizeof(ta_shader_lines_vertex), 0);
    glVertexAttribPointer(TA_SHADER_ATTR_COLOR, 4, GL_FLOAT, GL_FALSE,
        sizeof(ta_shader_lines_vertex), (void *)sizeof(ta_vec3));
}