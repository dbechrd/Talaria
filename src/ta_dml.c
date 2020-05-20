#include "ta_dml.h"
#include "dml_scanner.h"
#include "dml_parser.h"
#include "dml.h"
#include "ta_log.h"
#include "ta_file.h"
#include "ta_symbol.h"

#undef near
#undef far

static const char *ogx_result_str[OGX_RESULT_COUNT] = {
    [OGX_SUCCESS             ] = "OGX_SUCCESS",
    [OGX_FILE_INVALID        ] = "OGX_FILE_INVALID",
    [OGX_SYNTAX_ERROR        ] = "OGX_SYNTAX_ERROR",
    [OGX_UNEXPECTED_FIELD    ] = "OGX_UNEXPECTED_FIELD",
    [OGX_EXPECTED_LITERAL    ] = "OGX_EXPECTED_LITERAL",
    [OGX_EXPECTED_STRING     ] = "OGX_EXPECTED_STRING",
    [OGX_EXPECTED_FLOAT      ] = "OGX_EXPECTED_FLOAT",
    [OGX_EXPECTED_ARRAY      ] = "OGX_EXPECTED_ARRAY",
    [OGX_EXPECTED_OBJECT     ] = "OGX_EXPECTED_OBJECT",
    [OGX_INVALID_ARRAY_LENGTH] = "OGX_INVALID_ARRAY_LENGTH",
    [OGX_NOT_IMPLEMENTED     ] = "OGX_NOT_IMPLEMENTED",
};

static const char *ogx_key_kind_str[OGX_KEY_KIND_COUNT] = {
    [OGX_KEY_KIND_UNKNOWN    ] = "OGX_KEY_KIND_UNKNOWN",
    [OGX_KEY_KIND_VALUE      ] = "OGX_KEY_KIND_VALUE",
    [OGX_KEY_KIND_POS_CONTROL] = "OGX_KEY_KIND_POS_CONTROL",
    [OGX_KEY_KIND_NEG_CONTROL] = "OGX_KEY_KIND_NEG_CONTROL",
};

static const char *ogx_type_str[OGX_TYPE_COUNT] = {
    [OGX_TYPE_UNKNOWN] = "OGX_TYPE_UNKNOWN",
    [OGX_TYPE_FLOAT  ] = "OGX_TYPE_FLOAT",
    [OGX_TYPE_VEC2   ] = "OGX_TYPE_VEC2",
    [OGX_TYPE_VEC3   ] = "OGX_TYPE_VEC3",
    [OGX_TYPE_MAT4   ] = "OGX_TYPE_MAT4",
};

static const char *ogx_time_curve_str[OGX_TIME_CURVE_COUNT] = {
    [OGX_TIME_CURVE_UNKNOWN] = "OGX_TIME_CURVE_UNKNOWN",
    [OGX_TIME_CURVE_LINEAR ] = "OGX_TIME_CURVE_LINEAR",
    [OGX_TIME_CURVE_BEZIER ] = "OGX_TIME_CURVE_BEZIER",
};

static const char *ogx_value_curve_str[OGX_VALUE_CURVE_COUNT] = {
    [OGX_VALUE_CURVE_UNKNOWN] = "OGX_VALUE_CURVE_UNKNOWN",
    [OGX_VALUE_CURVE_LINEAR ] = "OGX_VALUE_CURVE_LINEAR",
    [OGX_VALUE_CURVE_BEZIER ] = "OGX_VALUE_CURVE_BEZIER",
};

#define SYMBOL_DECLARE(e) static const char *dmls_##e = 0;
#define SYMBOL_DEFINE(e) dmls_##e = ta_symbol_intern(CSTR(#e));
#define DML_SYMBOLS(f)  \
    f(albedo_factor)    \
    f(animation)        \
    f(atten)            \
    f(attrib)           \
    f(bind_poses)       \
    f(bones)            \
    f(bone_count_array) \
    f(bone_index_array) \
    f(bone_node)        \
    f(bone_weight_array)\
    f(camera)           \
    f(camera_node)      \
    f(children)         \
    f(color)            \
    f(curve)            \
    f(data)             \
    f(far)              \
    f(fov)              \
    f(geometry)         \
    f(geometry_node)    \
    f(index_array)      \
    f(intensity)        \
    f(key)              \
    f(kind)             \
    f(light)            \
    f(light_node)       \
    f(material)         \
    f(materials)        \
    f(mesh)             \
    f(name)             \
    f(near)             \
    f(node)             \
    f(normal_factor)    \
    f(parent)           \
    f(roughness_factor) \
    f(scale)            \
    f(shadow)           \
    f(skeleton)         \
    f(skin)             \
    f(target)           \
    f(time)             \
    f(track)            \
    f(transform)        \
    f(type)             \
    f(value)            \
    f(vertex_array)     \

DML_SYMBOLS(SYMBOL_DECLARE);

static ogx_result dml_load_node(ogx_node *node, DMLValue *value);
static ogx_result dml_load_document(ogx_scene *scene, DMLObject *document);

ogx_result dml_load(const char *filename)
{
    ogx_result result = OGX_SUCCESS;
    bool echo = tg_debug_log.echo_stdout;
    tg_debug_log.echo_stdout = true;

    if (!dmls_light_node) {
        DML_SYMBOLS(SYMBOL_DEFINE);
    }

    ta_log_timed_region_start(&tg_debug_log, SRC_DML, CSTR("dml_load"));
    ta_log_write(&tg_debug_log, SRC_DML, "Loading %s\n", filename);

    char *source = ta_file_read_all(filename);
    size_t source_len = dlb_vec_len(source) - 1;
    if (!source) {
        ta_log_write(&tg_debug_log, SRC_DML, "[%s] failed to open file\n", filename);
        result = OGX_FILE_INVALID;
        goto cleanup;
    }
    if (!source_len) {
        ta_log_write(&tg_debug_log, SRC_DML, "[%s] empty file\n", filename);
        result = OGX_FILE_INVALID;
        goto cleanup;
    }

    DMLScanner scanner = { 0 };
    DMLToken *tokens = 0;

    ta_log_write(&tg_debug_log, SRC_DML, "Scanning...\n");
    DMLScannerInit(&scanner, source, source_len);
    if (!DMLScannerScanTokens(&scanner, &tokens)) {
        ta_log_write(&tg_debug_log, SRC_DML, "Scanner produced errors, skipping parse stage.\n");
#if 0
        ta_log_write(&tg_debug_log, SRC_DML, "Token stream:\n");
        dlb_vec_each(DMLToken *, token, tokens) {
            ta_log_write(&tg_debug_log, SRC_DML, "[%04d:%04d] %18s %s", token->line, token->column,
                DMLTokenTypeToString(token->type), token->lexeme);
            if (token->type == TOK_NUMBER) {
                ta_log_write(&tg_debug_log, SRC_DML, " (%f)\n", token->literal.as_float);
            } else {
                ta_log_write(&tg_debug_log, SRC_DML, "\n");
            }
        }
#endif
        result = OGX_SYNTAX_ERROR;
        goto cleanup;
    }

    ta_log_write(&tg_debug_log, SRC_DML, "Parsing...\n");

    DMLParser parser = { 0 };
    DMLParserInit(&parser, tokens, filename, source, source_len);

    DMLObject document = { 0 };
    DMLParserParse(&parser, &document);

#if 0
    fputs("Document:\n", stdout);
    DMLPrintObject(&document, 0);
    fputc('\n', stdout);
#endif

    ta_log_write(&tg_debug_log, SRC_DML, "Loading...\n");
    ogx_scene scene = { 0 };
    result = dml_load_document(&scene, &document);
    if (result != OGX_SUCCESS) {
        ta_log_write(&tg_debug_log, SRC_DML, "Load failed: %s.\n", ogx_result_str[result]);
        goto cleanup;
    }

    ta_log_write(&tg_debug_log, SRC_DML, "Loaded successfully.\n");

cleanup:
    dlb_vec_free(source);
    // TODO: DMLObjectFree(document);

    ta_log_timed_region_end(&tg_debug_log, CSTR("dml_load"));
    tg_debug_log.echo_stdout = echo;
    return result;
}

