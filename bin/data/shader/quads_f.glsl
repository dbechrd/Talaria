#version 330 core

in vs_out {
    vec4 color;
	vec2 uv;
    //vec3 position;
} vertex;

//------------------------------------------------------
// Textures
//------------------------------------------------------
// NOTE: Needs to match the #define in ta_texture.h
#define TA_TEXTURE_POOL_MAX 8
uniform sampler2DArray u_textures[TA_TEXTURE_POOL_MAX];

uniform uint u_texture_pool_index;
uniform uint u_texture_array_layer;

out vec4 final_color;

void main()
{
	vec4 tex_color = texture(u_textures[u_texture_pool_index], vec3(vertex.uv, u_texture_array_layer));
    // Need to set alpha to 1.0 for textures with only red channel. We do this by assuming RGB/RGBA textures with
    // alpha zero pixels have their RGB values set to black.
    if (tex_color.rgb != vec3(0.0) && tex_color.a == 0.0) {
        tex_color.a = 1.0;
    }
    if (vertex.color.a > 0) {
        final_color = vertex.color;
    } else {
        final_color = tex_color;
    }
}
