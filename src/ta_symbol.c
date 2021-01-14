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
const char *SYM_ATTR_COLOR;
const char *SYM_ATTR_UV;
const char *SYM_ATTR_POSITION;
const char *SYM_ATTR_NORMAL;
const char *SYM_ATTR_TANGENT;
const char *SYM_ATTR_MORPH1_POSITION;
const char *SYM_ATTR_MORPH1_NORMAL;
const char *SYM_ATTR_MORPH1_TANGENT;
const char *SYM_ATTR_BONES;
const char *SYM_ATTR_WEIGHTS;

// Shader uniforms
const char *SYM_U_CAMERA_POS;
const char *SYM_U_COLOR;
const char *SYM_U_DEBUG_CHANNEL;
const char *SYM_U_FACE;
const char *SYM_U_LIGHT_POS;
const char *SYM_U_LIGHT_PVM;
const char *SYM_U_LIGHT_ZFAR;
const char *SYM_U_LIGHTS_COUNT;
const char *SYM_U_MATERIAL_INDEX;
const char *SYM_U_MODEL;
const char *SYM_U_MORPH_WEIGHTS[TA_MODEL_MAX_MORPHS];
const char *SYM_U_PROJ;
const char *SYM_U_SELECTED;
const char *SYM_U_TEX;
const char *SYM_U_TEXTURES[TA_TEXTURE_POOL_MAX];
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

// OGEX animation track target paths
const char *SYM_TRANSLATION;
const char *SYM_ROTATION;

// TODO: It may be useful to have multiple symbol tables to allow freeing
//       symbols that are no longer in use (e.g. table per scene file). This
//       hasn't been necessary yet, so I'm not going to do it preemptively.
static dlb_hash symbol_table;

void symbol_gen_array(const char **array, size_t array_len, const char *format)
{
    char sym_buf[TA_SYMBOL_MAX_LEN] = { 0 };
    size_t sym_len = 0;
    for (int i = 0; i < (int)array_len; ++i) {
        sym_len = snprintf(sym_buf, sizeof(sym_buf), format, i);
        DLB_ASSERT(sym_len < sizeof(sym_buf));
        array[i] = ta_symbol_intern(sym_buf, sym_len);
    }
}

#define SYMBOL_GEN_ARRAY(array, format) symbol_gen_array(array, ARRAY_SIZE(array), format);

void ta_symbol_init() {
    dlb_hash_init(&symbol_table, DLB_HASH_STRING, "[symbol_table]", 2048);

    tg_mesh_default     = INTERN("prim_unknown");
    tg_material_default = INTERN("material_unknown");

    SYM_GLINT     = INTERN("glint");
    SYM_GLUINT    = INTERN("gluint");
    SYM_SAMPLER2D = INTERN("sampler2D");
    SYM_VEC2      = INTERN("vec2");
    SYM_VEC3      = INTERN("vec3");
    SYM_VEC4      = INTERN("vec4");
    SYM_MAT4      = INTERN("mat3");
    SYM_MAT4      = INTERN("mat4");
    SYM_STRUCT    = INTERN("struct");

    SYM_ATTR_COLOR           = INTERN("attr_color");
    SYM_ATTR_UV              = INTERN("attr_uv");
    SYM_ATTR_POSITION        = INTERN("attr_position");
    SYM_ATTR_NORMAL          = INTERN("attr_normal");
    SYM_ATTR_TANGENT         = INTERN("attr_tangent");
    SYM_ATTR_MORPH1_POSITION = INTERN("attr_morph1_position");
    SYM_ATTR_MORPH1_NORMAL   = INTERN("attr_morph1_normal");
    SYM_ATTR_MORPH1_TANGENT  = INTERN("attr_morph1_tangent");
    SYM_ATTR_BONES           = INTERN("attr_bones");
    SYM_ATTR_WEIGHTS         = INTERN("attr_weights");

    SYM_U_CAMERA_POS            = INTERN("u_camera_pos");
    SYM_U_COLOR                 = INTERN("u_color");
    SYM_U_DEBUG_CHANNEL         = INTERN("u_debug_channel");
    SYM_U_FACE                  = INTERN("u_face");

    SYM_U_LIGHT_POS             = INTERN("u_light_pos");
    SYM_U_LIGHT_PVM             = INTERN("u_light_pvm");
    SYM_U_LIGHT_ZFAR            = INTERN("u_light_zfar");
    SYM_U_LIGHTS_COUNT          = INTERN("u_lights_count");

    SYM_U_MATERIAL_INDEX        = INTERN("u_material_index");

    SYM_U_TEXTURE_POOL_INDEX    = INTERN("u_texture_pool_index");
    SYM_U_TEXTURE_ARRAY_LAYER   = INTERN("u_texture_array_layer");
    SYM_U_TEXTURE_ARRAY_LAYERS  = INTERN("u_texture_array_layers");

    SYMBOL_GEN_ARRAY(SYM_U_TEXTURES,      "u_textures[%d]");
    SYMBOL_GEN_ARRAY(SYM_U_MORPH_WEIGHTS, "u_morph_weights[%d]");

    SYM_U_MODEL            = INTERN("u_model");
    SYM_U_PROJ             = INTERN("u_proj");
    SYM_U_SELECTED         = INTERN("u_selected");
    SYM_U_TEX              = INTERN("u_tex");
    SYM_U_VIEW             = INTERN("u_view");

    SYM_ENTITY_BACKGROUND_MUSIC = INTERN("background_music");
    SYM_ENTITY_FREECAM          = INTERN("freecam");
    SYM_ENTITY_PLAYER_CAMERA    = INTERN("player_camera");
    SYM_ENTITY_PLAYER_ONE       = INTERN("player_one");
    SYM_SHADER_EDITOR_SELECT    = INTERN("editor_select");

    SYM_TRANSLATION = INTERN("translation");
    SYM_ROTATION    = INTERN("rotation");
}

#undef SYMBOL_GEN_ARRAY

const char *ta_symbol_intern(const char *s, size_t len) {
    DLB_ASSERT(len);
    DLB_ASSERT(len < TA_SYMBOL_MAX_LEN);

    char *sym = dlb_hash_search(&symbol_table, s, len, 0);
    if (sym) return sym;

    sym = dlb_symbol_alloc(s, len);
    dlb_hash_insert(&symbol_table, sym, len, sym);
    return sym;
}