static ogx_result dml_load_bool(bool *boool, DMLValue *value)
{
    assert(boool);
    assert(value);

    ogx_result result = OGX_SUCCESS;
    if (value->type != DML_VALUE_LITERAL) {
#if _DEBUG
        ta_log_write(&tg_debug_log, SRC_DML, "[%s:%zu:%zu] expected literal, found %s\n", value->dbg_symbol.filename,
            value->dbg_symbol.line, value->dbg_symbol.column, DMLValueTypeStr[value->type]);
#endif
        result = OGX_EXPECTED_LITERAL;
    } else if (value->data.as_literal.type != DML_LITERAL_BOOL) {
#if _DEBUG
        ta_log_write(&tg_debug_log, SRC_DML, "[%s:%zu:%zu] expected bool literal, found %s\n", value->dbg_symbol.filename,
            value->dbg_symbol.line, value->dbg_symbol.column, DMLLiteralTypeStr[value->data.as_literal.type]);
#endif
        result = OGX_EXPECTED_BOOL;
    } else {
        *boool = value->data.as_literal.data.as_bool;
    }
    return result;
}

static ogx_result dml_load_string(const char **string, DMLValue *value)
{
    assert(string);
    assert(value);

    ogx_result result = OGX_SUCCESS;
    if (value->type != DML_VALUE_LITERAL) {
#if _DEBUG
        ta_log_write(&tg_debug_log, SRC_DML, "[%s:%zu:%zu] expected literal, found %s\n", value->dbg_symbol.filename,
            value->dbg_symbol.line, value->dbg_symbol.column, DMLValueTypeStr[value->type]);
#endif
        result = OGX_EXPECTED_LITERAL;
    } else if (value->data.as_literal.type != DML_LITERAL_STRING) {
#if _DEBUG
        ta_log_write(&tg_debug_log, SRC_DML, "[%s:%zu:%zu] expected string literal, found %s\n", value->dbg_symbol.filename,
            value->dbg_symbol.line, value->dbg_symbol.column, DMLLiteralTypeStr[value->data.as_literal.type]);
#endif
        result = OGX_EXPECTED_STRING;
    } else {
        *string = value->data.as_literal.data.as_string;
    }
    return result;
}

static ogx_result dml_load_float(float *f, DMLValue *value)
{
    assert(f);
    assert(value);

    ogx_result result = OGX_SUCCESS;
    if (value->type != DML_VALUE_LITERAL) {
#if _DEBUG
        ta_log_write(&tg_debug_log, SRC_DML, "[%s:%zu:%zu] expected literal, found %s\n", value->dbg_symbol.filename,
            value->dbg_symbol.line, value->dbg_symbol.column, DMLValueTypeStr[value->type]);
#endif
        result = OGX_EXPECTED_LITERAL;
    } else if (value->data.as_literal.type != DML_LITERAL_FLOAT) {
#if _DEBUG
        ta_log_write(&tg_debug_log, SRC_DML, "[%s:%zu:%zu] expected float literal, found %s\n", value->dbg_symbol.filename,
            value->dbg_symbol.line, value->dbg_symbol.column, DMLLiteralTypeStr[value->data.as_literal.type]);
#endif
        result = OGX_EXPECTED_FLOAT;
    } else {
        *f = value->data.as_literal.data.as_float;
    }
    return result;
}

static ogx_result dml_load_vec3(ogx_vec3 *vec, DMLValue *value)
{
    assert(vec);
    assert(value);

    ogx_result result = OGX_SUCCESS;
    if (value->type != DML_VALUE_ARRAY) {
#if _DEBUG
        ta_log_write(&tg_debug_log, SRC_DML, "[%s:%zu:%zu] expected array, found %s\n", value->dbg_symbol.filename,
            value->dbg_symbol.line, value->dbg_symbol.column, DMLValueTypeStr[value->type]);
#endif
        result = OGX_EXPECTED_ARRAY;
    } else if (dlb_vec_len(value->data.as_array.values) != 3) {
#if _DEBUG
        ta_log_write(&tg_debug_log, SRC_DML, "[%s:%zu:%zu] expected array length 3 for vec3, array length is %zu\n", value->dbg_symbol.filename,
            value->dbg_symbol.line, value->dbg_symbol.column, dlb_vec_len(value->data.as_array.values));
#endif
        result = OGX_INVALID_ARRAY_LENGTH;
    } else {
        for (size_t i = 0; i < 3; i++) {
            DMLValue *arr_value = value->data.as_array.values + i;
            if (arr_value->type != DML_VALUE_LITERAL) {
#if _DEBUG
                ta_log_write(&tg_debug_log, SRC_DML, "[%s:%zu:%zu] expected literal, found %s\n", value->dbg_symbol.filename,
                    value->dbg_symbol.line, value->dbg_symbol.column, DMLValueTypeStr[value->type]);
#endif
                result = OGX_EXPECTED_LITERAL;
                break;
            } else if (arr_value->data.as_literal.type != DML_LITERAL_FLOAT) {
#if _DEBUG
                ta_log_write(&tg_debug_log, SRC_DML, "[%s:%zu:%zu] expected float literal, found %s\n", value->dbg_symbol.filename,
                    value->dbg_symbol.line, value->dbg_symbol.column, DMLLiteralTypeStr[value->data.as_literal.type]);
#endif
                result = OGX_EXPECTED_FLOAT;
                break;
            } else {
                (*vec)[i] = arr_value->data.as_literal.data.as_float;
            }
        }
    }
    return result;
}

static ogx_result dml_load_mat4(ogx_mat4 *matrix, DMLValue *value)
{
    assert(matrix);
    assert(value);

    ogx_result result = OGX_SUCCESS;
    if (value->type != DML_VALUE_ARRAY) {
#if _DEBUG
        ta_log_write(&tg_debug_log, SRC_DML, "[%s:%zu:%zu] expected array, found %s\n", value->dbg_symbol.filename,
            value->dbg_symbol.line, value->dbg_symbol.column, DMLValueTypeStr[value->type]);
#endif
        result = OGX_EXPECTED_ARRAY;
    } else if (dlb_vec_len(value->data.as_array.values) != 16) {
#if _DEBUG
        ta_log_write(&tg_debug_log, SRC_DML, "[%s:%zu:%zu] expected array length 16 for mat4, array length is %zu\n", value->dbg_symbol.filename,
            value->dbg_symbol.line, value->dbg_symbol.column, dlb_vec_len(value->data.as_array.values));
#endif
        result = OGX_INVALID_ARRAY_LENGTH;
    } else {
        for (size_t i = 0; i < 16; i++) {
            DMLValue *arr_value = value->data.as_array.values + i;
            if (arr_value->type != DML_VALUE_LITERAL) {
#if _DEBUG
                ta_log_write(&tg_debug_log, SRC_DML, "[%s:%zu:%zu] expected literal, found %s\n", value->dbg_symbol.filename,
                    value->dbg_symbol.line, value->dbg_symbol.column, DMLValueTypeStr[value->type]);
#endif
                result = OGX_EXPECTED_LITERAL;
                break;
            } else if (arr_value->data.as_literal.type != DML_LITERAL_FLOAT) {
#if _DEBUG
                ta_log_write(&tg_debug_log, SRC_DML, "[%s:%zu:%zu] expected float literal, found %s\n", value->dbg_symbol.filename,
                    value->dbg_symbol.line, value->dbg_symbol.column, DMLLiteralTypeStr[value->data.as_literal.type]);
#endif
                result = OGX_EXPECTED_FLOAT;
                break;
            } else {
                (*matrix)[i] = arr_value->data.as_literal.data.as_float;
            }
        }
    }
    return result;
}

