#version 330 core

in vs_out {
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
    mat4 light_pv;
    // Point
    samplerCube shadowmap3d;
    float shadowmap_zfar;
};
uniform int u_lights_count;
uniform Light[8] u_lights;

uniform vec3 u_camera_pos;

// TODO: Use this to color selected object differently
uniform bool u_selected;

#define DBG_VTX_COLOR       1
#define DBG_VTX_UV          2
#define DBG_VTX_NORMAL      3
#define DBG_VTX_TANGENT     4
#define DBG_VTX_TBN_NORMAL  5
#define DBG_NORMAL_MAP      6
#define DBG_MTL_ALBEDO      7
#define DBG_MTL_METALLIC    8
#define DBG_MTL_ROUGHNESS   9
#define DBG_MTL_OCCLUSION   10
uniform int u_debug_channel;

float DistributionGGX(vec3 N, vec3 H, float roughness);
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness);
vec3 FresnelSchlick(float cosTheta, vec3 F0);

void main()
{
    vec2 vertex_uv = vertex.uv;// * 8.0;
    // TODO: Use defaults if texture not present
    // NOTE: Currently implemented in ta_model.c, maybe it should stay there?
    vec4  tex_albedo    = texture(u_tex_albedo,    vertex_uv);
    vec3  mtl_albedo    = tex_albedo.rgb;                           // default: none
    float mtl_opacity   = tex_albedo.a;                             // default: 1.0
    float mtl_height    = texture(u_tex_height,    vertex_uv).r;    // default: 0.0
    float mtl_metallic  = texture(u_tex_metallic,  vertex_uv).r;    // default: 0.0
    vec3  mtl_normal    = texture(u_tex_normal,    vertex_uv).rgb;  // default: vec3(0.0, 0.0, 1.0)
    float mtl_occlusion = texture(u_tex_occlusion, vertex_uv).r;    // default: 0.0
    float mtl_roughness = texture(u_tex_roughness, vertex_uv).r;    // default: 0.5
    mtl_roughness = max(mtl_roughness, 0.001);

    vec3 N = normalize(mtl_normal * 2.0 - 1.0);
    vec3 V = normalize(vertex.tbn_camera_pos - vertex.tbn_position);

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, mtl_albedo, mtl_metallic);

    float shadows[8];
    float shadow_map_depths[8];
    float shadow_dists[8];
    vec3 debug;

    vec3 L0 = vec3(0.0);
    for (int i = 0; i < u_lights_count; ++i) {
        vec3 fragToLight;
        float shadow_map_depth = 0.0;
        float shadow_bias = 0.0;
        float dist = 0.0;
        float attenuation;
		float shadow = 0.0;

        switch(u_lights[i].type) {
            case LIGHT_DIRECTIONAL: {
                fragToLight = -u_lights[i].direction;

                vec3 projCoords = vertex.light_pvm[i].xyz / vertex.light_pvm[i].w;
                projCoords = projCoords * 0.5 + 0.5;
                debug = projCoords;
                dist = projCoords.z;
                attenuation = u_lights[i].intensity;

                if (u_lights[i].cast_shadows) {
                    shadow_bias = 0.00001;
#if 0
                    shadow_map_depth = texture(u_lights[i].shadowmap2d, projCoords.st).r;
                    // TODO: Better bias based on direction of light? This code
                    // doesn't work, but tried to write it based on:
                    // https://learnopengl.com/Advanced-Lighting/Shadows/Shadow-Mapping
                    //shadow_bias = max(0.05 * (1.0 - dot(N, u_lights[i].direction)), 0.001);
		            shadow = step(shadow_map_depth, dist - shadow_bias);
#else
                    // Soft shadows
                    // TODO: Clean this crap up!
				    float ss_bias = shadow_bias;//0.004;
                    float ss_count = 0.0;
				    for (float x = -1.0; x <= 1.0; x += 1.0) {
					    for (float y = -1.0; y <= 1.0; y += 1.0) {
							vec2 ss_offset = vec2(x, y) * 0.001;
							float ss_depth = texture(u_lights[i].shadowmap2d, projCoords.st + ss_offset).r;
			                shadow += step(ss_depth, dist - ss_bias);
                            ss_count += 1.0;
					    }
				    }
                    shadow /= ss_count;
#endif

                    if (projCoords.z > 1.0) {
                        shadow = 0.0;
                    }
                }

                fragToLight = -vertex.tbn_light_dir[i];
                break;
            } case LIGHT_POINT: {
                fragToLight = u_lights[i].position - vertex.position;

                dist = length(fragToLight);
                attenuation = u_lights[i].intensity / dist * dist;

                if (u_lights[i].cast_shadows) {
                    shadow_bias = 0.05;
#if 0
                    shadow_map_depth = texture(u_lights[i].shadowmap3d, -fragToLight).r;
                    shadow_map_depth *= u_lights[i].shadowmap_zfar;
		            shadow = step(shadow_map_depth, dist - shadow_bias);
#else
                    // Soft shadows
                    // TODO: Clean this crap up!
                    float ss_count = 0.0;
				    for (float x = -1.0; x <= 1.0; x += 1.0) {
					    for (float y = -1.0; y <= 1.0; y += 1.0) {
					        for (float z = -1.0; z <= 1.0; z += 1.0) {
							    vec3 ss_offset = vec3(x, y, z) * 0.004 * dist;
							    float ss_depth = texture(u_lights[i].shadowmap3d, -fragToLight + ss_offset).r;
							    ss_depth *= u_lights[i].shadowmap_zfar;
			                    shadow += step(ss_depth, dist - shadow_bias);
                                ss_count += 1.0;
					        }
					    }
				    }
                    shadow /= ss_count;
		            shadow = smoothstep(0.01, 1.0, shadow);

                    // // https://learnopengl.com/Advanced-Lighting/Shadows/Point-Shadows
                    // float shadow  = 0.0;
                    // float bias    = 0.05;
                    // float samples = 4.0;
                    // float offset  = 0.1;
                    // for(float x = -offset; x < offset; x += offset / (samples * 0.5))
                    // {
                    //     for(float y = -offset; y < offset; y += offset / (samples * 0.5))
                    //     {
                    //         for(float z = -offset; z < offset; z += offset / (samples * 0.5))
                    //         {
                    //             float closestDepth = texture(depthMap, fragToLight + vec3(x, y, z)).r;
                    //             closestDepth *= far_plane;   // Undo mapping [0;1]
                    //             if(currentDepth - bias > closestDepth)
                    //                 shadow += 1.0;
                    //         }
                    //     }
                    // }
                    // shadow /= (samples * samples * samples);
#endif
                }

                fragToLight = vertex.tbn_light_pos[i] - vertex.tbn_position;
                break;
            }
        }
        shadows[i] = shadow;
        shadow_map_depths[i] = shadow_map_depth;
        shadow_dists[i] = dist;
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

    mtl_albedo = mix(mtl_albedo, vec3(0.18, 0.28, 0.35), 0.5);
    vec3 ambient = 0.01 * mtl_albedo * mtl_occlusion;
    vec3 color = ambient + L0;

    // Tone mapping (Reinhard operator)
    color /= color + vec3(1.0);
    // Gamma correction
    color = pow(color, vec3(1.0 / 2.2));

    final_color = vec4(color, mtl_opacity);

    //-------------------------------------------------------------------
    // debug channels
    //-------------------------------------------------------------------

    // vertex properties
    vec4 dbg_vtx_color = vertex.color;
    vec4 dbg_vtx_uv = vec4(vertex_uv.x, vertex_uv.y, 0.0, 1.0);
    vec4 dbg_vtx_normal = vec4((vertex.normal * 0.5) + 0.5, 1.0);
    vec4 dbg_vtx_tangent = vec4((vertex.tangent * 0.5) + 0.5, 1.0);
    vec4 dbg_vtx_tbn_normal = vec4((vertex.tbn_normal * 0.5) + 0.5, 1.0);
    vec4 dbg_normal_map = vec4((N * 0.5) + 0.5, 1.0);

    // material properties
    vec4 dbg_mtl_albedo = vec4(mtl_albedo, 1.0);
    vec4 dbg_mtl_metallic = vec4(vec3(mtl_metallic), 1.0);
    vec4 dbg_mtl_roughness = vec4(vec3(mtl_roughness), 1.0);
    vec4 dbg_mtl_occlusion = vec4(vec3(mtl_occlusion), 1.0);

     // lighting
    //vec4 dbg_shadow0 = vec4(vec3(1.0 - shadows[0]), 1.0);
    //vec4 dbg_shadow1 = vec4(vec3(1.0 - shadows[1]), 1.0);
    //int light_idx = 0;
    //final_color = vec4(vec3(1.0 - shadows[light_idx]), 1.0);
    //final_color = vec4(vec3(1.0 - (shadow_map_depths[light_idx] / 30.0f)), 1.0);
    //final_color = vec4(vec3(1.0 - shadow_dists[light_idx] / 40.0f), 1.0);
    //final_color = vec4(vertex.tbn_light_dir[light_idx], 1.0);

    switch (u_debug_channel) {
        case DBG_VTX_COLOR:
            final_color = dbg_vtx_color;
            break;
        case DBG_VTX_UV:
            final_color = dbg_vtx_uv;
            break;
        case DBG_VTX_NORMAL:
            final_color = dbg_vtx_normal;
            break;
        case DBG_VTX_TANGENT:
            final_color = dbg_vtx_tangent;
            break;
        case DBG_VTX_TBN_NORMAL:
            final_color = dbg_vtx_tbn_normal;
            break;
        case DBG_NORMAL_MAP:
            final_color = dbg_normal_map;
            break;
        case DBG_MTL_ALBEDO:
            final_color = dbg_mtl_albedo;
            break;
        case DBG_MTL_METALLIC:
            final_color = dbg_mtl_metallic;
            break;
        case DBG_MTL_ROUGHNESS:
            final_color = dbg_mtl_roughness;
            break;
        case DBG_MTL_OCCLUSION:
            final_color = dbg_mtl_occlusion;
            break;
    };
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
