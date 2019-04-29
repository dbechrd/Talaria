#include "ta_symbol.h"
#include "ta_parse.h"
#include "dlb_types.h"
#include "dlb_arena.h"
#include "dlb_hash.h"

#define TA_SYMBOL_MAX_LEN 256

const char *sym_ident_name;
const char *sym_kw_null;
const char *sym_kw_true;
const char *sym_kw_false;

// TODO: It may be useful to have multiple symbol tables to allow freeing
//       symbols that are no longer in use (e.g. table per scene file). This
//       hasn't been necessary yet, so I'm not going to do it preemptively.
static dlb_hash symbol_table;

void ta_symbol_init() {
    dlb_hash_init(&symbol_table, DLB_HASH_STRING, "[symbol_table]", 128);
    sym_ident_name  = INTERN(IDENT_NAME);
    sym_kw_null     = INTERN(KEYWORD_NULL);
    sym_kw_true     = INTERN(KEYWORD_TRUE);
    sym_kw_false    = INTERN(KEYWORD_FALSE);
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