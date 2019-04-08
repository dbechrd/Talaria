#pragma once
#include "ta_file.h"
#include "misc/gl3w.h"

typedef enum {
	TA_SHADER_ATTR_POSITION = 0,
	TA_SHADER_ATTR_COLOR    = 1,
	TA_SHADER_ATTR_UV       = 2,
	TA_SHADER_ATTR_NORMAL   = 3,
	TA_SHADER_ATTR_COUNT
} ta_shader_attr;

GLuint ta_shader_compile(GLenum type, ta_buffer *buf);
GLuint ta_shader_compile_file(GLenum type, const char *filename);
GLuint ta_shader_program_init();
void ta_shader_program_link(GLuint program);
GLint ta_shader_attribute_location(GLuint program, const char *name);
GLint ta_shader_uniform_location(GLuint program, const char *name);
void ta_shader_free(GLuint shader);
void ta_shader_program_free(GLuint program);
