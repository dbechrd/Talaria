#include "ta_scene.h"
#include "ta_parse.h"
#include "ta_symbol.h"
#include "ta_camera.h"
#include "ta_material.h"
#include "ta_mesh_group.h"
#include "ta_shader.h"
#include "ta_texture.h"
#include "ta_entity.h"
#include "ta_rigid_body.h"
#include "ta_light.h"
#include "ta_log.h"
#include "ta_game.h"
#include "dlb_vector.h"
#include <stdlib.h>

typedef enum token_type {
    TOKEN_UNKNOWN,
    TOKEN_EOF,
    TOKEN_WHITESPACE,
    TOKEN_NEWLINE,
    TOKEN_INDENT,
    TOKEN_COMMENT,
    TOKEN_IDENTIFIER,
    TOKEN_KW_NULL,
    TOKEN_KW_TRUE,
    TOKEN_KW_FALSE,
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
    switch(type) {
        case TOKEN_UNKNOWN:        return "????????";
        case TOKEN_EOF:            return "EOF";
        case TOKEN_WHITESPACE:     return "WHITESPACE";
        case TOKEN_NEWLINE:        return "NEWLINE";
        case TOKEN_INDENT:         return "INDENT";
        case TOKEN_COMMENT:        return "COMMENT";
        case TOKEN_IDENTIFIER:     return "IDENTIFIER";
        case TOKEN_KW_NULL:        return "KEYWORD";
        case TOKEN_KW_TRUE:        return "KEYWORD";
        case TOKEN_KW_FALSE:       return "KEYWORD";
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

static void scene_ref_add(ta_scene_ref *ref)
{
    ta_scene *scene = ref->scene;
    ta_scene_ref *scene_ref = dlb_vec_alloc(scene->refs);
    *scene_ref = *ref;
    dlb_hash_insert(&scene->refs_by_uid, SYM(ref->uid), scene_ref);
}

static token *token_read(ta_file *f, token **tokens)
{
    token *token = dlb_vec_alloc(*tokens);
    token->file_pos = f->pos;
    char c = ta_file_peek(f);
    switch(c) {
        case EOF:
        {
            token->type = TOKEN_EOF;
            break;
        }
        case ' ':
        {
            ta_file_expect_char(f, C_WHITESPACE, 1);
            token_type prev_token_type = TOKEN_UNKNOWN;
            int tokens_len = dlb_vec_len(*tokens);
            if (tokens_len > 1) {
                prev_token_type = (*tokens)[tokens_len - 2].type;
            }
            if (prev_token_type == TOKEN_NEWLINE ||
                prev_token_type == TOKEN_INDENT)
            {
                ta_file_expect_char(f, C_WHITESPACE, 1);
                token->type = TOKEN_INDENT;
            } else {
                token->type = TOKEN_WHITESPACE;
            }
            break;
        }
        case '\n':
        {
            token->type = TOKEN_NEWLINE;
            ta_file_expect_char(f, C_NEWLINE, 1);
            break;
        }
        case '#':
        {
            token->type = TOKEN_COMMENT;
            ta_file_expect_char(f, C_COMMENT_START, 1);
            char buf[MAX_COMMENT_LEN + 1] = { 0 };
            int len = 0;
            ta_file_read(f, buf, MAX_COMMENT_LEN, C_COMMENT, C_COMMENT_END, &len);
            token->length = len;
            token->value.string = ta_symbol_intern(buf, len);
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
            token->length = len;
            token->value.string = ta_symbol_intern(buf, len);
            if (ta_file_allow_char(f, C_IDENT_END, 1)) {
                token->type = TOKEN_IDENTIFIER;
            } else if (token->value.string == SYM_NULL) {
                token->type = TOKEN_KW_NULL;
            } else if (token->value.string == SYM_TRUE) {
                token->type = TOKEN_KW_TRUE;
            } else if (token->value.string == SYM_FALSE) {
                token->type = TOKEN_KW_FALSE;
            } else {
                PANIC_FILE(f, "Expected : after identifier '%s'\n", token->value.string);
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
                    token->type = TOKEN_FLOAT;
                    buf[len++] = ta_file_char(f);
                    next = ta_file_read(f, buf + len, 8, C_NUMBER_HEX, 0, &read);
                    len += read;
                    ta_file_allow_char(f, C_WHITESPACE, 0);
                    if (ta_file_allow_char(f, "(", 1)) {
                        ta_file_read(f, 0, 0, 0, ")", 0);
                        ta_file_expect_char(f, ")", 1);
                    }
                } else if (next == 'b') {
                    token->type = TOKEN_INT;
                    buf[len++] = ta_file_char(f);
                    ta_file_read(f, buf + len, 32, C_NUMBER_BINARY, 0, &read);
                    len += read;
                }
            }
            if (token->type == TOKEN_UNKNOWN) {
                ta_file_read(f, buf, 1, C_NUMBER_SIGN, 0, &read);
                len += read;
                ta_file_read(f, buf + len, MAX_NUMBER_LEN - len, C_NUMBER_INT, 0, &read);
                len += read;
                next = ta_file_peek(f);
                if (next == '.') {
                    token->type = TOKEN_FLOAT;
                    ta_file_read(f, buf + len, MAX_NUMBER_LEN - len,
                        C_NUMBER_FLOAT, 0, &read);
                    len += read;
                } else {
                    token->type = TOKEN_INT;
                }
            }
            switch (token->type) {
                case TOKEN_INT: {
                    token->value.as_int = parse_int(buf);
                    break;
                } case TOKEN_FLOAT: {
                    token->value.as_float = parse_float(buf);
                    break;
                } default: {
                    DLB_ASSERT(!"Token type could not be resolved");
                }
            }
            break;
        }
        case '"':
        {
            token->type = TOKEN_STRING;
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
            token->length = len;
            token->value.string = len ? ta_symbol_intern(buf, len) : 0;
            break;
        }
        case '[':
        {
            token->type = TOKEN_ARRAY_START;
            ta_file_expect_char(f, C_ARRAY_START, 1);
            break;
        }
        case ']':
        {
            token->type = TOKEN_ARRAY_END;
            ta_file_expect_char(f, C_ARRAY_END, 1);
            break;
        }
        case '{':
        {
            token->type = TOKEN_OBJECT_START;
            ta_file_expect_char(f, C_OBJECT_START, 1);
            break;
        }
        case '}':
        {
            token->type = TOKEN_OBJECT_END;
            ta_file_expect_char(f, C_OBJECT_END, 1);
            break;
        }
        case ',':
        {
            token->type = TOKEN_LIST_SEPARATOR;
            ta_file_expect_char(f, C_LIST_SEPARATOR, 1);
            break;
        }
        default:
        {
            PANIC_FILE(f, "I don't know what's going on.. weird tokens bro.\n");
        }
    }
    return token;
}

static token *tokenize(ta_file *f)
{
    token *tokens = 0;
    while (token_read(f, &tokens)->type != TOKEN_EOF) {}
    return tokens;
}

static void tokens_print(FILE *f, token *tokens)
{
    for (token *tok = tokens; tok != dlb_vec_end(tokens); tok++) {
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
            case TOKEN_KW_NULL: case TOKEN_KW_TRUE: case TOKEN_KW_FALSE:
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
}

static void tokens_print_debug(FILE *f, token *tokens)
{
    for (token *tok = tokens; tok != dlb_vec_end(tokens); tok++) {
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
            case TOKEN_KW_NULL: case TOKEN_KW_TRUE: case TOKEN_KW_FALSE:
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
}

static void tokens_parse(ta_scene *scene, token *tokens)
{
    struct {
        int indent;
        ta_schema_field_type type;
        u32 array_len;   // 0 = not array, 1 = vector, >1 = fixed array size
        u32 array_elem;  // Current element of array we're writing to
        const char *name;
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

    token *prev = tokens;
    for (token *tok = tokens; tok != dlb_vec_end(tokens); tok++) {
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
                    PANIC("Expected array start, line %d column %d\n",
                        tok->file_pos.line, tok->file_pos.column);
                }
                if (braces) {
                    stack[sp].indent = stack[sp-1].indent + 1;
                } else {
                    for (int i = sp; i >= 0; i--) {
                        if (indent >= stack[i].indent) {
                            break;
                        }
                        DLB_ASSERT(sp);
                        stack[sp].type = 0;  // Cleanup: Easier debug
                        sp--;
                    }
                    stack[sp].indent = indent;
                }

                if (array) {
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
                        PANIC("Unexpected field '%s' on %s '%s'\n",
                            tok->value.string,
                            ta_schema_field_type_str(stack[sp-1].type),
                            stack[sp-1].name);
                    }
                    if (field->in_union) {
                        if (!stack[sp-1].is_union) {
                            PANIC("Unexpected union field '%s' before union type field found in %s '%s'\n",
                                tok->value.string,
                                ta_schema_field_type_str(stack[sp-1].type),
                                stack[sp-1].name);
                        }
                        if (field->union_type != stack[sp-1].union_type) {
                            PANIC("Unexpected union field '%s' in %s '%s' with union_type = %d\n",
                                tok->value.string,
                                ta_schema_field_type_str(stack[sp-1].type),
                                stack[sp-1].name,
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
                        PANIC("Unexpected type name '%s'\n", tok->value.string);
                    }
                    DLB_ASSERT(schema->type);
                    DLB_ASSERT(schema->name == tok->value.string);
                    stack[sp].type = schema->type;
                    stack[sp].array_len = 0;
                    stack[sp].array_elem = 0;
                    stack[sp].name = tok->value.string;
                    stack[sp].ptr = dlb_vec_alloc_size(scene->pools[schema->type], schema->size);
                    stack[sp].size = schema->size;
                    stack[sp].is_union_type = false;
                    stack[sp].is_union = false;
                    stack[sp].union_type = 0;
                }

                if (stack[sp].array_len) {
                    array++;
                    expect_array_start = true;
                }

                if (!braces && !array && stack[sp].type < F_TA_COUNT) {
                    sp++;
                }
                break;
            } case TOKEN_KW_NULL: {
                if (expect_array_start) {
                    PANIC("Expected array start, line %d column %d\n",
                        tok->file_pos.line, tok->file_pos.column);
                }
                DLB_ASSERT(stack[sp].type == F_ATOM_STRING);
                break;
            } case TOKEN_INT: {
                if (expect_array_start) {
                    PANIC("Expected array start, line %d column %d\n",
                        tok->file_pos.line, tok->file_pos.column);
                }
                DLB_ASSERT(stack[sp].type == F_ATOM_INT ||
                           stack[sp].type == F_ATOM_UINT ||
                           stack[sp].type == F_ATOM_ENUM);
                int *fp = stack[sp].ptr;
                *fp = tok->value.as_int;
                if (stack[sp].is_union_type) {
                    stack[sp-1].is_union = true;
                    stack[sp-1].union_type = *fp;
                }
                break;
            } case TOKEN_FLOAT: {
                if (expect_array_start) {
                    PANIC("Expected array start, line %d column %d\n",
                        tok->file_pos.line, tok->file_pos.column);
                }
                if (stack[sp].type != F_ATOM_FLOAT) {
                    PANIC("Unexpected float token, expected %s, line %d column %d\n",
                        ta_schema_field_type_str(stack[sp].type),
                        tok->file_pos.line, tok->file_pos.column);
                }
                float *fp = stack[sp].ptr;
                *fp = tok->value.as_float;
                break;
            } case TOKEN_STRING: {
                if (expect_array_start) {
                    PANIC("Expected array start, line %d column %d\n",
                        tok->file_pos.line, tok->file_pos.column);
                }
                if (stack[sp].type != F_ATOM_STRING) {
                    PANIC("Unexpected string token, expected %s, line %d column %d\n",
                        ta_schema_field_type_str(stack[sp].type),
                        tok->file_pos.line, tok->file_pos.column);
                }
                const char **fp = stack[sp].ptr;
                *fp = tok->value.string;
                if (stack[sp].name == SYM_UID) {
                    ta_scene_ref *ref = stack[sp-1].ptr;
                    ref->scene = scene;
                    ref->type = stack[sp-1].type;
                    ref->uid = tok->value.string;
                    ref->ptr = stack[sp-1].ptr;
                    scene_ref_add(ref);
                }
                break;
            } case TOKEN_ARRAY_START: {
                if (!expect_array_start) {
                    PANIC("Did not expect array start, line %d column %d\n",
                        tok->file_pos.line, tok->file_pos.column);
                }
                expect_array_start = false;
                sp++;
                break;
            } case TOKEN_LIST_SEPARATOR: {
                if (expect_array_start) {
                    PANIC("Expected array start, line %d column %d\n",
                        tok->file_pos.line, tok->file_pos.column);
                }
                break;
            } case TOKEN_ARRAY_END: {
                if (expect_array_start) {
                    PANIC("Expected array start, line %d column %d\n",
                        tok->file_pos.line, tok->file_pos.column);
                }
                DLB_ASSERT(array);
                array--;
                DLB_ASSERT(sp);
                stack[sp].type = 0;  // Cleanup: Easier debug
                sp--;
                break;
            } case TOKEN_OBJECT_START: {
                if (expect_array_start) {
                    PANIC("Expected array start, line %d column %d\n",
                        tok->file_pos.line, tok->file_pos.column);
                }
                DLB_ASSERT(stack[sp-1].type > 0);
                DLB_ASSERT(stack[sp-1].type < F_TA_COUNT);
                braces++;
                break;
            } case TOKEN_OBJECT_END: {
                if (expect_array_start) {
                    PANIC("Expected array start, line %d column %d\n",
                        tok->file_pos.line, tok->file_pos.column);
                }
                DLB_ASSERT(braces);
                braces--;
                DLB_ASSERT(sp);
                stack[sp].type = 0;  // Cleanup: Easier debug
                sp--;
                break;
            } default: {
                PANIC("Unexpected token %s\n", token_type_str(tok->type));
            }
        }
    }
}

#if 0
static void scene_add_ref(ta_scene *scene, ta_schema_field_type type,
    const char *uid, void *ptr)
{
    DLB_ASSERT(uid);
    ta_scene_ref *ref = dlb_vec_alloc(scene->refs);
    ref->scene = scene;
    ref->type = type;
    ref->uid = uid;
    ref->ptr = ptr;
    dlb_hash_insert(&scene->refs_by_uid, SYM(uid), ref);
}
#endif

static void scene_load_placeholders(ta_scene *scene)
{
    // Fallback resources
    ta_texture *texture = ta_scene_obj_alloc(scene, F_TA_TEXTURE,
        INTERN("TEXTURE_DEFAULT"));
    texture->path = INTERN("data/texture/default_1024_1024.png");
    scene->default_texture_uid = texture->ref.uid;

    ta_mesh_group *mesh_group = ta_scene_obj_alloc(scene, F_TA_MESH_GROUP,
        INTERN("MESH_GROUP_DEFAULT"));
    mesh_group->path = INTERN("data/mesh/default.obj");
    scene->default_mesh_group_uid = mesh_group->ref.uid;

    ta_material *material = ta_scene_obj_alloc(scene, F_TA_MATERIAL,
        INTERN("MATERIAL_DEFAULT"));
    // TODO: Hard-code default shader instead of hoping it's in the scene file
    material->shader_uid = INTERN("shader_mesh");
    material->texture_uid = texture->ref.uid;
    scene->default_material_uid = material->ref.uid;
}

ta_scene *ta_scene_init(const char *name)
{
    ta_scene *scene = dlb_calloc(1, sizeof(ta_scene));
    scene->name = name;
    // TODO: Read ref count from file
    const u32 ref_count = 32;
    // NOTE: If this resizes it will invalidate the pointers stored in the hash
    //       table. I just made it fixed size for now. Might be fun to have an
    //       optional "resized" callback on dlb_vec.
    dlb_vec_reserve_fixed(scene->refs, ref_count);
    dlb_hash_init(&scene->refs_by_uid, DLB_HASH_STRING, scene->name, ref_count);
    scene_load_placeholders(scene);
    return scene;
}

ta_scene *ta_scene_load(ta_file *f)
{
    ta_scene *scene = ta_scene_init(f->filename);

    // TODO: Reserve arrays based on scene header (which doesn't exist yet)
    //dlb_vec_reserve(scn->entities, 2);

    token *tokens = tokenize(f);
    //tokens_print(tg_debug_log->stream, tokens);
    //tokens_print_debug(tg_debug_log->stream, tokens);
    tokens_parse(scene, tokens);
    dlb_vec_free(tokens);

    return scene;
}

void ta_scene_free(ta_scene *scene)
{
    dlb_vec_each(ta_scene_ref *, ref, scene->refs) {
        switch (ref->type) {
            case F_TA_CAMERA: {
                // TODO: Free cameras
                break;
            } case F_TA_LIGHT: {
                // TODO: Free lights
                break;
            } case F_TA_MATERIAL: {
                // TODO: Free materials
                break;
            } case F_TA_MESH_GROUP: {
                ta_mesh_group_free(ref->ptr);
                break;
            } case F_TA_SHADER: {
                // TODO: Free shaders
                break;
            } case F_TA_TEXTURE: {
                ta_texture_free(ref->ptr);
                break;
            } case F_TA_RIGID_BODY: {
                // TODO: Free rigid bodies
                break;
            } case F_TA_ENTITY: {
                // TODO: Free entities
                break;
            } default: {
                DLB_ASSERT("Invalid scene ref type");
            }
        }
    }

    for (int i = 0; i < ARRAY_COUNT(scene->pools); i++) {
        dlb_vec_free(scene->pools[i]);
    }

    dlb_vec_free(scene->refs);
    dlb_hash_free(&scene->refs_by_uid);

    dlb_free(scene);
}

void ta_scene_print(ta_scene *scene, FILE *hnd)
{
    // TODO: Register scene as a schema that has OBJ_ARRAY of entities
    printf("Scene name: %s\n", scene->name);
    dlb_vec_each(ta_scene_ref *, ref, scene->refs) {
        ta_schema_print(hnd, ref->type, ref->ptr, 0, 0);
    }
    fflush(hnd);
}

void *ta_scene_obj_alloc(ta_scene *scene, ta_schema_field_type type,
    const char *uid)
{
    DLB_ASSERT(uid);

    ta_schema *schema = ta_schema_find_by_type(type);
    ta_scene_ref *obj = dlb_vec_alloc_size(scene->pools[type], schema->size);
    obj->scene = scene;
    obj->type = type;
    obj->uid = uid;
    obj->ptr = obj;
    scene_ref_add(obj);
    return obj;
}

void ta_scene_initialize_objects(ta_scene *scene)
{
    dlb_vec_each(ta_scene_ref *, ref, scene->refs) {
        switch (ref->type) {
            case F_TA_CAMERA: {
                ta_camera_init(ref->ptr);
                break;
            } case F_TA_LIGHT: {
                ta_light_init(ref->ptr);
                break;
            } case F_TA_MATERIAL: {
                break;
            } case F_TA_MESH_GROUP: {
                ta_mesh_group_load(ref->ptr);
                break;
            } case F_TA_SHADER: {
                ta_shader_create(ref->ptr);
                break;
            } case F_TA_TEXTURE: {
                ta_texture_create(ref->ptr);
                break;
            } case F_TA_RIGID_BODY: {
                ta_rigid_body_init(ref->ptr);
                break;
            } case F_TA_ENTITY: {
                ta_entity_init(ref->ptr);
                break;
            } default: {
                DLB_ASSERT("Invalid scene ref type");
            }
        }
    }
}

void *ta_scene_find(ta_scene *scene, ta_schema_field_type type, const char *uid)
{
    ta_scene_ref *ref = dlb_hash_search(&scene->refs_by_uid, SYM(uid));
    DLB_ASSERT(ref);
    DLB_ASSERT(ref->type == type);
    DLB_ASSERT(ref->uid == uid);
    return ref->ptr;
}

static ta_manifold *detect_collisions(ta_scene *scene, double dt)
{
    UNUSED(dt);

    static int print = 5;
    if (print) ta_log_write(tg_debug_log, "[START] ta_rigid_body_resolve\n");

    ta_manifold *manifolds = 0;
    ta_manifold manifold;
    dlb_vec_each(ta_scene_ref *, ref_a, scene->refs) {
        if (ref_a->type != F_TA_RIGID_BODY) continue;
        ta_rigid_body *a = ref_a->ptr;
        dlb_vec_range(ta_scene_ref *, ref_b, ref_a + 1, dlb_vec_end(scene->refs)) {
            if (ref_b->type != F_TA_RIGID_BODY) continue;
            ta_rigid_body *b = ref_b->ptr;

            if (ta_rigid_body_intersect(a, b, &manifold)) {
                ta_manifold *m = dlb_vec_alloc(manifolds);
                *m = manifold;

                if (print) {
                    ta_log_write(tg_debug_log, "  '%s' collided with '%s'\n",
                        a->ref.uid, b->ref.uid);
                }
            }
        }
    }

    if (print) ta_log_write(tg_debug_log, "[END]\n");
    if (print) print--;

    return manifolds;
}

void ta_scene_update(ta_scene *scene, float dt)
{
    dlb_vec_each(ta_scene_ref *, ref, scene->refs) {
        if (ref->type != F_TA_RIGID_BODY) continue;

        ta_rigid_body *body = ref->ptr;
        ta_rigid_body_update(body, dt);
    }

    // Collision detection & resolution
    ta_manifold *manifolds = detect_collisions(scene, dt);
    dlb_vec_each(ta_manifold *, manifold, manifolds) {
        ta_rigid_body_resolve_collision(manifold);
    }
    dlb_vec_clear(manifolds);

    // Update entities
    dlb_vec_each(ta_scene_ref *, ref, scene->refs) {
        if (ref->type != F_TA_ENTITY) continue;

        ta_entity *entity = ref->ptr;
        ta_entity_update(entity);
    }
}

void ta_scene_render(ta_scene *scene, ta_camera *camera, float alpha)
{
    // TODO: Group by shader / material to minimize redundant uniform calls
    dlb_vec_each(ta_scene_ref *, ref, scene->refs) {
        switch (ref->type) {
            case F_TA_ENTITY: {
                ta_entity *entity = ref->ptr;
                ta_entity_render(entity, camera, alpha);
                break;
            } case F_TA_CAMERA: {
                ta_camera *cam = ref->ptr;
                if (cam != tg_game.camera) {
                    ta_sphere sphere = { 0 };
                    sphere.center = cam->position;
                    sphere.radius = 0.2f;
                    ta_primitive_push_sphere(sphere, TA_COLOR_YELLOW);
                }
                break;
            }
        }
    }

    ta_shader_set_mat4(tg_shader_lines, SYM_U_PROJ, &camera->projection);
    ta_shader_set_mat4(tg_shader_lines, SYM_U_VIEW, &camera->look_at);
    ta_shader_set_mat4(tg_shader_lines, SYM_U_MODEL, &MAT4_IDENT);
    ta_shader_set_mat4(tg_shader_quads, SYM_U_PROJ, &camera->projection);
    ta_shader_set_mat4(tg_shader_quads, SYM_U_VIEW, &camera->look_at);
    ta_shader_set_mat4(tg_shader_quads, SYM_U_MODEL, &MAT4_IDENT);
}