static ogx_result dml_load_string_array(const char ***array, DMLValue *value)
{
    assert(array);
    assert(value);

    ogx_result result = OGX_SUCCESS;
    if (value->type != DML_VALUE_ARRAY) {
        result = OGX_EXPECTED_ARRAY;
    } else {
        size_t len = dlb_vec_len(value->data.as_array.values);
        dlb_vec_reserve(*array, len);
        dlb_vec_each(DMLValue *, arr_value, value->data.as_array.values) {
            const char **string = dlb_vec_alloc(*array);
            result = dml_load_string(string, arr_value);
            if (result != OGX_SUCCESS) {
                dlb_vec_free(*array);
                break;
            }
        }
    }
    return result;
}

static ogx_result dml_load_float_array(float **array, size_t array_len, DMLValue *value)
{
    assert(array);
    assert(value);

    ogx_result result = OGX_SUCCESS;
    if (value->type != DML_VALUE_ARRAY) {
        result = OGX_EXPECTED_ARRAY;
    } else {
        size_t len = dlb_vec_len(value->data.as_array.values);
        if (array_len) {
            if (len == array_len) {
                for (size_t i = 0; i < array_len; i++) {
                    result = dml_load_float(*array + i, &value->data.as_array.values[i]);
                    if (result != OGX_SUCCESS) {
                        break;
                    }
                }
            } else {
#if _DEBUG
                ta_log_write(&tg_debug_log, SRC_DML, "[%s:%zu:%zu] array length %zu does not match requested length %zu\n",
                    value->dbg_symbol.filename, value->dbg_symbol.line, value->dbg_symbol.column, len, array_len);
#endif
                result = OGX_INVALID_ARRAY_LENGTH;
            }
        } else {
            dlb_vec_reserve(*array, len);
            dlb_vec_each(DMLValue *, arr_value, value->data.as_array.values) {
                float *f = dlb_vec_alloc(*array);
                result = dml_load_float(f, arr_value);
                if (result != OGX_SUCCESS) {
                    dlb_vec_free(*array);
                    break;
                }
            }
        }
    }
    return result;
}

static ogx_result dml_load_vec2_array(ogx_vec2 **array, DMLValue *value)
{
    assert(array);
    assert(value);

    ogx_result result = OGX_SUCCESS;
    if (value->type != DML_VALUE_ARRAY) {
        result = OGX_EXPECTED_ARRAY;
    } else {
        size_t len = dlb_vec_len(value->data.as_array.values);
        dlb_vec_reserve(*array, len);
        dlb_vec_each(DMLValue *, arr_value, value->data.as_array.values) {
            ogx_vec2 *vec = dlb_vec_alloc(*array);
            result = dml_load_float_array(&(float *)vec, 2, arr_value);
            if (result != OGX_SUCCESS) {
                dlb_vec_free(*array);
                break;
            }
        }
    }
    return result;
}

static ogx_result dml_load_vec3_array(ogx_vec3 **array, DMLValue *value)
{
    assert(array);
    assert(value);

    ogx_result result = OGX_SUCCESS;
    if (value->type != DML_VALUE_ARRAY) {
        result = OGX_EXPECTED_ARRAY;
    } else {
        size_t len = dlb_vec_len(value->data.as_array.values);
        dlb_vec_reserve(*array, len);
        dlb_vec_each(DMLValue *, arr_value, value->data.as_array.values) {
            ogx_vec3 *vec = dlb_vec_alloc(*array);
            result = dml_load_float_array(&(float *)vec, 3, arr_value);
            if (result != OGX_SUCCESS) {
                dlb_vec_free(*array);
                break;
            }
        }
    }
    return result;
}

static ogx_result dml_load_mat4_array(ogx_mat4 **array, DMLValue *value)
{
    assert(array);
    assert(value);

    ogx_result result = OGX_SUCCESS;
    if (value->type != DML_VALUE_ARRAY) {
        result = OGX_EXPECTED_ARRAY;
    } else {
        size_t len = dlb_vec_len(value->data.as_array.values);
        dlb_vec_reserve(*array, len);
        dlb_vec_each(DMLValue *, arr_value, value->data.as_array.values) {
            ogx_mat4 *mat = dlb_vec_alloc(*array);
            result = dml_load_float_array(&(float *)mat, 16, arr_value);
            if (result != OGX_SUCCESS) {
                dlb_vec_free(*array);
                break;
            }
        }
    }
    return result;
}

static ogx_result dml_load_transform(ogx_transform *transform, DMLValue *value)
{
    assert(transform);
    assert(value);

    ogx_result result = OGX_SUCCESS;
    if (value->type != DML_VALUE_OBJECT) {
        result = OGX_EXPECTED_OBJECT;
    } else {
        dlb_vec_each(DMLField *, field, value->data.as_object.fields) {
            if (field->name == dmls_type) {
                result = dml_load_string(&transform->type, &field->value);
            } else if (field->name == dmls_data) {
                result = dml_load_mat4(&transform->data, &field->value);
            } else {
#if _DEBUG
                ta_log_write(&tg_debug_log, SRC_DML, "[%s:%zu:%zu] unexpected field '%s'\n", value->dbg_symbol.filename,
                    value->dbg_symbol.line, value->dbg_symbol.column, field->name);
#endif
                result = OGX_UNEXPECTED_FIELD;
            }

            if (result != OGX_SUCCESS) {
                break;
            }
        }
    }
    return result;
}

static ogx_result dml_load_key(ogx_key *key, DMLValue *value)
{
    assert(key);
    assert(value);

    ogx_result result = OGX_SUCCESS;
    if (value->type != DML_VALUE_OBJECT) {
        result = OGX_EXPECTED_OBJECT;
    } else {
        dlb_vec_each(DMLField *, field, value->data.as_object.fields) {
            if (field->name == dmls_kind) {
                const char *key_kind = 0;
                result = dml_load_string(&key_kind, &field->value);
                if (result == OGX_SUCCESS) {
                    if (!strcmp(key_kind, "value")) {
                        key->kind = OGX_KEY_KIND_VALUE;
                    } else if (!strcmp(key_kind, "+control")) {
                        key->kind = OGX_KEY_KIND_POS_CONTROL;
                    } else if (!strcmp(key_kind, "-control")) {
                        key->kind = OGX_KEY_KIND_NEG_CONTROL;
                    } else {
#if _DEBUG
                        ta_log_write(&tg_debug_log, SRC_DML,
                            "[%s:%zu:%zu] unexpected key kind, expected 'value', '+control' or '-control', found '%s'\n",
                            value->dbg_symbol.filename, value->dbg_symbol.line, value->dbg_symbol.column, key_kind);
#endif
                        result = OGX_UNEXPECTED_VALUE;
                    }
                }
            } else if (field->name == dmls_type) {
                const char *type = 0;
                result = dml_load_string(&type, &field->value);
                if (result == OGX_SUCCESS) {
                    if (!strcmp(type, "float")) {
                        key->type = OGX_TYPE_FLOAT;
                    } else if (!strcmp(type, "mat4")) {
                        key->type = OGX_TYPE_MAT4;
                    } else {
#if _DEBUG
                        ta_log_write(&tg_debug_log, SRC_DML,
                            "[%s:%zu:%zu] unexpected key type, expected 'float' or 'mat4', found '%s'\n",
                            value->dbg_symbol.filename, value->dbg_symbol.line, value->dbg_symbol.column, type);
#endif
                        result = OGX_UNEXPECTED_TYPE;
                    }
                }
            } else if (field->name == dmls_data) {
                if (key->type == OGX_TYPE_FLOAT) {
                    result = dml_load_float_array(&key->values.as_float, 0, &field->value);
                } else if (key->type == OGX_TYPE_MAT4) {
                    result = dml_load_mat4_array(&key->values.as_mat4, &field->value);
                } else {
#if _DEBUG
                    ta_log_write(&tg_debug_log, SRC_DML,
                        "[%s:%zu:%zu] expected 'type' field, '%s' field cannot be loaded before type is known\n",
                        value->dbg_symbol.filename, value->dbg_symbol.line, value->dbg_symbol.column, field->name);
#endif
                    result = OGX_UNKNOWN_TYPE;
                }
            } else {
#if _DEBUG
                ta_log_write(&tg_debug_log, SRC_DML, "[%s:%zu:%zu] unexpected field '%s'\n", value->dbg_symbol.filename,
                    value->dbg_symbol.line, value->dbg_symbol.column, field->name);
#endif
                result = OGX_UNEXPECTED_FIELD;
            }

            if (result != OGX_SUCCESS) {
                break;
            }
        }
    }
    return result;
}

