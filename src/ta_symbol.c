#include "ta_symbol.h"
#include "ta_parse.h"
#include "dlb/dlb_types.h"
#include "dlb/dlb_arena.h"
#include "dlb/dlb_hash.h"

#define TA_SYMBOL_MAX_LEN 256

// GLSL types
const char *SYM_GLINT;
const char *SYM_GLUINT;
const char *SYM_SAMPLER2D;
const char *SYM_VEC2;
const char *SYM_VEC3;
const char *SYM_VEC4;
const char *SYM_MAT3;
const char *SYM_MAT4;
const char *SYM_STRUCT;

// Shader attributes
const char *SYM_ATTR_POSITION;
const char *SYM_ATTR_COLOR;
const char *SYM_ATTR_UV;
const char *SYM_ATTR_NORMAL;
const char *SYM_ATTR_TANGENT;
const char *SYM_ATTR_MORPH0_POSITION;
const char *SYM_ATTR_MORPH0_COLOR;
const char *SYM_ATTR_MORPH0_UV;
const char *SYM_ATTR_MORPH0_NORMAL;
const char *SYM_ATTR_MORPH0_TANGENT;
const char *SYM_ATTR_JOINTS;
const char *SYM_ATTR_WEIGHTS;

// Shader uniforms
const char *SYM_U_CAMERA_POS;
const char *SYM_U_COLOR;
const char *SYM_U_DEBUG_CHANNEL;
const char *SYM_U_FACE;
const char *SYM_U_LIGHT_POS;
const char *SYM_U_LIGHT_PVM;
const char *SYM_U_LIGHT_ZFAR;
const char *SYM_U_LIGHTS;
const char *SYM_U_LIGHTS_CAST_SHADOWS[8];
const char *SYM_U_LIGHTS_COLOR[8];
const char *SYM_U_LIGHTS_COUNT;
const char *SYM_U_LIGHTS_DIRECTION[8];
const char *SYM_U_LIGHTS_INTENSITY[8];
const char *SYM_U_LIGHTS_LIGHT_PV[8];
const char *SYM_U_LIGHTS_POSITION[8];
const char *SYM_U_LIGHTS_SHADOWMAP_ZFAR[8];
const char *SYM_U_LIGHTS_SHADOWMAP_TEXTURE_POOL_INDEX[8];
const char *SYM_U_LIGHTS_SHADOWMAP_TEXTURE_ARRAY_LAYERS[8];
const char *SYM_U_LIGHTS_TYPE[8];
const char *SYM_U_MATERIAL;
const char *SYM_U_MATERIAL_ALBEDO_FACTOR;
const char *SYM_U_MATERIAL_ALBEDO_TEXTURE_POOL_INDEX;
const char *SYM_U_MATERIAL_ALBEDO_TEXTURE_POOL_LAYER;
const char *SYM_U_MATERIAL_EMISSION_FACTOR;
const char *SYM_U_MATERIAL_EMISSION_TEXTURE_POOL_INDEX;
const char *SYM_U_MATERIAL_EMISSION_TEXTURE_POOL_LAYER;
const char *SYM_U_MATERIAL_HEIGHT_FACTOR;
const char *SYM_U_MATERIAL_HEIGHT_TEXTURE_POOL_INDEX;
const char *SYM_U_MATERIAL_HEIGHT_TEXTURE_POOL_LAYER;
const char *SYM_U_MATERIAL_METALLIC_FACTOR;
const char *SYM_U_MATERIAL_METALLIC_TEXTURE_POOL_INDEX;
const char *SYM_U_MATERIAL_METALLIC_TEXTURE_POOL_LAYER;
const char *SYM_U_MATERIAL_NORMAL_TEXTURE_POOL_INDEX;
const char *SYM_U_MATERIAL_NORMAL_TEXTURE_POOL_LAYER;
const char *SYM_U_MATERIAL_OCCLUSION_TEXTURE_POOL_INDEX;
const char *SYM_U_MATERIAL_OCCLUSION_TEXTURE_POOL_LAYER;
const char *SYM_U_MATERIAL_ROUGHNESS_FACTOR;
const char *SYM_U_MATERIAL_ROUGHNESS_TEXTURE_POOL_INDEX;
const char *SYM_U_MATERIAL_ROUGHNESS_TEXTURE_POOL_LAYER;
const char *SYM_U_MATERIAL_SLOT;
const char *SYM_U_MODEL;
const char *SYM_U_MORPH_WEIGHTS[1];
const char *SYM_U_PROJ;
const char *SYM_U_SELECTED;
const char *SYM_U_TEX;
const char *SYM_U_TEXTURES[8];
const char *SYM_U_TEXTURE_POOL_INDEX;
const char *SYM_U_TEXTURE_ARRAY_LAYER;
const char *SYM_U_TEXTURE_ARRAY_LAYERS;
const char *SYM_U_VIEW;

