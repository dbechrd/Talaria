#version 330 core

layout(location = 0) in vec3 attr_position;

uniform mat4 u_light_pvm;

void main() {
    gl_Position = u_light_pvm * vec4(attr_position, 1.0);
}