#pragma once

// TODO: Move the MAX defines elsewhere to avoid this dependency
#include "ta_light.h"
#include "ta_model.h"
#include "ta_texture.h"

#include "dlb/dlb_types.h"
#include "dlb/dlb_memory.h"

typedef struct dlb_symbol__hdr {
    size_t len;
} dlb_symbol__hdr;

#define dlb_symbol__hdr(s) ((dlb_symbol__hdr *)((char *)(s) - sizeof(dlb_symbol__hdr)))
#define dlb_symbol_len(s) ((s) ? (dlb_symbol__hdr(s)->len) : (0))

//#define dlb_symbol_end(s) ((s) + dlb_symbol_len(s))
//#define dlb_symbol_last(s) (&(s)[dlb_symbol__hdr(s)->len-1])
//#define dlb_symbol_cstr(s) (dlb_symbol__alloc((s), sizeof(s)))
#define dlb_symbol_alloc(s, len) (dlb_symbol__alloc((s), (len)))
#define dlb_symbol_free(s) ((s) ? (dlb_free(dlb_symbol__hdr(s)), (s) = NULL) : 0)

// TODO: Use arena allocator for symbols (see dlb_string.h)
static inline char *dlb_symbol__alloc(const char *buf, size_t len) {
    size_t new_size = sizeof(dlb_symbol__hdr) + len + 1;
    dlb_symbol__hdr *sym = (dlb_symbol__hdr *)dlb_calloc(1, new_size);
    sym->len = len;
    char *str = (char *)sym + sizeof(dlb_symbol__hdr);
    dlb_memcpy(str, buf, len);
    str[len] = 0;
    return str;
}

#define SYM(s) (s), dlb_symbol_len(s)
#define SYM32(s) (s), (u32)dlb_symbol_len(s)
#define INTERN(s) ta_symbol_intern(CSTR(s))

// GLSL types
extern const char *SYM_GLINT;
extern const char *SYM_GLUINT;
extern const char *SYM_SAMPLER2D;
extern const char *SYM_VEC2;
extern const char *SYM_VEC3;
extern const char *SYM_VEC4;
extern const char *SYM_MAT3;
extern const char *SYM_MAT4;
extern const char *SYM_STRUCT;

// Shader attributes
extern const char *SYM_ATTR_COLOR;
extern const char *SYM_ATTR_UV;
extern const char *SYM_ATTR_POSITION;
extern const char *SYM_ATTR_NORMAL;
extern const char *SYM_ATTR_TANGENT;
extern const char *SYM_ATTR_MORPH1_POSITION;
extern const char *SYM_ATTR_MORPH1_NORMAL;
extern const char *SYM_ATTR_MORPH1_TANGENT;
extern const char *SYM_ATTR_BONES;
extern const char *SYM_ATTR_WEIGHTS;

// Shader uniforms
extern const char *SYM_U_CAMERA_POS;
extern const char *SYM_U_COLOR;
extern const char *SYM_U_DEBUG_CHANNEL;
extern const char *SYM_U_FACE;
extern const char *SYM_U_LIGHT_POS;
extern const char *SYM_U_LIGHT_PVM;
extern const char *SYM_U_LIGHT_ZFAR;
extern const char *SYM_U_LIGHTS_COUNT;
extern const char *SYM_U_EXPOSURE;
extern const char *SYM_U_MATERIAL_INDEX;
extern const char *SYM_U_MODEL;
extern const char *SYM_U_MORPH_WEIGHTS[TA_MODEL_MAX_MORPHS];
extern const char *SYM_U_PROJ;
extern const char *SYM_U_SELECTED;
extern const char *SYM_U_TEX;
extern const char *SYM_U_TEXTURES[TA_TEXTURE_POOL_MAX];
extern const char *SYM_U_TEXTURE_POOL_INDEX;
extern const char *SYM_U_TEXTURE_ARRAY_LAYER;
extern const char *SYM_U_TEXTURE_ARRAY_LAYERS;
extern const char *SYM_U_VIEW;

// Constants
extern const char *SYM_ENTITY_PLAYER_ONE;
extern const char *SYM_ENTITY_PLAYER_CAMERA;
extern const char *SYM_ENTITY_FREECAM;
extern const char *SYM_ENTITY_BACKGROUND_MUSIC;
extern const char *SYM_SHADER_EDITOR_SELECT;

// OGEX animation track target paths
extern const char *SYM_TRANSLATION;
extern const char *SYM_ROTATION;

const char *ta_symbol_intern(const char *s, size_t len);
void ta_symbol_init();