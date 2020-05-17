#pragma once

typedef enum DMLTokenType {
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
} DMLTokenType;

const char *DMLTokenTypeToString(DMLTokenType type);

typedef struct DMLToken {
    DMLTokenType type;
    const char *lexeme;     // Can be null (e.g. EOF)
    union {
        float as_float;
        const char *as_string;
    } literal;
    size_t line;
    size_t column;
    size_t start;
    size_t length;
} DMLToken;

void DMLTokenInit(DMLToken *token, DMLTokenType type, const char *lexeme, size_t line, size_t column, size_t start,
    size_t length);
const char *TokenToString(DMLToken *token);