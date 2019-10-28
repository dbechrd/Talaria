#include "ta_scene.h"
#include "ta_parse.h"
#include "ta_symbol.h"
#include "ta_camera.h"
#include "ta_material.h"
#include "ta_mesh_group.h"
#include "ta_shader.h"
#include "ta_texture.h"
#include "ta_button.h"
#include "ta_font.h"
#include "ta_rigid_body.h"
#include "ta_light.h"
#include "ta_log.h"
#include "ta_game.h"
#include "ta_window.h"
#include "ta_file.h"
#include "ta_audio.h"
#include "ta_primitive.h"
#include "ta_editor.h"
#include "ta_position.h"
#include "ta_entity.h"
#include "ta_model.h"
#include "ta_buffer.h"
#include "dlb/dlb_vector.h"
#include "dlb/dlb_index.h"
#include <stdlib.h>
#include <float.h>

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
    token_type type;
    u32 length;
    union {
        bool as_bool;
        s32 as_int;
        float as_float;
        s32 *int_array;
        float *float_array;
        const char *string;
    } value;
    ta_file_pos file_pos;
} token;

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
            int tokens_len = dlb_vec_len(*tokens);
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
            tok->value.string = ta_symbol_intern(buf, len);
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
                    tok->type = TOKEN_FLOAT;
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

static token *tokenize(ta_file *f)
{
    token *tokens = 0;
    while (token_read(f, &tokens)->type != TOKEN_EOF) {}
    return tokens;
}

static void tokens_print(FILE *f, token *tokens)
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

static void tokens_print_debug(FILE *f, token *tokens)
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

#define PANIC_HEADER "%s:%zd:%zd: error: "
#define FILE_POS_ARGS scene->filename, tok->file_pos.line, tok->file_pos.column

void debug_open_in_vs_code(const char *filename, int line, int column)
{
    DLB_ASSERT(filename);
    char buf[512] = { 0 };
    snprintf(buf, sizeof(buf) - 1, "start /b code -g %s:%d:%d", filename, line, column+1);
    system(buf);
}
#define OPEN_VS_CODE() debug_open_in_vs_code(FILE_POS_ARGS)

void bad_token(ta_scene *scene, token *tok, const char *typ, const char *arr, const char *uni) {
    OPEN_VS_CODE();
    PANIC(PANIC_HEADER "expected %s%s%s, found (%s) instead.\n",
        FILE_POS_ARGS, typ, arr, uni, token_type_str(tok->type)
    )
}
#define BAD_TOKEN() bad_token(scene, tok, \
    ta_schema_field_type_str(stack[sp - (array > 0)].type), \
    (stack[sp - (array > 0)].array_len > 0 ? " (array)" : ""), \
    (stack[sp - (array > 0)].is_union_type > 0 ? " (union)" : ""))

