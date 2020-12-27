#version 330 core

//------------------------------------------------------
// Constants
//------------------------------------------------------
const float PI = 3.14159265359;
const float GAMMA = 2.2;  // TODO: Make this a user-configurable uniform

//------------------------------------------------------
// Materials
//------------------------------------------------------
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

// TODO: Premultiplied alpha?
// TODO: Combine channels for performance
struct Material {
    uint    albedo_texture_pool_index;
    uint    albedo_texture_pool_layer;
    vec4    albedo_factor;
    uint    emission_texture_pool_index;
    uint    emission_texture_pool_layer;
    vec3    emission_factor;
    uint    metallic_texture_pool_index;
    uint    metallic_texture_pool_layer;
    float   metallic_factor;
    uint    roughness_texture_pool_index;
    uint    roughness_texture_pool_layer;
    float   roughness_factor;
    uint    height_texture_pool_index;
    uint    height_texture_pool_layer;
    float   height_factor;   // 0.02
    uint    normal_texture_pool_index;
    uint    normal_texture_pool_layer;
    uint    occlusion_texture_pool_index;
    uint    occlusion_texture_pool_layer;
};
uniform Material u_material;

//------------------------------------------------------
// Lights
//------------------------------------------------------
#define TA_LIGHTING_MAX_ACTIVE_LIGHTS 4

#define LIGHT_AMBIENT       0
#define LIGHT_DIRECTIONAL   1
#define LIGHT_POINT         2
#define LIGHT_SPOT          3
struct Light {
    // Common
    float intensity;
    vec3 position;
    vec3 color;
    int type;

    // Directional / Point / Spot
    bool cast_shadows;

    // Directional / Spot
    vec3 direction;
    mat4 light_pv;

    // Shadow mapping
    float shadowmap_zfar;                   // Point
    uint shadowmap_texture_pool_index;
    uint shadowmap_texture_array_layers[6]; // Point light "cubemaps" require 6 layers, other lights only use index 0
};
uniform int u_lights_count;
uniform Light u_lights[TA_LIGHTING_MAX_ACTIVE_LIGHTS];

struct UboLight {
    int type;                                // ta_light_type: type of light
    float intensity;                         // light intensity [0.0, +INF]
    bool cast_shadows;                       // bool: light casts dynamic shadows if true
    float pad0;

    vec3 position;   float pad1;             // light position in world space (note: for directional lights, position determines where the entity is renderered in the editor
    vec3 color;      float pad2;             // RGB light color ([0.0, 1.0], [0.0, 1.0], [0.0, 1.0])
    vec3 direction;  float pad3;             // light direction in world space (note: ignored for point lights)
    mat4 light_pv;                           // light projection-view matrix

    float shadowmap_zfar;                    // z-far perspective divide for point light shadow maps (ignored for all other light types)
    uint shadowmap_texture_pool_index;       // texture pool index where shadowmap is stored (note: pools are grouped by texture size)
    float pad4;
    float pad5;

    uint shadowmap_texture_array_layers[6];  // array texture layer (determines which texture in the pool to use, where "pool" is an array texture)
} ta_lighting_record;
layout (std140) uniform ubo_lights {
    UboLight lights[TA_LIGHTING_MAX_ACTIVE_LIGHTS];
};

//------------------------------------------------------
// Textures
//------------------------------------------------------
// NOTE: Needs to match the #define in ta_texture.h
#define TA_TEXTURE_POOL_MAX 16

uniform sampler2DArray u_textures[TA_TEXTURE_POOL_MAX];
// TODO: Should we just use regular cubemaps? Idk what overhead of the xyz -> cube_uv mapping function is, or what
// trade-offs there are. If performance is similiar, I'd much rather have just pools of sampler2Ds for simplicity.
//uniform samplerCube u_cubemaps[TA_LIGHTING_MAX_ACTIVE_LIGHTS];

//------------------------------------------------------
// Camera
//------------------------------------------------------
uniform vec3 u_camera_pos;

