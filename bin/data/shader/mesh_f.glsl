#version 330 core

in vs_out {
    vec3 position;
    vec4 color;
	vec2 uv;
	vec3 normal;
} vertex;

out vec4 final_color;

const float PI = 3.14159265359;

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
uniform vec3 u_camera_pos;
uniform sampler2D u_tex0;

float DistributionGGX(vec3 N, vec3 H, float roughness);
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness);
vec3 FresnelSchlick(float cosTheta, vec3 F0);

#define mtl_metallic    0.1
#define mtl_roughness   0.3
#define mtl_albedo      pow(tex_color.rgb, vec3(2.2))
#define mtl_ao          1.0
#define mtl_opacity     tex_color.a

void main()
{
    vec4 tex_color = texture(u_tex0, vertex.uv);

    vec3 N = vertex.normal;
    vec3 V = normalize(u_camera_pos - vertex.position);

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, mtl_albedo, mtl_metallic);

    ////////////////////////////////////////////////////////////////////////////
    // Per-light calculations
    //float distance = length(u_sun.position - vertex.position);
    //float attenuation = 1.0 / (distance * distance);
    float attenuation = 1.0;
    vec3 radiance = u_sun.color * attenuation;

    //vec3 L = normalize(u_sun.position - vertex.position);
    vec3 L = normalize(-u_sun.direction);
    vec3 H = normalize(V + L);

    float D = DistributionGGX(N, H, mtl_roughness);
    vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);
    float G = GeometrySmith(N, V, L, mtl_roughness);

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - mtl_metallic;

    vec3 numer = D * F * G;
    float NdotL = max(dot(N, L), 0.0);
    float denom = 4.0 * max(dot(N, V), 0.0) * NdotL;
    vec3 specular = numer / max(denom, 0.001);

    vec3 L0 = (kD * mtl_albedo / PI + specular) * radiance * NdotL;
    ////////////////////////////////////////////////////////////////////////////

    vec3 ambient = vec3(0.03) * mtl_albedo * mtl_ao;
    vec3 color = ambient + L0;
    color /= color + vec3(1.0);
    color = pow(color, vec3(1.0 / 2.2));

    final_color = vec4(color, mtl_opacity);

	//final_color = mix(tex_color, vertex.color, vertex.color.a > 0);
	//final_color = final_color + 0.0000001 * (tex_color);

    // colors
    //final_color = vertex.color;

    // normals
    //final_color = vec4(abs(vertex.normal), 1.0);

    // uv coords
    //final_color = vec4(vertex.uv, 0.0, 1.0);

    // shitty diffuse
    //float strength = dot(-u_sun.direction, vertex.normal);
    //final_color = vec4(tex_color.rgb * clamp(strength, 0.2, 0.6), tex_color.a);

    // shitty ambient
    //final_color = tex_color * vec4(vec3(0.1), 1.0);
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a      = roughness*roughness;
    float a2     = a * a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}