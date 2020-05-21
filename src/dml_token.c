#include "dml_token.h"

const char *dml_token_type_str(dml_token_type type)
{
    switch (type) {
        case TOK_UNKNOWN:               return "TOK_UNKNOWN";
        case TOK_TAB:                   return "TOK_TAB";
        case TOK_LEFT_CURLY_BRACE:      return "TOK_LEFT_BRACE";
        case TOK_RIGHT_CURLY_BRACE:     return "TOK_RIGHT_BRACE";
        case TOK_LEFT_SQUARE_BRACKET:   return "TOK_LEFT_BRACKET";
        case TOK_RIGHT_SQUARE_BRACKET:  return "TOK_RIGHT_BRACKET";
        case TOK_COMMA:                 return "TOK_COMMA";
        case TOK_COLON:                 return "TOK_COLON";
        case TOK_HASH:                  return "TOK_HASH";
        case TOK_IDENTIFIER:            return "TOK_IDENTIFIER";
        case TOK_STRING:                return "TOK_STRING";
        case TOK_NUMBER:                return "TOK_NUMBER";
        case TOK_FALSE:                 return "TOK_FALSE";
        case TOK_NULL:                  return "TOK_NULL";
        case TOK_TRUE:                  return "TOK_TRUE";
        case TOK_EOF:                   return "TOK_EOF";
        default: {
            assert(!"<INVALID_TOKEN_TYPE>");
            return "<INVALID_TOKEN_TYPE>";
        }
    }
}

void dml_token_init(dml_token *token, dml_token_type type, const char *lexeme, size_t line, size_t column, size_t start,
    size_t length)
{
    token->type = type;
    token->lexeme = lexeme;
    token->line = line;
    token->column = column;
    token->start = start;
    token->length = length;
}