#include "ta_scene.h"
#include "ta_parse.h"
#include "ta_symbol.h"
#include "ta_camera.h"
#include "ta_material.h"
#include "ta_mesh_group.h"
#include "ta_shader.h"
#include "ta_texture.h"
#include "ta_node.h"
#include "ta_button.h"
#include "ta_rigid_body.h"
#include "ta_light.h"
#include "ta_log.h"
#include "ta_game.h"
#include "ta_window.h"
#include "dlb_vector.h"
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

typedef void (pool_init)(void *ptr);
typedef void (pool_free)(void *ptr);
typedef struct pool_info_s {
    u32 size;
    pool_init *init;
    pool_free *free;
} pool_info;

static pool_info pool_infos[] = {
    [TA_CAMERA]       = { sizeof(ta_camera),        ta_camera_init,       0 },
    [TA_LIGHT]        = { sizeof(ta_light),         ta_light_init,        0 },
    [TA_MATERIAL]     = { sizeof(ta_material),      0,                    0 },
    [TA_MESH_GROUP]   = { sizeof(ta_mesh_group),    ta_mesh_group_load,   ta_mesh_group_free },
    [TA_SHADER]       = { sizeof(ta_shader),        ta_shader_load,     0 },
    [TA_TEXTURE]      = { sizeof(ta_texture),       ta_texture_init,      ta_texture_free },
    [TA_NODE]         = { sizeof(ta_node),          ta_node_init,         0 },
    [TA_AUDIO_BUFFER] = { sizeof(ta_audio_buffer),  ta_audio_buffer_init, 0 },
    [TA_AUDIO_SOURCE] = { sizeof(ta_audio_source),  ta_audio_source_init, 0 },
    [TA_RIGID_BODY]   = { sizeof(ta_rigid_body),    ta_rigid_body_init,   0 },
    [TA_BUTTON]       = { sizeof(ta_button),        ta_button_init,       0 },
};

static void scene_ref_add(ta_scene_ref *ref)
{
	DLB_ASSERT(ref->scene);
	DLB_ASSERT(ref->type < TA_COUNT_POOLS);
	DLB_ASSERT(ref->uid);
	DLB_ASSERT(ref->ptr);

	ta_scene *scene = ref->scene;
    ta_scene_ref *new_ref = dlb_vec_alloc(scene->refs);
	dlb_memcpy(new_ref, ref, sizeof(*ref));
	u32 refs_index = dlb_vec_len(scene->refs) - 1;
    dlb_hash_insert(&scene->refs_by_uid, SYM(ref->uid), (void *)refs_index);
}

void *ta_scene_alloc(ta_scene *scene, ta_schema_field_type type,
    const char *uid)
{
    DLB_ASSERT(type < TA_COUNT_POOLS);
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
                token->type = TOKEN_NULL;
            } else if (token->value.string == SYM_TRUE) {
                token->type = TOKEN_BOOL;
                token->value.as_bool = true;
            } else if (token->value.string == SYM_FALSE) {
                token->type = TOKEN_BOOL;
                token->value.as_bool = false;
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
            case TOKEN_NULL: case TOKEN_BOOL:
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
            case TOKEN_NULL: case TOKEN_BOOL:
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

#define BAD_TOKEN() PANIC("[%d:%d] Expected %s%s%s, found %s instead.\n", \
    tok->file_pos.line, tok->file_pos.column, \
    ta_schema_field_type_str(stack[sp].type), \
    stack[sp].array_len > 0 ? " (array)" : "", \
    stack[sp].is_union_type > 0 ? " (union)" : "", \
    token_type_str(tok->type))

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
					if (schema->type >= TA_COUNT_POOLS) {
						PANIC("Type '%s' is not a scene-level type; invalid pool ID.\n", tok->value.string);
					}
                    DLB_ASSERT(schema->size);
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

                if (!braces && !array && stack[sp].type < TA_COUNT) {
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
                if (expect_array_start || (stack[sp].type != ATOM_INT &&
                                           stack[sp].type != ATOM_UINT &&
                                           stack[sp].type != ATOM_FLOAT &&
                                           stack[sp].type != ATOM_ENUM))
                {
                    BAD_TOKEN();
                }
                int *fp = stack[sp].ptr;
                *fp = tok->value.as_int;
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
                    BAD_TOKEN();
                }
                expect_array_start = false;
                sp++;
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
                stack[sp].type = 0;  // Cleanup: Easier debug
                sp--;
                break;
            } case TOKEN_OBJECT_START: {
                if (expect_array_start) {
                    BAD_TOKEN();
                }
                DLB_ASSERT(stack[sp-1].type >= 0);
                DLB_ASSERT(stack[sp-1].type < TA_COUNT);
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
    // Fallback resources
    ta_texture *tex_albedo = ta_scene_alloc(scene, TA_TEXTURE,
        INTERN("TEXTURE_ALBEDO"));
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
        scene->default_texture_uid = tex_albedo->ref.uid;
    }

    ta_texture *tex_metallic = ta_scene_alloc(scene, TA_TEXTURE,
        INTERN("TEXTURE_METALLIC"));
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
        scene->default_texture_uid = tex_metallic->ref.uid;
    }

    ta_mesh_group *mesh_group = ta_scene_alloc(scene, TA_MESH_GROUP,
        INTERN("MESH_GROUP_DEFAULT"));
    mesh_group->path = INTERN("data/mesh/default.obj");
    scene->default_mesh_group_uid = mesh_group->ref.uid;

    ta_material *material = ta_scene_alloc(scene, TA_MATERIAL,
        INTERN("MATERIAL_DEFAULT"));
    // TODO: Hard-code default shader instead of hoping it's in the scene file
    material->shader_uid = INTERN("shader_mesh");
    material->texture_albedo_uid = tex_albedo->ref.uid;
    material->texture_metallic_uid = tex_metallic->ref.uid;
    scene->default_material_uid = material->ref.uid;

	scene->refs_placeholder_count = dlb_vec_len(scene->refs);
}