//------------------------------------------------------
// Editor
//------------------------------------------------------
uniform bool u_selected;

//------------------------------------------------------
// Debug
//------------------------------------------------------
#define DBG_VTX_COLOR       1
#define DBG_VTX_UV          2
#define DBG_VTX_NORMAL      3
#define DBG_VTX_TANGENT     4
#define DBG_VTX_TBN_NORMAL  5
#define DBG_NORMAL_MAP      6
#define DBG_MTL_ALBEDO      7
#define DBG_MTL_EMISSION    8
#define DBG_MTL_METALLIC    9
#define DBG_MTL_ROUGHNESS   10
#define DBG_MTL_OCCLUSION   11
#define DBG_SHADOW_0        12
#define DBG_SHADOW_1        13
#define DBG_SHADOW_2        14
#define DBG_SHADOW_3        15
uniform int u_debug_channel;
//------------------------------------------------------

in vs_out {
	vec2 uv;
    vec3 position;
    vec3 tbn_position;
    vec3 tbn_camera_pos;
    vec3 tbn_light_pos[TA_LIGHTING_MAX_ACTIVE_LIGHTS];
    vec3 tbn_light_dir[TA_LIGHTING_MAX_ACTIVE_LIGHTS];
    vec4 light_pvm[TA_LIGHTING_MAX_ACTIVE_LIGHTS];

    // NOTE: These are just passed to allow debug channels to display them
    // TODO: Use a separate shader for each debug channel? Would simplify this shader and reduce interface block size
    vec4 color;
    vec3 normal;
    vec3 tangent;
	vec3 tbn_normal;
    vec4 bone_indices;
    vec4 bone_weights;
} vertex;

out vec4 final_color;

vec2 ParallaxMapping(sampler2D heightmap, vec2 texCoords, vec3 viewDir);
float DistributionGGX(vec3 N, vec3 H, float roughness);
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness);
vec3 FresnelSchlick(float cosTheta, vec3 F0);
vec3 convert_xyz_to_cube_uv(vec3 uvw);








// http://developer.download.nvidia.com/whitepapers/2008/PCSS_Integration.pdf
#define BLOCKER_SEARCH_NUM_SAMPLES  16
#define PCF_NUM_SAMPLES             16
#define NEAR_PLANE                  abs(-40) // -40
#define LIGHT_WORLD_SIZE            0.002    // .002
#define MIN_PCF_FILTER_RADIUS       0.0002
#define MAX_PCF_FILTER_RADIUS       0.002
#define LIGHT_FRUSTUM_WIDTH         40
// Assuming that LIGHT_FRUSTUM_WIDTH == LIGHT_FRUSTUM_HEIGHT
#define LIGHT_SIZE_UV               (LIGHT_WORLD_SIZE / LIGHT_FRUSTUM_WIDTH)

vec2 poissonDisk[16] = vec2[](
    vec2(-0.94201624, -0.39906216),
    vec2(0.94558609, -0.76890725),
    vec2(-0.094184101, -0.92938870),
    vec2(0.34495938, 0.29387760),
    vec2(-0.91588581, 0.45771432),
    vec2(-0.81544232, -0.87912464),
    vec2(-0.38277543, 0.27676845),
    vec2(0.97484398, 0.75648379),
    vec2(0.44323325, -0.97511554),
    vec2(0.53742981, -0.47373420),
    vec2(-0.26496911, -0.41893023),
    vec2(0.79197514, 0.19090188),
    vec2(-0.24188840, 0.99706507),
    vec2(-0.81409955, 0.91437590),
    vec2(0.19984126, 0.78641367),
    vec2(0.14383161, -0.14100790)
);

float PenumbraSize(float zReceiver, float zBlocker) //Parallel plane estimation
{
    return (zReceiver - zBlocker) / zBlocker;
}

