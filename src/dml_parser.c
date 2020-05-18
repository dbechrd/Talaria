#include "dml_parser.h"
#include "dml_token.h"
#include "dml.h"

#if _DEBUG
    #define FILL_DEBUG_SYMBOL(dbg_symbol, token) \
        dbg_symbol.filename = parser->filename;  \
        dbg_symbol.line = token->line;           \
        dbg_symbol.column = token->column;
#else
    #define FILL_DEBUG_SYMBOL
#endif

void DMLParserInit(DMLParser *parser, struct DMLToken *tokens, const char *filename, const char *source,
    size_t source_len)
{
    parser->tokens = tokens;
    parser->current = 0;
    parser->filename = filename;
    parser->source = source;
    parser->source_len = source_len;
}

static DMLToken *DMLParserPeek(DMLParser *parser)
{
    return &parser->tokens[parser->current];
}

static DMLToken *DMLParserPrevious(DMLParser *parser)
{
    return &parser->tokens[parser->current - 1];
}

static bool DMLParserIsAtEnd(DMLParser *parser)
{
    DMLToken *peek = DMLParserPeek(parser);
    return peek->type == TOK_EOF;
}

static bool DMLParserCheck(DMLParser *parser, DMLTokenType type)
{
    if (DMLParserIsAtEnd(parser)) return false;

    DMLToken *peek = DMLParserPeek(parser);
    return peek->type == type;
}

static DMLToken *DMLParserAdvance(DMLParser *parser)
{
    if (!DMLParserIsAtEnd(parser)) {
        parser->current++;
    }

    return DMLParserPrevious(parser);
}

static DMLToken *DMLParserConsume(DMLParser *parser, DMLTokenType type)
{
    DMLToken *token;
    if (!DMLParserCheck(parser, type)) {
        return NULL;
    }

    token = DMLParserAdvance(parser);
    return token;
}

static bool DMLParserMatch(DMLParser *parser, DMLTokenType type)
{
    if (DMLParserCheck(parser, type)) {
        DMLParserAdvance(parser);
        return true;
    }
    return false;
}

static void DMLParserSynchronize(DMLParser *parser)
{
    DMLParserAdvance(parser);

    while (!DMLParserIsAtEnd(parser)) {
        // Skip all tokens until next identifier
        DMLToken *peek = DMLParserPeek(parser);
        if (peek->type == TOK_IDENTIFIER) {
            return;
        }
        DMLParserAdvance(parser);
    }
}

static void DMLParserErrorContext(DMLParser *parser, const char *message, DMLToken *token)
{
    //DMLToken *token = DMLParserPeek(parser);
    printf("[%04zu:%04zu] error: %s, found %s '%s'\n", token->line, token->column, message,
        DMLTokenTypeToString(token->type), token->lexeme);

    if (!parser->source_len) {
        return;
    }
    assert(parser->source);

    // max number of characters to display on either side of the error
    static size_t context_limit = 39;

    size_t line_start = token->start;
    while (line_start > 0 && parser->source[line_start - 1] != '\n') {
        line_start--;
    }

    size_t line_end = token->start + token->length;
    while (line_end < parser->source_len && parser->source[line_end] != '\n') {
        line_end++;
    }

    size_t context_start = MAX(token->start - context_limit, line_start);
    size_t context_end = MIN(token->start + context_limit, line_end);

    const char *context = parser->source + context_start;
    size_t context_len = (context_end - context_start);
    size_t tab_count = 0;

    char context_buf[80] = { 0 };
    char caret_buf[80] = { 0 };
    int caret_found = 0;

    size_t out = 0;
    for (size_t in = 0; in < context_len; in++) {
        if (context[in] == '\t') {
            // NOTE: force tab to be a known width in console so that caret lines up properly
            for (size_t s = 0; s < DML_ERROR_CONTEXT_TAB_WIDTH; s++) {
                context_buf[out] = ' ';
                if (!caret_found) caret_buf[out] = ' ';
                out++;
            }
        } else {
            context_buf[out] = context[in];
            if (!caret_found) caret_buf[out] = ' ';
            out++;
        }
        if (!caret_found && in == token->start - context_start) {
            caret_buf[out-1] = '^';
            caret_found = 1;
        }
    }

    fputs(context_buf, stdout);
    fputc('\n', stdout);
    fputs(caret_buf, stdout);
    fputc('\n\n', stdout);
}