ta_scene *ta_scene_init(const char *name)
{
    ta_scene *scene = dlb_calloc(1, sizeof(ta_scene));
    scene->name = name;
	//scene->refs_by_uid.debug = tg_debug_log->stream;
    dlb_hash_init(&scene->refs_by_uid, DLB_HASH_STRING, scene->name, 64);
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

    DLB_ASSERT(ARRAY_COUNT(scene->pools) == TA_COUNT_POOLS);

    for (int i = 0; i < TA_COUNT_POOLS; i++) {
        if (pool_infos[i].init) {
            s8 *end = dlb_vec_end_size(scene->pools[i], pool_infos[i].size);
            for (s8 *ptr = scene->pools[i]; ptr != end; ptr += pool_infos[i].size) {
                pool_infos[i].init(ptr);
            }
        }
    }

    return scene;
}

void ta_scene_free(ta_scene *scene)
{
	DLB_ASSERT(ARRAY_COUNT(scene->pools) == TA_COUNT_POOLS);

    for (int i = 0; i < TA_COUNT_POOLS; i++) {
		if (pool_infos[i].free) {
            s8 *end = dlb_vec_end_size(scene->pools[i], pool_infos[i].size);
			for (s8 *ptr = scene->pools[i]; ptr != end; ptr += pool_infos[i].size) {
                pool_infos[i].free(ptr);
			}
		}
        dlb_vec_free(scene->pools[i]);
    }
    dlb_vec_free(scene->refs);
    dlb_hash_free(&scene->refs_by_uid);
    dlb_free(scene);
}

void ta_scene_print(ta_scene *scene, FILE *hnd)
{
    fprintf(hnd, "#-------------------------------------------------------------------------------\n");
    fprintf(hnd, "# [SCENE] %s\n", scene->name);
    dlb_vec_range(ta_scene_ref *, ref, scene->refs + scene->refs_placeholder_count, dlb_vec_end(scene->refs)) {
        fprintf(hnd, "#-------------------------------------------------------------------------------\n");
        fprintf(hnd, "# %s\n", ta_schema_field_type_str(ref->type));
        fprintf(hnd, "#-------------------------------------------------------------------------------\n");
        ta_schema_print(hnd, ref->type, ref->ptr, 0, 0);
    }
    fflush(hnd);
}