static ogx_result dml_load_time(ogx_time *time, DMLValue *value)
{
    assert(time);
    assert(value);

    ogx_result result = OGX_SUCCESS;
    if (value->type != DML_VALUE_OBJECT) {
        result = OGX_EXPECTED_OBJECT;
    } else {
        dlb_vec_each(DMLField *, field, value->data.as_object.fields) {
            if (field->name == dmls_curve) {
                const char *curve = 0;
                result = dml_load_string(&curve, &field->value);
                if (result == OGX_SUCCESS) {
                    if (!strcmp(curve, "linear")) {
                        time->curve = OGX_TIME_CURVE_LINEAR;
                    } else if (!strcmp(curve, "bezier")) {
                        time->curve = OGX_TIME_CURVE_BEZIER;
                    } else {
#if _DEBUG
                        ta_log_write(&tg_debug_log, SRC_DML,
                            "[%s:%zu:%zu] unexpected time curve, expected 'linear' or 'bezier', found '%s'\n",
                            value->dbg_symbol.filename, value->dbg_symbol.line, value->dbg_symbol.column, curve);
#endif
                        result = OGX_UNEXPECTED_VALUE;
                    }
                }
            } else if (field->name == dmls_key) {
                if (time->curve != OGX_TIME_CURVE_UNKNOWN) {
                    result = dml_load_key(&time->key, &field->value);
                    if (result == OGX_SUCCESS) {
                        if (time->curve != OGX_TIME_CURVE_BEZIER) {
                            if (time->key.kind == OGX_KEY_KIND_POS_CONTROL ||
                                time->key.kind == OGX_KEY_KIND_NEG_CONTROL)
                            {
#if _DEBUG
                                ta_log_write(&tg_debug_log, SRC_DML,
                                    "[%s:%zu:%zu] unexpected key kind, '%s' not valid for curve type '%s'\n",
                                    value->dbg_symbol.filename, value->dbg_symbol.line, value->dbg_symbol.column,
                                    ogx_key_kind_str[time->key.kind], ogx_time_curve_str[time->curve]);
#endif
                                result = OGX_UNEXPECTED_VALUE;
                            }
                        }
                    }
                } else {
#if _DEBUG
                    ta_log_write(&tg_debug_log, SRC_DML,
                        "[%s:%zu:%zu] expected 'curve' field, '%s' field cannot be loaded before curve type is known\n",
                        value->dbg_symbol.filename, value->dbg_symbol.line, value->dbg_symbol.column, field->name);
#endif
                    result = OGX_UNKNOWN_TYPE;
                }
            } else {
#if _DEBUG
                ta_log_write(&tg_debug_log, SRC_DML, "[%s:%zu:%zu] unexpected field '%s'\n", value->dbg_symbol.filename,
                    value->dbg_symbol.line, value->dbg_symbol.column, field->name);
#endif
                result = OGX_UNEXPECTED_FIELD;
            }

            if (result != OGX_SUCCESS) {
                break;
            }
        }
    }
    return result;
}

static ogx_result dml_load_value(ogx_value *val, DMLValue *value)
{
    assert(val);
    assert(value);

    ogx_result result = OGX_SUCCESS;
    if (value->type != DML_VALUE_OBJECT) {
        result = OGX_EXPECTED_OBJECT;
    } else {
        dlb_vec_each(DMLField *, field, value->data.as_object.fields) {
            if (field->name == dmls_curve) {
                const char *curve = 0;
                result = dml_load_string(&curve, &field->value);
                if (result == OGX_SUCCESS) {
                    if (!strcmp(curve, "linear")) {
                        val->curve = OGX_VALUE_CURVE_LINEAR;
                    } else if (!strcmp(curve, "bezier")) {
                        val->curve = OGX_VALUE_CURVE_BEZIER;
                    } else {
#if _DEBUG
                        ta_log_write(&tg_debug_log, SRC_DML,
                            "[%s:%zu:%zu] unexpected value curve, expected 'linear' or 'bezier', found '%s'\n",
                            value->dbg_symbol.filename, value->dbg_symbol.line, value->dbg_symbol.column, curve);
#endif
                        result = OGX_UNEXPECTED_VALUE;
                    }
                }
            } else if (field->name == dmls_key) {
                if (val->curve != OGX_VALUE_CURVE_UNKNOWN) {
                    result = dml_load_key(&val->key, &field->value);
                    if (result == OGX_SUCCESS) {
                        if (val->curve != OGX_VALUE_CURVE_BEZIER) {
                            if (val->key.kind == OGX_KEY_KIND_POS_CONTROL ||
                                val->key.kind == OGX_KEY_KIND_NEG_CONTROL)
                            {
#if _DEBUG
                                ta_log_write(&tg_debug_log, SRC_DML,
                                    "[%s:%zu:%zu] unexpected key kind, '%s' not valid for curve type '%s'\n",
                                    value->dbg_symbol.filename, value->dbg_symbol.line, value->dbg_symbol.column,
                                    ogx_key_kind_str[val->key.kind], ogx_value_curve_str[val->curve]);
#endif
                                result = OGX_UNEXPECTED_VALUE;
                            }
                        }
                    }
                } else {
#if _DEBUG
                    ta_log_write(&tg_debug_log, SRC_DML,
                        "[%s:%zu:%zu] expected 'curve' field, '%s' field cannot be loaded before curve type is known\n",
                        value->dbg_symbol.filename, value->dbg_symbol.line, value->dbg_symbol.column, field->name);
#endif
                    result = OGX_UNKNOWN_TYPE;
                }
            } else {
#if _DEBUG
                ta_log_write(&tg_debug_log, SRC_DML, "[%s:%zu:%zu] unexpected field '%s'\n", value->dbg_symbol.filename,
                    value->dbg_symbol.line, value->dbg_symbol.column, field->name);
#endif
                result = OGX_UNEXPECTED_FIELD;
            }

            if (result != OGX_SUCCESS) {
                break;
            }
        }
    }
    return result;
}

