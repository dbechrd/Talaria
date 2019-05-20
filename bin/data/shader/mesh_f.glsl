#version 330 core

in vs_out {
    vec3 position;
    vec4 color;
	vec2 uv;
	vec3 normal;
    vec4 position_sun; // Position with respect to sunlight
} vertex;

struct light {
    int type;
    vec3 position;
    vec3 direction;
    vec3 color;
    // float intensity;
};
//uniform Light[8] lights;
//uniform int num_lights;
uniform light u_sun;

uniform sampler2D u_tex0;

out vec4 final_color;

void main()
{
	vec4 tex_color = texture(u_tex0, vertex.uv);
	//final_color = mix(tex_color, vertex.color, vertex.color.a > 0);
	//final_color = vec4(vertex.uv, 0.0, 1.0);
	//final_color = final_color + 0.0000001 * (tex_color);

    // Render faces as the color of their normal
    //final_color = vec4(abs(vertex.normal), 1.0);

    float strength = dot(-u_sun.direction, vertex.normal);
    final_color = vec4(tex_color.rgb * clamp(strength, 0.2, 0.6), tex_color.a);

	//final_color = tex_color * vec4(vec3(0.1), 1.0);
}
