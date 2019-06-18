#version 330 core

in vs_out {
    vec3 position;
    vec4 color;
	vec2 uv;
} vertex;

uniform sampler2D u_tex;

out vec4 final_color;

void main()
{
	//final_color = vec4(vertex.color.rgb, 1.0);
	vec4 tex_color = texture(u_tex, vertex.uv);
	final_color = mix(tex_color, vertex.color, vertex.color.a > 0);
}
