#version 330 core

in vs_out {
    //vec3 position;
    vec4 color;
	vec2 uv;
} vertex;

uniform sampler2D u_tex;

out vec4 final_color;

void main()
{
    final_color = vertex.color;
    final_color.a *= texture(u_tex, vertex.uv).r;
}
