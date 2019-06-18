#include "ta_symbol.h"
#include "ta_parse.h"
#include "dlb_types.h"
#include "dlb_arena.h"
#include "dlb_hash.h"

#define TA_SYMBOL_MAX_LEN 256

// Special identifiers
const char *SYM_UID;

// DML keywords
const char *SYM_NULL;
const char *SYM_TRUE;
const char *SYM_FALSE;

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

// Shader uniforms
const char *SYM_U_PROJ;
const char *SYM_U_VIEW;
const char *SYM_U_MODEL;
const char *SYM_U_SUN;
const char *SYM_U_CAMERA_POS;
const char *SYM_U_TEX;
const char *SYM_U_TEX_ALBEDO;
const char *SYM_U_TEX_METALLIC;

// TODO: It may be useful to have multiple symbol tables to allow freeing
//       symbols that are no longer in use (e.g. table per scene file). This
//       hasn't been necessary yet, so I'm not going to do it preemptively.
static dlb_hash symbol_table;

void ta_symbol_init() {
    dlb_hash_init(&symbol_table, DLB_HASH_STRING, "[symbol_table]", 256);

    SYM_UID   = INTERN(IDENT_UID);
    SYM_NULL  = INTERN(KEYWORD_NULL);
    SYM_TRUE  = INTERN(KEYWORD_TRUE);
    SYM_FALSE = INTERN(KEYWORD_FALSE);

    SYM_GLINT     = INTERN("glint");
    SYM_GLUINT    = INTERN("gluint");
    SYM_SAMPLER2D = INTERN("sampler2D");
    SYM_VEC2      = INTERN("vec2");
    SYM_VEC3      = INTERN("vec3");
    SYM_VEC4      = INTERN("vec4");
    SYM_MAT4      = INTERN("mat3");
    SYM_MAT4      = INTERN("mat4");
    SYM_STRUCT    = INTERN("struct");

    SYM_ATTR_POSITION = INTERN("attr_position");
    SYM_ATTR_COLOR    = INTERN("attr_color");
    SYM_ATTR_UV       = INTERN("attr_uv");
    SYM_ATTR_NORMAL   = INTERN("attr_normal");

    SYM_U_PROJ          = INTERN("u_proj");
    SYM_U_VIEW          = INTERN("u_view");
    SYM_U_MODEL         = INTERN("u_model");
    SYM_U_SUN           = INTERN("u_sun");
    SYM_U_CAMERA_POS    = INTERN("u_camera_pos");
    SYM_U_TEX           = INTERN("u_tex");
    SYM_U_TEX_ALBEDO    = INTERN("u_tex_albedo");
    SYM_U_TEX_METALLIC  = INTERN("u_tex_metallic");
}

const char *ta_symbol_intern(const char *s, u32 len) {
    DLB_ASSERT(len);
    DLB_ASSERT(len < TA_SYMBOL_MAX_LEN);

    char *sym = dlb_hash_search(&symbol_table, s, len);
    if (sym) return sym;

    sym = dlb_symbol_alloc(s, len);
    dlb_hash_insert(&symbol_table, sym, len, sym);
    return sym;
}