void FindBlocker(out float avgBlockerDepth, out float numBlockers, uint mapPool, uint mapLayer, vec2 uv, float zReceiver) {
    //This uses similar triangles to compute what
    //area of the shadow map we should search
    float searchWidth = LIGHT_SIZE_UV * (zReceiver - NEAR_PLANE) / zReceiver;
    float blockerSum = 0;
    numBlockers = 0;

    for (int i = 0; i < BLOCKER_SEARCH_NUM_SAMPLES; ++i) {
        float shadowMapDepth = texture(u_textures[mapPool], vec3(uv + poissonDisk[i] * searchWidth, mapLayer)).r;
        if (shadowMapDepth < zReceiver) {
            blockerSum += shadowMapDepth;
            numBlockers++;
        }
    }
    avgBlockerDepth = blockerSum / numBlockers;
}

float PCF_Filter(uint mapPool, uint mapLayer, vec2 uv, float zReceiver, float filterRadiusUV) {
    float sum = 0.0;
    float shadow_bias = 0.005;
    for (int i = 0; i < PCF_NUM_SAMPLES; ++i) {
        vec2 offset = poissonDisk[i] * filterRadiusUV;
        float shadowMapDepth = texture(u_textures[mapPool], vec3(uv + offset, mapLayer)).r;
        sum += float(shadowMapDepth < zReceiver - shadow_bias);
    }
    return sum / PCF_NUM_SAMPLES;
}

float PCSS(uint mapPool, uint mapLayer, vec3 coords) {
    vec2 uv = coords.xy;
    float zReceiver = coords.z; // Assumed to be eye-space z in this code

    // STEP 1: blocker search
    float avgBlockerDepth = 0;
    float numBlockers = 0;
    FindBlocker(avgBlockerDepth, numBlockers, mapPool, mapLayer, uv, zReceiver);
    if (numBlockers < 1) {
        //There are no occluders so early out (this saves filtering)
        return 1.0;
    }

    // STEP 2: penumbra size
    float penumbraRatio = PenumbraSize(zReceiver, avgBlockerDepth);
    float filterRadiusUV = penumbraRatio * LIGHT_SIZE_UV * NEAR_PLANE / coords.z;
    filterRadiusUV = clamp(filterRadiusUV, MIN_PCF_FILTER_RADIUS, MAX_PCF_FILTER_RADIUS);

    // STEP 3: filtering
    return PCF_Filter(mapPool, mapLayer, uv, zReceiver, filterRadiusUV);
    //return PCF_Filter(mapPool, mapLayer, uv, zReceiver, 0);
}










