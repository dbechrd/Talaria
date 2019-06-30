#version 330 core

layout(location = 0) in vec3 attr_position;
layout(location = 1) in vec4 attr_color;
layout(location = 2) in vec2 attr_uv;
layout(location = 3) in vec3 attr_normal;

out vs_out {
    vec3 position;
    vec4 color;
	vec2 uv;
	vec3 normal;
} vertex;

uniform mat4 u_proj;
uniform mat4 u_view;
uniform mat4 u_model;

void main()
{
    vec4 pos = u_model * vec4(attr_position, 1.0);
    // TODO: Premultiply MVP matrix and pass as uniform
    gl_Position = u_proj * u_view * pos;
    vertex.position = pos.xyz;
	vertex.color = attr_color;
	vertex.uv = attr_uv;
    vertex.normal = (u_model * vec4(attr_normal, 0.0)).xyz;
}