static void tokens_parse(ta_scene *scene, token *tokens)
{
    struct {
        int indent;
        u32 resource_id; // 0 = not a resource (i.e. field)
        ta_schema_field_type type;
        u32 array_len;   // 0 = not array, 1 = vector, >1 = fixed array size
        u32 array_elem;  // Current element of array we're writing to
        const char *name;
        u32 index;       // pool index (resource_data)
        void *ptr;
        u32 size;
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
                if (expect_array_start || (stack[sp].type != ATOM_UINT8 &&
                                           stack[sp].type != ATOM_INT &&
                                           stack[sp].type != ATOM_UINT &&
                                           stack[sp].type != ATOM_FLOAT &&
                                           stack[sp].type != ATOM_ENUM))
                {
                    BAD_TOKEN();
                }

#if 0
                // TODO: Need to cast *fp to u8 for this to work?
                //if (stack[sp].type == ATOM_UINT8) {
                //    DLB_ASSERT(1);
                //}

                switch (stack[sp].type) {
                    case ATOM_UINT8: {
                        u8 *fp = stack[sp].ptr;
                        *fp = tok->value.as_int;
                        break;
                    } case ATOM_INT: case ATOM_UINT: case ATOM_FLOAT:
                      case ATOM_ENUM:
                    {
                        int *fp = stack[sp].ptr;
                        *fp = tok->value.as_int;
                        break;
                    }
                }
#else
                int *fp = stack[sp].ptr;
                *fp = tok->value.as_int;
#endif
                if (stack[sp].is_union_type) {
                    stack[sp-1].is_union = true;
                    stack[sp-1].union_type = *fp;
                }
                break;
            } case TOKEN_FLOAT: {
                if (expect_array_start || stack[sp].type != ATOM_FLOAT) {
                    BAD_TOKEN();
                }
                float *fp = stack[sp].ptr;
                *fp = tok->value.as_float;
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

                            u32 hash = dlb_murmur3(SYM(resource->name));
                            dlb_index_insert(&scene->index_by_name[res_type], hash,
                                resource->index);
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
                    // NOTE: Only string vectors are supported for now
                    if (stack[sp].type != ATOM_STRING) {
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

static void scene_load_placeholders(ta_scene *scene)
{
    UNUSED(scene);
    // TODO: Fix fallback resources
#if 0
    // Fallback resources
    ta_texture *tex_albedo = ta_scene_alloc(scene, COMP_TEXTURE,
        INTERN("DEFAULT_TEXTURE_ALBEDO"));
    {
#if 0
        tex_albedo->path = INTERN("data/texture/default_1024_1024.png");
#else
        // Generate magenta/white grid pattern
        tex_albedo->width = 64;
        tex_albedo->height = 64;
        tex_albedo->channels = 3;
        tex_albedo->linear = true;
        u8 *albedo_pixels = 0;
        u32 bytes = tex_albedo->width * tex_albedo->height * tex_albedo->channels;
        dlb_vec_reserve(albedo_pixels, bytes);
        u8 toggle = 0;
        u8 toggle_width = 4;
        for (int y = 0; y < tex_albedo->height; y++) {
            if (y % toggle_width == 0) toggle = !toggle;
            for (int x = 0; x < tex_albedo->width; x++) {
                if (x % toggle_width == 0) toggle = !toggle;
                dlb_vec_push(albedo_pixels, 255);
                dlb_vec_push(albedo_pixels, toggle * 255);
                dlb_vec_push(albedo_pixels, 255);
            }
        }
        DLB_ASSERT(dlb_vec_len(albedo_pixels) == bytes);
        tex_albedo->pixels = albedo_pixels;
#endif
    }

    ta_texture *tex_metallic = ta_scene_alloc(scene, COMP_TEXTURE,
        INTERN("DEFAULT_TEXTURE_METALLIC"));
    {
#if 0
        tex_metallic->path = INTERN("data/texture/default_1024_1024.png");
#else
        tex_metallic->width = 1;
        tex_metallic->height = 1;
        tex_metallic->channels = 1;
        tex_metallic->linear = true;
        u8 *metallic = 0;
        dlb_vec_alloc(metallic);
        tex_metallic->pixels = metallic;
#endif
    }

    ta_material *material = ta_scene_alloc(scene, COMP_MATERIAL,
        INTERN("DEFAULT_MATERIAL"));
    // TODO: Hard-code default shader instead of hoping it's in the scene file
    material->shader_uid = INTERN("shader_mesh");
    material->texture_albedo_uid = tex_albedo->hnd.uid;
    material->texture_metallic_uid = tex_metallic->hnd.uid;

    ta_mesh_group *mesh_group = ta_scene_alloc(scene, COMP_MESH_GROUP,
        INTERN("DEFAULT_MESH_GROUP"));
    mesh_group->path = INTERN("data/mesh/default.obj");

    scene->components[COMP_MATERIAL][0] = material->hnd.uid;
    scene->components[COMP_MESH_GROUP][0] = mesh_group->hnd.uid;
#endif
}

void ta_scene_init(ta_scene *scene)
{
    DLB_ASSERT(scene->filename);
    if (!scene->name) {
        scene->name = scene->filename;  // TODO: Load name from scene file
    }

    // TODO(perf): Fine-tune reservations (e.g. scene header)
    // TODO(perf): This is a lot of back-to-back allocations, can we avoid?
    for (ta_resource_type type = 0; type < RES_COUNT; type++) {
        dlb_index_init(&scene->index_by_name[type], 128, 128);
    }
    scene_load_placeholders(scene);
}

// TODO: This should take a ta_buffer pointer. Load entire file into memory
//       and refactor all of the e.g. read_char and expect_char logic out from
//       ta_file into ta_buffer.
ta_scene *ta_scene_load(ta_file *file)
{
    ta_log_write(&tg_debug_log, "[Scene] Loading %s\n", file->filename);
    ta_scene *scene = dlb_calloc(1, sizeof(ta_scene));
    scene->filename = file->filename;
    scene->name = file->filename;  // TODO: Load name from scene file
    ta_scene_init(scene);

    // TODO: Reserve arrays based on scene header (which doesn't exist yet)
    //dlb_vec_reserve(scn->entities, 2);
    token *tokens = tokenize(file);

    //tokens_print(tg_debug_log->stream, tokens);
    //tokens_print_debug(tg_debug_log->stream, tokens);
    tokens_parse(scene, tokens);
    dlb_vec_free(tokens);

    tg_game.scene = scene;

    // Initialize resources
    for (ta_resource_type res_type = 0; res_type < RES_COUNT; ++res_type) {
        ta_schema_field_type schema_type = res_to_typ(res_type);
        if (tg_schemas[schema_type].init) {
            u32 size = tg_schemas[schema_type].size;
            void *pool = scene->resource_data[res_type];
            u8 *end = dlb_vec_end_size(pool, size);
            for (u8 *ptr = pool; ptr != end; ptr += size) {
                tg_schemas[schema_type].init(ptr);
            }
        }
    }

    ta_log_write(&tg_debug_log, "[Scene] Loaded successfully\n");
    return scene;
}

ta_scene *ta_scene_load_file(const char *filename)
{
    //ta_buffer *buffer = ta_file_read_all(filename);
    //ta_scene *scene = ta_scene_load(buffer);
    ta_file *file = ta_file_open(filename, FILE_READ);
    ta_scene *scene = ta_scene_load(file);
    return scene;
}

void ta_scene_free(ta_scene *scene)
{
    // Free resources
    for (ta_resource_type res_type = 0; res_type < RES_COUNT; ++res_type) {
        ta_schema_field_type schema_type = res_to_typ(res_type);
        if (tg_schemas[schema_type].free) {
            u32 size = tg_schemas[schema_type].size;
            void *pool = scene->resource_data[res_type];
            u8 *end = dlb_vec_end_size(pool, size);
            for (u8 *ptr = pool; ptr != end; ptr += size) {
                tg_schemas[schema_type].free(ptr);
            }
        }
        dlb_vec_free(scene->resource_data[res_type]);
        dlb_index_free(&scene->index_by_name[res_type]);
    }

    dlb_free(scene);
}

void ta_scene_save(ta_buffer *buffer)
{
    UNUSED(buffer);
    // TODO: Write scene to memory buffer
    DLB_ASSERT(0);
}

void ta_scene_save_file(ta_scene *scene, const char *filename)
{
    // TODO: Alloc dynamic buffer to write arbitrary data to
    //ta_buffer *buffer = ??
    ta_file *file = ta_file_open(filename, FILE_WRITE);
    ta_scene_print(scene, file->hnd);
    ta_file_close(file);
}

void ta_scene_print(ta_scene *scene, FILE *hnd)
{
    fprintf(hnd, "#-------------------------------------------------------------------------------\n");
    fprintf(hnd, "# [SCENE] %s\n", scene->name);
    for (ta_resource_type res_type = 0; res_type < RES_COUNT; ++res_type) {
        ta_schema_field_type schema_type = res_to_typ(res_type);
        fprintf(hnd, "#-------------------------------------------------------------------------------\n");
        fprintf(hnd, "# %s\n", ta_schema_field_type_str(schema_type));
        fprintf(hnd, "#-------------------------------------------------------------------------------\n");

        u32 size = tg_schemas[schema_type].size;
        void *pool = scene->resource_data[res_type];
        u8 *end = dlb_vec_end_size(pool, size);
        for (u8 *ptr = pool; ptr != end; ptr += size) {
            ta_schema_print(hnd, schema_type, ptr, 0, 0);
        }
    }
    fflush(hnd);
}

void *ta_scene_find_at(ta_scene *scene, ta_resource_type type, u32 index)
{
    DLB_ASSERT(scene);
    DLB_ASSERT(type >= 0 && type < RES_COUNT);

    ta_schema_field_type schema_type = res_to_typ(type);
    u32 size = tg_schemas[schema_type].size;
    void *resource = dlb_vec_index_size(scene->resource_data[type], index, size);
    return resource;
}

// If not found, returns NULL
void *ta_scene_find_by_name_try(ta_scene *scene, ta_resource_type type,
    const char *name)
{
    DLB_ASSERT(scene);
    DLB_ASSERT(name);

    ta_schema_field_type schema_type = res_to_typ(type);
    u32 size = tg_schemas[schema_type].size;

    u32 hash = dlb_murmur3(SYM(name));
    dlb_index *store = &scene->index_by_name[type];
    for (u32 i = dlb_index_first(store, hash); i != DLB_INDEX_EMPTY; i = dlb_index_next(store, i)) {
        ta_resource *res = dlb_vec_index_size(scene->resource_data[type], i, size);
        if (res->name == name) {
            DLB_ASSERT(res->index == i);
            return res;
        }
    }
    return 0;
}

// If not found, ASSERT
void *ta_scene_find_by_name(ta_scene *scene, ta_resource_type type,
    const char *name)
{
    void *resource = ta_scene_find_by_name_try(scene, type, name);
    DLB_ASSERT(resource);
    return resource;
}

// If not found, returns the first resource of the given type
void *ta_scene_find_by_name_or_default(ta_scene *scene, ta_resource_type type,
    const char *name)
{
    ta_schema_field_type schema_type = res_to_typ(type);
    u32 size = tg_schemas[schema_type].size;

    void *resource = ta_scene_find_by_name_try(scene, type, name);
    if (!resource) {
        resource = dlb_vec_index_size(scene->resource_data[type], 0, size);
    }
    return resource;
}

void *ta_scene_alloc(ta_scene *scene, ta_resource_type type, const char *name)
{
    DLB_ASSERT(scene);
    DLB_ASSERT(type < RES_COUNT);
    DLB_ASSERT(name);

    ta_schema_field_type schema_type = res_to_typ(type);
    u32 size = tg_schemas[schema_type].size;

    ta_resource *res = dlb_vec_alloc_size(scene->resource_data[type], size);
    res->index = dlb_vec_len(scene->resource_data[type]) - 1;
    res->name = name;

    dlb_index *store = &scene->index_by_name[type];
    u32 hash = dlb_murmur3(SYM(name));
    dlb_index_insert(store, hash, res->index);

    if (tg_schemas[schema_type].init) {
        tg_schemas[schema_type].init(res);
    }

    return res;
}

void ta_scene_destroy(ta_scene *scene, ta_resource_type type, const char *name)
{
    DLB_ASSERT(scene);
    // TODO: if type is a component type, find and update parent entity:
    // entity->components[type] = 0
    DLB_ASSERT(type >= RES_COMP_COUNT && type < RES_COUNT);

    ta_schema_field_type schema_type = res_to_typ(type);
    u32 size = tg_schemas[schema_type].size;

    // TODO: Find resource
    DLB_ASSERT(0);
    ta_resource *res = 0000000;
    if (tg_schemas[schema_type].free) {
        tg_schemas[schema_type].free(res);
    }

    // Remove name from index
    dlb_index *store = &scene->index_by_name[type];
    u32 hash = dlb_murmur3(SYM(name));
    // TODO: Find index
    DLB_ASSERT(0);
    u32 index = 0000000;
    dlb_index_delete(store, hash, index);

    // TODO: Remove data from pool
    DLB_ASSERT(0);
    //dlb_vec_delete(scene->resource_data[type], index);
}

void *ta_scene_component_add(ta_scene *scene, ta_resource_type type,
    const char *entity)
{
    DLB_ASSERT(scene);
    DLB_ASSERT(type >= 0 && type < RES_COMP_COUNT);

    // Prevent duplicates
    ta_component *component = ta_scene_component(scene, type, entity);
    DLB_ASSERT(!component);

    // TODO: Build better component name (or guarantee name == entity_name)
    component = ta_scene_alloc(scene, type, entity);
    DLB_ASSERT(component);
    component->entity_name = entity;
    return component;
}

void *ta_scene_component_try(ta_scene *scene, ta_resource_type type,
    const char *entity)
{
    DLB_ASSERT(scene);
    DLB_ASSERT(entity);
    DLB_ASSERT(type >= 0 && type < RES_COMP_COUNT);

    void *component = ta_scene_find_by_name_try(scene, type, entity);
    return component;
}

void *ta_scene_component(ta_scene *scene, ta_resource_type type,
    const char *entity)
{
    DLB_ASSERT(scene);
    DLB_ASSERT(entity);
    DLB_ASSERT(type >= 0 && type < RES_COMP_COUNT);

    void *component = ta_scene_component_try(scene, type, entity);
    DLB_ASSERT(component);
    return component;
}

static ta_rigid_body_pair *collision_broadphase(ta_scene *scene, double dt)
{
    // Box2D supports 16 collision categories. For each fixture you can
    // specify which category it belongs to. You also specify what other
    // categories this fixture can collide with.
    //
    //   if ((categoryA & maskB) != 0 && (categoryB & maskA) != 0)
    //
    // Collision groups let you specify an integral group index. You can
    // have all fixtures with the same group index always collide
    // (positive index) or never collide (negative index). Group indices
    // are usually used for things that are somehow related, like the
    // parts of a bicycle.
    //
    // Collisions between fixtures of different group indices are
    // filtered according the category and mask bits. In other words,
    // group filtering has higher precedence than category filtering.
    //
    // - A fixture on a static body can only collide with a dynamic
    //   body.
    // - A fixture on a kinematic body can only collide with a dynamic
    //   body.
    // - Fixtures on the same body never collide with each other.
    // - You can optionally enable/disable collision between fixtures on
    //   bodies connected by a joint.
    //
    // Sensor: Fixture which only detects collision, no response.
    // -----------------------------------------------------------------
    // Depth-first traversal of AABB tree to find islands. Put islands
    // to sleep when all objects in island are resting. Wake up when
    // anything interacts or applies a force to any body in the island.

    UNUSED(dt);

    static ta_rigid_body_pair *pairs = 0;

    ta_rigid_body *rigid_bodies = scene->resource_data[RES_COMP_RIGID_BODY];
    dlb_vec_each(ta_rigid_body *, a, rigid_bodies) {
        dlb_vec_range(ta_rigid_body *, b, a + 1, dlb_vec_end(rigid_bodies)) {
            // Don't let entities collide with themselves
            if (a->entity_name == b->entity_name) {
                continue;
            }
            if (ta_aabb_v_aabb(&a->aabb, &b->aabb, 0))
            {
                ta_rigid_body_pair *pair = dlb_vec_alloc(pairs);
                pair->a = a;
                pair->b = b;
            }
        }
    }

    return pairs;
}

static ta_manifold *detect_collisions(ta_rigid_body_pair *pairs, double dt)
{
    UNUSED(dt);

    static ta_manifold *manifolds = 0;

    ta_manifold manifold;
    dlb_vec_each(ta_rigid_body_pair *, pair, pairs) {
        if (ta_rigid_body_intersect(pair->a, pair->b, &manifold)) {
            ta_manifold *m = dlb_vec_alloc(manifolds);
            *m = manifold;
        }
    }

    return manifolds;
}

void ta_scene_update(ta_scene *scene, float dt)
{
    // https://www.toptal.com/game/video-game-physics-part-type-an-introduction-to-rigid-body-dynamics

    // Simulate rigid bodies
    dlb_vec_each(ta_rigid_body *, body, scene->resource_data[RES_COMP_RIGID_BODY]) {
        ta_rigid_body_update(body, dt);
    }

    // Collision broad phase
    ta_rigid_body_pair *pairs = collision_broadphase(scene, dt);
    if (pairs) {
        // Collision narrow phase
        ta_manifold *manifolds = detect_collisions(pairs, dt);
        dlb_vec_each(ta_manifold *, manifold, manifolds) {
            // Collision resolution
            ta_rigid_body_resolve_collision(manifold);
            ta_rigid_body_positional_correction(manifold);
        }
        dlb_vec_zero(manifolds);
        dlb_vec_zero(pairs);
    }

    // Update positions
    dlb_vec_each(ta_position *, position, scene->resource_data[RES_COMP_POSITION]) {
        position->transform_prev = position->transform;
    }
    dlb_vec_each(ta_rigid_body *, body, scene->resource_data[RES_COMP_RIGID_BODY]) {
        ta_position *position = ta_scene_component(scene, RES_COMP_POSITION,
            body->entity_name);
        position->transform.position = body->position;
        position->transform.orientation = body->orientation;
    }

    // Update buttons
    dlb_vec_each(ta_e_button *, button, scene->resource_data[RES_COMP_BUTTON]) {
        e_button_update(button);
    }

#if 0
    dlb_vec_each(ta_entity *, entity, scene->pools[TYP_BUTTON]) {
        ta_node_update(entity);
    }
#endif
}

void ta_scene_shadow_pass(ta_scene *scene, ta_shader *shader, float alpha)
{
    glEnable(GL_CULL_FACE);
    //glCullFace(GL_FRONT);
    //glClearColor(FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    ta_shader_bind(shader);
    dlb_vec_each(ta_light *, light, scene->resource_data[RES_COMP_LIGHT]) {
        if (light->disabled) continue;
        if (light->type != TA_LIGHT_POINT) {
            // TODO: Handle shadows for other light types
            continue;
        }
        // TODO: Disable shadows per light (pass cast_shadows as light uniform)
        //if (!light->cast_shadows) continue;

        ta_shader_set_vec3(shader, SYM_U_LIGHT_POS, &light->position);
        ta_shader_set_float(shader, SYM_U_LIGHT_ZFAR, light->shadowmap.zfar);
        ta_light_shadowpass_render(light, shader, alpha,
            scene->resource_data[RES_COMP_MODEL]);

        // TODO: Make button a component that an entity can have (*button_uid)
        //       instead of having it contain entity. It probably needs to have
        //       (*entity_uid) pointer as well in order to find the rigid body?
        //       Alternatively, it can have an explicit rigid body of its own
        //       which defaults to entity->rigid_body on initialization.
        //ta_light_shadowpass_render(light, shader, alpha, scene->pools[TYP_BUTTON]);
    }
    ta_shader_unbind(shader);
}

void ta_scene_render(ta_scene *scene, ta_camera *render_camera, float alpha)
{
    glViewport(0, 0, WINDOW_W, WINDOW_H);
    glCullFace(GL_BACK);
    //glDisable(GL_CULL_FACE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glStencilMask(0xFF);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glStencilMask(0x00);

    // TODO: This is going to make a zillion extranous calls
    static GLenum tg_polygon_mode = GL_FILL;
    GLenum camera_poly_mode = render_camera->debug_wireframe ? GL_LINE : GL_FILL;
    if (camera_poly_mode != tg_polygon_mode) {
        glPolygonMode(GL_FRONT_AND_BACK, camera_poly_mode);
        tg_polygon_mode = camera_poly_mode;
    }

    ta_shader_set_mat4(tg_shader_lines, SYM_U_PROJ, &render_camera->projection);
    ta_shader_set_mat4(tg_shader_lines, SYM_U_VIEW, &render_camera->look_at);
    ta_shader_set_mat4(tg_shader_quads, SYM_U_PROJ, &render_camera->projection);
    ta_shader_set_mat4(tg_shader_quads, SYM_U_VIEW, &render_camera->look_at);

    // TODO: Group by shader / material to minimize redundant uniform calls
    dlb_vec_each(ta_model *, model, scene->resource_data[RES_COMP_MODEL]) {
        ta_model_render(model, render_camera, alpha);
    }

    ta_shader_set_mat4(tg_shader_lines, SYM_U_MODEL, &MAT4_IDENT);
    ta_shader_set_mat4(tg_shader_quads, SYM_U_MODEL, &MAT4_IDENT);

#if 1
    dlb_vec_each(ta_camera *, camera, scene->resource_data[RES_COMP_CAMERA]) {
        if (camera->entity_name != tg_game.e_active_camera) {
            ta_sphere sphere = { 0 };
            sphere.center = camera->position;
            sphere.radius = 0.2f;
            ta_primitive_push_rgb_sphere(sphere);
            //ta_primitive_push_sphere(sphere, TA_COLOR_GREEN);
        }
    }
    dlb_vec_each(ta_light *, light, scene->resource_data[RES_COMP_LIGHT]) {
        ta_sphere light_pos = { 0 };
        light_pos.center = light->position;
        light_pos.radius = 0.2f;
        ta_rgba color = { 0 };
        if (light->disabled) {
            color.r = 0.5f;
            color.g = 0.5f;
            color.b = 0.5f;
        } else {
            color.r = light->color.r;
            color.g = light->color.g;
            color.b = light->color.b;
        }
        ta_primitive_push_sphere(light_pos, color);

        //ta_sphere light_aoe = { 0 };
        //light_aoe.center = light->position;
        //light_aoe.radius = light->shadowmap.zfar;
        //ta_primitive_push_rgb_sphere(light_aoe);
    }

    ta_shader_set_mat4(tg_shader_lines, SYM_U_MODEL, &MAT4_IDENT);
    ta_shader_set_mat4(tg_shader_quads, SYM_U_MODEL, &MAT4_IDENT);
    ta_primitive_render(true, false);
#endif
}