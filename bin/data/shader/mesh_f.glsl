#version 330 core

in vs_out {
    vec3 position;
    vec4 color;
	vec2 uv;
	vec3 normal;
    vec3 tbn_position;
    vec3 tbn_camera_pos;
    vec3 tbn_light_pos[8];
    vec3 tbn_light_dir[8];
} vertex;

out vec4 final_color;

const float PI = 3.14159265359;

// https://forum.substance3d.com/index.php?topic=3243.0#msg14976
// Albedo
// Raw color with no lighting information. Small amount of ambient occlusion can
// be baked in if using it for micro-surface occlusion. The color range for dark
// values should stay within 30-50 RGB. Never have dark values below 30 RGB. The
// brightest color value should not go above 240 RGB.
//
// Roughness
// Describes the microsurface of the object. 1.0 is rough and 0.0 is smooth. The
// microsurface if rough can cause the light rays to scatter and make the
// highlight appear dimmer and more broad. The same amount of light energy is
// reflected going out as coming into the surface. This map has the most
// artistic freedom. There is no wrong answers here. This map gives the asset
// the most character as it truly describes the surface e.g. scratches,
// fingerprints, smudges, grime etc.
//
// Metallic
// Non-metal = 0.0, metal = 1.0. There can be transitional gray values that
// indicate something covering the raw metal such as dirt.

//#define mtl_albedo      pow(tex_albedo.rgb, vec3(2.2))
//#define mtl_albedo      tex_albedo.rgb
//#define mtl_opacity     tex_albedo.a
//#define mtl_metallic    tex_metallic.r
//#define mtl_roughness   tex_metallic.g
//#define mtl_ao          tex_metallic.b

//#define mtl_albedo    texture(material.tex0, vertex.uv).rgb
//#define mtl_opacity   texture(material.tex0, vertex.uv).a
//#define mtl_metallic  texture(material.tex1, vertex.uv).r
//#define mtl_roughness texture(material.tex1, vertex.uv).g
//#define mtl_ao        texture(material.tex1, vertex.uv).b
//#define mtl_emission  texture(material.tex2, vertex.uv).rgb
//#define mtl_emit      step(0.01, texture(material.tex2, vertex.uv).a)

// TODO: Premultiplied alpha
struct Material {
    // rgb: metallic ? specular.rgb : albedo.rgb
    //   a: metallic ?            1 : albedo.a
    sampler2D tex0;

    // r: metallic
    // g: roughness
    // b: ao
    // a: UNUSED
    sampler2D tex1;

    // rgb: emission color
    //   a: UNUSED
    sampler2D tex2;
};
uniform Material u_material;

// TODO: Combine these as above for performance; want ease-of-use for dev
uniform sampler2D u_tex_albedo;
uniform sampler2D u_tex_height;
uniform sampler2D u_tex_metallic;
uniform sampler2D u_tex_normal;
uniform sampler2D u_tex_occlusion;
uniform sampler2D u_tex_roughness;

#define LIGHT_AMBIENT       0
#define LIGHT_DIRECTIONAL   1
#define LIGHT_POINT         2
#define LIGHT_SPOT          3

#define L_DIR_DIRECTION(light) light.direction
#define L_DIR_COLOR(light) light.color

#define L_POINT_POSITION(light) light.position
#define L_POINT_COLOR(light) light.color

struct Light {
    float intensity;
    vec3 position;
    vec3 color;
    int type;
    bool cast_shadows;
    // Directional / Spot
    vec3 direction;
    sampler2D shadowmap2d;
    // Point
    samplerCube shadowmap3d;
    float shadowmap_zfar;
};
uniform uint u_lights_count;
uniform Light[8] u_lights;

uniform vec3 u_camera_pos;

// TODO: Use this to color selected object differently
uniform bool u_selected;

float DistributionGGX(vec3 N, vec3 H, float roughness);
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness);
vec3 FresnelSchlick(float cosTheta, vec3 F0);