static ogx_result dml_load_track(ogx_track *track, DMLValue *value)
{
    assert(track);
    assert(value);

    ogx_result result = OGX_SUCCESS;
    if (value->type != DML_VALUE_OBJECT) {
        result = OGX_EXPECTED_OBJECT;
    } else {
        dlb_vec_each(DMLField *, field, value->data.as_object.fields) {
            if (field->name == dmls_target) {
                result = dml_load_string(&track->target, &field->value);
            } else if (field->name == dmls_time) {
                result = dml_load_time(&track->time, &field->value);
            } else if (field->name == dmls_value) {
                result = dml_load_value(&track->value, &field->value);
            } else {
#if _DEBUG
                ta_log_write(&tg_debug_log, SRC_DML, "[%s:%zu:%zu] unexpected field '%s'\n", value->dbg_symbol.filename,
                    value->dbg_symbol.line, value->dbg_symbol.column, field->name);
#endif
                result = OGX_UNEXPECTED_FIELD;
            }

            if (result != OGX_SUCCESS) {
                break;
            }
        }
    }
    return result;
}

static ogx_result dml_load_animation(ogx_animation *animation, DMLValue *value)
{
    assert(animation);
    assert(value);

    ogx_result result = OGX_SUCCESS;
    if (value->type != DML_VALUE_OBJECT) {
        result = OGX_EXPECTED_OBJECT;
    } else {
        dlb_vec_each(DMLField *, field, value->data.as_object.fields) {
            if (field->name == dmls_track) {
                result = dml_load_track(&animation->track, &field->value);
            } else {
#if _DEBUG
                ta_log_write(&tg_debug_log, SRC_DML, "[%s:%zu:%zu] unexpected field '%s'\n", value->dbg_symbol.filename,
                    value->dbg_symbol.line, value->dbg_symbol.column, field->name);
#endif
                result = OGX_UNEXPECTED_FIELD;
            }

            if (result != OGX_SUCCESS) {
                break;
            }
        }
    }
    return result;
}

static ogx_result dml_load_basic_node_field(ogx_basic_node *node, DMLField *field)
{
    assert(node);
    assert(field);

    ogx_result result;
    if (field->name == dmls_name) {
        result = dml_load_string(&node->name, &field->value);
    } else if (field->name == dmls_transform) {
        result = dml_load_transform(&node->transform, &field->value);
    } else if (field->name == dmls_bone_node) {
        ogx_node *child = dlb_vec_alloc(node->children);
        child->basic_node.parent = (ogx_node *)node;
        child->basic_node.type = OGX_BONE_NODE;
        result = dml_load_node(child, &field->value);
    } else if (field->name == dmls_camera_node) {
        ogx_node *child = dlb_vec_alloc(node->children);
        child->basic_node.parent = (ogx_node *)node;
        child->basic_node.type = OGX_CAMERA_NODE;
        result = dml_load_node(child, &field->value);
    } else if (field->name == dmls_geometry_node) {
        ogx_node *child = dlb_vec_alloc(node->children);
        child->basic_node.parent = (ogx_node *)node;
        child->basic_node.type = OGX_GEOMETRY_NODE;
        result = dml_load_node(child, &field->value);
    } else if (field->name == dmls_light_node) {
        ogx_node *child = dlb_vec_alloc(node->children);
        child->basic_node.parent = (ogx_node *)node;
        child->basic_node.type = OGX_LIGHT_NODE;
        result = dml_load_node(child, &field->value);
    } else if (field->name == dmls_node) {
        ogx_node *child = dlb_vec_alloc(node->children);
        child->basic_node.parent = (ogx_node *)node;
        child->basic_node.type = OGX_BASIC_NODE;
        result = dml_load_node(child, &field->value);
    } else {
        result = OGX_UNEXPECTED_FIELD;
    }
    return result;
}

static ogx_result dml_load_bone_node_field(ogx_bone_node *node, DMLField *field)
{
    assert(node);
    assert(field);

    ogx_result result;
    if (field->name == dmls_animation) {
        result = dml_load_animation(&node->animation, &field->value);
    } else {
        result = OGX_UNEXPECTED_FIELD;
    }
    return result;
}

static ogx_result dml_load_camera_node_field(ogx_camera_node *node, DMLField *field)
{
    assert(node);
    assert(field);

    ogx_result result;
    if (field->name == dmls_camera) {
        result = dml_load_string(&node->camera, &field->value);
    } else {
        result = OGX_UNEXPECTED_FIELD;
    }
    return result;
}

static ogx_result dml_load_geometry_node_field(ogx_geometry_node *node, DMLField *field)
{
    assert(node);
    assert(field);

    ogx_result result;
    if (field->name == dmls_mesh) {
        result = dml_load_string(&node->mesh, &field->value);
    } else if (field->name == dmls_materials) {
        result = dml_load_string_array(&node->materials, &field->value);
    } else {
        result = OGX_UNEXPECTED_FIELD;
    }
    return result;
}

static ogx_result dml_load_light_node_field(ogx_light_node *node, DMLField *field)
{
    assert(node);
    assert(field);

    ogx_result result;
    if (field->name == dmls_light) {
        result = dml_load_string(&node->light, &field->value);
    } else {
        result = OGX_UNEXPECTED_FIELD;
    }
    return result;
}

static ogx_result dml_load_node(ogx_node *node, DMLValue *value)
{
    assert(node);
    assert(value);

    ogx_result result = OGX_SUCCESS;
    if (value->type != DML_VALUE_OBJECT) {
#if _DEBUG
        ta_log_write(&tg_debug_log, SRC_DML, "[%s:%zu:%zu] expected object, found %s\n", value->dbg_symbol.filename,
            value->dbg_symbol.line, value->dbg_symbol.column, DMLValueTypeStr[value->type]);
#endif
        result = OGX_EXPECTED_OBJECT;
    } else {
        dlb_vec_each(DMLField *, field, value->data.as_object.fields) {
            result = dml_load_basic_node_field(&node->basic_node, field);
            if (result == OGX_UNEXPECTED_FIELD) {
                switch (node->basic_node.type) {
                    case OGX_BONE_NODE:
                        result = dml_load_bone_node_field(&node->bone_node, field);
                        break;
                    case OGX_CAMERA_NODE:
                        result = dml_load_camera_node_field(&node->camera_node, field);
                        break;
                    case OGX_GEOMETRY_NODE:
                        result = dml_load_geometry_node_field(&node->geometry_node, field);
                        break;
                    case OGX_LIGHT_NODE:
                        result = dml_load_light_node_field(&node->light_node, field);
                        break;
                }
            }

            if (result != OGX_SUCCESS) {
#if _DEBUG
                ta_log_write(&tg_debug_log, SRC_DML, "[%s:%zu:%zu] %s '%s'\n", field->dbg_symbol.filename,
                    field->dbg_symbol.line, field->dbg_symbol.column, ogx_result_str[result], field->name);
#endif
                break;
            }
        }
    }
    return result;
}

