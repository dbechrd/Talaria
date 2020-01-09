#version 330 core

layout(location = 0) in vec3 attr_position;
layout(location = 1) in vec4 attr_color;
layout(location = 2) in vec2 attr_uv;
layout(location = 3) in vec3 attr_normal;
layout(location = 4) in vec3 attr_tangent;

struct Light {
    float intensity;
    vec3 position;
    vec3 color;
    int type;
    bool cast_shadows;
    // Directional / Spot
    vec3 direction;
    sampler2D shadowmap2d;
    mat4 light_pv;
    // Point
    samplerCube shadowmap3d;
    float shadowmap_zfar;
};
uniform int u_lights_count;
uniform Light[8] u_lights;

uniform vec3 u_camera_pos;

out vs_out {
    vec3 position;
    vec4 color;
	vec2 uv;
    vec3 normal;
    vec3 tangent;
    vec3 tbn_position;
	vec3 tbn_normal;
    vec3 tbn_camera_pos;
    vec3 tbn_light_pos[8];
    vec3 tbn_light_dir[8];
    vec4 light_pvm[8];
} vertex;

uniform mat4 u_proj;
uniform mat4 u_view;
uniform mat4 u_model;

void main()
{
    // TODO: Premultiply MVP matrix and pass as uniform
    vec4 position = u_model * vec4(attr_position, 1.0);
    vertex.position = vec3(position);
	vertex.color = attr_color;
	vertex.uv = attr_uv;
    vertex.normal = attr_normal;
    vertex.tangent = attr_tangent;

    mat3 normal_matrix = transpose(inverse(mat3(u_model)));
    vec3 T = normalize(normal_matrix * attr_tangent);
    vec3 N = normalize(normal_matrix * attr_normal);
    T = normalize(T - dot(T, N) * N);  // re-orthogonalize T with respect to N
    vec3 B = cross(N, T);
    mat3 TBN = transpose(mat3(T, B, N));

    vertex.tbn_normal = TBN * normalize(vec3(u_model * vec4(attr_normal, 0.0)));

    vertex.tbn_position = TBN * vertex.position;
    vertex.tbn_camera_pos = TBN * u_camera_pos;
    for (int i = 0; i < u_lights_count; i++) {
        vertex.tbn_light_pos[i] = TBN * u_lights[i].position;
        vertex.tbn_light_dir[i] = TBN * u_lights[i].direction;

        // TODO: Why the fuck isn't this working? Causes linker error!?!?
        vertex.light_pvm[i] = u_lights[i].light_pv * position;
    }

    // HACK: GLSL bullshit preventing this from happening in for loop
    vertex.light_pvm[0] = u_lights[0].light_pv * position;
    vertex.light_pvm[1] = u_lights[1].light_pv * position;
    vertex.light_pvm[2] = u_lights[2].light_pv * position;
    vertex.light_pvm[3] = u_lights[3].light_pv * position;
    vertex.light_pvm[4] = u_lights[4].light_pv * position;
    vertex.light_pvm[5] = u_lights[5].light_pv * position;
    vertex.light_pvm[6] = u_lights[6].light_pv * position;
    vertex.light_pvm[7] = u_lights[7].light_pv * position;

    gl_Position = u_proj * u_view * position;
}