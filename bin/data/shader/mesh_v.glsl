#version 330 core

//------------------------------------------------------
// Vertex attributes
//------------------------------------------------------
layout(location = 0) in vec4 attr_color;
layout(location = 1) in vec2 attr_uv;
layout(location = 2) in vec3 attr_position;
layout(location = 3) in vec3 attr_normal;
layout(location = 4) in vec3 attr_tangent;
layout(location = 5) in vec3 attr_morph1_position;
layout(location = 6) in vec3 attr_morph1_normal;
layout(location = 7) in vec3 attr_morph1_tangent;
layout(location = 8) in vec4 attr_bone_indices;  // up to 4 bone indices that influence this vertex; packed
layout(location = 9) in vec4 attr_bone_weights;  // TODO: respective weights for each of the influencing bones

//------------------------------------------------------
// Camera, model, animation
//------------------------------------------------------
// TODO: Uniform block(s) grouped by update frequency
// https://learnopengl.com/Advanced-OpenGL/Advanced-GLSL
uniform mat4 u_proj;
uniform mat4 u_view;
uniform vec3 u_camera_pos;

uniform mat4 u_model;

//------------------------------------------------------
// Animations
//------------------------------------------------------
#define TA_MODEL_MAX_MORPHS 1

uniform float u_morph_weights[TA_MODEL_MAX_MORPHS];

#define TA_SKIN_MAX_BONES 64

// NOTE: Max array size is determined by GL_MAX_VERTEX_UNIFORM_COMPONENTS / 4
// TODO: Precalculated vertex pose matrix for every bone (w = 1)
//       M B^-1 T P
//       curent_bone_transform * bone_bind_pose_transform_inv * skin_bind_pose_transform * vec4(attr_position, 1.0)
layout (std140) uniform ubo_bone_xforms {
    mat4 bone_xforms[TA_SKIN_MAX_BONES];
};

// TODO: Precalculated normal pose matrix for every bone (w = 0)
//       N T^-1 B M^-1
//       skin_bind_pose_transform_inv * bone_bind_pose_transform * curent_bone_transform_inv
layout (std140) uniform ubo_bone_normal_xforms {
    mat4 bone_normal_xforms[TA_SKIN_MAX_BONES];
};

//------------------------------------------------------
// Lights
//------------------------------------------------------
#define TA_LIGHT_MAX_ACTIVE_LIGHTS 4

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
    UboLight lights[TA_LIGHT_MAX_ACTIVE_LIGHTS];
};
uniform uint u_lights_count;
//------------------------------------------------------

out vs_out {
	vec2 uv;
    vec3 position;
    vec3 tbn_position;
    vec3 tbn_camera_pos;
    vec3 tbn_light_pos[TA_LIGHT_MAX_ACTIVE_LIGHTS];
    vec3 tbn_light_dir[TA_LIGHT_MAX_ACTIVE_LIGHTS];
    vec4 light_pvm[TA_LIGHT_MAX_ACTIVE_LIGHTS];

    // NOTE: These are just passed to allow debug channels to display them
    // TODO: Use a separate shader for each debug channel? Would simplify this shader and reduce interface block size
    vec4 color;
    vec3 normal;
    vec3 tangent;
	vec3 tbn_normal;

    // TODO: Delete these.. we can't afford the space!
    vec4 bone_indices;
    vec4 bone_weights;
} vertex;

void main()
{
#if 0
    //----------------------------------
    // Morph target blending
    //----------------------------------
    // http://antongerdelan.net/opengl/blend_shapes.html
    // if other weights add up to less than 1, use neutral target
    float neutral_w = 1.0 - u_morph_weights[0];  // - u_morph_weights[1]
    clamp(neutral_w, 0.0, 1.0);

    // get a sum of weights and work out factors for each target
    float sum_w = neutral_w + u_morph_weights[0];
    float neutral_f = neutral_w / sum_w;
    float morph1_f = u_morph_weights[0] / sum_w;

    // interpolate targets to give us current pose
    vec4 position_morphed = vec4(
        neutral_f * attr_position +
        morph1_f * attr_morph1_position
    , 1.0);

    vec4 normal_morphed = vec4(
        neutral_f * attr_normal +
        morph1_f * attr_morph1_normal
     , 0.0);
#else
    vec4 position_morphed = vec4(attr_position, 1.0);
    vec4 normal_morphed   = vec4(attr_normal  , 0.0);
#endif

#if 1
    // TODO: Should this happen before or after morph target blending? Should we even mix those?
    //----------------------------------
    // Skinning
    //----------------------------------
    vec4 position_skinned = (attr_bone_weights.x * position_morphed * bone_xforms[int(attr_bone_indices.x)]) +
                            (attr_bone_weights.y * position_morphed * bone_xforms[int(attr_bone_indices.y)]) +
                            (attr_bone_weights.z * position_morphed * bone_xforms[int(attr_bone_indices.z)]) +
                            (attr_bone_weights.w * position_morphed * bone_xforms[int(attr_bone_indices.w)]) +
                            (float(attr_bone_weights.x == 0.0) * position_morphed);

    vec4 normal_skinned = (attr_bone_weights[0] * normal_morphed * bone_normal_xforms[int(attr_bone_indices.x)]) +
                          (attr_bone_weights[1] * normal_morphed * bone_normal_xforms[int(attr_bone_indices.y)]) +
                          (attr_bone_weights[2] * normal_morphed * bone_normal_xforms[int(attr_bone_indices.z)]) +
                          (attr_bone_weights[3] * normal_morphed * bone_normal_xforms[int(attr_bone_indices.w)]) +
                          (float(attr_bone_weights.x == 0.0) * normal_morphed);
#else
    vec4 position_skinned = position_morphed;
    vec4 normal_skinned = normal_morphed;
#endif

    vec4 position = u_model * position_skinned;
    vec4 normal = u_model * normal_skinned;

    vertex.position = vec3(position);
	vertex.color = attr_color;
	vertex.uv = attr_uv;
    vertex.normal = normalize(vec3(normal));
    vertex.tangent = attr_tangent;

    // TODO: Calculate model inverse on CPU side
    mat3 normal_matrix = transpose(inverse(mat3(u_model)));
    vec3 T = normalize(normal_matrix * attr_tangent);
    vec3 N = normalize(normal_matrix * attr_normal);
    T = normalize(T - dot(T, N) * N);  // re-orthogonalize T with respect to N
    vec3 B = cross(N, T);
    mat3 TBN = transpose(mat3(T, B, N));

    vertex.tbn_normal = TBN * vertex.normal;
    vertex.tbn_position = TBN * vertex.position;
    vertex.tbn_camera_pos = TBN * u_camera_pos;

    for (uint i = uint(0); i < u_lights_count; i++) {
        vertex.tbn_light_pos[i] = TBN * lights[i].position;
        vertex.tbn_light_dir[i] = TBN * lights[i].direction;
        vertex.light_pvm[i] = lights[i].light_pv * position;
    }

    vertex.bone_indices = attr_bone_indices;
    vertex.bone_weights = attr_bone_weights;

    gl_Position = u_proj * u_view * position;
}