static ogx_result dml_load_camera(ogx_camera *camera, DMLValue *value)
{
    assert(camera);
    assert(value);

    ogx_result result = OGX_SUCCESS;
    if (value->type != DML_VALUE_OBJECT) {
#if _DEBUG
        ta_log_write(&tg_debug_log, SRC_DML, "[%s:%zu:%zu] expected object, found %s\n", value->dbg_symbol.filename,
            value->dbg_symbol.line, value->dbg_symbol.column, DMLValueTypeStr[value->type]);
#endif
        result = OGX_EXPECTED_OBJECT;
    } else {
        dlb_vec_each(DMLField *, field, value->data.as_object.fields) {
            if (field->name == dmls_name) {
                result = dml_load_string(&camera->name, &field->value);
            } else if (field->name == dmls_fov) {
                result = dml_load_float(&camera->fov, &field->value);
            } else if (field->name == dmls_near) {
                result = dml_load_float(&camera->nearz, &field->value);
            } else if (field->name == dmls_far) {
                result = dml_load_float(&camera->farz, &field->value);
            } else {
#if _DEBUG
                ta_log_write(&tg_debug_log, SRC_DML, "[%s:%zu:%zu] unexpected field '%s'\n", value->dbg_symbol.filename,
                    value->dbg_symbol.line, value->dbg_symbol.column, field->name);
#endif
                result = OGX_UNEXPECTED_FIELD;
            }

            if (result != OGX_SUCCESS) {
                break;
            }
        }
    }
    return result;
}

static ogx_result dml_load_vertex_array(ogx_vertex_array *vertex_array, DMLValue *value)
{
    assert(vertex_array);
    assert(value);

    ogx_result result = OGX_SUCCESS;
    if (value->type != DML_VALUE_OBJECT) {
#if _DEBUG
        ta_log_write(&tg_debug_log, SRC_DML, "[%s:%zu:%zu] expected object, found %s\n", value->dbg_symbol.filename,
            value->dbg_symbol.line, value->dbg_symbol.column, DMLValueTypeStr[value->type]);
#endif
        result = OGX_EXPECTED_OBJECT;
    } else {
        dlb_vec_each(DMLField *, field, value->data.as_object.fields) {
            if (field->name == dmls_attrib) {
                const char *attrib = 0;
                result = dml_load_string(&attrib, &field->value);
                if (result == OGX_SUCCESS) {
                    if (!strcmp(attrib, "position")) {
                        vertex_array->attrib = OGX_VERTEX_ATTRIB_POSIITON;
                    } else if (!strcmp(attrib, "normal")) {
                        vertex_array->attrib = OGX_VERTEX_ATTRIB_NORMAL;
                    } else if (!strcmp(attrib, "texcoord0")) {
                        vertex_array->attrib = OGX_VERTEX_ATTRIB_TEXCOORD0;
                    } else {
#if _DEBUG
                        ta_log_write(&tg_debug_log, SRC_DML,
                            "[%s:%zu:%zu] unexpected attrib type, expected 'position', 'normal' or 'texcoord0', found '%s'\n",
                            value->dbg_symbol.filename, value->dbg_symbol.line, value->dbg_symbol.column, attrib);
#endif
                        result = OGX_UNEXPECTED_VALUE;
                    }
                }
            } else if (field->name == dmls_data) {
                static size_t components_by_attrib[OGX_VERTEX_ATTRIB_COUNT] = {
                    [OGX_VERTEX_ATTRIB_POSIITON ] = 3,
                    [OGX_VERTEX_ATTRIB_NORMAL   ] = 3,
                    [OGX_VERTEX_ATTRIB_TEXCOORD0] = 2,
                };
                size_t comp = components_by_attrib[vertex_array->attrib];
                if (comp == 1) {
                    result = dml_load_float_array(&vertex_array->values.as_float, 0, &field->value);
                } else if (comp == 2) {
                    result = dml_load_vec2_array(&vertex_array->values.as_vec2, &field->value);
                } else if (comp == 3) {
                    result = dml_load_vec3_array(&vertex_array->values.as_vec3, &field->value);
                } else {
#if _DEBUG
                    ta_log_write(&tg_debug_log, SRC_DML,
                        "[%s:%zu:%zu] expected 'attrib' field, '%s' field cannot be loaded before attribute type is known\n",
                        value->dbg_symbol.filename, value->dbg_symbol.line, value->dbg_symbol.column, field->name);
#endif
                    result = OGX_UNKNOWN_TYPE;
                }
            } else {
#if _DEBUG
                ta_log_write(&tg_debug_log, SRC_DML, "[%s:%zu:%zu] unexpected field '%s'\n", value->dbg_symbol.filename,
                    value->dbg_symbol.line, value->dbg_symbol.column, field->name);
#endif
                result = OGX_UNEXPECTED_FIELD;
            }

            if (result != OGX_SUCCESS) {
                dlb_vec_free(vertex_array->values.as_float);
                break;
            }
        }
    }
    return result;
}

static ogx_result dml_load_index_array(ogx_index_array *index_array, DMLValue *value)
{
    assert(index_array);
    assert(value);

    ogx_result result = OGX_SUCCESS;
    if (value->type != DML_VALUE_OBJECT) {
#if _DEBUG
        ta_log_write(&tg_debug_log, SRC_DML, "[%s:%zu:%zu] expected object, found %s\n", value->dbg_symbol.filename,
            value->dbg_symbol.line, value->dbg_symbol.column, DMLValueTypeStr[value->type]);
#endif
        result = OGX_EXPECTED_OBJECT;
    } else {
        dlb_vec_each(DMLField *, field, value->data.as_object.fields) {
            if (field->name == dmls_material) {
                result = dml_load_string(&index_array->material, &field->value);
            } else if (field->name == dmls_data) {
                result = dml_load_float_array(&index_array->values.as_float, 0, &field->value);
            } else {
#if _DEBUG
                ta_log_write(&tg_debug_log, SRC_DML, "[%s:%zu:%zu] unexpected field '%s'\n", value->dbg_symbol.filename,
                    value->dbg_symbol.line, value->dbg_symbol.column, field->name);
#endif
                result = OGX_UNEXPECTED_FIELD;
            }

            if (result != OGX_SUCCESS) {
                dlb_vec_free(index_array->values.as_float);
                break;
            }
        }
    }
    return result;
}

static ogx_result dml_load_skeleton(ogx_skeleton *skeleton, DMLValue *value)
{
    assert(skeleton);
    assert(value);

    ogx_result result = OGX_SUCCESS;
    if (value->type != DML_VALUE_OBJECT) {
#if _DEBUG
        ta_log_write(&tg_debug_log, SRC_DML, "[%s:%zu:%zu] expected object, found %s\n", value->dbg_symbol.filename,
            value->dbg_symbol.line, value->dbg_symbol.column, DMLValueTypeStr[value->type]);
#endif
        result = OGX_EXPECTED_OBJECT;
    } else {
        dlb_vec_each(DMLField *, field, value->data.as_object.fields) {
            if (field->name == dmls_bones) {
                result = dml_load_string_array(&skeleton->bones, &field->value);
            } else if (field->name == dmls_bind_poses) {
                // TODO: Is this a safe cast? Do we even need ogx_mat4 type?
                result = dml_load_mat4_array(&skeleton->bind_poses, &field->value);
            } else {
#if _DEBUG
                ta_log_write(&tg_debug_log, SRC_DML, "[%s:%zu:%zu] unexpected field '%s'\n", value->dbg_symbol.filename,
                    value->dbg_symbol.line, value->dbg_symbol.column, field->name);
#endif
                result = OGX_UNEXPECTED_FIELD;
            }

            if (result != OGX_SUCCESS) {
                dlb_vec_free(skeleton->bones);
                dlb_vec_free(skeleton->bind_poses);
                break;
            }
        }
    }
    return result;
}

