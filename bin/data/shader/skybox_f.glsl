#version 330 core

in vs_out {
	vec3 uvw;
} vertex;

uniform samplerCube u_tex;

out vec4 final_color;

void main()
{
    final_color = texture(u_tex, vertex.uvw);
    //final_color = vec4(1.0, 0.0, 0.0, 1.0);
}