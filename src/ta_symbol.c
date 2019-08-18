#include "ta_symbol.h"
#include "ta_parse.h"
#include "dlb/dlb_types.h"
#include "dlb/dlb_arena.h"
#include "dlb/dlb_hash.h"

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
const char *SYM_U_FACE;
const char *SYM_U_LIGHT_PVM;
const char *SYM_U_LIGHT_POS;
const char *SYM_U_LIGHT_ZFAR;
const char *SYM_U_CAMERA_POS;
const char *SYM_U_TEX;
const char *SYM_U_TEX_ALBEDO;
const char *SYM_U_TEX_METALLIC;
const char *SYM_U_LIGHTS_COUNT;
const char *SYM_U_LIGHTS;
const char *SYM_U_LIGHTS_INTENSITY[8];
const char *SYM_U_LIGHTS_POSITION[8];
const char *SYM_U_LIGHTS_COLOR[8];
const char *SYM_U_LIGHTS_TYPE[8];
const char *SYM_U_LIGHTS_DIRECTION[8];
const char *SYM_U_LIGHTS_SHADOWMAP2D[8];
const char *SYM_U_LIGHTS_SHADOWMAP3D[8];
const char *SYM_U_LIGHTS_SHADOWMAP_ZFAR[8];

// TODO: It may be useful to have multiple symbol tables to allow freeing
//       symbols that are no longer in use (e.g. table per scene file). This
//       hasn't been necessary yet, so I'm not going to do it preemptively.
static dlb_hash symbol_table;