void *ta_scene_find(ta_scene *scene, ta_schema_field_type type, const char *uid)
{
	DLB_ASSERT(scene);
	DLB_ASSERT(uid);

	bool found = false;
	u32 refs_index = (u32)dlb_hash_search(&scene->refs_by_uid, SYM(uid), &found);
	DLB_ASSERT(found);
	u32 refs_len = dlb_vec_len(scene->refs);
	DLB_ASSERT(refs_index < refs_len);

	ta_scene_ref *ref = &scene->refs[refs_index];
    DLB_ASSERT(ref);
    DLB_ASSERT(ref->type == type);
    DLB_ASSERT(ref->uid == uid);

	return ref->ptr;
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

    ta_rigid_body_pair *pairs = 0;

    dlb_vec_each(ta_rigid_body *, a, scene->pools[TA_RIGID_BODY]) {
        dlb_vec_range(ta_rigid_body *, b, a + 1, dlb_vec_end((ta_rigid_body *)scene->pools[TA_RIGID_BODY])) {
            if (ta_aabb_v_aabb(&a->aabb, &b->aabb, 0)) {
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

    ta_manifold *manifolds = 0;
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
    // https://www.toptal.com/game/video-game-physics-part-i-an-introduction-to-rigid-body-dynamics

    // Apply forces
    // Update velocities / positions
    // Detect collisions
    // Resolve contraints

    dlb_vec_each(ta_rigid_body *, body, scene->pools[TA_RIGID_BODY]) {
        ta_rigid_body_update(body, dt);
    }

    // Collision broad phase
    ta_rigid_body_pair *pairs = collision_broadphase(scene, dt);
    if (pairs) {
        // Collision narrow phase
        ta_manifold *manifolds = detect_collisions(pairs, dt);
        dlb_vec_each(ta_manifold *, manifold, manifolds) {
            ta_rigid_body_resolve_collision(manifold);
            ta_rigid_body_positional_correction(manifold);
        }
        dlb_vec_clear(manifolds);
        dlb_vec_clear(pairs);
    }

    // Update entities
    dlb_vec_each(ta_node *, entity, scene->pools[TA_NODE]) {
        ta_node_update(entity);
    }
    dlb_vec_each(ta_node *, entity, scene->pools[TA_BUTTON]) {
        ta_node_update(entity);
    }
}

void ta_scene_shadow_pass(ta_scene *scene, ta_shader *shader, float alpha)
{
	glEnable(GL_CULL_FACE);
	//glCullFace(GL_FRONT);
    glClearColor(FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX);

    ta_shader_bind(shader);
    dlb_vec_each(ta_light *, light, scene->pools[TA_LIGHT]) {
        // TODO: Handle shadows for other light types
        if (light->type != TA_LIGHT_POINT) {
            continue;
        }

        ta_shader_set_vec3(shader, SYM_U_LIGHT_POS, &light->position);
        ta_shader_set_float(shader, SYM_U_LIGHT_ZFAR, light->shadowmap.zfar);
        ta_light_shadowpass_render(light, shader, alpha, scene->pools[TA_NODE]);

        // TODO: Make button a component that an entity can have (*button_uid)
        //       instead of having it contain entity. It probably needs to have
        //       (*entity_uid) pointer as well in order to find the rigid body?
        //       Alternatively, it can have an explicit rigid body of its own
        //       which defaults to entity->rigid_body on initialization.
        //ta_light_shadowpass_render(light, shader, alpha, scene->pools[TA_BUTTON]);
    }
    ta_shader_unbind(shader);
}

void ta_scene_render(ta_scene *scene, ta_camera *camera, float alpha)
{
    glViewport(0, 0, tg_window.rect.w, tg_window.rect.h);
    glCullFace(GL_BACK);
	//glDisable(GL_CULL_FACE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    //glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClearColor(0.9f, 0.9f, 0.9f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    ta_shader_set_mat4(tg_shader_lines, SYM_U_PROJ, &camera->projection);
    ta_shader_set_mat4(tg_shader_lines, SYM_U_VIEW, &camera->look_at);
    ta_shader_set_mat4(tg_shader_lines, SYM_U_MODEL, &MAT4_IDENT);
    ta_shader_set_mat4(tg_shader_quads, SYM_U_PROJ, &camera->projection);
    ta_shader_set_mat4(tg_shader_quads, SYM_U_VIEW, &camera->look_at);
    ta_shader_set_mat4(tg_shader_quads, SYM_U_MODEL, &MAT4_IDENT);

    // TODO: Group by shader / material to minimize redundant uniform calls
    dlb_vec_each(ta_node *, node, scene->pools[TA_NODE]) {
        ta_node_render(node, camera, alpha);
    }
	// TODO: Render nodes should handle this?
    //dlb_vec_each(ta_button *, button, scene->pools[TA_BUTTON]) {
    //    ta_node_render(&button->base, camera, alpha);
    //}
#if 1
    dlb_vec_each(ta_camera *, cam, scene->pools[TA_CAMERA]) {
        if (cam != tg_game.camera) {
            ta_sphere sphere = { 0 };
            sphere.center = cam->position;
            sphere.radius = 0.2f;
            ta_primitive_push_sphere(sphere, TA_COLOR_GREEN);
        }
    }
    dlb_vec_each(ta_light *, light, scene->pools[TA_LIGHT]) {
        ta_sphere sphere = { 0 };
        sphere.center = light->position;
        sphere.radius = 0.2f;
        ta_primitive_push_sphere(sphere, TA_COLOR_YELLOW);
    }
#endif

    ta_shader_set_mat4(tg_shader_lines, SYM_U_PROJ, &camera->projection);
    ta_shader_set_mat4(tg_shader_lines, SYM_U_VIEW, &camera->look_at);
    ta_shader_set_mat4(tg_shader_lines, SYM_U_MODEL, &MAT4_IDENT);
    ta_shader_set_mat4(tg_shader_quads, SYM_U_PROJ, &camera->projection);
    ta_shader_set_mat4(tg_shader_quads, SYM_U_VIEW, &camera->look_at);
    ta_shader_set_mat4(tg_shader_quads, SYM_U_MODEL, &MAT4_IDENT);
}