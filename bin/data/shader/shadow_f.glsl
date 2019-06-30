#version 330 core

in vs_out {
    vec3 position;
} vertex;

uniform vec3 u_light_pos;
uniform float u_light_farz;

out float frag_color;

void main() {
#if 0
    //gl_FragDepth = gl_FragCoord.z / u_light_farz;
#else
    vec3 light_to_vert = vertex.position - u_light_pos;
    frag_color = length(light_to_vert);
#endif
}