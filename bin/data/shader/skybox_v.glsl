#version 330 core

layout(location = 0) in vec3 attr_position;

out vs_out {
	vec3 uvw;
} vertex;

uniform mat4 u_proj;
uniform mat4 u_view;

void main()
{
    vertex.uvw = vec3(attr_position.x, -attr_position.y, -attr_position.z);
    vec4 pos = u_proj * u_view * vec4(attr_position, 1.0);
    gl_Position = pos.xyww;
}