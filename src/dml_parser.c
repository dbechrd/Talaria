#include "dml_parser.h"
#include "dml_token.h"
#include "dml.h"
#include <string.h>

static size_t **dbg_array_pointers = 0;

#if _DEBUG
    #define FILL_DEBUG_SYMBOL(dbg_symbol, token) \
        dbg_symbol.filename = parser->filename;  \
        dbg_symbol.line = token->line;           \
        dbg_symbol.column = token->column;
#else
    #define FILL_DEBUG_SYMBOL(dbg_symbol, token)
#endif

void dml_parser_init(dml_parser *parser, struct dml_token *tokens, const char *filename, const char *source,
    size_t source_len)
{
    parser->tokens = tokens;
    parser->current = 0;
    parser->filename = filename;
    parser->source = source;
    parser->source_len = source_len;
}

static inline dml_token *dml_parser_peek(dml_parser *parser)
{
    return &parser->tokens[parser->current];
}

static inline dml_token *dml_parser_previous(dml_parser *parser)
{
    return &parser->tokens[parser->current - 1];
}

static inline bool dml_parser_eof(dml_parser *parser)
{
    dml_token *peek = dml_parser_peek(parser);
    return peek->type == TOK_EOF;
}

static inline bool dml_parser_check(dml_parser *parser, dml_token_type type)
{
    dml_token *peek = dml_parser_peek(parser);
    return peek->type == type;
}

static inline void dml_parser_advance(dml_parser *parser)
{
    dml_token *token = dml_parser_peek(parser);
    if (token->type != TOK_EOF) {
        parser->current++;
    }
}

static bool dml_parser_consume(dml_parser *parser, dml_token_type type)
{
    if (type != TOK_EOF) {
        dml_token *peek = dml_parser_peek(parser);
        if (peek->type == type) {
            parser->current++;
            return true;
        }
    }
    return false;
}

static void dml_parser_sync(dml_parser *parser)
{
    dml_parser_advance(parser);

    while (!dml_parser_eof(parser)) {
        // Skip all tokens until next identifier
        dml_token *peek = dml_parser_peek(parser);
        if (peek->type == TOK_IDENTIFIER) {
            return;
        }
        dml_parser_advance(parser);
    }
}