// Constants
const char *SYM_ENTITY_PLAYER_ONE;
const char *SYM_ENTITY_PLAYER_CAMERA;
const char *SYM_ENTITY_FREECAM;
const char *SYM_ENTITY_BACKGROUND_MUSIC;
const char *SYM_SHADER_EDITOR_SELECT;

// TODO: It may be useful to have multiple symbol tables to allow freeing
//       symbols that are no longer in use (e.g. table per scene file). This
//       hasn't been necessary yet, so I'm not going to do it preemptively.
static dlb_hash symbol_table;

void ta_symbol_init() {
    dlb_hash_init(&symbol_table, DLB_HASH_STRING, "[symbol_table]", 1024);

    SYM_GLINT     = INTERN("glint");
    SYM_GLUINT    = INTERN("gluint");
    SYM_SAMPLER2D = INTERN("sampler2D");
    SYM_VEC2      = INTERN("vec2");
    SYM_VEC3      = INTERN("vec3");
    SYM_VEC4      = INTERN("vec4");
    SYM_MAT4      = INTERN("mat3");
    SYM_MAT4      = INTERN("mat4");
    SYM_STRUCT    = INTERN("struct");

    SYM_ATTR_POSITION        = INTERN("attr_position");
    SYM_ATTR_COLOR           = INTERN("attr_color");
    SYM_ATTR_UV              = INTERN("attr_uv");
    SYM_ATTR_NORMAL          = INTERN("attr_normal");
    SYM_ATTR_TANGENT         = INTERN("attr_tangent");
    SYM_ATTR_MORPH0_POSITION = INTERN("attr_morph0_position");
    SYM_ATTR_MORPH0_COLOR    = INTERN("attr_morph0_color");
    SYM_ATTR_MORPH0_UV       = INTERN("attr_morph0_uv");
    SYM_ATTR_MORPH0_NORMAL   = INTERN("attr_morph0_normal");
    SYM_ATTR_MORPH0_TANGENT  = INTERN("attr_morph0_tangent");
    SYM_ATTR_JOINTS          = INTERN("attr_joints");
    SYM_ATTR_WEIGHTS         = INTERN("attr_weights");

    SYM_U_CAMERA_POS               = INTERN("u_camera_pos");
    SYM_U_COLOR                    = INTERN("u_color");
    SYM_U_DEBUG_CHANNEL            = INTERN("u_debug_channel");
    SYM_U_FACE                     = INTERN("u_face");
    SYM_U_LIGHT_POS                = INTERN("u_light_pos");
    SYM_U_LIGHT_PVM                = INTERN("u_light_pvm");
    SYM_U_LIGHT_ZFAR               = INTERN("u_light_zfar");
    SYM_U_LIGHTS                   = INTERN("u_lights");
    SYM_U_LIGHTS_CAST_SHADOWS[0]   = INTERN("u_lights[0].cast_shadows");
    SYM_U_LIGHTS_CAST_SHADOWS[1]   = INTERN("u_lights[1].cast_shadows");
    SYM_U_LIGHTS_CAST_SHADOWS[2]   = INTERN("u_lights[2].cast_shadows");
    SYM_U_LIGHTS_CAST_SHADOWS[3]   = INTERN("u_lights[3].cast_shadows");
    SYM_U_LIGHTS_CAST_SHADOWS[4]   = INTERN("u_lights[4].cast_shadows");
    SYM_U_LIGHTS_CAST_SHADOWS[5]   = INTERN("u_lights[5].cast_shadows");
    SYM_U_LIGHTS_CAST_SHADOWS[6]   = INTERN("u_lights[6].cast_shadows");
    SYM_U_LIGHTS_CAST_SHADOWS[7]   = INTERN("u_lights[7].cast_shadows");
    SYM_U_LIGHTS_COLOR[0]          = INTERN("u_lights[0].color");
    SYM_U_LIGHTS_COLOR[1]          = INTERN("u_lights[1].color");
    SYM_U_LIGHTS_COLOR[2]          = INTERN("u_lights[2].color");
    SYM_U_LIGHTS_COLOR[3]          = INTERN("u_lights[3].color");
    SYM_U_LIGHTS_COLOR[4]          = INTERN("u_lights[4].color");
    SYM_U_LIGHTS_COLOR[5]          = INTERN("u_lights[5].color");
    SYM_U_LIGHTS_COLOR[6]          = INTERN("u_lights[6].color");
    SYM_U_LIGHTS_COLOR[7]          = INTERN("u_lights[7].color");
    SYM_U_LIGHTS_COUNT             = INTERN("u_lights_count");
    SYM_U_LIGHTS_DIRECTION[0]      = INTERN("u_lights[0].direction");
    SYM_U_LIGHTS_DIRECTION[1]      = INTERN("u_lights[1].direction");
    SYM_U_LIGHTS_DIRECTION[2]      = INTERN("u_lights[2].direction");
    SYM_U_LIGHTS_DIRECTION[3]      = INTERN("u_lights[3].direction");
    SYM_U_LIGHTS_DIRECTION[4]      = INTERN("u_lights[4].direction");
    SYM_U_LIGHTS_DIRECTION[5]      = INTERN("u_lights[5].direction");
    SYM_U_LIGHTS_DIRECTION[6]      = INTERN("u_lights[6].direction");
    SYM_U_LIGHTS_DIRECTION[7]      = INTERN("u_lights[7].direction");
    SYM_U_LIGHTS_INTENSITY[0]      = INTERN("u_lights[0].intensity");
    SYM_U_LIGHTS_INTENSITY[1]      = INTERN("u_lights[1].intensity");
    SYM_U_LIGHTS_INTENSITY[2]      = INTERN("u_lights[2].intensity");
    SYM_U_LIGHTS_INTENSITY[3]      = INTERN("u_lights[3].intensity");
    SYM_U_LIGHTS_INTENSITY[4]      = INTERN("u_lights[4].intensity");
    SYM_U_LIGHTS_INTENSITY[5]      = INTERN("u_lights[5].intensity");
    SYM_U_LIGHTS_INTENSITY[6]      = INTERN("u_lights[6].intensity");
    SYM_U_LIGHTS_INTENSITY[7]      = INTERN("u_lights[7].intensity");
    SYM_U_LIGHTS_LIGHT_PV[0]       = INTERN("u_lights[0].light_pv");
    SYM_U_LIGHTS_LIGHT_PV[1]       = INTERN("u_lights[1].light_pv");
    SYM_U_LIGHTS_LIGHT_PV[2]       = INTERN("u_lights[2].light_pv");
    SYM_U_LIGHTS_LIGHT_PV[3]       = INTERN("u_lights[3].light_pv");
    SYM_U_LIGHTS_LIGHT_PV[4]       = INTERN("u_lights[4].light_pv");
    SYM_U_LIGHTS_LIGHT_PV[5]       = INTERN("u_lights[5].light_pv");
    SYM_U_LIGHTS_LIGHT_PV[6]       = INTERN("u_lights[6].light_pv");
    SYM_U_LIGHTS_LIGHT_PV[7]       = INTERN("u_lights[7].light_pv");
    SYM_U_LIGHTS_POSITION[0]       = INTERN("u_lights[0].position");
    SYM_U_LIGHTS_POSITION[1]       = INTERN("u_lights[1].position");
    SYM_U_LIGHTS_POSITION[2]       = INTERN("u_lights[2].position");
    SYM_U_LIGHTS_POSITION[3]       = INTERN("u_lights[3].position");
    SYM_U_LIGHTS_POSITION[4]       = INTERN("u_lights[4].position");
    SYM_U_LIGHTS_POSITION[5]       = INTERN("u_lights[5].position");
    SYM_U_LIGHTS_POSITION[6]       = INTERN("u_lights[6].position");
    SYM_U_LIGHTS_POSITION[7]       = INTERN("u_lights[7].position");
    SYM_U_LIGHTS_SHADOWMAP_ZFAR[0] = INTERN("u_lights[0].shadowmap_zfar");
    SYM_U_LIGHTS_SHADOWMAP_ZFAR[1] = INTERN("u_lights[1].shadowmap_zfar");
    SYM_U_LIGHTS_SHADOWMAP_ZFAR[2] = INTERN("u_lights[2].shadowmap_zfar");
    SYM_U_LIGHTS_SHADOWMAP_ZFAR[3] = INTERN("u_lights[3].shadowmap_zfar");
    SYM_U_LIGHTS_SHADOWMAP_ZFAR[4] = INTERN("u_lights[4].shadowmap_zfar");
    SYM_U_LIGHTS_SHADOWMAP_ZFAR[5] = INTERN("u_lights[5].shadowmap_zfar");
    SYM_U_LIGHTS_SHADOWMAP_ZFAR[6] = INTERN("u_lights[6].shadowmap_zfar");
    SYM_U_LIGHTS_SHADOWMAP_ZFAR[7] = INTERN("u_lights[7].shadowmap_zfar");
    SYM_U_LIGHTS_SHADOWMAP_TEXTURE_POOL_INDEX[0]    = INTERN("u_lights[0].shadowmap_texture_pool_index");
    SYM_U_LIGHTS_SHADOWMAP_TEXTURE_POOL_INDEX[1]    = INTERN("u_lights[1].shadowmap_texture_pool_index");
    SYM_U_LIGHTS_SHADOWMAP_TEXTURE_POOL_INDEX[2]    = INTERN("u_lights[2].shadowmap_texture_pool_index");
    SYM_U_LIGHTS_SHADOWMAP_TEXTURE_POOL_INDEX[3]    = INTERN("u_lights[3].shadowmap_texture_pool_index");
    SYM_U_LIGHTS_SHADOWMAP_TEXTURE_POOL_INDEX[4]    = INTERN("u_lights[4].shadowmap_texture_pool_index");
    SYM_U_LIGHTS_SHADOWMAP_TEXTURE_POOL_INDEX[5]    = INTERN("u_lights[5].shadowmap_texture_pool_index");
    SYM_U_LIGHTS_SHADOWMAP_TEXTURE_POOL_INDEX[6]    = INTERN("u_lights[6].shadowmap_texture_pool_index");
    SYM_U_LIGHTS_SHADOWMAP_TEXTURE_POOL_INDEX[7]    = INTERN("u_lights[7].shadowmap_texture_pool_index");
    SYM_U_LIGHTS_SHADOWMAP_TEXTURE_ARRAY_LAYERS[0]  = INTERN("u_lights[0].shadowmap_texture_array_layers");
    SYM_U_LIGHTS_SHADOWMAP_TEXTURE_ARRAY_LAYERS[1]  = INTERN("u_lights[1].shadowmap_texture_array_layers");
    SYM_U_LIGHTS_SHADOWMAP_TEXTURE_ARRAY_LAYERS[2]  = INTERN("u_lights[2].shadowmap_texture_array_layers");
    SYM_U_LIGHTS_SHADOWMAP_TEXTURE_ARRAY_LAYERS[3]  = INTERN("u_lights[3].shadowmap_texture_array_layers");
    SYM_U_LIGHTS_SHADOWMAP_TEXTURE_ARRAY_LAYERS[4]  = INTERN("u_lights[4].shadowmap_texture_array_layers");
    SYM_U_LIGHTS_SHADOWMAP_TEXTURE_ARRAY_LAYERS[5]  = INTERN("u_lights[5].shadowmap_texture_array_layers");
    SYM_U_LIGHTS_SHADOWMAP_TEXTURE_ARRAY_LAYERS[6]  = INTERN("u_lights[6].shadowmap_texture_array_layers");
    SYM_U_LIGHTS_SHADOWMAP_TEXTURE_ARRAY_LAYERS[7]  = INTERN("u_lights[7].shadowmap_texture_array_layers");
    SYM_U_LIGHTS_TYPE[0]           = INTERN("u_lights[0].type");
    SYM_U_LIGHTS_TYPE[1]           = INTERN("u_lights[1].type");
    SYM_U_LIGHTS_TYPE[2]           = INTERN("u_lights[2].type");
    SYM_U_LIGHTS_TYPE[3]           = INTERN("u_lights[3].type");
    SYM_U_LIGHTS_TYPE[4]           = INTERN("u_lights[4].type");
    SYM_U_LIGHTS_TYPE[5]           = INTERN("u_lights[5].type");
    SYM_U_LIGHTS_TYPE[6]           = INTERN("u_lights[6].type");
    SYM_U_LIGHTS_TYPE[7]           = INTERN("u_lights[7].type");

    SYM_U_MATERIAL                              = INTERN("u_material");
    SYM_U_MATERIAL_ALBEDO_FACTOR                = INTERN("u_material.albedo_factor");
    SYM_U_MATERIAL_ALBEDO_TEXTURE_POOL_INDEX    = INTERN("u_material.albedo_texture_pool_index");
    SYM_U_MATERIAL_ALBEDO_TEXTURE_POOL_LAYER    = INTERN("u_material.albedo_texture_pool_layer");
    SYM_U_MATERIAL_EMISSION_FACTOR              = INTERN("u_material.emission_factor");
    SYM_U_MATERIAL_EMISSION_TEXTURE_POOL_INDEX  = INTERN("u_material.emission_texture_pool_index");
    SYM_U_MATERIAL_EMISSION_TEXTURE_POOL_LAYER  = INTERN("u_material.emission_texture_pool_layer");
    SYM_U_MATERIAL_HEIGHT_FACTOR                = INTERN("u_material.height_factor");
    SYM_U_MATERIAL_HEIGHT_TEXTURE_POOL_INDEX    = INTERN("u_material.height_texture_pool_index");
    SYM_U_MATERIAL_HEIGHT_TEXTURE_POOL_LAYER    = INTERN("u_material.height_texture_pool_layer");
    SYM_U_MATERIAL_METALLIC_FACTOR              = INTERN("u_material.metallic_factor");
    SYM_U_MATERIAL_METALLIC_TEXTURE_POOL_INDEX  = INTERN("u_material.metallic_texture_pool_index");
    SYM_U_MATERIAL_METALLIC_TEXTURE_POOL_LAYER  = INTERN("u_material.metallic_texture_pool_layer");
    SYM_U_MATERIAL_NORMAL_TEXTURE_POOL_INDEX    = INTERN("u_material.normal_texture_pool_index");
    SYM_U_MATERIAL_NORMAL_TEXTURE_POOL_LAYER    = INTERN("u_material.normal_texture_pool_layer");
    SYM_U_MATERIAL_OCCLUSION_TEXTURE_POOL_INDEX = INTERN("u_material.occlusion_texture_pool_index");
    SYM_U_MATERIAL_OCCLUSION_TEXTURE_POOL_LAYER = INTERN("u_material.occlusion_texture_pool_layer");
    SYM_U_MATERIAL_ROUGHNESS_FACTOR             = INTERN("u_material.roughness_factor");
    SYM_U_MATERIAL_ROUGHNESS_TEXTURE_POOL_INDEX = INTERN("u_material.roughness_texture_pool_index");
    SYM_U_MATERIAL_ROUGHNESS_TEXTURE_POOL_LAYER = INTERN("u_material.roughness_texture_pool_layer");
    SYM_U_MATERIAL_SLOT                         = INTERN("u_material_slot");

    SYM_U_TEXTURES[0] = INTERN("u_textures[0]");
    SYM_U_TEXTURES[1] = INTERN("u_textures[1]");
    SYM_U_TEXTURES[2] = INTERN("u_textures[2]");
    SYM_U_TEXTURES[3] = INTERN("u_textures[3]");
    SYM_U_TEXTURES[4] = INTERN("u_textures[4]");
    SYM_U_TEXTURES[5] = INTERN("u_textures[5]");
    SYM_U_TEXTURES[6] = INTERN("u_textures[6]");
    SYM_U_TEXTURES[7] = INTERN("u_textures[7]");
    SYM_U_TEXTURE_POOL_INDEX   = INTERN("u_texture_pool_index");
    SYM_U_TEXTURE_ARRAY_LAYER  = INTERN("u_texture_array_layer");
    SYM_U_TEXTURE_ARRAY_LAYERS = INTERN("u_texture_array_layers");

    SYM_U_MODEL            = INTERN("u_model");
    SYM_U_MORPH_WEIGHTS[0] = INTERN("u_morph_weights[0]");
    SYM_U_PROJ             = INTERN("u_proj");
    SYM_U_SELECTED         = INTERN("u_selected");
    SYM_U_TEX              = INTERN("u_tex");
    SYM_U_VIEW             = INTERN("u_view");

    SYM_ENTITY_BACKGROUND_MUSIC = INTERN("background_music");
    SYM_ENTITY_FREECAM          = INTERN("freecam");
    SYM_ENTITY_PLAYER_CAMERA    = INTERN("player_camera");
    SYM_ENTITY_PLAYER_ONE       = INTERN("player_one");
    SYM_SHADER_EDITOR_SELECT    = INTERN("editor_select");
}

const char *ta_symbol_intern(const char *s, size_t len) {
    DLB_ASSERT(len);
    DLB_ASSERT(len < TA_SYMBOL_MAX_LEN);

    char *sym = dlb_hash_search(&symbol_table, s, len, 0);
    if (sym) return sym;

    sym = dlb_symbol_alloc(s, len);
    dlb_hash_insert(&symbol_table, sym, len, sym);
    return sym;
}