static ogx_result dml_load_skin(ogx_skin *skin, DMLValue *value)
{
    assert(skin);
    assert(value);

    ogx_result result = OGX_SUCCESS;
    if (value->type != DML_VALUE_OBJECT) {
#if _DEBUG
        ta_log_write(&tg_debug_log, SRC_DML, "[%s:%zu:%zu] expected object, found %s\n", value->dbg_symbol.filename,
            value->dbg_symbol.line, value->dbg_symbol.column, DMLValueTypeStr[value->type]);
#endif
        result = OGX_EXPECTED_OBJECT;
    } else {
        dlb_vec_each(DMLField *, field, value->data.as_object.fields) {
            if (field->name == dmls_transform) {
                result = dml_load_transform(&skin->transform, &field->value);
            } else if (field->name == dmls_skeleton) {
                result = dml_load_skeleton(&skin->skeleton, &field->value);
            } else if (field->name == dmls_bone_count_array) {
                result = dml_load_float_array(&skin->bone_count_array, 0, &field->value);
            } else if (field->name == dmls_bone_index_array) {
                result = dml_load_float_array(&skin->bone_index_array, 0, &field->value);
            } else if (field->name == dmls_bone_weight_array) {
                result = dml_load_float_array(&skin->bone_weight_array, 0, &field->value);
            } else {
#if _DEBUG
                ta_log_write(&tg_debug_log, SRC_DML, "[%s:%zu:%zu] unexpected field '%s'\n", value->dbg_symbol.filename,
                    value->dbg_symbol.line, value->dbg_symbol.column, field->name);
#endif
                result = OGX_UNEXPECTED_FIELD;
            }

            if (result != OGX_SUCCESS) {
                dlb_vec_free(skin->bone_count_array);
                dlb_vec_free(skin->bone_index_array);
                dlb_vec_free(skin->bone_weight_array);
                break;
            }
        }
    }
    return result;
}

static ogx_result dml_load_mesh(ogx_mesh *mesh, DMLValue *value)
{
    assert(mesh);
    assert(value);

    ogx_result result = OGX_SUCCESS;
    if (value->type != DML_VALUE_OBJECT) {
#if _DEBUG
        ta_log_write(&tg_debug_log, SRC_DML, "[%s:%zu:%zu] expected object, found %s\n", value->dbg_symbol.filename,
            value->dbg_symbol.line, value->dbg_symbol.column, DMLValueTypeStr[value->type]);
#endif
        result = OGX_EXPECTED_OBJECT;
    } else {
        dlb_vec_each(DMLField *, field, value->data.as_object.fields) {
            if (field->name == dmls_vertex_array) {
                ogx_vertex_array *vertex_array = dlb_vec_alloc(mesh->vertex_arrays);
                result = dml_load_vertex_array(vertex_array, &field->value);
            } else if (field->name == dmls_index_array) {
                ogx_index_array *index_array = dlb_vec_alloc(mesh->index_arrays);
                result = dml_load_index_array(index_array, &field->value);
            } else if (field->name == dmls_skin) {
                result = dml_load_skin(&mesh->skin, &field->value);
            } else {
#if _DEBUG
                ta_log_write(&tg_debug_log, SRC_DML, "[%s:%zu:%zu] unexpected field '%s'\n", value->dbg_symbol.filename,
                    value->dbg_symbol.line, value->dbg_symbol.column, field->name);
#endif
                result = OGX_UNEXPECTED_FIELD;
            }

            if (result != OGX_SUCCESS) {
                dlb_vec_free(mesh->vertex_arrays);
                break;
            }
        }
    }
    return result;
}

static ogx_result dml_load_geometry(ogx_geometry *geometry, DMLValue *value)
{
    assert(geometry);
    assert(value);

    ogx_result result = OGX_SUCCESS;
    if (value->type != DML_VALUE_OBJECT) {
#if _DEBUG
        ta_log_write(&tg_debug_log, SRC_DML, "[%s:%zu:%zu] expected object, found %s\n", value->dbg_symbol.filename,
            value->dbg_symbol.line, value->dbg_symbol.column, DMLValueTypeStr[value->type]);
#endif
        result = OGX_EXPECTED_OBJECT;
    } else {
        dlb_vec_each(DMLField *, field, value->data.as_object.fields) {
            if (field->name == dmls_name) {
                result = dml_load_string(&geometry->name, &field->value);
            } else if (field->name == dmls_mesh) {
                result = dml_load_mesh(&geometry->mesh, &field->value);
            } else {
#if _DEBUG
                ta_log_write(&tg_debug_log, SRC_DML, "[%s:%zu:%zu] unexpected field '%s'\n", value->dbg_symbol.filename,
                    value->dbg_symbol.line, value->dbg_symbol.column, field->name);
#endif
                result = OGX_UNEXPECTED_FIELD;
            }

            if (result != OGX_SUCCESS) {
                break;
            }
        }
    }
    return result;
}

static ogx_result dml_load_light_atten(ogx_light_atten *atten, DMLValue *value)
{
    assert(atten);
    assert(value);

    ogx_result result = OGX_SUCCESS;
    if (value->type != DML_VALUE_OBJECT) {
#if _DEBUG
        ta_log_write(&tg_debug_log, SRC_DML, "[%s:%zu:%zu] expected object, found %s\n", value->dbg_symbol.filename,
            value->dbg_symbol.line, value->dbg_symbol.column, DMLValueTypeStr[value->type]);
#endif
        result = OGX_EXPECTED_OBJECT;
    } else {
        dlb_vec_each(DMLField *, field, value->data.as_object.fields) {
            if (field->name == dmls_kind) {
                const char *atten_kind = 0;
                result = dml_load_string(&atten_kind, &field->value);
                if (result == OGX_SUCCESS) {
                    if (!strcmp(atten_kind, "distance")) {
                        atten->kind = OGX_LIGHT_ATTEN_KIND_DISTANCE;
                    } else {
#if _DEBUG
                        ta_log_write(&tg_debug_log, SRC_DML,
                            "[%s:%zu:%zu] unexpected atten kind, expected 'distance', found '%s'\n",
                            value->dbg_symbol.filename, value->dbg_symbol.line, value->dbg_symbol.column, atten_kind);
#endif
                        result = OGX_UNEXPECTED_VALUE;
                    }
                }
            } else if (field->name == dmls_curve) {
                const char *atten_curve = 0;
                result = dml_load_string(&atten_curve, &field->value);
                if (result == OGX_SUCCESS) {
                    if (!strcmp(atten_curve, "inverse_square")) {
                        atten->curve = OGX_LIGHT_ATTEN_CURVE_INVERSE_SQUARE;
                    } else {
#if _DEBUG
                        ta_log_write(&tg_debug_log, SRC_DML,
                            "[%s:%zu:%zu] unexpected atten curve, expected 'inverse_square', found '%s'\n",
                            value->dbg_symbol.filename, value->dbg_symbol.line, value->dbg_symbol.column, atten_curve);
#endif
                        result = OGX_UNEXPECTED_VALUE;
                    }
                }
            } else if (field->name == dmls_scale) {
                result = dml_load_float(&atten->scale, &field->value);
            } else {
#if _DEBUG
                ta_log_write(&tg_debug_log, SRC_DML, "[%s:%zu:%zu] unexpected field '%s'\n", value->dbg_symbol.filename,
                    value->dbg_symbol.line, value->dbg_symbol.column, field->name);
#endif
                result = OGX_UNEXPECTED_FIELD;
            }

            if (result != OGX_SUCCESS) {
                break;
            }
        }
    }
    return result;
}

