#pragma once
#include "dlb/dlb_types.h"
#include "ta_file.h"

struct ta_scene;

typedef enum token_type {
    TOKEN_UNKNOWN,
    TOKEN_EOF,
    TOKEN_WHITESPACE,
    TOKEN_NEWLINE,
    TOKEN_INDENT,
    TOKEN_COMMENT,
    TOKEN_IDENTIFIER,
    TOKEN_NULL,
    TOKEN_BOOL,
    TOKEN_INT,
    TOKEN_FLOAT,
    TOKEN_STRING,
    TOKEN_ARRAY_START,
    TOKEN_ARRAY_END,
    TOKEN_OBJECT_START,
    TOKEN_OBJECT_END,
    TOKEN_LIST_SEPARATOR,
} token_type;

typedef struct token {
    token_type type;  // Token type
    u32 length;       // Token length (for arrays and strings)
    union {
        bool        as_bool;  // NOTE: These are only prefixed with "as_" to avoid colliding with reserved keywords
        s32         as_int;
        float       as_float;
        s32         *int_array;
        float       *float_array;
        const char  *string;
    } value;
    ta_file_pos file_pos;  // DEBUG: Track file position for better error messages
} token;

token *tokenize(ta_file *f);
void tokens_parse(struct ta_scene *scene, token *tokens);
void tokens_print(FILE *f, token *tokens);
void tokens_print_debug(FILE *f, token *tokens);
