#include "ta_symbol.h"
#include "ta_parse.h"
#include "dlb_types.h"
#include "dlb_arena.h"
#include "dlb_hash.h"

#define TA_SYMBOL_MAX_LEN 256

// Special identifiers
const char *sym_ident_uid;

// DML keywords
const char *sym_kw_null;
const char *sym_kw_true;
const char *sym_kw_false;

// GLSL variable types
const char *sym_glsl_vec2;
const char *sym_glsl_vec3;
const char *sym_glsl_vec4;
const char *sym_glsl_mat4;
const char *sym_glsl_sampler2d;

// TODO: It may be useful to have multiple symbol tables to allow freeing
//       symbols that are no longer in use (e.g. table per scene file). This
//       hasn't been necessary yet, so I'm not going to do it preemptively.
static dlb_hash symbol_table;

void ta_symbol_init() {
    dlb_hash_init(&symbol_table, DLB_HASH_STRING, "[symbol_table]", 256);

    sym_ident_uid     = INTERN(IDENT_UID);

    sym_kw_null        = INTERN(KEYWORD_NULL);
    sym_kw_true        = INTERN(KEYWORD_TRUE);
    sym_kw_false       = INTERN(KEYWORD_FALSE);

    sym_glsl_vec2      = INTERN("vec2");
    sym_glsl_vec3      = INTERN("vec3");
    sym_glsl_vec4      = INTERN("vec4");
    sym_glsl_mat4      = INTERN("mat4");
    sym_glsl_sampler2d = INTERN("sampler2D");
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