static ogx_result dml_load_light(ogx_light *light, DMLValue *value)
{
    assert(light);
    assert(value);

    ogx_result result = OGX_SUCCESS;
    if (value->type != DML_VALUE_OBJECT) {
#if _DEBUG
        ta_log_write(&tg_debug_log, SRC_DML, "[%s:%zu:%zu] expected object, found %s\n", value->dbg_symbol.filename,
            value->dbg_symbol.line, value->dbg_symbol.column, DMLValueTypeStr[value->type]);
#endif
        result = OGX_EXPECTED_OBJECT;
    } else {
        dlb_vec_each(DMLField *, field, value->data.as_object.fields) {
            if (field->name == dmls_name) {
                result = dml_load_string(&light->name, &field->value);
            } else if (field->name == dmls_type) {
                const char *light_type = 0;
                result = dml_load_string(&light_type, &field->value);
                if (result == OGX_SUCCESS) {
                    if (!strcmp(light_type, "point")) {
                        light->type = OGX_LIGHT_TYPE_POINT;
                    } else {
#if _DEBUG
                        ta_log_write(&tg_debug_log, SRC_DML,
                            "[%s:%zu:%zu] unexpected light type, expected 'point', found '%s'\n",
                            value->dbg_symbol.filename, value->dbg_symbol.line, value->dbg_symbol.column, light_type);
#endif
                        result = OGX_UNEXPECTED_VALUE;
                    }
                }
            } else if (field->name == dmls_shadow) {
                result = dml_load_bool(&light->shadow, &field->value);
            } else if (field->name == dmls_color) {
                result = dml_load_vec3(&light->color, &field->value);
            } else if (field->name == dmls_intensity) {
                result = dml_load_float(&light->intensity, &field->value);
            } else if (field->name == dmls_atten) {
                //result = dml_load_float(&light->atten, &field->value);
                ogx_light_atten *atten = dlb_vec_alloc(light->attens);
                result = dml_load_light_atten(&atten, &field->value);
            } else {
#if _DEBUG
                ta_log_write(&tg_debug_log, SRC_DML, "[%s:%zu:%zu] unexpected field '%s'\n", value->dbg_symbol.filename,
                    value->dbg_symbol.line, value->dbg_symbol.column, field->name);
#endif
                result = OGX_UNEXPECTED_FIELD;
            }

            if (result != OGX_SUCCESS) {
                break;
            }
        }
    }
    return result;
}

static ogx_result dml_load_material(ogx_material *material, DMLValue *value)
{
    assert(material);
    assert(value);

    ogx_result result = OGX_SUCCESS;
    if (value->type != DML_VALUE_OBJECT) {
#if _DEBUG
        ta_log_write(&tg_debug_log, SRC_DML, "[%s:%zu:%zu] expected object, found %s\n", value->dbg_symbol.filename,
            value->dbg_symbol.line, value->dbg_symbol.column, DMLValueTypeStr[value->type]);
#endif
        result = OGX_EXPECTED_OBJECT;
    } else {
        dlb_vec_each(DMLField *, field, value->data.as_object.fields) {
            if (field->name == dmls_name) {
                result = dml_load_string(&material->name, &field->value);
            } else if (field->name == dmls_albedo_factor) {
                result = dml_load_vec3(&material->albedo_factor, &field->value);
            } else if (field->name == dmls_normal_factor) {
                result = dml_load_vec3(&material->normal_factor, &field->value);
            } else if (field->name == dmls_roughness_factor) {
                result = dml_load_float(&material->roughness_factor, &field->value);
            } else {
#if _DEBUG
                ta_log_write(&tg_debug_log, SRC_DML, "[%s:%zu:%zu] unexpected field '%s'\n", value->dbg_symbol.filename,
                    value->dbg_symbol.line, value->dbg_symbol.column, field->name);
#endif
                result = OGX_UNEXPECTED_FIELD;
            }

            if (result != OGX_SUCCESS) {
                break;
            }
        }
    }
    return result;
}

static ogx_result dml_load_document(ogx_scene *scene, DMLObject *document)
{
    ogx_result result = OGX_EMPTY_DOCUMENT;
    dlb_vec_each(DMLField *, field, document->fields) {
        if (field->name == dmls_bone_node) {
            ogx_node *node = dlb_vec_alloc(scene->nodes);
            node->basic_node.type = OGX_BONE_NODE;
            result = dml_load_node(node, &field->value);
        } else if (field->name == dmls_camera_node) {
            ogx_node *node = dlb_vec_alloc(scene->nodes);
            node->basic_node.type = OGX_CAMERA_NODE;
            result = dml_load_node(node, &field->value);
        } else if (field->name == dmls_geometry_node) {
            ogx_node *node = dlb_vec_alloc(scene->nodes);
            node->basic_node.type = OGX_GEOMETRY_NODE;
            result = dml_load_node(node, &field->value);
        } else if (field->name == dmls_light_node) {
            ogx_node *node = dlb_vec_alloc(scene->nodes);
            node->basic_node.type = OGX_LIGHT_NODE;
            result = dml_load_node(node, &field->value);
        } else if (field->name == dmls_node) {
            ogx_node *node = dlb_vec_alloc(scene->nodes);
            node->basic_node.type = OGX_BASIC_NODE;
            result = dml_load_node(node, &field->value);
        } else if (field->name == dmls_camera) {
            ogx_camera *camera = dlb_vec_alloc(scene->cameras);
            result = dml_load_camera(camera, &field->value);
        } else if (field->name == dmls_geometry) {
            ogx_geometry *geometry = dlb_vec_alloc(scene->geometry);
            result = dml_load_geometry(geometry, &field->value);
        } else if (field->name == dmls_light) {
            ogx_light *light = dlb_vec_alloc(scene->lights);
            result = dml_load_light(light, &field->value);
        } else if (field->name == dmls_material) {
            ogx_material *material = dlb_vec_alloc(scene->materials);
            result = dml_load_material(material, &field->value);
        } else {
#if _DEBUG
            ta_log_write(&tg_debug_log, SRC_DML, "[%s:%zu:%zu] unexpected field '%s'\n", field->dbg_symbol.filename,
                field->dbg_symbol.line, field->dbg_symbol.column, field->name);
#endif
            result = OGX_UNEXPECTED_FIELD;
        }

        if (result != OGX_SUCCESS) {
            break;
        }
    }
    return result;
}

#if 0
// TODO: Make a read_entire_contents helper in DLB
const char *source =
// Scanner errors
//"abc: % # unexpected character\n"
//"abc: ) # unexpected character\n"
//"\"abc  # unterminated string\n"

// Parser errors
//"abc    # missing : after identifier\n"  // note: this error propagates

// Valid document
"ta_light_node: { # This is a comment\n"
"	name: \"light_main_node\"\n"
"	light: \"light_main\"\n"
"	transform: {  # node.matrix_local (ExportNodeTransform)\n"
"		type: \"mat4\"\n"
"		data: [\n"
"			0xbe94ec36, 0x3f748619, 0xbd620dec, 0x00000000,\n"
"			0xbf4566dd, 0xbe4cae39, 0x3f1ac222, 0x00000000,\n"
"			0x3f10ff25, 0x3e5fa1f1, 0x3f4b6fa4, 0x00000000,\n"
"			0x4082709a, 0x3f80b2b7, 0x40bcec70, 0x3f800000\n"
"		]\n"
"	}\n"
"}\n"
"ta_camera_node: {\n"
"	name: \"camera_main_node\"\n"
"	camera: \"camera_main\"\n"
"	transform: {  # node.matrix_local (ExportNodeTransform)\n"
"		type: \"mat4\"\n"
"		data: [\n"
"			0x3f2f987f, 0x3f3a48ff, 0x00000000, 0x00000000,\n"
"			0xbea5e518, 0x3e9c601f, 0x3f6538a6, 0x00000000,\n"
"			0x3f26cc85, 0xbf1d3a45, 0x3ee3fa9d, 0x00000000,\n"
"			0x40eb7c0a, 0xc0dda014, 0x409eaa78, 0x3f800000\n"
"		]\n"
"	}\n"
"}\n";
size_t source_len = strlen(source);
#endif