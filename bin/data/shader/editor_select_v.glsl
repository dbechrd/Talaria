#version 330 core

layout(location = 2) in vec3 attr_position;

uniform mat4 u_proj;
uniform mat4 u_view;
uniform mat4 u_model;

void main()
{
    vec4 pos = u_model * vec4(attr_position, 1.0);
    gl_Position = u_proj * u_view * pos;
}
