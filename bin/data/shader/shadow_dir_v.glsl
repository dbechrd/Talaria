#version 330 core

layout(location = 0) in vec3 attr_position;

uniform mat4 u_model;
uniform mat4 u_light_pvm;

void main() {
    vec4 pos = vec4(attr_position, 1.0);
    gl_Position = u_light_pvm * pos;
}