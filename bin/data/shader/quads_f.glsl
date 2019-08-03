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
	vec4 tex_color = texture(u_tex, vertex.uv);
	//final_color = mix(vertex.color, tex_color, tex_color.r);
    final_color = vec4(vertex.color.rgb, tex_color.r);
}
