#version 330 core

in vs_out {
	vec3 uvw;
} vertex;

uniform samplerCube u_tex;

out vec4 final_color;

void main()
{
	vec4 tex_color = texture(u_tex, vertex.uvw);
	final_color = vec4(vec3(tex_color.r), 1.0);
}