void ta_symbol_init() {
    dlb_hash_init(&symbol_table, DLB_HASH_STRING, "[symbol_table]", 512);

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
    SYM_U_FACE            = INTERN("u_face");
    SYM_U_LIGHT_PVM     = INTERN("u_light_pvm");
    SYM_U_LIGHT_POS     = INTERN("u_light_pos");
    SYM_U_LIGHT_ZFAR    = INTERN("u_light_zfar");
    SYM_U_CAMERA_POS    = INTERN("u_camera_pos");
    SYM_U_TEX           = INTERN("u_tex");
    SYM_U_TEX_ALBEDO    = INTERN("u_tex_albedo");
    SYM_U_TEX_METALLIC  = INTERN("u_tex_metallic");
    SYM_U_LIGHTS_COUNT  = INTERN("u_lights_count");
    SYM_U_LIGHTS        = INTERN("u_lights");

    SYM_U_LIGHTS_INTENSITY[0] = INTERN("u_lights[0].intensity");
    SYM_U_LIGHTS_INTENSITY[1] = INTERN("u_lights[1].intensity");
    SYM_U_LIGHTS_INTENSITY[2] = INTERN("u_lights[2].intensity");
    SYM_U_LIGHTS_INTENSITY[3] = INTERN("u_lights[3].intensity");
    SYM_U_LIGHTS_INTENSITY[4] = INTERN("u_lights[4].intensity");
    SYM_U_LIGHTS_INTENSITY[5] = INTERN("u_lights[5].intensity");
    SYM_U_LIGHTS_INTENSITY[6] = INTERN("u_lights[6].intensity");
    SYM_U_LIGHTS_INTENSITY[7] = INTERN("u_lights[7].intensity");

    SYM_U_LIGHTS_POSITION[0] = INTERN("u_lights[0].position");
    SYM_U_LIGHTS_POSITION[1] = INTERN("u_lights[1].position");
    SYM_U_LIGHTS_POSITION[2] = INTERN("u_lights[2].position");
    SYM_U_LIGHTS_POSITION[3] = INTERN("u_lights[3].position");
    SYM_U_LIGHTS_POSITION[4] = INTERN("u_lights[4].position");
    SYM_U_LIGHTS_POSITION[5] = INTERN("u_lights[5].position");
    SYM_U_LIGHTS_POSITION[6] = INTERN("u_lights[6].position");
    SYM_U_LIGHTS_POSITION[7] = INTERN("u_lights[7].position");

    SYM_U_LIGHTS_COLOR[0] = INTERN("u_lights[0].color");
    SYM_U_LIGHTS_COLOR[1] = INTERN("u_lights[1].color");
    SYM_U_LIGHTS_COLOR[2] = INTERN("u_lights[2].color");
    SYM_U_LIGHTS_COLOR[3] = INTERN("u_lights[3].color");
    SYM_U_LIGHTS_COLOR[4] = INTERN("u_lights[4].color");
    SYM_U_LIGHTS_COLOR[5] = INTERN("u_lights[5].color");
    SYM_U_LIGHTS_COLOR[6] = INTERN("u_lights[6].color");
    SYM_U_LIGHTS_COLOR[7] = INTERN("u_lights[7].color");

    SYM_U_LIGHTS_TYPE[0] = INTERN("u_lights[0].type");
    SYM_U_LIGHTS_TYPE[1] = INTERN("u_lights[1].type");
    SYM_U_LIGHTS_TYPE[2] = INTERN("u_lights[2].type");
    SYM_U_LIGHTS_TYPE[3] = INTERN("u_lights[3].type");
    SYM_U_LIGHTS_TYPE[4] = INTERN("u_lights[4].type");
    SYM_U_LIGHTS_TYPE[5] = INTERN("u_lights[5].type");
    SYM_U_LIGHTS_TYPE[6] = INTERN("u_lights[6].type");
    SYM_U_LIGHTS_TYPE[7] = INTERN("u_lights[7].type");

    SYM_U_LIGHTS_DIRECTION[0] = INTERN("u_lights[0].direction");
    SYM_U_LIGHTS_DIRECTION[1] = INTERN("u_lights[1].direction");
    SYM_U_LIGHTS_DIRECTION[2] = INTERN("u_lights[2].direction");
    SYM_U_LIGHTS_DIRECTION[3] = INTERN("u_lights[3].direction");
    SYM_U_LIGHTS_DIRECTION[4] = INTERN("u_lights[4].direction");
    SYM_U_LIGHTS_DIRECTION[5] = INTERN("u_lights[5].direction");
    SYM_U_LIGHTS_DIRECTION[6] = INTERN("u_lights[6].direction");
    SYM_U_LIGHTS_DIRECTION[7] = INTERN("u_lights[7].direction");

    SYM_U_LIGHTS_SHADOWMAP2D[0] = INTERN("u_lights[0].shadowmap2d");
    SYM_U_LIGHTS_SHADOWMAP2D[1] = INTERN("u_lights[1].shadowmap2d");
    SYM_U_LIGHTS_SHADOWMAP2D[2] = INTERN("u_lights[2].shadowmap2d");
    SYM_U_LIGHTS_SHADOWMAP2D[3] = INTERN("u_lights[3].shadowmap2d");
    SYM_U_LIGHTS_SHADOWMAP2D[4] = INTERN("u_lights[4].shadowmap2d");
    SYM_U_LIGHTS_SHADOWMAP2D[5] = INTERN("u_lights[5].shadowmap2d");
    SYM_U_LIGHTS_SHADOWMAP2D[6] = INTERN("u_lights[6].shadowmap2d");
    SYM_U_LIGHTS_SHADOWMAP2D[7] = INTERN("u_lights[7].shadowmap2d");

    SYM_U_LIGHTS_SHADOWMAP3D[0] = INTERN("u_lights[0].shadowmap3d");
    SYM_U_LIGHTS_SHADOWMAP3D[1] = INTERN("u_lights[1].shadowmap3d");
    SYM_U_LIGHTS_SHADOWMAP3D[2] = INTERN("u_lights[2].shadowmap3d");
    SYM_U_LIGHTS_SHADOWMAP3D[3] = INTERN("u_lights[3].shadowmap3d");
    SYM_U_LIGHTS_SHADOWMAP3D[4] = INTERN("u_lights[4].shadowmap3d");
    SYM_U_LIGHTS_SHADOWMAP3D[5] = INTERN("u_lights[5].shadowmap3d");
    SYM_U_LIGHTS_SHADOWMAP3D[6] = INTERN("u_lights[6].shadowmap3d");
    SYM_U_LIGHTS_SHADOWMAP3D[7] = INTERN("u_lights[7].shadowmap3d");

    SYM_U_LIGHTS_SHADOWMAP_ZFAR[0] = INTERN("u_lights[0].shadowmap_zfar");
    SYM_U_LIGHTS_SHADOWMAP_ZFAR[1] = INTERN("u_lights[1].shadowmap_zfar");
    SYM_U_LIGHTS_SHADOWMAP_ZFAR[2] = INTERN("u_lights[2].shadowmap_zfar");
    SYM_U_LIGHTS_SHADOWMAP_ZFAR[3] = INTERN("u_lights[3].shadowmap_zfar");
    SYM_U_LIGHTS_SHADOWMAP_ZFAR[4] = INTERN("u_lights[4].shadowmap_zfar");
    SYM_U_LIGHTS_SHADOWMAP_ZFAR[5] = INTERN("u_lights[5].shadowmap_zfar");
    SYM_U_LIGHTS_SHADOWMAP_ZFAR[6] = INTERN("u_lights[6].shadowmap_zfar");
    SYM_U_LIGHTS_SHADOWMAP_ZFAR[7] = INTERN("u_lights[7].shadowmap_zfar");
}

const char *ta_symbol_intern(const char *s, u32 len) {
    DLB_ASSERT(len);
    DLB_ASSERT(len < TA_SYMBOL_MAX_LEN);

    char *sym = dlb_hash_search(&symbol_table, s, len, 0);
    if (sym) return sym;

    sym = dlb_symbol_alloc(s, len);
    dlb_hash_insert(&symbol_table, sym, len, sym);
    return sym;
}