/*
document  → field* EOF ;
object    → IDENTIFIER ":" field* ;
field     → IDENTIFIER ":" value ;
value     → object
          | array
          | literal ;
array     → "[" value* "]" ;
literal   → "true" | "false" | "null"
          | NUMBER | STRING
          | IDENTIFIER ;
*/

static bool DMLParserObject(DMLParser *parser, DMLObject *object);
static bool DMLParserField(DMLParser *parser, DMLField *field);
static bool DMLParserValue(DMLParser *parser, DMLValue *value);
static bool DMLParserArray(DMLParser *parser, DMLArray *array);
static bool DMLParserLiteral(DMLParser *parser, DMLLiteral *literal);

static bool DMLParserObject(DMLParser *parser, DMLObject *object)
{
    if (!DMLParserConsume(parser, TOK_LEFT_CURLY_BRACE)) {
        DMLParserErrorContext(parser, "expected '{' to begin object", DMLParserPeek(parser));
        return false;
    }

    FILL_DEBUG_SYMBOL(object->dbg_symbol, DMLParserPrevious(parser));

    DMLField *fields = 0;
    while (!DMLParserCheck(parser, TOK_RIGHT_CURLY_BRACE) && !DMLParserIsAtEnd(parser)) {
        DMLField *field = dlb_vec_alloc(fields);
        if (!DMLParserField(parser, field)) {
            dlb_vec_free(fields);
            return false;
        }
    }

    if (!DMLParserConsume(parser, TOK_RIGHT_CURLY_BRACE)) {
        DMLParserErrorContext(parser, "expected '}' to end object", DMLParserPeek(parser));
        return false;
    }

    object->fields = fields;
    return true;
}

static bool DMLParserField(DMLParser *parser, DMLField *field)
{
    DMLToken *name = DMLParserConsume(parser, TOK_IDENTIFIER);
    if (!name) {
        DMLParserErrorContext(parser, "expected field identifier", DMLParserPeek(parser));
        return false;
    }

    FILL_DEBUG_SYMBOL(field->dbg_symbol, DMLParserPrevious(parser));

    if (!DMLParserConsume(parser, TOK_COLON)) {
        DMLParserErrorContext(parser, "expected ':' after field identifier", DMLParserPeek(parser));
        return false;
    }

    field->name = ta_symbol_intern(name->lexeme, strlen(name->lexeme));
    if (!DMLParserValue(parser, &field->value)) {
        // TODO: Implement a ta_symbol_free() method that deletes from hash table then calls dlb_symbol_free()
        //ta_symbol_free(field->name);
        return false;
    }

    return true;
}

static bool DMLParserValue(DMLParser *parser, DMLValue *value)
{
    DMLToken *token = DMLParserPeek(parser);
    switch (token->type) {
        case TOK_LEFT_CURLY_BRACE:
            value->type = DML_VALUE_OBJECT;
            FILL_DEBUG_SYMBOL(value->dbg_symbol, token);
            if (!DMLParserObject(parser, &value->data.as_object)) {
                return false;
            }
            break;
        case TOK_LEFT_SQUARE_BRACKET:
            value->type = DML_VALUE_ARRAY;
            FILL_DEBUG_SYMBOL(value->dbg_symbol, token);
            if (!DMLParserArray(parser, &value->data.as_array)) {
                return false;
            }
            break;
        case TOK_NULL:
        case TOK_TRUE:
        case TOK_FALSE:
        case TOK_NUMBER:
        case TOK_STRING:
            value->type = DML_VALUE_LITERAL;
            FILL_DEBUG_SYMBOL(value->dbg_symbol, token);
            if (!DMLParserLiteral(parser, &value->data.as_literal)) {
                return false;
            }
            break;
        default: {
            DMLParserErrorContext(parser, "expected value", token);
            if (!parser->expected_value_context_shown) {
                fputs("  Expected one of the following value tokens:\n\n"
                    "    type    | starts with\n"
                    "    --------|--------------------------\n"
                    "    object  | '{'\n"
                    "    array   | '['\n"
                    "    string  | '\"'\n"
                    "    number  | [0-9]\n"
                    "    keyword | true | false | null\n\n", stdout);
                parser->expected_value_context_shown = 1;
            }
            return false;
        }
    }
    return true;
}

