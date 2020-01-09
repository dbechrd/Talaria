#version 330 core

in vs_out {
    vec3 position;
} vertex;

uniform vec3 u_light_pos;
uniform float u_light_zfar;

void main() {
    vec3 light_to_vert = vertex.position - u_light_pos;
    gl_FragDepth = length(light_to_vert) / u_light_zfar;
}