#version 330 core

layout(location = 0) in vec3 attr_position;
layout(location = 1) in vec4 attr_color;

uniform mat4 u_proj;
uniform mat4 u_view;
uniform mat4 u_model;

out vs_out {
    vec3 position;
    vec4 color;
} vertex;

void main()
{
    vec4 pos = u_model * vec4(attr_position, 1.0);
    vertex.position = pos.xyz;
    vertex.color = attr_color;
    gl_Position = u_proj * u_view * pos;
}
