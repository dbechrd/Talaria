#version 330 core

//------------------------------------------------------
// Vertex attributes
//------------------------------------------------------
layout(location = 2) in vec3 attr_position;
layout(location = 5) in vec3 attr_morph1_position;
layout(location = 8) in vec4 attr_bone_indices;  // up to 4 bone indices that influence this vertex; packed
layout(location = 9) in vec4 attr_bone_weights;  // TODO: respective weights for each of the influencing bones

uniform mat4 u_light_pvm;

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

void main() {
    //----------------------------------
    // Morph target blending
    //----------------------------------
    vec3 morphed_position = attr_position;// + u_morph_weights[0] * attr_morph1_position;

    //----------------------------------
    // Skinning
    //----------------------------------
    vec4 position_skinned = (attr_bone_weights.x * vec4(morphed_position, 1.0) * bone_xforms[int(attr_bone_indices.x)]) +
                            (attr_bone_weights.y * vec4(morphed_position, 1.0) * bone_xforms[int(attr_bone_indices.y)]) +
                            (attr_bone_weights.z * vec4(morphed_position, 1.0) * bone_xforms[int(attr_bone_indices.z)]) +
                            (attr_bone_weights.w * vec4(morphed_position, 1.0) * bone_xforms[int(attr_bone_indices.w)]) +
                            (float(attr_bone_weights.x == 0.0) * vec4(morphed_position, 1.0));

    gl_Position = u_light_pvm * position_skinned;
}