void main()
{
    vec2 vertex_uv = vertex.uv * 1.0;
    // TODO: Use defaults if texture not present
    // NOTE: Currently implemented in ta_model.c, maybe it should stay there?
    vec3  mtl_albedo    = texture(u_tex_albedo,    vertex_uv).rgb;  // default: none
    float mtl_opacity   = texture(u_tex_albedo,    vertex_uv).a;    // default: 1.0
    float mtl_height    = texture(u_tex_height,    vertex_uv).r;    // default: 0.0
    float mtl_metallic  = texture(u_tex_metallic,  vertex_uv).r;    // default: 0.0
    vec3  mtl_normal    = texture(u_tex_normal,    vertex_uv).rgb;  // default: vec3(0.0, 0.0, 1.0)
    float mtl_occlusion = texture(u_tex_occlusion, vertex_uv).r;    // default: 0.0
    float mtl_roughness = texture(u_tex_roughness, vertex_uv).r;    // default: 0.5

    vec3 N = normalize(mtl_normal * 2.0 - 1.0);
    vec3 V = normalize(vertex.tbn_camera_pos - vertex.tbn_position);

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, mtl_albedo, mtl_metallic);

    vec3 L0 = vec3(0.0);
    for (uint i = 0U; i < u_lights_count; ++i) {
        vec3 fragToLight;
        float shadow_map_depth = 0.0;
        float shadow_bias = 0.0;
        float dist = 0.0;
        float attenuation;
		float shadow = 0.0;

        switch(u_lights[i].type) {
            case LIGHT_DIRECTIONAL: {
                fragToLight = -u_lights[i].direction;

                //vec3 projCoords = vertex.light_space.xyz / vertex.light_space.w;
                //projCoords = projCoords * 0.5 + 0.5;
                //shadow_map_depth = texture(shadow_textures[TEXTURE_IDX],
                //                           projCoords.st).r;
                //shadow_bias = 0.0001;

                //dist = projCoords.z;
                attenuation = u_lights[i].intensity;

                fragToLight = -vertex.tbn_light_dir[i];
                break;
            } case LIGHT_POINT: {
                fragToLight = u_lights[i].position - vertex.position;

                dist = length(fragToLight);
                attenuation = u_lights[i].intensity / dist * dist;

                if (u_lights[i].cast_shadows) {
                    shadow_map_depth = texture(u_lights[i].shadowmap3d, -fragToLight).r;
                    shadow_map_depth *= u_lights[i].shadowmap_zfar;
                    shadow_bias = 0.1;

                    // Soft shadows
                    // TODO: Clean this crap up via:
                    // https://learnopengl.com/Advanced-Lighting/Shadows/Point-Shadows
				    float ss_bias = 0.04;
                    float ss_count = 0.0;
				    for (float x = -1.0; x <= 1.0; x += 1.0) {
					    for (float y = -1.0; y <= 1.0; y += 1.0) {
					        for (float z = -1.0; z <= 1.0; z += 1.0) {
							    vec3 ss_offset = vec3(x, y, z) * 0.04;
							    float ss_depth = texture(u_lights[i].shadowmap3d, -fragToLight + ss_offset).r;
							    ss_depth *= u_lights[i].shadowmap_zfar;
							    float shadow_i = step(ss_depth, dist - ss_bias);
			                    shadow += step(ss_depth, dist - ss_bias);
                                ss_count += 1.0;
					        }
					    }
				    }
                    shadow /= ss_count;
		            shadow = smoothstep(0.01, 1.0, shadow);
                }

                fragToLight = vertex.tbn_light_pos[i] - vertex.tbn_position;
                break;
            }
        }
        //shadow = 0.0;

        vec3 radiance = u_lights[i].color * attenuation;

        vec3 L = normalize(fragToLight);
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

        //L0 += (kD * mtl_albedo / PI + specular) * radiance * NdotL;
        L0 += (kD * mtl_albedo / PI + specular) * radiance * NdotL * (1.0 - shadow);
        //L0 += mtl_albedo * (1.0 - shadow);
    }

    vec3 ambient = 0.01 * mtl_albedo * mtl_occlusion;
    vec3 color = ambient + L0;
    color /= color + vec3(1.0);
    color = pow(color, vec3(1.0 / 2.2));

    final_color = vec4(color, mtl_opacity);

    // vertex colors
    //final_color = vertex.color;
	//final_color = mix(tex_albedo, vertex.color, vertex.color.a > 0);

    // normals
    //final_color = vec4((N * 0.5) + 0.5, 1.0);

    // uv coords
    //final_color = vec4(vertex_uv.x, vertex_uv.y, 0.0, 1.0);

    // albedo
    //final_color = vec4(mtl_albedo, 1.0);
    //final_color = vec4(pow(mtl_albedo, vec3(1.0 / 2.2)), tex_albedo.a);

    // metallic
    //final_color = vec4(vec3(mtl_metallic), 1.0);

    // roughness
    //final_color = vec4(vec3(mtl_roughness), 1.0);

    // ambient occlusion
    //final_color = vec4(vec3(mtl_occlusion), 1.0);
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
