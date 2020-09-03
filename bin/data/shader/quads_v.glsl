#version 330 core

layout(location = 0) in vec4 attr_color;
layout(location = 1) in vec2 attr_uv;
layout(location = 2) in vec3 attr_position;

uniform mat4 u_proj;
uniform mat4 u_view;
uniform mat4 u_model;

out vs_out {
    vec4 color;
	vec2 uv;
    //vec3 position;
} vertex;

void main()
{
    vec4 pos = u_model * vec4(attr_position, 1.0);
    gl_Position = u_proj * u_view * pos;

    //vertex.position = pos.xyz;
	vertex.color = attr_color;
	vertex.uv = attr_uv;
}