static void dml_parser_error_context(dml_parser *parser, const char *message, dml_token *token)
{
    //dml_token *token = dml_parser_peek(parser);
    printf("[%04zu:%04zu] error: %s, found %s '%s'\n", token->line, token->column, message,
        dml_token_type_str(token->type), token->lexeme);

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

static bool dml_parser_object(dml_parser *parser, dml_document *document, size_t object_value_idx);
static bool dml_parser_field(dml_parser *parser, dml_document *document, size_t field_idx);
static bool dml_parser_value(dml_parser *parser, dml_document *document, size_t value_idx);
static bool dml_parser_array(dml_parser *parser, dml_document *document, size_t array_value_idx);
static bool dml_parser_literal(dml_parser *parser, dml_literal *literal);

// Before recursion removed
//[2020-05-21 07:13:29][    0][       DML][   0.913s] [dml_document_load:   0.006ms] START
//[2020-05-21 07:13:29][    0][       DML][   0.913s] [dml_document_load:   0.523ms] Loading data/mesh/button.ogex
//[2020-05-21 07:13:29][    0][       DML][   0.914s] [dml_document_load:   1.207ms] Scanning...
//[2020-05-21 07:13:29][    0][       DML][   0.928s] [dml_document_load:  15.860ms] Parsing...
//[2020-05-21 07:13:29][    0][       DML][   0.937s] [dml_document_load:  24.069ms] Loading...

static bool dml_parser_object(dml_parser *parser, dml_document *document, size_t object_value_idx)
{
    if (!dml_parser_consume(parser, TOK_LEFT_CURLY_BRACE)) {
        dml_parser_error_context(parser, "expected '{' to begin object", dml_parser_peek(parser));
        return false;
    }

    dml_value *value = &document->value_pool[object_value_idx];
    DLB_ASSERT(value->type == DML_VALUE_OBJECT);
    dml_object *object = &value->data.as_object;
    FILL_DEBUG_SYMBOL(object->dbg_symbol, dml_parser_previous(parser));

    size_t *fields = 0;
    while (!dml_parser_check(parser, TOK_RIGHT_CURLY_BRACE) && !dml_parser_eof(parser)) {
        size_t field_idx = dlb_vec_len(document->field_pool);
        dlb_vec_push(fields, field_idx);
        dlb_vec_alloc(document->field_pool);
        if (!dml_parser_field(parser, document, field_idx)) {
            dlb_vec_free(fields);
            dml_parser_sync(parser);
            return false;
        }
    }

    if (!dml_parser_consume(parser, TOK_RIGHT_CURLY_BRACE)) {
        dml_parser_error_context(parser, "expected '}' to end object", dml_parser_peek(parser));
        return false;
    }

    value = &document->value_pool[object_value_idx];
    DLB_ASSERT(value->type == DML_VALUE_OBJECT);
    object = &value->data.as_object;
    object->fields = fields;
    return true;
}

static bool dml_parser_field(dml_parser *parser, dml_document *document, size_t field_idx)
{
    if (!dml_parser_consume(parser, TOK_IDENTIFIER)) {
        dml_parser_error_context(parser, "expected field identifier", dml_parser_peek(parser));
        return false;
    }
    dml_token *name = dml_parser_previous(parser);
    dml_field *field = &document->field_pool[field_idx];
    FILL_DEBUG_SYMBOL(field->dbg_symbol, dml_parser_previous(parser));

    if (!dml_parser_consume(parser, TOK_COLON)) {
        dml_parser_error_context(parser, "expected ':' after field identifier", dml_parser_peek(parser));
        return false;
    }

    field->name = ta_symbol_intern(name->lexeme, strlen(name->lexeme));
    field->value_idx = dlb_vec_len(document->value_pool);
    dlb_vec_alloc(document->value_pool);
    if (!dml_parser_value(parser, document, field->value_idx)) {
        // TODO: Implement a ta_symbol_free() method that deletes from hash table then calls dlb_symbol_free()
        //ta_symbol_free(field->name);
        return false;
    }

    return true;
}

static bool dml_parser_value(dml_parser *parser, dml_document *document, size_t value_idx)
{
    dml_value *value = &document->value_pool[value_idx];
    dml_token *token = dml_parser_peek(parser);
    switch (token->type) {
        case TOK_LEFT_CURLY_BRACE:
            value->type = DML_VALUE_OBJECT;
            FILL_DEBUG_SYMBOL(value->dbg_symbol, token);
            if (!dml_parser_object(parser, document, value_idx)) {
                return false;
            }
            break;
        case TOK_LEFT_SQUARE_BRACKET:
            value->type = DML_VALUE_ARRAY;
            FILL_DEBUG_SYMBOL(value->dbg_symbol, token);
            if (!dml_parser_array(parser, document, value_idx)) {
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
            if (!dml_parser_literal(parser, &value->data.as_literal)) {
                return false;
            }
            break;
        default: {
            dml_parser_error_context(parser, "expected value", token);
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

// NOTE: Allows array elements to be any type, the consumer of the DML scene should validate this
static bool dml_parser_array(dml_parser *parser, dml_document *document, size_t array_value_idx)
{
    if (!dml_parser_consume(parser, TOK_LEFT_SQUARE_BRACKET)) {
        dml_parser_error_context(parser, "expected '[' to begin array", dml_parser_peek(parser));
        return false;
    }

    dml_value *value = &document->value_pool[array_value_idx];
    DLB_ASSERT(value->type == DML_VALUE_ARRAY);
    dml_array *array = &value->data.as_array;
    FILL_DEBUG_SYMBOL(array->dbg_symbol, dml_parser_previous(parser));

    bool first_element = true;
    size_t *values = 0;
    while (!dml_parser_check(parser, TOK_RIGHT_SQUARE_BRACKET) && !dml_parser_eof(parser)) {
        if (!first_element && !dml_parser_consume(parser, TOK_COMMA)) {
            dlb_vec_free(values);
            dml_parser_error_context(parser, "expected ',' between array values", dml_parser_peek(parser));
            return false;
        }

        size_t value_idx = dlb_vec_len(document->value_pool);
        dlb_vec_push(values, value_idx);
        dlb_vec_alloc(document->value_pool);
        if (!dml_parser_value(parser, document, value_idx)) {
            dlb_vec_free(values);
            return false;
        }
        first_element = false;
    }

    if (!dml_parser_consume(parser, TOK_RIGHT_SQUARE_BRACKET)) {
        dml_parser_error_context(parser, "expected ']' to end array", dml_parser_peek(parser));
        return false;
    }

    dlb_vec_push(dbg_array_pointers, values);

    value = &document->value_pool[array_value_idx];
    DLB_ASSERT(value->type == DML_VALUE_ARRAY);
    array = &value->data.as_array;
    array->values = values;
    return true;
}

static bool dml_parser_literal(dml_parser *parser, dml_literal *literal)
{
    bool result = false;
    if (dml_parser_consume(parser, TOK_NULL)) {
        literal->type = DML_LITERAL_NULL;
        FILL_DEBUG_SYMBOL(literal->dbg_symbol, dml_parser_previous(parser));
        result = true;
    } else if (dml_parser_consume(parser, TOK_TRUE)) {
        literal->type = DML_LITERAL_BOOL;
        literal->data.as_bool = true;
        FILL_DEBUG_SYMBOL(literal->dbg_symbol, dml_parser_previous(parser));
        result = true;
    } else if (dml_parser_consume(parser, TOK_FALSE)) {
        literal->type = DML_LITERAL_BOOL;
        literal->data.as_bool = false;
        FILL_DEBUG_SYMBOL(literal->dbg_symbol, dml_parser_previous(parser));
        result = true;
    } else if (dml_parser_consume(parser, TOK_NUMBER)) {
        // TODO: int vs float vs double (signed vs. unsigned)?
        dml_token *prev = dml_parser_previous(parser);
        literal->type = DML_LITERAL_FLOAT;
        literal->data.as_float = prev->literal.as_float;
        FILL_DEBUG_SYMBOL(literal->dbg_symbol, prev);
        result = true;
    } else if (dml_parser_consume(parser, TOK_STRING)) {
        dml_token *prev = dml_parser_previous(parser);
        literal->type = DML_LITERAL_STRING;
        literal->data.as_string = prev->literal.as_string;
        FILL_DEBUG_SYMBOL(literal->dbg_symbol, prev);
        result = true;
    }
    return result;
}

void dml_parser_parse(dml_parser *parser, dml_document *document)
{
    dlb_vec_reserve(document->field_pool, 256);
    dlb_vec_reserve(document->value_pool, 256);

    // NOTE: First value is document root
    dlb_vec_alloc(document->value_pool);
    dml_parser_value(parser, document, 0);

    // TODO(cleanup): temp validate
    for (size_t i = 0; i < dlb_vec_len(document->value_pool); i++) {
        if (document->value_pool[i].type == DML_VALUE_ARRAY &&
            document->value_pool[i].data.as_array.values == 0) {
            DLB_ASSERT(!"fuck");
        }
    }
}