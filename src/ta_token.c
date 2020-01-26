#include "ta_token.h"
#include "ta_file.h"
#include "ta_parse.h"
#include "ta_symbol.h"
#include "ta_schema.h"
#include "ta_scene.h"
#include "dlb/dlb_vector.h"

#define PANIC_HEADER "%s:%llu:%llu: error: "
#define FILE_POS_ARGS scene->filename, tok->file_pos.line, tok->file_pos.column
#define OPEN_VS_CODE() debug_open_in_vs_code(FILE_POS_ARGS)
#define BAD_TOKEN() bad_token(scene, tok, \
    ta_schema_field_type_str(stack[sp - (array > 0)].type), \
    (stack[sp - (array > 0)].array_len > 0 ? " (array)" : ""), \
    (stack[sp - (array > 0)].is_union_type > 0 ? " (union)" : ""))

static const char *token_type_str(token_type type)
{
    switch (type) {
        case TOKEN_UNKNOWN:        return "????????";
        case TOKEN_EOF:            return "EOF";
        case TOKEN_WHITESPACE:     return "WHITESPACE";
        case TOKEN_NEWLINE:        return "NEWLINE";
        case TOKEN_INDENT:         return "INDENT";
        case TOKEN_COMMENT:        return "COMMENT";
        case TOKEN_IDENTIFIER:     return "IDENTIFIER";
        case TOKEN_NULL:           return "NULL";
        case TOKEN_BOOL:           return "BOOL";
        case TOKEN_INT:            return "INT";
        case TOKEN_FLOAT:          return "FLOAT";
        case TOKEN_STRING:         return "STRING";
        case TOKEN_ARRAY_START:    return "ARRAY_START";
        case TOKEN_ARRAY_END:      return "ARRAY_END";
        case TOKEN_OBJECT_START:   return "OBJECT_START";
        case TOKEN_OBJECT_END:     return "OBJECT_END";
        case TOKEN_LIST_SEPARATOR: return "LIST_SEPARATOR";
        default: DLB_ASSERT(!"Unknown token type");  return 0;
    }
};
static token *token_read(ta_file *f, token **tokens)
{
    token *tok = dlb_vec_alloc(*tokens);
    tok->file_pos = f->pos;
    char c = ta_file_peek(f);
    switch(c) {
        case EOF:
        {
            tok->type = TOKEN_EOF;
            break;
        }
        case ' ':
        {
            ta_file_expect_char(f, C_WHITESPACE, 1);
            token_type prev_token_type = TOKEN_UNKNOWN;
            size_t tokens_len = dlb_vec_len(*tokens);
            if (tokens_len > 1) {
                prev_token_type = (tok - 1)->type;
            }
            if (prev_token_type == TOKEN_NEWLINE ||
                prev_token_type == TOKEN_INDENT)
            {
                ta_file_expect_char(f, C_WHITESPACE, 1);
                tok->type = TOKEN_INDENT;
            } else {
                tok->type = TOKEN_WHITESPACE;
            }
            break;
        }
        case '\n':
        {
            tok->type = TOKEN_NEWLINE;
            ta_file_expect_char(f, C_NEWLINE, 1);
            break;
        }
        case '#':
        {
            tok->type = TOKEN_COMMENT;
            ta_file_expect_char(f, C_COMMENT_START, 1);
            char buf[MAX_COMMENT_LEN + 1] = { 0 };
            int len = 0;
            ta_file_read(f, buf, MAX_COMMENT_LEN, C_COMMENT, C_COMMENT_END, &len);
            tok->length = len;
            // NOTE: Allow empty comments ('#' on a line by itself)
            if (tok->length) {
                tok->value.string = ta_symbol_intern(buf, len);
            }
            break;
        }
        case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g':
        case 'h': case 'i': case 'j': case 'k': case 'l': case 'm': case 'n':
        case 'o': case 'p': case 'q': case 'r': case 's': case 't': case 'u':
        case 'v': case 'w': case 'x': case 'y': case 'z':
        case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
        case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
        case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
        case 'V': case 'W': case 'X': case 'Y': case 'Z':
        {
            char buf[MAX_IDENT_LEN + 1] = { 0 };
            int len = 0;
            ta_file_read(f, buf, MAX_IDENT_LEN, C_IDENT, 0, &len);
            tok->length = len;
            tok->value.string = ta_symbol_intern(buf, len);
            if (ta_file_allow_char(f, C_IDENT_END, 1)) {
                tok->type = TOKEN_IDENTIFIER;
            } else if (tok->value.string == SYM_NULL) {
                tok->type = TOKEN_NULL;
            } else if (tok->value.string == SYM_TRUE) {
                tok->type = TOKEN_BOOL;
                tok->value.as_bool = true;
            } else if (tok->value.string == SYM_FALSE) {
                tok->type = TOKEN_BOOL;
                tok->value.as_bool = false;
            } else {
                PANIC_FILE(f, "Expected : after identifier '%s'\n", tok->value.string);
            }
            break;
        }
        case '+': case '-': case '0': case '1': case '2':case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
        {
            char buf[MAX_NUMBER_LEN + 1] = { 0 };
            int len = 0;
            int read = 0;
            char next = ta_file_peek(f);
            if (next == '0') {
                buf[len++] = ta_file_char(f);
                next = ta_file_peek(f);
                if (next == 'x') {
                    tok->type = TOKEN_INT;
                    buf[len++] = ta_file_char(f);
                    next = ta_file_read(f, buf + len, 8, C_NUMBER_HEX, 0, &read);
                    len += read;
                    ta_file_allow_char(f, C_WHITESPACE, 0);
                    if (ta_file_allow_char(f, "(", 1)) {
                        ta_file_read(f, 0, 0, 0, ")", 0);
                        ta_file_expect_char(f, ")", 1);
                    }
                } else if (next == 'b') {
                    tok->type = TOKEN_INT;
                    buf[len++] = ta_file_char(f);
                    ta_file_read(f, buf + len, 32, C_NUMBER_BINARY, 0, &read);
                    len += read;
                }
            }
            if (tok->type == TOKEN_UNKNOWN) {
                ta_file_read(f, buf, 1, C_NUMBER_SIGN, 0, &read);
                len += read;
                ta_file_read(f, buf + len, MAX_NUMBER_LEN - len, C_NUMBER_INT, 0, &read);
                len += read;
                next = ta_file_peek(f);
                if (next == '.') {
                    tok->type = TOKEN_FLOAT;
                    ta_file_read(f, buf + len, MAX_NUMBER_LEN - len,
                        C_NUMBER_FLOAT, 0, &read);
                    len += read;
                } else {
                    tok->type = TOKEN_INT;
                }
            }
            switch (tok->type) {
                case TOKEN_INT: {
                    tok->value.as_int = parse_int(buf);
                    break;
                } case TOKEN_FLOAT: {
                    tok->value.as_float = parse_float(buf);
                    break;
                } default: {
                    DLB_ASSERT(!"Token type could not be resolved");
                }
            }
            break;
        }
        case '"':
        {
            tok->type = TOKEN_STRING;
            ta_file_expect_char(f, "\"", 1);
            char buf[MAX_STRING_LEN + 1] = { 0 };
            int len = 0;
            char delim = 0;
            int read = 0;
            do {
                delim = ta_file_read(f, buf + len, MAX_STRING_LEN - len,
                    C_STRING, "\\\"", &read);
                len += read;
                if (delim == '\\') {
                    buf[len++] = ta_file_char_escaped(f);
                }
            } while (delim == '\\');
            ta_file_expect_char(f, "\"", 1);
            tok->length = len;
            tok->value.string = len ? ta_symbol_intern(buf, len) : 0;
            break;
        }
        case '[':
        {
            tok->type = TOKEN_ARRAY_START;
            ta_file_expect_char(f, C_ARRAY_START, 1);
            break;
        }
        case ']':
        {
            tok->type = TOKEN_ARRAY_END;
            ta_file_expect_char(f, C_ARRAY_END, 1);
            break;
        }
        case '{':
        {
            tok->type = TOKEN_OBJECT_START;
            ta_file_expect_char(f, C_OBJECT_START, 1);
            break;
        }
        case '}':
        {
            tok->type = TOKEN_OBJECT_END;
            ta_file_expect_char(f, C_OBJECT_END, 1);
            break;
        }
        case ',':
        {
            tok->type = TOKEN_LIST_SEPARATOR;
            ta_file_expect_char(f, C_LIST_SEPARATOR, 1);
            break;
        }
        default:
        {
            PANIC_FILE(f, "I don't know what's going on.. weird tokens bro.\n");
        }
    }
    return tok;
}
token *tokenize(ta_file *f)
{
    token *tokens = 0;
    while (token_read(f, &tokens)->type != TOKEN_EOF) {}
    return tokens;
}
void tokens_print(FILE *f, token *tokens)
{
    dlb_vec_each(token *, tok, tokens) {
        switch (tok->type) {
            case TOKEN_EOF: {
                break;
            } case TOKEN_WHITESPACE: {
                fprintf(f, " ");
                break;
            } case TOKEN_NEWLINE: {
                fprintf(f, "\n");
                break;
            } case TOKEN_INDENT: {
                fprintf(f, "  ");
                break;
            }
            case TOKEN_COMMENT: case TOKEN_IDENTIFIER: case TOKEN_STRING:
            case TOKEN_NULL:
            {
                if (tok->type == TOKEN_COMMENT) {
                    fprintf(f, "#");
                } else if (tok->type == TOKEN_STRING) {
                    fprintf(f, "\"");
                }

                if (tok->length && tok->value.string) {
                    fprintf(f, "%.*s", tok->length, tok->value.string);
                }

                if (tok->type == TOKEN_IDENTIFIER) {
                    fprintf(f, ":");
                } else if (tok->type == TOKEN_STRING) {
                    fprintf(f, "\"");
                }
                break;
            } case TOKEN_BOOL: {
                fprintf(f, tok->value.as_bool ? "true" : "false");
                break;
            } case TOKEN_INT: {
                fprintf(f, "%d", tok->value.as_int);
                break;
            } case TOKEN_FLOAT: {
                fprintf(f, "%f", tok->value.as_float);
                break;
            } case TOKEN_ARRAY_START: {
                fprintf(f, "[");
                break;
            } case TOKEN_ARRAY_END: {
                fprintf(f, "]");
                break;
            } case TOKEN_OBJECT_START: {
                fprintf(f, "{");
                break;
            } case TOKEN_OBJECT_END: {
                fprintf(f, "}");
                break;
            } case TOKEN_LIST_SEPARATOR: {
                fprintf(f, ",");
                break;
            } default: {
                DLB_ASSERT(!"Unexpected token type, don't know how to print");
            }
        }
    }
    fprintf(f, "\n");
    fflush(f);
}
void tokens_print_debug(FILE *f, token *tokens)
{
    dlb_vec_each(token *, tok, tokens) {
        fprintf(f, "%-16s", token_type_str(tok->type));
        switch (tok->type) {
            case TOKEN_EOF:
            {
                break;
            }
            case TOKEN_WHITESPACE:
            case TOKEN_NEWLINE:
            case TOKEN_INDENT:
            {
                break;
            }
            case TOKEN_COMMENT: case TOKEN_IDENTIFIER: case TOKEN_STRING:
            case TOKEN_NULL:
            {
                if (tok->type == TOKEN_COMMENT) {
                    fprintf(f, "#");
                } else if (tok->type == TOKEN_STRING) {
                    fprintf(f, "\"");
                }

                if (tok->length && tok->value.string) {
                    fprintf(f, "%.*s", tok->length, tok->value.string);
                }

                if (tok->type == TOKEN_IDENTIFIER) {
                    fprintf(f, ":");
                } else if (tok->type == TOKEN_STRING) {
                    fprintf(f, "\"");
                }
                break;
            } case TOKEN_BOOL: {
                fprintf(f, tok->value.as_bool ? "true" : "false");
                break;
            } case TOKEN_INT: {
                fprintf(f, "%d", tok->value.as_int);
                break;
            } case TOKEN_FLOAT: {
                fprintf(f, "%f", tok->value.as_float);
                break;
            }
            case TOKEN_ARRAY_START: case TOKEN_ARRAY_END:
            case TOKEN_OBJECT_START: case TOKEN_OBJECT_END:
            case TOKEN_LIST_SEPARATOR:
            {
                break;
            } default: {
                DLB_ASSERT(!"Unexpected token type, don't know how to print");
            }
        }
        fprintf(f, "\n");
    }
    fprintf(f, "\n");
    fflush(f);
}
static void debug_open_in_vs_code(const char *filename, u64 line, u64 column)
{
    DLB_ASSERT(filename);
    char buf[512] = { 0 };
    snprintf(buf, sizeof(buf) - 1, "start /b code -g %s:%llu:%llu", filename, line, column+1);
    system(buf);
}
static void bad_token(ta_scene *scene, token *tok, const char *typ, const char *arr, const char *uni) {
    OPEN_VS_CODE();
    PANIC(PANIC_HEADER "expected %s%s%s, found (%s) instead.\n",
        FILE_POS_ARGS, typ, arr, uni, token_type_str(tok->type)
    )
}
void tokens_parse(ta_scene *scene, token *tokens)
{
    struct {
        int indent;
        u32 resource_id; // 0 = not a resource (i.e. field)
        ta_schema_field_type type;
        size_t array_len;   // 0 = not array, 1 = vector, >1 = fixed array size
        size_t array_elem;  // Current element of array we're writing to
        const char *name;
        size_t index;       // pool index (resource_data)
        void *ptr;
        size_t size;
        bool is_union_type;
        bool is_union;
        int union_type;
    } stack[8] = { 0 };

    int indent = 0;  // Current line indent counter
    bool expect_array_start = false;

    //int level = 0;   // Current level of indentation
    int sp = 0;          // "Stack pointer" index into stack
    int braces = 0;      // Current level of curly braces
    int array = 0;       // Current level of square brackets

    dlb_vec_each(token *, tok, tokens) {
        switch (tok->type) {
            case TOKEN_EOF: {
                break;
            } case TOKEN_WHITESPACE: {
                break;
            } case TOKEN_NEWLINE: {
                indent = 0;
                break;
            } case TOKEN_INDENT: {
                indent++;
                break;
            } case TOKEN_COMMENT: {
                break;
            } case TOKEN_IDENTIFIER: {
                if (expect_array_start) {
                    BAD_TOKEN();
                }
                if (braces) {
                    stack[sp].indent = stack[sp-1].indent + 1;
                } else {
                    for (int i = sp; i >= 0; i--) {
                        if (indent >= stack[i].indent) {
                            break;
                        }
                        DLB_ASSERT(sp);
                        stack[sp].indent = 0;
                        stack[sp].type = 0;  // Cleanup: Easier debug
                        sp--;
                    }
                    stack[sp].indent = indent;
                }

                if (array) {
                    DLB_ASSERT(stack[sp-1].ptr);
                    // NOTE: Commas allowed but not required when reading in objects
                    if (stack[sp-1].array_len) {
                        void **arr = stack[sp-1].ptr;

                        stack[sp].type = stack[sp-1].type;
                        stack[sp].array_len = 0;
                        stack[sp].array_elem = 0;
                        if (stack[sp-1].array_len == 1) {
                            stack[sp].name = "[VECTOR_ELEMENT]";
                            stack[sp].ptr = dlb_vec_alloc_size(*arr, stack[sp-1].size);
                        } else {
                            stack[sp].name = "[ARRAY_ELEMENT]";
                            stack[sp].ptr = (u8 *)arr + (stack[sp-1].array_elem * stack[sp-1].size);
                            stack[sp-1].array_elem++;
                        }
                        stack[sp].size = stack[sp-1].size;
                        stack[sp].is_union_type = false;
                        stack[sp].is_union = false;
                        stack[sp].union_type = 0;
                        sp++;
                    }
                }

                if (sp) {
                    ta_schema_field *field = ta_schema_field_find(stack[sp-1].type,
                        tok->value.string);
                    if (!field) {
                        OPEN_VS_CODE();
                        PANIC(PANIC_HEADER "unexpected field '%s' in '%s' (%s)\n",
                            FILE_POS_ARGS,
                            tok->value.string,
                            stack[sp-1].name,
                            ta_schema_field_type_str(stack[sp-1].type));
                    }
                    if (field->in_union) {
                        if (!stack[sp-1].is_union) {
                            OPEN_VS_CODE();
                            PANIC(PANIC_HEADER "unexpected union field '%s' before union type field found in '%s' (%s)\n",
                                FILE_POS_ARGS,
                                tok->value.string,
                                stack[sp-1].name,
                                ta_schema_field_type_str(stack[sp-1].type));
                        }
                        if (field->union_type != stack[sp-1].union_type) {
                            OPEN_VS_CODE();
                            PANIC(PANIC_HEADER "unexpected union field '%s' in %s (%s) with union_type = %d\n",
                                FILE_POS_ARGS,
                                tok->value.string,
                                stack[sp-1].name,
                                ta_schema_field_type_str(stack[sp-1].type),
                                stack[sp-1].union_type);
                        }
                    }
                    DLB_ASSERT(field->type);
                    if (field->type == TYP_FONT) {
                        DLB_ASSERT(1);
                    }
                    stack[sp].type = field->type;
                    stack[sp].array_len = field->array_len;
                    stack[sp].array_elem = 0;
                    stack[sp].name = field->name;
                    stack[sp].ptr = ((u8 *)stack[sp-1].ptr + field->offset);
                    stack[sp].size = field->size;
                    stack[sp].is_union_type = field->is_union_type;
                    stack[sp].is_union = false;
                    stack[sp].union_type = 0;
                } else {
                    ta_schema *schema = ta_schema_find_by_name(tok->value.string, tok->length);
                    if (!schema) {
                        OPEN_VS_CODE();
                        PANIC(PANIC_HEADER "unexpected type name '%s'\n",
                            FILE_POS_ARGS,
                            tok->value.string);
                    }
                    ta_resource_type res_type = typ_to_res(schema->type);
                    if (res_type == RES_COUNT) {
                        OPEN_VS_CODE();
                        PANIC(PANIC_HEADER "type '%s' is not a scene-level resource type.\n",
                            FILE_POS_ARGS,
                            tok->value.string);
                    }
                    DLB_ASSERT(schema->size);
                    DLB_ASSERT(schema->name == tok->value.string);
                    stack[sp].type = schema->type;
                    stack[sp].array_len = 0;
                    stack[sp].array_elem = 0;
                    stack[sp].name = tok->value.string;
                    stack[sp].index = dlb_vec_len(scene->resource_data[res_type]);
                    stack[sp].ptr = dlb_vec_alloc_size(scene->resource_data[res_type], schema->size);
                    stack[sp].size = schema->size;
                    stack[sp].is_union_type = false;
                    stack[sp].is_union = false;
                    stack[sp].union_type = 0;
                }

                if (stack[sp].array_len) {
                    array++;
                    expect_array_start = true;
                }

                if (!braces && !array && stack[sp].type < TYP_COUNT) {
                    sp++;
                }
                break;
            } case TOKEN_NULL: {
                if (expect_array_start || stack[sp].type != ATOM_STRING) {
                    BAD_TOKEN();
                }
                break;
            } case TOKEN_BOOL: {
                if (expect_array_start || stack[sp].type != ATOM_BOOL) {
                    BAD_TOKEN();
                }
                int *fp = stack[sp].ptr;
                *fp = tok->value.as_bool;
                break;
            } case TOKEN_INT: {
                if (expect_array_start || (
                    stack[sp].type != ATOM_UINT8 &&
                    stack[sp].type != ATOM_INT &&
                    stack[sp].type != ATOM_UINT &&
                    stack[sp].type != ATOM_ENUM &&
                    stack[sp].type != ATOM_FLOAT))
                {
                    BAD_TOKEN();
                }

                if (stack[sp].type == ATOM_UINT8) {
                    DLB_ASSERT(tok->value.as_int <= UINT8_MAX);
                    if (stack[sp].array_len) {
                        // NOTE: Fixed-sized atomic arrays not supported
                        DLB_ASSERT(stack[sp].array_len == 1);
                        DLB_ASSERT(stack[sp].ptr);
                        u8 **arr = stack[sp].ptr;
                        dlb_vec_push(*arr, (u8)tok->value.as_int);
                    } else {
                        u8 *fp = stack[sp].ptr;
                        *fp = (u8)tok->value.as_int;
                    }
                } else {
                    if (stack[sp].array_len) {
                        // NOTE: Fixed-sized atomic arrays not supported
                        DLB_ASSERT(stack[sp].array_len == 1);
                        DLB_ASSERT(stack[sp].ptr);
                        u32 **arr = stack[sp].ptr;
                        dlb_vec_push(*arr, tok->value.as_int);
                    } else {
                        u32 *fp = stack[sp].ptr;
                        *fp = tok->value.as_int;
                    }

                    if (stack[sp].is_union_type) {
                        stack[sp-1].is_union = true;
                        stack[sp-1].union_type = *(u32 *)stack[sp].ptr;
                    }
                }
                break;
            } case TOKEN_FLOAT: {
                if (expect_array_start || stack[sp].type != ATOM_FLOAT) {
                    BAD_TOKEN();
                }

                if (stack[sp].array_len) {
                    // NOTE: Fixed-sized atomic arrays not supported
                    DLB_ASSERT(stack[sp].array_len == 1);
                    DLB_ASSERT(stack[sp].ptr);
                    float **arr = stack[sp].ptr;
                    dlb_vec_push(*arr, tok->value.as_float);
                } else {
                    float *fp = stack[sp].ptr;
                    *fp = tok->value.as_float;
                }
                break;
            } case TOKEN_STRING: {
                if (expect_array_start || stack[sp].type != ATOM_STRING) {
                    BAD_TOKEN();
                }
                DLB_ASSERT(sp > 0);

                if (stack[sp].array_len) {
                    // NOTE: Fixed-sized atomic arrays not supported
                    DLB_ASSERT(stack[sp].array_len == 1);
                    DLB_ASSERT(stack[sp].ptr);
                    const char ***arr = stack[sp].ptr;
                    dlb_vec_push(*arr, tok->value.string);
                } else {
                    const char **fp = stack[sp].ptr;
                    *fp = tok->value.string;
                    if (stack[sp].name == SYM_NAME) {
                        // NOTE: Ignore "name" fields for non-resource types
                        ta_resource_type res_type = typ_to_res(stack[sp-1].type);
                        if (res_type < RES_COUNT) {
                            ta_resource *resource = stack[sp-1].ptr;
                            resource->index = stack[sp-1].index;
                            DLB_ASSERT(resource->name == tok->value.string);  // TODO: Cleanup

                            u32 hash = dlb_murmur3(SYM32(resource->name));
                            dlb_index_insert(&scene->index_by_name[res_type], hash,
                                resource->index);
                        } else {
                            DLB_ASSERT(1);
                        }
                    } else if (stack[sp].name == SYM_ENTITY_NAME) {
                        // NOTE: Ignore "entity_name" fields for non-component types
                        ta_resource_type res_type = typ_to_res(stack[sp-1].type);
                        if (res_type < RES_COMP_COUNT) {
                            ta_component *comp = stack[sp-1].ptr;
                            comp->index = stack[sp-1].index;
                            DLB_ASSERT(comp->entity_name == tok->value.string);  // TODO: Cleanup

                            u32 hash = dlb_murmur3(SYM32(comp->entity_name));
                            dlb_index_insert(&scene->index_by_entity[res_type], hash,
                                comp->index);
                        } else {
                            DLB_ASSERT(1);
                        }
                    }
                }
                break;
            } case TOKEN_ARRAY_START: {
                if (!expect_array_start) {
                    BAD_TOKEN();
                }
                expect_array_start = false;
                if (stack[sp].type < TYP_COUNT) {
                    sp++;
                } else {
                    if (stack[sp].type != ATOM_STRING &&
                        stack[sp].type != ATOM_FLOAT &&
                        stack[sp].type != ATOM_UINT8 &&
                        stack[sp].type != ATOM_INT &&
                        stack[sp].type != ATOM_UINT)
                    {
                        BAD_TOKEN();
                    }
                }
                break;
            } case TOKEN_LIST_SEPARATOR: {
                if (expect_array_start) {
                    BAD_TOKEN();
                }
                break;
            } case TOKEN_ARRAY_END: {
                if (expect_array_start) {
                    BAD_TOKEN();
                }
                DLB_ASSERT(array);
                array--;
                DLB_ASSERT(sp);
                if (stack[sp].type < TYP_COUNT) {
                    stack[sp].type = 0;  // Cleanup: Easier debug
                    sp--;
                }
                break;
            } case TOKEN_OBJECT_START: {
                if (expect_array_start) {
                    BAD_TOKEN();
                }
                DLB_ASSERT(sp > 0);
                DLB_ASSERT(stack[sp-1].type >= 0);
                DLB_ASSERT(stack[sp-1].type < TYP_COUNT);
                braces++;
                break;
            } case TOKEN_OBJECT_END: {
                if (expect_array_start) {
                    BAD_TOKEN();
                }
                DLB_ASSERT(braces);
                braces--;
                DLB_ASSERT(sp);
                stack[sp].type = 0;  // Cleanup: Easier debug
                sp--;
                break;
            } default: {
                BAD_TOKEN();
            }
        }
    }

#undef BAD_TOKEN
}