void main()
{
    vec3 V = normalize(vertex.tbn_camera_pos - vertex.tbn_position);
    vec2 scaled_uv = vertex.uv;// * 8.0;

    // TODO: Don't pass height 0.0 into shader
    // https://learnopengl.com/Advanced-Lighting/Parallax-Mapping
#if 0
    float height_factor = u_material.height_factor;
    float mtl_height = texture(u_textures[u_material.height_texture_pool_index], vec3(scaled_uv, u_material.height_texture_pool_layer)).r  * u_material.height_factor;
    vec2 displacement = V.xy / V.z * (height_factor - mtl_height);  // NOTE: Invert to get depth instead of height
    vec2 displaced_uv = scaled_uv - displacement;
    // Edge artifacts can sometimes be cleaned up like so, but I don't like this idea since it disallows UVs > 1.0
    //if (displaced_uv.x > 1.0 || displaced_uv.y > 1.0 || displaced_uv.x < 0.0 || displaced_uv.y < 0.0)
    //    discard;
#else
    vec2 displaced_uv = scaled_uv;
#endif
    vec4  mtl_albedo    = texture(u_textures[u_material.albedo_texture_pool_index   ], vec3(displaced_uv, u_material.albedo_texture_pool_layer   )).rgba * u_material.albedo_factor;
    vec3  mtl_emission  = texture(u_textures[u_material.emission_texture_pool_index ], vec3(displaced_uv, u_material.emission_texture_pool_layer )).rgb  * u_material.emission_factor;
    float mtl_metallic  = texture(u_textures[u_material.metallic_texture_pool_index ], vec3(displaced_uv, u_material.metallic_texture_pool_layer )).r    * u_material.metallic_factor;
    float mtl_roughness = texture(u_textures[u_material.roughness_texture_pool_index], vec3(displaced_uv, u_material.roughness_texture_pool_layer)).r    * u_material.roughness_factor;
    vec3  mtl_normal    = texture(u_textures[u_material.normal_texture_pool_index   ], vec3(displaced_uv, u_material.normal_texture_pool_layer   )).rgb;
    float mtl_occlusion = texture(u_textures[u_material.occlusion_texture_pool_index], vec3(displaced_uv, u_material.occlusion_texture_pool_layer)).r;

    // NOTE(debug): Override material with safe defaults
    //mtl_albedo = vec4(1.0);
    //mtl_emission = vec3(1.0);
    //mtl_metallic = 0.0;
    //mtl_normal = vec3(0.5, 0.5, 1.0);
    //mtl_occlusion = 1.0;
    //mtl_roughness = 0.5;

    // Convert sRGB textures to linear color space
    // https://learnopengl.com/Advanced-Lighting/Gamma-Correction
    mtl_albedo.rgb   = pow(mtl_albedo.rgb,   vec3(GAMMA));
    mtl_emission.rgb = pow(mtl_emission.rgb, vec3(GAMMA));

    // TODO: Why did I do this?
    //mtl_roughness = max(mtl_roughness, 0.001);

    vec3 N = normalize(mtl_normal * 2.0 - 1.0);

    const vec3 dielectricSpecular = vec3(0.04);
    vec3 F0 = mix(dielectricSpecular, mtl_albedo.rgb, mtl_metallic);

    float shadows[TA_LIGHTING_MAX_ACTIVE_LIGHTS];
    float shadow_map_depths[TA_LIGHTING_MAX_ACTIVE_LIGHTS];
    float shadow_dists[TA_LIGHTING_MAX_ACTIVE_LIGHTS];
    vec3 debug;

    vec3 L0 = vec3(0.0);
    for (int i = 0; i < u_lights_count; ++i) {
        vec3 fragToLight = vec3(0.0);
        float shadow_map_depth = 0.0;
        float shadow_bias = 0.0;
        float dist = 0.0;
        float attenuation = 0.0;
		float shadow = 0.0;

        switch (lights[i].type) {
            case LIGHT_DIRECTIONAL: {
                fragToLight = -lights[i].direction;

                vec3 projCoords = vertex.light_pvm[i].xyz / vertex.light_pvm[i].w;
                projCoords = projCoords * 0.5 + 0.5;
                debug = projCoords;
                dist = projCoords.z;
                attenuation = lights[i].intensity;

                if (lights[i].cast_shadows && projCoords.z <= 1.0) {
#if 0
                    shadow_bias = 0.0002;
                    shadow_map_depth = texture(u_textures[lights[i].shadowmap_texture_pool_index],
                        vec3(projCoords.st, lights[i].shadowmap_texture_array_layers[0])).r;
                    // TODO: Better bias based on direction of light? This code
                    // doesn't work, but tried to write it based on:
                    // https://learnopengl.com/Advanced-Lighting/Shadows/Shadow-Mapping
                    //shadow_bias = max(0.05 * (1.0 - dot(N, lights[i].direction)), 0.001);
		            shadow = step(shadow_map_depth, dist - shadow_bias);
		            shadow = smoothstep(0.01, 1.0, shadow);
#elif 0
                    //--------------------------------------------------
                    // PCF, regular sampling (banding)
                    //--------------------------------------------------
                    // TODO: Clean this crap up!
                    shadow_bias = 0.005;
                    float ss_count = 0.0;
				    for (float x = -1.0; x <= 1.0; x += 1.0) {
					    for (float y = -1.0; y <= 1.0; y += 1.0) {
							vec2 ss_offset = vec2(x, y) * 0.0002;
                            float ss_depth = texture(u_textures[lights[i].shadowmap_texture_pool_index],
                                vec3(projCoords.st + ss_offset, lights[i].shadowmap_texture_array_layers[0])).r;
			                shadow += step(ss_depth, dist - shadow_bias);
                            ss_count += 1.0;
					    }
				    }
                    shadow /= ss_count;
		            shadow = smoothstep(0.01, 1.0, shadow);
#elif 0
                    //--------------------------------------------------
                    // PCF, irregular sampling w/ Poisson disk (noise)
                    //--------------------------------------------------
                    shadow_bias = 0.005;
                    float filterRadiusUV = 0.0005;  // larger radius -> softer shadows
                    float pcf_sum = 0.0;
                    for (int diskIdx = 0; diskIdx < PCF_NUM_SAMPLES; ++diskIdx) {
                        vec2 ss_offset = poissonDisk[diskIdx] * filterRadiusUV;
                        float ss_depth = texture(u_textures[lights[i].shadowmap_texture_pool_index],
                                vec3(projCoords.st + ss_offset, lights[i].shadowmap_texture_array_layers[0])).r;
			            //pcf_sum += step(ss_depth, dist - shadow_bias);
                        pcf_sum += float(ss_depth < dist - shadow_bias);
                        //pcf_sum += ss_depth > dist ? 0.0 : 1.0; // - shadow_bias);
                    }
                    pcf_sum = pcf_sum / PCF_NUM_SAMPLES;
		            pcf_sum = smoothstep(0.01, 1.0, pcf_sum);
                    shadow = pcf_sum;
#else
                    //--------------------------------------------------
                    // PCSS
                    //--------------------------------------------------
                    shadow = PCSS(
                        lights[i].shadowmap_texture_pool_index,
                        lights[i].shadowmap_texture_array_layers[0],
                        vec3(projCoords.st, dist)
                    );
#endif
                }

                fragToLight = -vertex.tbn_light_dir[i];
                break;
            } case LIGHT_POINT: {
                fragToLight = lights[i].position - vertex.position;

                dist = length(fragToLight);
                attenuation = lights[i].intensity / dist * dist;
                // Alternative attenutation for finer control:
                // https://learnopengl.com/Lighting/Light-casters
                //attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

                if (lights[i].cast_shadows) {
                    shadow_bias = 0.05;
#if 0
                    vec3 sample_uvw = -fragToLight;
                    vec3 uv_face = convert_xyz_to_cube_uv(sample_uvw);
                    vec2 sample_uv = uv_face.xy;
                    int sample_face = int(uv_face.z);

                    shadow_map_depth = texture(u_textures[lights[i].shadowmap_texture_pool_index],
                        vec3(sample_uv, lights[i].shadowmap_texture_array_layers[sample_face])).r;
                    shadow_map_depth *= lights[i].shadowmap_zfar;
		            shadow = step(shadow_map_depth, dist - shadow_bias);
#else
                    // Soft shadows
                    // TODO: Clean this crap up!
                    float ss_count = 0.0;
				    for (float x = -1.0; x <= 1.0; x += 1.0) {
					    for (float y = -1.0; y <= 1.0; y += 1.0) {
					        for (float z = -1.0; z <= 1.0; z += 1.0) {

                                /////////////////////////////////////////////////////////////////////////////////////
                                // TODO: Need to convert 3D sample direction to UV coords and face index
							    vec3 ss_offset = vec3(x, y, z) * 0.04;
                                vec3 sample_uvw = -fragToLight + ss_offset;

                                vec3 uv_face = convert_xyz_to_cube_uv(sample_uvw);
                                vec2 sample_uv = uv_face.xy;
                                int sample_face = int(uv_face.z);

                                float ss_depth = texture(u_textures[lights[i].shadowmap_texture_pool_index],
                                    vec3(sample_uv, lights[i].shadowmap_texture_array_layers[sample_face])).r;
                                /////////////////////////////////////////////////////////////////////////////////////

							    ss_depth *= lights[i].shadowmap_zfar;
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

        vec3 radiance = lights[i].color * attenuation;

        vec3 L = normalize(fragToLight);
        vec3 H = normalize(V + L);

        float D = DistributionGGX(N, H, mtl_roughness);
        vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);
        float G = GeometrySmith(N, V, L, mtl_roughness);

        vec3 kS = F;
        vec3 kD = (vec3(1.0) - kS) * (1.0 - mtl_metallic);

        vec3 numer = D * F * G;
        float NdotL = max(dot(N, L), 0.0);
        float denom = 4.0 * max(dot(N, V), 0.0) * NdotL;
        vec3 specular = numer / max(denom, 0.001);

        L0 += (kD * mtl_albedo.rgb / PI + specular) * radiance * NdotL * (1.0 - shadow);
    }

    // TODO: What is this? Hard-coded ambient, or...?
    //mtl_albedo.rgb = mix(mtl_albedo.rgb, vec3(0.18, 0.28, 0.35), 0.5);
    vec3 ambient = 0.01 * mtl_albedo.rgb * mtl_occlusion;
    vec3 color = ambient + L0 + mtl_emission.rgb;

    // Tone mapping (Reinhard operator)
    color /= color + vec3(1.0);
    // Gamma correction
    color = pow(color, vec3(1.0 / 2.2));

#if 0
    // Distance-based "fog" (not good.. random hacks heh)
    float cam_dist = length(vertex.tbn_camera_pos - vertex.tbn_position);
    color = mix(color, vec3(0.7), clamp((cam_dist - 10.0)/50.0, 0.0, 0.9));
#endif

#if 0
    if (u_selected) {
        // TODO: Time-based pulse
    }
#endif

    final_color = vec4(color, mtl_albedo.a);

    //-------------------------------------------------------------------
    // debug channels
    //-------------------------------------------------------------------

    // vertex properties
    vec4 dbg_vtx_color = vertex.color;
    vec4 dbg_vtx_uv = vec4(vertex.uv.x, vertex.uv.y, 0.0, 1.0);
    vec4 dbg_vtx_normal = vec4((vertex.normal * 0.5) + 0.5, 1.0);
    vec4 dbg_vtx_tangent = vec4((vertex.tangent * 0.5) + 0.5, 1.0);
    vec4 dbg_vtx_tbn_normal = vec4((vertex.tbn_normal * 0.5) + 0.5, 1.0);
    vec4 dbg_normal_map = vec4((N * 0.5) + 0.5, 1.0);

    // material properties
    vec4 dbg_mtl_albedo = vec4(mtl_albedo);
    vec4 dbg_mtl_emission = vec4(vec3(mtl_emission), 1.0);
    vec4 dbg_mtl_metallic = vec4(vec3(mtl_metallic), 1.0);
    vec4 dbg_mtl_roughness = vec4(vec3(mtl_roughness), 1.0);
    vec4 dbg_mtl_occlusion = vec4(vec3(mtl_occlusion), 1.0);

     // lighting
    //int light_idx = 0;
    //final_color = vec4(vertex.tbn_light_dir[light_idx], 1.0);
    //final_color = vec4(vec3(1.0 - (shadow_map_depths[light_idx] / 30.0f)), 1.0);
    //final_color = vec4(vec3(1.0 - shadow_dists[light_idx] / 40.0f), 1.0);

    //final_color = vec4(debug, 1.0);
    //final_color = vec4(vec3(debug.z), 1.0);

    // bone weights
    //final_color = vec4(vertex.bone_weights.xyz, 1.0);

    // bone counts
    //int bone_count = int(vertex.bone_weights.x > 0.0) +
    //                 int(vertex.bone_weights.y > 0.0) +
    //                 int(vertex.bone_weights.z > 0.0) +
    //                 int(vertex.bone_weights.w > 0.0);
    //vec3 bone_count_thing = vec3(bone_count * 0.2);
    //final_color = vec4(bone_count_thing, 1.0);

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
        case DBG_MTL_EMISSION:
            final_color = dbg_mtl_emission;
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
        case DBG_SHADOW_0:
            final_color = vec4(vec3(1.0 - shadows[0]), 1.0);
            break;
        case DBG_SHADOW_1:
            final_color = vec4(vec3(1.0 - shadows[1]), 1.0);
            break;
        case DBG_SHADOW_2:
            final_color = vec4(vec3(1.0 - shadows[2]), 1.0);
            break;
        case DBG_SHADOW_3:
            final_color = vec4(vec3(1.0 - shadows[3]), 1.0);
            break;
    };
}

// Microfacet Distribution (D):
// Trowbridge-Reitz
//
// Implementation of microfacet distrubtion from "Average Irregularity Representation of a Roughened Surface for
// Ray Reflection" by T. S. Trowbridge, and K. P. Reit
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

// Geometric Occlusion (G):
// This implementation is from learnopengl.com. I'm not sure where it originated.
// "a combination of the GGX and Schlick-Beckmann approximation known as Schlick-GGX"
//
// NOTE: The glTF spec recommends "Smith Joint GGX" as described in "Understanding the Masking-Shadowing Function in
// Microfacet-Based BRDFs" by Eric Heitz, but it has multiple sqrt operations. Going to stick with this version for now
// without understanding the differences. Should revisit and do a proper evaluation.
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

// Surface Reflection Ratio (F):
// Fresnel Schlick
//
// Simplified implementation of Fresnel from "An Inexpensive BRDF Model for Physically based Rendering"
// by Christophe Schlick.
vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

// https://en.wikipedia.org/wiki/Cube_mapping#Memory_addressing
// returns u, v, face as vec
vec3 convert_xyz_to_cube_uv(vec3 uvw)
{
    vec3 uv_face;

    float absX = abs(uvw.x);
    float absY = abs(uvw.y);
    float absZ = abs(uvw.z);

    bool isXPositive = uvw.x > 0;
    bool isYPositive = uvw.y > 0;
    bool isZPositive = uvw.z > 0;

    float maxAxis, uc, vc;
    int index = 0;

    // POSITIVE X
    if (isXPositive && absX >= absY && absX >= absZ) {
        // u (0 to 1) goes from +z to -z
        // v (0 to 1) goes from -y to +y
        maxAxis = absX;
        uc = -uvw.z;
        vc = uvw.y;
        index = 0;
    }
    // NEGATIVE X
    if (!isXPositive && absX >= absY && absX >= absZ) {
        // u (0 to 1) goes from -z to +z
        // v (0 to 1) goes from -y to +y
        maxAxis = absX;
        uc = uvw.z;
        vc = uvw.y;
        index = 1;
    }
    // POSITIVE Y
    if (isYPositive && absY >= absX && absY >= absZ) {
        // u (0 to 1) goes from -x to +x
        // v (0 to 1) goes from +z to -z
        maxAxis = absY;
        uc = uvw.x;
        vc = -uvw.z;
        index = 2;
    }
    // NEGATIVE Y
    if (!isYPositive && absY >= absX && absY >= absZ) {
        // u (0 to 1) goes from -x to +x
        // v (0 to 1) goes from -z to +z
        maxAxis = absY;
        uc = uvw.x;
        vc = uvw.z;
        index = 3;
    }
    // POSITIVE Z
    if (isZPositive && absZ >= absX && absZ >= absY) {
        // u (0 to 1) goes from -x to +x
        // v (0 to 1) goes from -y to +y
        maxAxis = absZ;
        uc = uvw.x;
        vc = uvw.y;
        index = 4;
    }
    // NEGATIVE Z
    if (!isZPositive && absZ >= absX && absZ >= absY) {
        // u (0 to 1) goes from +x to -x
        // v (0 to 1) goes from -y to +y
        maxAxis = absZ;
        uc = -uvw.x;
        vc = uvw.y;
        index = 5;
    }

    // Convert range from -1 to 1 to 0 to 1
    uv_face.x = 0.5 * (uc / maxAxis + 1.0);
    uv_face.y = 0.5 * (vc / maxAxis + 1.0);
    uv_face.z = index;

    // NOTE: Our UV coords are upside down
    uv_face.y = 1.0 - uv_face.y;

    return uv_face;
}