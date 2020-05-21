#pragma once

typedef enum dml_token_type {
    TOK_UNKNOWN,

    // 1 character tokens
    TOK_TAB,
    TOK_LEFT_CURLY_BRACE,
    TOK_RIGHT_CURLY_BRACE,
    TOK_LEFT_SQUARE_BRACKET,
    TOK_RIGHT_SQUARE_BRACKET,
    TOK_COMMA,
    TOK_COLON,
    TOK_HASH,

    // literals
    TOK_STRING,
    //TOK_COMMENT,
    TOK_NUMBER,
    TOK_IDENTIFIER,

    // keywords
    TOK_NULL,
    TOK_FALSE,
    TOK_TRUE,

    TOK_EOF
} dml_token_type;

const char *dml_token_type_str(dml_token_type type);

typedef struct dml_token {
    dml_token_type type;
    const char *lexeme;     // Can be null (e.g. EOF)
    union {
        float as_float;
        const char *as_string;
    } literal;
    size_t line;
    size_t column;
    size_t start;
    size_t length;
} dml_token;

void dml_token_init(dml_token *token, dml_token_type type, const char *lexeme, size_t line, size_t column, size_t start,
    size_t length);
const char *TokenToString(dml_token *token);