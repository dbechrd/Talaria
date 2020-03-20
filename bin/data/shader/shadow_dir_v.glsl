#version 330 core

layout(location = 0) in vec3 attr_position;
layout(location = 5) in vec3 attr_morph0_position;

uniform float u_morph_weights[1];
uniform mat4 u_model;
uniform mat4 u_light_pvm;

void main() {
    // add weighted morph targets to give us current pose
    vec3 morphed_pos =
        attr_position +
        u_morph_weights[0] * attr_morph0_position;

    vec4 pos = vec4(morphed_pos, 1.0);
    gl_Position = u_light_pvm * pos;
}