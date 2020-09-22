#version 330 core

in vs_out {
	vec3 uvw;
} vertex;

//------------------------------------------------------
// Textures
//------------------------------------------------------
// NOTE: Needs to match the #define in ta_texture.h
#define TA_TEXTURE_POOL_MAX 16
uniform sampler2DArray u_textures[TA_TEXTURE_POOL_MAX];

uniform uint u_texture_pool_index;
uniform uint u_texture_array_layers[6];

out vec4 final_color;

vec3 convert_xyz_to_cube_uv(vec3 uvw);

void main()
{
    vec3 uv_face = convert_xyz_to_cube_uv(vertex.uvw);
    vec2 sample_uv = uv_face.xy;
    int sample_face = int(uv_face.z);
#if 0
    switch (sample_face) {
        case 0:  // +X (red)
            final_color = vec4(1.0, 0.0, 0.0, 1.0);
            break;
        case 1:  // -X (dark red)
            final_color = vec4(0.35, 0.0, 0.0, 1.0);
            break;
        case 2:  // +Y (green)
            final_color = vec4(0.0, 1.0, 0.0, 1.0);
            break;
        case 3:  // -Y (dark green)
            final_color = vec4(0.0, 0.35, 0.0, 1.0);
            break;
        case 4:  // +Z (blue)
            final_color = vec4(0.0, 0.0, 1.0, 1.0);
            break;
        case 5:  // -Z (dark blue)
            final_color = vec4(0.0, 0.0, 0.35, 1.0);
            break;
    }
#else
    final_color = texture(u_textures[u_texture_pool_index], vec3(sample_uv, u_texture_array_layers[sample_face]));
#endif
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
        uc = uvw.z;
        vc = uvw.y;
        index = 0;
    }
    // NEGATIVE X
    if (!isXPositive && absX >= absY && absX >= absZ) {
        // u (0 to 1) goes from -z to +z
        // v (0 to 1) goes from -y to +y
        maxAxis = absX;
        uc = -uvw.z;
        vc = uvw.y;
        index = 1;
    }
    // POSITIVE Y
    if (isYPositive && absY >= absX && absY >= absZ) {
        // u (0 to 1) goes from -x to +x
        // v (0 to 1) goes from +z to -z
        maxAxis = absY;
        uc = uvw.x;
        vc = uvw.z;
        index = 2;
    }
    // NEGATIVE Y
    if (!isYPositive && absY >= absX && absY >= absZ) {
        // u (0 to 1) goes from -x to +x
        // v (0 to 1) goes from -z to +z
        maxAxis = absY;
        uc = uvw.x;
        vc = -uvw.z;
        index = 3;
    }
    // POSITIVE Z
    if (isZPositive && absZ >= absX && absZ >= absY) {
        // u (0 to 1) goes from -x to +x
        // v (0 to 1) goes from -y to +y
        maxAxis = absZ;
        uc = -uvw.x;
        vc = uvw.y;
        index = 4;
    }
    // NEGATIVE Z
    if (!isZPositive && absZ >= absX && absZ >= absY) {
        // u (0 to 1) goes from +x to -x
        // v (0 to 1) goes from -y to +y
        maxAxis = absZ;
        uc = uvw.x;
        vc = uvw.y;
        index = 5;
    }

    // Convert range from -1 to 1 to 0 to 1
    uv_face.x = 0.5 * (uc / maxAxis + 1.0);
    uv_face.y = 0.5 * (vc / maxAxis + 1.0);
    uv_face.z = index;

    return uv_face;
}