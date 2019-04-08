#version 330 core

out vs_out {
    vec3 position;
    vec4 color;
} vertex;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

layout(location = 0) in vec3 attr_position;
layout(location = 1) in vec4 attr_color;

void main()
{
    //vec4 pos = model * vec4(attr_position, 1.0);
    //vertex.position = pos.xyz;
	//vertex.color = attr_color;
    //gl_Position = proj * view * pos;

    vec4 pos = vec4(attr_position, 1.0);
    vertex.position = pos.xyz;
	vertex.color = attr_color;
    gl_Position = pos;
}
