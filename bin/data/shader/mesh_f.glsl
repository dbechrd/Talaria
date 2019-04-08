#version 330 core

in vs_out {
    vec3 position;
    vec4 color;
	vec2 uv;
	vec3 normal;
} vertex;

uniform sampler2D u_tex0;

out vec4 final_color;

void main()
{
	vec4 tex_color = texture(u_tex0, vertex.uv);
	//final_color = mix(tex_color, vertex.color, vertex.color.a > 0);
	//final_color = vec4(vertex.uv, 0.0, 1.0);
	//final_color = final_color + 0.0000001 * (tex_color);
	final_color = tex_color;
}