static bool DMLParserArray(DMLParser *parser, DMLArray *array)
{
    if (!DMLParserConsume(parser, TOK_LEFT_SQUARE_BRACKET)) {
        DMLParserErrorContext(parser, "expected '[' to begin array", DMLParserPeek(parser));
        return false;
    }

    FILL_DEBUG_SYMBOL(array->dbg_symbol, DMLParserPrevious(parser));

    // NOTE: Allow array elements to be any type, the consumer of the DML scene should validate this??
    DMLValue *values = 0;
    bool first_element = true;
    while (!DMLParserCheck(parser, TOK_RIGHT_SQUARE_BRACKET) && !DMLParserIsAtEnd(parser)) {
        if (!first_element && !DMLParserConsume(parser, TOK_COMMA)) {
            dlb_vec_free(values);
            DMLParserErrorContext(parser, "expected ',' between array values", DMLParserPeek(parser));
            return false;
        }

        DMLValue *value = dlb_vec_alloc(values);
        if (!DMLParserValue(parser, value)) {
            dlb_vec_free(values);
            return false;
        }
        first_element = false;
    }

    if (!DMLParserConsume(parser, TOK_RIGHT_SQUARE_BRACKET)) {
        dlb_vec_free(values);
        DMLParserErrorContext(parser, "expected ']' to end array", DMLParserPeek(parser));
        return false;
    }

    array->values = values;
    return true;
}

static bool DMLParserLiteral(DMLParser *parser, DMLLiteral *literal)
{
    bool result = false;
    if (DMLParserMatch(parser, TOK_NULL)) {
        literal->type = DML_LITERAL_NULL;
        FILL_DEBUG_SYMBOL(literal->dbg_symbol, DMLParserPrevious(parser));
        result = true;
    } else if (DMLParserMatch(parser, TOK_TRUE)) {
        literal->type = DML_LITERAL_BOOL;
        literal->data.as_bool = true;
        FILL_DEBUG_SYMBOL(literal->dbg_symbol, DMLParserPrevious(parser));
        result = true;
    } else if (DMLParserMatch(parser, TOK_FALSE)) {
        literal->type = DML_LITERAL_BOOL;
        literal->data.as_bool = false;
        FILL_DEBUG_SYMBOL(literal->dbg_symbol, DMLParserPrevious(parser));
        result = true;
    } else if (DMLParserMatch(parser, TOK_NUMBER)) {
        // TODO: int vs float vs double (signed vs. unsigned)?
        DMLToken *prev = DMLParserPrevious(parser);
        literal->type = DML_LITERAL_FLOAT;
        literal->data.as_float = prev->literal.as_float;
        FILL_DEBUG_SYMBOL(literal->dbg_symbol, prev);
        result = true;
    } else if (DMLParserMatch(parser, TOK_STRING)) {
        DMLToken *prev = DMLParserPrevious(parser);
        literal->type = DML_LITERAL_STRING;
        literal->data.as_string = prev->literal.as_string;
        FILL_DEBUG_SYMBOL(literal->dbg_symbol, prev);
        result = true;
    }
    return result;
}

void DMLParserParse(DMLParser *parser, DMLObject *document)
{
    while (!DMLParserIsAtEnd(parser)) {
        DMLField *field = dlb_vec_alloc(document->fields);
        if (!DMLParserField(parser, field)) {
            dlb_vec_popz(document->fields);
            DMLParserSynchronize(parser);
        }
    }
}