#include "ta_scene.h"
#include "ta_parse.h"
#include "ta_symbol.h"
#include "ta_camera.h"
#include "ta_material.h"
#include "ta_mesh_group.h"
#include "ta_shader.h"
#include "ta_texture.h"
#include "ta_entity.h"
#include "ta_collide.h"
#include "ta_log.h"
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
        u32 array;
        u32 array_elem;
        const char *name;
        void *ptr;
        u32 size;
        bool is_union;
        int union_type;
    } stack[8] = { 0 };

    int indent = 0;  // Current line indent counter
    bool expect_array_start = false;

    //int level = 0;   // Current level of indentation
    int sp = 0;          // "Stack pointer" index into stack
    int braces = 0;      // Current level of curly braces
    int array = 0;       // Current level of square brackets
    int array_elem = 0;  // Current element of array we're writing to

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
                    if (stack[sp-1].array) {
                        void **arr = stack[sp-1].ptr;

                        stack[sp].type = stack[sp-1].type;
                        stack[sp].array = 0;
                        stack[sp].array_elem = 0;
                        stack[sp].name = "[ARRAY_ELEMENT]";
                        if (stack[sp-1].array == 1) {
                            stack[sp].ptr = dlb_vec_alloc_size(*arr, stack[sp-1].size);
                        } else {
                            stack[sp].ptr = (u8 *)arr + (stack[sp-1].array_elem * stack[sp-1].size);
                            stack[sp-1].array_elem++;
                        }
                        stack[sp].size = stack[sp-1].size;
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
                    stack[sp].array = field->array;
                    stack[sp].array_elem = 0;
                    stack[sp].name = field->name;
                    stack[sp].ptr = ((u8 *)stack[sp-1].ptr + field->offset);
                    stack[sp].size = field->size;
                    stack[sp].is_union = false;
                    stack[sp].union_type = 0;
                } else {
                    ta_schema *schema = ta_schema_find(tok->value.string, tok->length);
                    if (!schema) {
                        PANIC("Unexpected type name '%s'\n", tok->value.string);
                    }
                    DLB_ASSERT(schema->type);
                    DLB_ASSERT(schema->name == tok->value.string);
                    stack[sp].type = schema->type;
                    stack[sp].array = 0;
                    stack[sp].array_elem = 0;
                    stack[sp].name = tok->value.string;
                    stack[sp].ptr = ta_scene_obj_alloc(scene, schema->type);
                    stack[sp].size = schema->size;
                    stack[sp].is_union = false;
                    stack[sp].union_type = INT_MIN;
                }

                if (stack[sp].array) {
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
                DLB_ASSERT(
                    stack[sp].type == F_ATOM_INT ||
                    stack[sp].type == F_ATOM_UINT ||
                    stack[sp].type == F_ATOM_UNION_TYPE
                );
                int *fp = stack[sp].ptr;
                *fp = tok->value.as_int;
                if (stack[sp].type == F_ATOM_UNION_TYPE) {
                    stack[sp-1].is_union = true;
                    stack[sp-1].union_type = *fp;
                }
                break;
            } case TOKEN_FLOAT: {
                if (expect_array_start) {
                    PANIC("Expected array start, line %d column %d\n",
                        tok->file_pos.line, tok->file_pos.column);
                }
                DLB_ASSERT(stack[sp].type == F_ATOM_FLOAT);
                float *fp = stack[sp].ptr;
                *fp = tok->value.as_float;
                break;
            } case TOKEN_STRING: {
                if (expect_array_start) {
                    PANIC("Expected array start, line %d column %d\n",
                        tok->file_pos.line, tok->file_pos.column);
                }
                DLB_ASSERT(stack[sp].type == F_ATOM_STRING);
                const char **fp = stack[sp].ptr;
                *fp = tok->value.string;
                if (stack[sp].name == SYM_UID) {
                    ta_schema_ref *ref = dlb_vec_alloc(scene->refs);
                    ref->type = stack[sp-1].type;
                    ref->ptr = stack[sp-1].ptr;
                    dlb_hash_insert(&scene->refs_by_uid, tok->value.string,
                        tok->length, ref);
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

ta_scene *ta_scene_init(const char *name)
{
    ta_scene *scene = dlb_calloc(1, sizeof(ta_scene));
    scene->name = name;
    return scene;
}

ta_scene *ta_scene_load(ta_file *f)
{
    ta_scene *scene = ta_scene_init(f->filename);

    // TODO: Reserve the correct amount for this scene, or implement resize at
    //       least.
    dlb_hash_init(&scene->refs_by_uid, DLB_HASH_STRING, scene->name, 32);
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
    // TODO: Call free on each object individually
    //for (ta_entity *e = scn->entities; e != dlb_vec_end(scn->entities); e++) {
    //    entity_free(e);
    //}
    dlb_hash_free(&scene->refs_by_uid);
    for (ta_camera *o = scene->cameras; o != dlb_vec_end(scene->cameras); o++) {
        // TODO: Free cameras
    }
    dlb_vec_free(scene->cameras);
    for (ta_sun_light *o = scene->sun_lights; o != dlb_vec_end(scene->sun_lights); o++) {
        // TODO: Free sun lights
    }
    dlb_vec_free(scene->sun_lights);
    for (ta_point_light *o = scene->point_lights; o != dlb_vec_end(scene->point_lights); o++) {
        // TODO: Free point lights
    }
    dlb_vec_free(scene->point_lights);
    for (ta_material *o = scene->materials; o != dlb_vec_end(scene->materials); o++) {
        // TODO: Free materials
    }
    dlb_vec_free(scene->materials);
    for (ta_mesh_group *o = scene->mesh_groups; o != dlb_vec_end(scene->mesh_groups); o++) {
        ta_mesh_group_free(o);
    }
    dlb_vec_free(scene->mesh_groups);
    for (ta_shader *o = scene->shaders; o != dlb_vec_end(scene->shaders); o++) {
        // TODO: Free shaders
    }
    dlb_vec_free(scene->shaders);
    for (ta_texture *o = scene->textures; o != dlb_vec_end(scene->textures); o++) {
        ta_texture_free(o);
    }
    dlb_vec_free(scene->textures);
    for (ta_rigid_body *o = scene->rigid_bodies; o != dlb_vec_end(scene->rigid_bodies); o++) {
        // TODO: Free rigid bodies
    }
    dlb_vec_free(scene->rigid_bodies);
    for (ta_entity *o = scene->entities; o != dlb_vec_end(scene->entities); o++) {
        // TODO: Free entities
    }
    dlb_vec_free(scene->entities);
    dlb_free(scene);
}

void ta_scene_print(ta_scene *scene, FILE *hnd)
{
    // TODO: Register scene as a schema that has OBJ_ARRAY of entities
    printf("Scene name: %s\n", scene->name);
    for (ta_camera *o = scene->cameras; o != dlb_vec_end(scene->cameras); o++) {
        ta_schema_print(hnd, F_TA_CAMERA, (void *)o, 0, 0);
    }
    for (ta_sun_light *o = scene->sun_lights; o != dlb_vec_end(scene->sun_lights); o++) {
        ta_schema_print(hnd, F_TA_SUN_LIGHT, (void *)o, 0, 0);
    }
    for (ta_point_light *o = scene->point_lights; o != dlb_vec_end(scene->point_lights); o++) {
        ta_schema_print(hnd, F_TA_POINT_LIGHT, (void *)o, 0, 0);
    }
    for (ta_material *o = scene->materials; o != dlb_vec_end(scene->materials); o++) {
        ta_schema_print(hnd, F_TA_MATERIAL, (void *)o, 0, 0);
    }
    for (ta_mesh_group *o = scene->mesh_groups; o != dlb_vec_end(scene->mesh_groups); o++) {
        ta_schema_print(hnd, F_TA_MESH_GROUP, (void *)o, 0, 0);
    }
    for (ta_shader *o = scene->shaders; o != dlb_vec_end(scene->shaders); o++) {
        ta_schema_print(hnd, F_TA_SHADER, (void *)o, 0, 0);
    }
    for (ta_texture *o = scene->textures; o != dlb_vec_end(scene->textures); o++) {
        ta_schema_print(hnd, F_TA_TEXTURE, (void *)o, 0, 0);
    }
    for (ta_rigid_body *o = scene->rigid_bodies; o != dlb_vec_end(scene->rigid_bodies); o++) {
        ta_schema_print(hnd, F_TA_RIGID_BODY, (void *)o, 0, 0);
    }
    for (ta_entity *o = scene->entities; o != dlb_vec_end(scene->entities); o++) {
        ta_schema_print(hnd, F_TA_ENTITY, (void *)o, 0, 0);
    }
    fflush(hnd);
}


void *ta_scene_obj_alloc(ta_scene *scene, ta_schema_field_type type)
{
    void *obj = 0;
    switch (type) {
        case F_TA_CAMERA: {
            ta_camera *camera = dlb_vec_alloc(scene->cameras);
            camera->scene = scene;
            obj = camera;
            break;
        } case F_TA_SUN_LIGHT: {
            ta_sun_light *light = dlb_vec_alloc(scene->sun_lights);
            light->scene = scene;
            obj = light;
            break;
        } case F_TA_POINT_LIGHT: {
            ta_point_light *light = dlb_vec_alloc(scene->point_lights);
            light->scene = scene;
            obj = light;
            break;
        } case F_TA_MATERIAL: {
            ta_material *material = dlb_vec_alloc(scene->materials);
            material->scene = scene;
            obj = material;
            break;
        } case F_TA_MESH_GROUP: {
            ta_mesh_group *group = dlb_vec_alloc(scene->mesh_groups);
            group->scene = scene;
            obj = group;
            break;
        } case F_TA_SHADER: {
            ta_shader *shader = dlb_vec_alloc(scene->shaders);
            shader->scene = scene;
            obj = shader;
            break;
        } case F_TA_TEXTURE: {
            ta_texture *texture = dlb_vec_alloc(scene->textures);
            texture->scene = scene;
            obj = texture;
            break;
        } case F_TA_RIGID_BODY: {
            ta_rigid_body *rigid_body = dlb_vec_alloc(scene->rigid_bodies);
            rigid_body->scene = scene;
            obj = rigid_body;
            break;
        } case F_TA_ENTITY: {
            ta_entity *entity = dlb_vec_alloc(scene->entities);
            entity->scene = scene;
            obj = entity;
            break;
        } default: {
            DLB_ASSERT(!"Cannot initialize this type as a standalone object");
        }
    }
    return obj;
}

void ta_scene_obj_init(ta_scene *scene)
{
    dlb_vec_each(ta_camera *, cam, scene->cameras) {
        cam->dirty = true;
    }
    dlb_vec_each(ta_sun_light *, light, scene->sun_lights) {
    }
    dlb_vec_each(ta_point_light *, light, scene->point_lights) {
    }
    dlb_vec_each(ta_material *, mat, scene->materials) {
    }
    dlb_vec_each(ta_mesh_group *, group, scene->mesh_groups) {
        ta_mesh_group_load(group);
    }
    dlb_vec_each(ta_shader *, shader, scene->shaders) {
        ta_shader_create(shader);
    }
    dlb_vec_each(ta_texture *, tex, scene->textures) {
        ta_texture_create(tex);
    }
    dlb_vec_each(ta_rigid_body *, rigid_body, scene->rigid_bodies) {
    }
    dlb_vec_each(ta_entity *, entity, scene->entities) {
    }
}

void *ta_scene_find(ta_scene *scene, ta_schema_field_type type, const char *uid)
{
    ta_schema_ref *ref = dlb_hash_search(&scene->refs_by_uid, SYM(uid));
    DLB_ASSERT(ref);
    DLB_ASSERT(ref->type == type);
    DLB_ASSERT(ref->ptr);
    return ref->ptr;
}

void ta_scene_render(ta_scene *scene, ta_mat4 *proj, ta_mat4 *view)
{
    // TODO: Group by shader / material to minimize redundant uniform calls
    dlb_vec_each(ta_entity *, entity, scene->entities) {
        ta_entity_render(entity, proj, view);
    }
}