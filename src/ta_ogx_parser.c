#include "ta_ogx.h"
#include "ta_ogx_parser.h"
#include "dml_scanner.h"
#include "dml_parser.h"
#include "dml.h"
#include "ta_log.h"
#include "ta_file.h"
#include "ta_symbol.h"

#undef near
#undef far

const char *ogx_result_str[OGX_RESULT_COUNT] = {
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

#define OGX_SYMBOL_DECLARE(e) static const char *ogxs_##e = 0;
#define OGX_SYMBOL_DEFINE(e) ogxs_##e = ta_symbol_intern(CSTR(#e));

#define OGX_SYMBOLS(f)  \
    f(albedo_factor)    \
    f(albedo_texture)   \
    f(alpha_factor)     \
    f(alpha_texture)    \
    f(animation)        \
    f(atten)            \
    f(attrib)           \
    f(base)             \
    f(begin)            \
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
    f(end)              \
    f(emissive_factor)  \
    f(emissive_texture) \
    f(far)              \
    f(fov)              \
    f(geometry)         \
    f(geometry_node)    \
    f(index)            \
    f(index_array)      \
    f(intensity)        \
    f(key)              \
    f(kind)             \
    f(light)            \
    f(light_node)       \
    f(material)         \
    f(materials)        \
    f(mesh)             \
    f(metallic_factor)  \
    f(metallic_texture) \
    f(morph)            \
    f(morph_weights)    \
    f(morphs)           \
    f(name)             \
    f(near)             \
    f(node)             \
    f(normal_factor)    \
    f(normal_texture)   \
    f(parent)           \
    f(path)             \
    f(roughness_factor) \
    f(roughness_texture)\
    f(scale)            \
    f(shadow)           \
    f(skeleton)         \
    f(skin)             \
    f(target)           \
    f(target_index)     \
    f(texture)          \
    f(time)             \
    f(track)            \
    f(transform)        \
    f(type)             \
    f(value)            \
    f(vertex_array)     \

OGX_SYMBOLS(OGX_SYMBOL_DECLARE);

//static ogx_result ogx_load_node(dml_document *doc, ogx_node *node, dml_value *value);

static ogx_result ogx_load_bool(bool *boool, dml_value *value)
{
    assert(boool);
    assert(value);

    ogx_result result = OGX_SUCCESS;
    if (value->type != DML_VALUE_LITERAL) {
#if _DEBUG
        ta_log_write(&tg_debug_log, SRC_OGX, "[%s:%zu:%zu] expected literal, found %s\n", value->dbg_symbol.filename,
            value->dbg_symbol.line, value->dbg_symbol.column, dml_value_type_str[value->type]);
#endif
        result = OGX_EXPECTED_LITERAL;
    } else if (value->data.as_literal.type != DML_LITERAL_BOOL) {
#if _DEBUG
        ta_log_write(&tg_debug_log, SRC_OGX, "[%s:%zu:%zu] expected bool literal, found %s\n", value->dbg_symbol.filename,
            value->dbg_symbol.line, value->dbg_symbol.column, dml_literal_type_str[value->data.as_literal.type]);
#endif
        result = OGX_EXPECTED_BOOL;
    } else {
        *boool = value->data.as_literal.data.as_bool;
    }
    return result;
}

static ogx_result ogx_load_string(const char **string, dml_value *value)
{
    assert(string);
    assert(value);

    ogx_result result = OGX_SUCCESS;
    if (value->type != DML_VALUE_LITERAL) {
#if _DEBUG
        ta_log_write(&tg_debug_log, SRC_OGX, "[%s:%zu:%zu] expected literal, found %s\n", value->dbg_symbol.filename,
            value->dbg_symbol.line, value->dbg_symbol.column, dml_value_type_str[value->type]);
#endif
        result = OGX_EXPECTED_LITERAL;
    } else if (value->data.as_literal.type != DML_LITERAL_STRING) {
#if _DEBUG
        ta_log_write(&tg_debug_log, SRC_OGX, "[%s:%zu:%zu] expected string literal, found %s\n", value->dbg_symbol.filename,
            value->dbg_symbol.line, value->dbg_symbol.column, dml_literal_type_str[value->data.as_literal.type]);
#endif
        result = OGX_EXPECTED_STRING;
    } else {
        *string = value->data.as_literal.data.as_string;
    }
    return result;
}

static ogx_result ogx_load_float(float *f, dml_value *value)
{
    assert(f);
    assert(value);

    ogx_result result = OGX_SUCCESS;
    if (value->type != DML_VALUE_LITERAL) {
#if _DEBUG
        ta_log_write(&tg_debug_log, SRC_OGX, "[%s:%zu:%zu] expected literal, found %s\n", value->dbg_symbol.filename,
            value->dbg_symbol.line, value->dbg_symbol.column, dml_value_type_str[value->type]);
#endif
        result = OGX_EXPECTED_LITERAL;
    } else if (value->data.as_literal.type != DML_LITERAL_FLOAT) {
#if _DEBUG
        ta_log_write(&tg_debug_log, SRC_OGX, "[%s:%zu:%zu] expected float literal, found %s\n", value->dbg_symbol.filename,
            value->dbg_symbol.line, value->dbg_symbol.column, dml_literal_type_str[value->data.as_literal.type]);
#endif
        result = OGX_EXPECTED_FLOAT;
    } else {
        *f = value->data.as_literal.data.as_float;
    }
    return result;
}

static ogx_result ogx_load_vec3(dml_document *doc, ogx_vec3 *vec, dml_value *value)
{
    assert(vec);
    assert(value);

    ogx_result result = OGX_SUCCESS;
    if (value->type != DML_VALUE_ARRAY) {
#if _DEBUG
        ta_log_write(&tg_debug_log, SRC_OGX, "[%s:%zu:%zu] expected array, found %s\n", value->dbg_symbol.filename,
            value->dbg_symbol.line, value->dbg_symbol.column, dml_value_type_str[value->type]);
#endif
        result = OGX_EXPECTED_ARRAY;
    } else if (dlb_vec_len(value->data.as_array.values) != 3) {
#if _DEBUG
        ta_log_write(&tg_debug_log, SRC_OGX, "[%s:%zu:%zu] expected array length 3 for vec3, array length is %zu\n", value->dbg_symbol.filename,
            value->dbg_symbol.line, value->dbg_symbol.column, dlb_vec_len(value->data.as_array.values));
#endif
        result = OGX_INVALID_ARRAY_LENGTH;
    } else {
        for (size_t i = 0; i < 3; i++) {
            dml_value *arr_value = &doc->value_pool[value->data.as_array.values[i]];
            if (arr_value->type != DML_VALUE_LITERAL) {
#if _DEBUG
                ta_log_write(&tg_debug_log, SRC_OGX, "[%s:%zu:%zu] expected literal, found %s\n", value->dbg_symbol.filename,
                    value->dbg_symbol.line, value->dbg_symbol.column, dml_value_type_str[value->type]);
#endif
                result = OGX_EXPECTED_LITERAL;
                break;
            } else if (arr_value->data.as_literal.type != DML_LITERAL_FLOAT) {
#if _DEBUG
                ta_log_write(&tg_debug_log, SRC_OGX, "[%s:%zu:%zu] expected float literal, found %s\n", value->dbg_symbol.filename,
                    value->dbg_symbol.line, value->dbg_symbol.column, dml_literal_type_str[value->data.as_literal.type]);
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

static ogx_result ogx_load_mat4(dml_document *doc, ogx_mat4 *matrix, dml_value *value)
{
    assert(matrix);
    assert(value);

    ogx_result result = OGX_SUCCESS;
    if (value->type != DML_VALUE_ARRAY) {
#if _DEBUG
        ta_log_write(&tg_debug_log, SRC_OGX, "[%s:%zu:%zu] expected array, found %s\n", value->dbg_symbol.filename,
            value->dbg_symbol.line, value->dbg_symbol.column, dml_value_type_str[value->type]);
#endif
        result = OGX_EXPECTED_ARRAY;
    } else if (dlb_vec_len(value->data.as_array.values) != 16) {
#if _DEBUG
        ta_log_write(&tg_debug_log, SRC_OGX, "[%s:%zu:%zu] expected array length 16 for mat4, array length is %zu\n", value->dbg_symbol.filename,
            value->dbg_symbol.line, value->dbg_symbol.column, dlb_vec_len(value->data.as_array.values));
#endif
        result = OGX_INVALID_ARRAY_LENGTH;
    } else {
        for (size_t i = 0; i < 16; i++) {
            dml_value *arr_value = &doc->value_pool[value->data.as_array.values[i]];
            if (arr_value->type != DML_VALUE_LITERAL) {
#if _DEBUG
                ta_log_write(&tg_debug_log, SRC_OGX, "[%s:%zu:%zu] expected literal, found %s\n", value->dbg_symbol.filename,
                    value->dbg_symbol.line, value->dbg_symbol.column, dml_value_type_str[value->type]);
#endif
                result = OGX_EXPECTED_LITERAL;
                break;
            } else if (arr_value->data.as_literal.type != DML_LITERAL_FLOAT) {
#if _DEBUG
                ta_log_write(&tg_debug_log, SRC_OGX, "[%s:%zu:%zu] expected float literal, found %s\n", value->dbg_symbol.filename,
                    value->dbg_symbol.line, value->dbg_symbol.column, dml_literal_type_str[value->data.as_literal.type]);
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

static ogx_result ogx_load_string_array(dml_document *doc, const char ***array, dml_value *value)
{
    assert(array);
    assert(value);

    ogx_result result = OGX_SUCCESS;
    if (value->type != DML_VALUE_ARRAY) {
        result = OGX_EXPECTED_ARRAY;
    } else {
        size_t len = dlb_vec_len(value->data.as_array.values);
        dlb_vec_reserve(*array, len);
        dlb_vec_each(size_t *, value_idx, value->data.as_array.values) {
            dml_value *arr_value = &doc->value_pool[*value_idx];
            const char **string = dlb_vec_alloc(*array);
            result = ogx_load_string(string, arr_value);
            if (result != OGX_SUCCESS) {
                dlb_vec_free(*array);
                break;
            }
        }
    }
    return result;
}

static ogx_result ogx_load_float_array(dml_document *doc, float **array, size_t array_len, dml_value *value)
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
                    result = ogx_load_float(*array + i, &doc->value_pool[value->data.as_array.values[i]]);
                    if (result != OGX_SUCCESS) {
                        break;
                    }
                }
            } else {
#if _DEBUG
                ta_log_write(&tg_debug_log, SRC_OGX, "[%s:%zu:%zu] array length %zu does not match requested length %zu\n",
                    value->dbg_symbol.filename, value->dbg_symbol.line, value->dbg_symbol.column, len, array_len);
#endif
                result = OGX_INVALID_ARRAY_LENGTH;
            }
        } else {
            dlb_vec_reserve(*array, len);
            dlb_vec_each(size_t *, value_idx, value->data.as_array.values) {
                dml_value *arr_value = &doc->value_pool[*value_idx];
                float *f = dlb_vec_alloc(*array);
                result = ogx_load_float(f, arr_value);
                if (result != OGX_SUCCESS) {
                    dlb_vec_free(*array);
                    break;
                }
            }
        }
    }
    return result;
}

static ogx_result ogx_load_vec2_array(dml_document *doc, ogx_vec2 **array, dml_value *value)
{
    assert(array);
    assert(value);

    ogx_result result = OGX_SUCCESS;
    if (value->type != DML_VALUE_ARRAY) {
        result = OGX_EXPECTED_ARRAY;
    } else {
        size_t len = dlb_vec_len(value->data.as_array.values);
        dlb_vec_reserve(*array, len);
        dlb_vec_each(size_t *, value_idx, value->data.as_array.values) {
            dml_value *arr_value = &doc->value_pool[*value_idx];
            ogx_vec2 *vec = dlb_vec_alloc(*array);
            result = ogx_load_float_array(doc, &(float *)vec, 2, arr_value);
            if (result != OGX_SUCCESS) {
                dlb_vec_free(*array);
                break;
            }
        }
    }
    return result;
}

static ogx_result ogx_load_vec3_array(dml_document *doc, ogx_vec3 **array, dml_value *value)
{
    assert(array);
    assert(value);

    ogx_result result = OGX_SUCCESS;
    if (value->type != DML_VALUE_ARRAY) {
        result = OGX_EXPECTED_ARRAY;
    } else {
        size_t len = dlb_vec_len(value->data.as_array.values);
        dlb_vec_reserve(*array, len);
        dlb_vec_each(size_t *, value_idx, value->data.as_array.values) {
            dml_value *arr_value = &doc->value_pool[*value_idx];
            ogx_vec3 *vec = dlb_vec_alloc(*array);
            result = ogx_load_float_array(doc, &(float *)vec, 3, arr_value);
            if (result != OGX_SUCCESS) {
                dlb_vec_free(*array);
                break;
            }
        }
    }
    return result;
}

static ogx_result ogx_load_mat4_array(dml_document *doc, ogx_mat4 **array, dml_value *value)
{
    assert(array);
    assert(value);

    ogx_result result = OGX_SUCCESS;
    if (value->type != DML_VALUE_ARRAY) {
        result = OGX_EXPECTED_ARRAY;
    } else {
        size_t len = dlb_vec_len(value->data.as_array.values);
        dlb_vec_reserve(*array, len);
        dlb_vec_each(size_t *, value_idx, value->data.as_array.values) {
            dml_value *arr_value = &doc->value_pool[*value_idx];
            ogx_mat4 *mat = dlb_vec_alloc(*array);
            result = ogx_load_float_array(doc, &(float *)mat, 16, arr_value);
            if (result != OGX_SUCCESS) {
                dlb_vec_free(*array);
                break;
            }
        }
    }
    return result;
}

static ogx_result ogx_load_transform(dml_document *doc, ogx_transform *transform, dml_value *value)
{
    assert(transform);
    assert(value);

    ogx_result result = OGX_SUCCESS;
    if (value->type != DML_VALUE_OBJECT) {
        result = OGX_EXPECTED_OBJECT;
    } else {
        dlb_vec_each(size_t *, field_idx, value->data.as_object.fields) {
            dml_field *field = &doc->field_pool[*field_idx];
            if (field->name == ogxs_type) {
                result = ogx_load_string(&transform->type, &doc->value_pool[field->value_idx]);
            } else if (field->name == ogxs_data) {
                result = ogx_load_mat4(doc, &transform->data, &doc->value_pool[field->value_idx]);
            } else {
#if _DEBUG
                ta_log_write(&tg_debug_log, SRC_OGX, "[%s:%zu:%zu] unexpected field '%s'\n", value->dbg_symbol.filename,
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

static ogx_result ogx_load_key(dml_document *doc, ogx_key *key, dml_value *value)
{
    assert(key);
    assert(value);

    ogx_result result = OGX_SUCCESS;
    if (value->type != DML_VALUE_OBJECT) {
        result = OGX_EXPECTED_OBJECT;
    } else {
        dlb_vec_each(size_t *, field_idx, value->data.as_object.fields) {
            dml_field *field = &doc->field_pool[*field_idx];
            if (field->name == ogxs_kind) {
                const char *key_kind = 0;
                result = ogx_load_string(&key_kind, &doc->value_pool[field->value_idx]);
                if (result == OGX_SUCCESS) {
                    if (!strcmp(key_kind, "value")) {
                        key->kind = OGX_KEY_KIND_VALUE;
                    } else if (!strcmp(key_kind, "+control")) {
                        key->kind = OGX_KEY_KIND_POS_CONTROL;
                    } else if (!strcmp(key_kind, "-control")) {
                        key->kind = OGX_KEY_KIND_NEG_CONTROL;
                    } else {
#if _DEBUG
                        ta_log_write(&tg_debug_log, SRC_OGX,
                            "[%s:%zu:%zu] unexpected key kind, expected 'value', '+control' or '-control', found '%s'\n",
                            value->dbg_symbol.filename, value->dbg_symbol.line, value->dbg_symbol.column, key_kind);
#endif
                        result = OGX_UNEXPECTED_VALUE;
                    }
                }
            } else if (field->name == ogxs_type) {
                const char *type = 0;
                result = ogx_load_string(&type, &doc->value_pool[field->value_idx]);
                if (result == OGX_SUCCESS) {
                    if (!strcmp(type, "float")) {
                        key->type = OGX_TYPE_FLOAT;
                    } else if (!strcmp(type, "mat4")) {
                        key->type = OGX_TYPE_MAT4;
                    } else {
#if _DEBUG
                        ta_log_write(&tg_debug_log, SRC_OGX,
                            "[%s:%zu:%zu] unexpected key type, expected 'float' or 'mat4', found '%s'\n",
                            value->dbg_symbol.filename, value->dbg_symbol.line, value->dbg_symbol.column, type);
#endif
                        result = OGX_UNEXPECTED_TYPE;
                    }
                }
            } else if (field->name == ogxs_data) {
                if (key->type == OGX_TYPE_FLOAT) {
                    result = ogx_load_float_array(doc, &key->values.as_float, 0, &doc->value_pool[field->value_idx]);
                } else if (key->type == OGX_TYPE_MAT4) {
                    result = ogx_load_mat4_array(doc, &key->values.as_mat4, &doc->value_pool[field->value_idx]);
                } else {
#if _DEBUG
                    ta_log_write(&tg_debug_log, SRC_OGX,
                        "[%s:%zu:%zu] expected 'type' field, '%s' field cannot be loaded before type is known\n",
                        value->dbg_symbol.filename, value->dbg_symbol.line, value->dbg_symbol.column, field->name);
#endif
                    result = OGX_UNKNOWN_TYPE;
                }
            } else {
#if _DEBUG
                ta_log_write(&tg_debug_log, SRC_OGX, "[%s:%zu:%zu] unexpected field '%s'\n", value->dbg_symbol.filename,
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

static ogx_result ogx_load_time(dml_document *doc, ogx_time *time, dml_value *value)
{
    assert(time);
    assert(value);

    ogx_result result = OGX_SUCCESS;
    if (value->type != DML_VALUE_OBJECT) {
        result = OGX_EXPECTED_OBJECT;
    } else {
        dlb_vec_each(size_t *, field_idx, value->data.as_object.fields) {
            dml_field *field = &doc->field_pool[*field_idx];
            if (field->name == ogxs_curve) {
                const char *curve = 0;
                result = ogx_load_string(&curve, &doc->value_pool[field->value_idx]);
                if (result == OGX_SUCCESS) {
                    if (!strcmp(curve, "linear")) {
                        time->curve = OGX_TIME_CURVE_LINEAR;
                    } else if (!strcmp(curve, "bezier")) {
                        time->curve = OGX_TIME_CURVE_BEZIER;
                    } else {
#if _DEBUG
                        ta_log_write(&tg_debug_log, SRC_OGX,
                            "[%s:%zu:%zu] unexpected time curve, expected 'linear' or 'bezier', found '%s'\n",
                            value->dbg_symbol.filename, value->dbg_symbol.line, value->dbg_symbol.column, curve);
#endif
                        result = OGX_UNEXPECTED_VALUE;
                    }
                }
            } else if (field->name == ogxs_key) {
                if (time->curve != OGX_TIME_CURVE_UNKNOWN) {
                    result = ogx_load_key(doc, &time->key, &doc->value_pool[field->value_idx]);
                    if (result == OGX_SUCCESS) {
                        if (time->curve != OGX_TIME_CURVE_BEZIER) {
                            if (time->key.kind == OGX_KEY_KIND_POS_CONTROL ||
                                time->key.kind == OGX_KEY_KIND_NEG_CONTROL)
                            {
#if _DEBUG
                                ta_log_write(&tg_debug_log, SRC_OGX,
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
                    ta_log_write(&tg_debug_log, SRC_OGX,
                        "[%s:%zu:%zu] expected 'curve' field, '%s' field cannot be loaded before curve type is known\n",
                        value->dbg_symbol.filename, value->dbg_symbol.line, value->dbg_symbol.column, field->name);
#endif
                    result = OGX_UNKNOWN_TYPE;
                }
            } else {
#if _DEBUG
                ta_log_write(&tg_debug_log, SRC_OGX, "[%s:%zu:%zu] unexpected field '%s'\n", value->dbg_symbol.filename,
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

static ogx_result ogx_load_value(dml_document *doc, ogx_value *val, dml_value *value)
{
    assert(val);
    assert(value);

    ogx_result result = OGX_SUCCESS;
    if (value->type != DML_VALUE_OBJECT) {
        result = OGX_EXPECTED_OBJECT;
    } else {
        dlb_vec_each(size_t *, field_idx, value->data.as_object.fields) {
            dml_field *field = &doc->field_pool[*field_idx];
            if (field->name == ogxs_curve) {
                const char *curve = 0;
                result = ogx_load_string(&curve, &doc->value_pool[field->value_idx]);
                if (result == OGX_SUCCESS) {
                    if (!strcmp(curve, "linear")) {
                        val->curve = OGX_VALUE_CURVE_LINEAR;
                    } else if (!strcmp(curve, "bezier")) {
                        val->curve = OGX_VALUE_CURVE_BEZIER;
                    } else {
#if _DEBUG
                        ta_log_write(&tg_debug_log, SRC_OGX,
                            "[%s:%zu:%zu] unexpected value curve, expected 'linear' or 'bezier', found '%s'\n",
                            value->dbg_symbol.filename, value->dbg_symbol.line, value->dbg_symbol.column, curve);
#endif
                        result = OGX_UNEXPECTED_VALUE;
                    }
                }
            } else if (field->name == ogxs_key) {
                if (val->curve != OGX_VALUE_CURVE_UNKNOWN) {
                    result = ogx_load_key(doc, &val->key, &doc->value_pool[field->value_idx]);
                    if (result == OGX_SUCCESS) {
                        if (val->curve != OGX_VALUE_CURVE_BEZIER) {
                            if (val->key.kind == OGX_KEY_KIND_POS_CONTROL ||
                                val->key.kind == OGX_KEY_KIND_NEG_CONTROL)
                            {
#if _DEBUG
                                ta_log_write(&tg_debug_log, SRC_OGX,
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
                    ta_log_write(&tg_debug_log, SRC_OGX,
                        "[%s:%zu:%zu] expected 'curve' field, '%s' field cannot be loaded before curve type is known\n",
                        value->dbg_symbol.filename, value->dbg_symbol.line, value->dbg_symbol.column, field->name);
#endif
                    result = OGX_UNKNOWN_TYPE;
                }
            } else {
#if _DEBUG
                ta_log_write(&tg_debug_log, SRC_OGX, "[%s:%zu:%zu] unexpected field '%s'\n", value->dbg_symbol.filename,
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

static ogx_result ogx_load_track(dml_document *doc, ogx_track *track, dml_value *value)
{
    assert(track);
    assert(value);

    ogx_result result = OGX_SUCCESS;
    if (value->type != DML_VALUE_OBJECT) {
        result = OGX_EXPECTED_OBJECT;
    } else {
        dlb_vec_each(size_t *, field_idx, value->data.as_object.fields) {
            dml_field *field = &doc->field_pool[*field_idx];
            if (field->name == ogxs_target) {
                result = ogx_load_string(&track->target, &doc->value_pool[field->value_idx]);
            } else if (field->name == ogxs_target_index) {
                result = ogx_load_float(&track->morph_weight_idx, &doc->value_pool[field->value_idx]);
            } else if (field->name == ogxs_time) {
                result = ogx_load_time(doc, &track->time, &doc->value_pool[field->value_idx]);
            } else if (field->name == ogxs_value) {
                result = ogx_load_value(doc, &track->value, &doc->value_pool[field->value_idx]);
            } else {
#if _DEBUG
                ta_log_write(&tg_debug_log, SRC_OGX, "[%s:%zu:%zu] unexpected field '%s'\n", value->dbg_symbol.filename,
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

static ogx_result ogx_load_animation(dml_document *doc, ogx_animation *animation, dml_value *value)
{
    assert(animation);
    assert(value);

    ogx_result result = OGX_SUCCESS;
    if (value->type != DML_VALUE_OBJECT) {
        result = OGX_EXPECTED_OBJECT;
    } else {
        dlb_vec_each(size_t *, field_idx, value->data.as_object.fields) {
            dml_field *field = &doc->field_pool[*field_idx];
            if (field->name == ogxs_begin) {
                result = ogx_load_float(&animation->begin, &doc->value_pool[field->value_idx]);
            } else if (field->name == ogxs_end) {
                result = ogx_load_float(&animation->end, &doc->value_pool[field->value_idx]);
            } else if (field->name == ogxs_track) {
                result = ogx_load_track(doc, &animation->track, &doc->value_pool[field->value_idx]);
            } else {
#if _DEBUG
                ta_log_write(&tg_debug_log, SRC_OGX, "[%s:%zu:%zu] unexpected field '%s'\n", value->dbg_symbol.filename,
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

static ogx_result ogx_load_node(dml_document *doc, ogx_node *node, dml_value *value);
static ogx_result ogx_load_basic_node_field(dml_document *doc, ogx_basic_node *node, dml_field *field)
{
    assert(node);
    assert(field);

    ogx_result result;
    if (field->name == ogxs_name) {
        result = ogx_load_string(&node->name, &doc->value_pool[field->value_idx]);
    } else if (field->name == ogxs_transform) {
        result = ogx_load_transform(doc, &node->transform, &doc->value_pool[field->value_idx]);
    } else if (field->name == ogxs_animation) {
        ogx_animation *animation = dlb_vec_alloc(node->animations);
        result = ogx_load_animation(doc, animation, &doc->value_pool[field->value_idx]);
    } else if (field->name == ogxs_bone_node) {
        ogx_node *child = dlb_vec_alloc(node->children);
        child->basic_node.parent = (ogx_node *)node;
        child->basic_node.type = OGX_BONE_NODE;
        result = ogx_load_node(doc, child, &doc->value_pool[field->value_idx]);
    } else if (field->name == ogxs_camera_node) {
        ogx_node *child = dlb_vec_alloc(node->children);
        child->basic_node.parent = (ogx_node *)node;
        child->basic_node.type = OGX_CAMERA_NODE;
        result = ogx_load_node(doc, child, &doc->value_pool[field->value_idx]);
    } else if (field->name == ogxs_geometry_node) {
        ogx_node *child = dlb_vec_alloc(node->children);
        child->basic_node.parent = (ogx_node *)node;
        child->basic_node.type = OGX_GEOMETRY_NODE;
        result = ogx_load_node(doc, child, &doc->value_pool[field->value_idx]);
    } else if (field->name == ogxs_light_node) {
        ogx_node *child = dlb_vec_alloc(node->children);
        child->basic_node.parent = (ogx_node *)node;
        child->basic_node.type = OGX_LIGHT_NODE;
        result = ogx_load_node(doc, child, &doc->value_pool[field->value_idx]);
    } else if (field->name == ogxs_node) {
        ogx_node *child = dlb_vec_alloc(node->children);
        child->basic_node.parent = (ogx_node *)node;
        child->basic_node.type = OGX_BASIC_NODE;
        result = ogx_load_node(doc, child, &doc->value_pool[field->value_idx]);
    } else {
        result = OGX_UNEXPECTED_FIELD;
    }
    return result;
}

#if 0
static ogx_result ogx_load_bone_node_field(dml_document *doc, ogx_bone_node *node, dml_field *field)
{
    UNUSED(doc);
    assert(node);
    assert(field);

    ogx_result result = OGX_SUCCESS;
    if (false) {
        result = OGX_UNEXPECTED_FIELD;
    }
    return result;
}
#endif

static ogx_result ogx_load_camera_node_field(dml_document *doc, ogx_camera_node *node, dml_field *field)
{
    UNUSED(doc);
    assert(node);
    assert(field);

    ogx_result result;
    if (field->name == ogxs_camera) {
        result = ogx_load_string(&node->camera, &doc->value_pool[field->value_idx]);
    } else {
        result = OGX_UNEXPECTED_FIELD;
    }
    return result;
}

static ogx_result ogx_load_geometry_node_field(dml_document *doc, ogx_geometry_node *node, dml_field *field)
{
    assert(node);
    assert(field);

    ogx_result result;
    if (field->name == ogxs_mesh) {
        result = ogx_load_string(&node->mesh, &doc->value_pool[field->value_idx]);
    } else if (field->name == ogxs_materials) {
        result = ogx_load_string_array(doc, &node->materials, &doc->value_pool[field->value_idx]);
    } else if (field->name == ogxs_morph_weights) {
        result = ogx_load_float_array(doc, &node->morph_weights, 0, &doc->value_pool[field->value_idx]);
    } else {
        result = OGX_UNEXPECTED_FIELD;
    }
    return result;
}

static ogx_result ogx_load_light_node_field(dml_document *doc, ogx_light_node *node, dml_field *field)
{
    UNUSED(doc);
    assert(node);
    assert(field);

    ogx_result result;
    if (field->name == ogxs_light) {
        result = ogx_load_string(&node->light, &doc->value_pool[field->value_idx]);
    } else {
        result = OGX_UNEXPECTED_FIELD;
    }
    return result;
}

static ogx_result ogx_load_node(dml_document *doc, ogx_node *node, dml_value *value)
{
    assert(node);
    assert(value);

    ogx_result result = OGX_SUCCESS;
    if (value->type != DML_VALUE_OBJECT) {
#if _DEBUG
        ta_log_write(&tg_debug_log, SRC_OGX, "[%s:%zu:%zu] expected object, found %s\n", value->dbg_symbol.filename,
            value->dbg_symbol.line, value->dbg_symbol.column, dml_value_type_str[value->type]);
#endif
        result = OGX_EXPECTED_OBJECT;
    } else {
        dlb_vec_each(size_t *, field_idx, value->data.as_object.fields) {
            dml_field *field = &doc->field_pool[*field_idx];
            result = ogx_load_basic_node_field(doc, &node->basic_node, field);
            if (result == OGX_UNEXPECTED_FIELD) {
                switch (node->basic_node.type) {
                    case OGX_BONE_NODE:
                        //result = ogx_load_bone_node_field(doc, &node->bone_node, field);
                        break;
                    case OGX_CAMERA_NODE:
                        result = ogx_load_camera_node_field(doc, &node->camera_node, field);
                        break;
                    case OGX_GEOMETRY_NODE:
                        result = ogx_load_geometry_node_field(doc, &node->geometry_node, field);
                        break;
                    case OGX_LIGHT_NODE:
                        result = ogx_load_light_node_field(doc, &node->light_node, field);
                        break;
                }
            }

            if (result != OGX_SUCCESS) {
#if _DEBUG
                ta_log_write(&tg_debug_log, SRC_OGX, "[%s:%zu:%zu] %s '%s'\n", field->dbg_symbol.filename,
                    field->dbg_symbol.line, field->dbg_symbol.column, ogx_result_str[result], field->name);
#endif
                break;
            }
        }
    }
    return result;
}

static ogx_result ogx_load_camera(dml_document *doc, ogx_camera *camera, dml_value *value)
{
    assert(camera);
    assert(value);

    ogx_result result = OGX_SUCCESS;
    if (value->type != DML_VALUE_OBJECT) {
#if _DEBUG
        ta_log_write(&tg_debug_log, SRC_OGX, "[%s:%zu:%zu] expected object, found %s\n", value->dbg_symbol.filename,
            value->dbg_symbol.line, value->dbg_symbol.column, dml_value_type_str[value->type]);
#endif
        result = OGX_EXPECTED_OBJECT;
    } else {
        dlb_vec_each(size_t *, field_idx, value->data.as_object.fields) {
            dml_field *field = &doc->field_pool[*field_idx];
            if (field->name == ogxs_name) {
                result = ogx_load_string(&camera->name, &doc->value_pool[field->value_idx]);
            } else if (field->name == ogxs_fov) {
                result = ogx_load_float(&camera->fov, &doc->value_pool[field->value_idx]);
            } else if (field->name == ogxs_near) {
                result = ogx_load_float(&camera->nearz, &doc->value_pool[field->value_idx]);
            } else if (field->name == ogxs_far) {
                result = ogx_load_float(&camera->farz, &doc->value_pool[field->value_idx]);
            } else {
#if _DEBUG
                ta_log_write(&tg_debug_log, SRC_OGX, "[%s:%zu:%zu] unexpected field '%s'\n", value->dbg_symbol.filename,
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

static ogx_result ogx_load_vertex_array(dml_document *doc, ogx_vertex_array *vertex_array, dml_value *value)
{
    assert(vertex_array);
    assert(value);

    ogx_result result = OGX_SUCCESS;
    if (value->type != DML_VALUE_OBJECT) {
#if _DEBUG
        ta_log_write(&tg_debug_log, SRC_OGX, "[%s:%zu:%zu] expected object, found %s\n", value->dbg_symbol.filename,
            value->dbg_symbol.line, value->dbg_symbol.column, dml_value_type_str[value->type]);
#endif
        result = OGX_EXPECTED_OBJECT;
    } else {
        dlb_vec_each(size_t *, field_idx, value->data.as_object.fields) {
            dml_field *field = &doc->field_pool[*field_idx];
            if (field->name == ogxs_attrib) {
                const char *attrib = 0;
                result = ogx_load_string(&attrib, &doc->value_pool[field->value_idx]);
                if (result == OGX_SUCCESS) {
                    if (!strcmp(attrib, "position")) {
                        vertex_array->attrib = OGX_VERTEX_ATTRIB_POSIITON;
                    } else if (!strcmp(attrib, "normal")) {
                        vertex_array->attrib = OGX_VERTEX_ATTRIB_NORMAL;
                    } else if (!strcmp(attrib, "tangent")) {
                        vertex_array->attrib = OGX_VERTEX_ATTRIB_TANGENT;
                    } else if (!strcmp(attrib, "texcoord0")) {
                        vertex_array->attrib = OGX_VERTEX_ATTRIB_TEXCOORD0;
                    } else {
#if _DEBUG
                        ta_log_write(&tg_debug_log, SRC_OGX,
                            "[%s:%zu:%zu] unexpected attrib type, expected 'position', 'normal' or 'texcoord0', found '%s'\n",
                            value->dbg_symbol.filename, value->dbg_symbol.line, value->dbg_symbol.column, attrib);
#endif
                        result = OGX_UNEXPECTED_VALUE;
                    }
                }
            } else if (field->name == ogxs_data) {
                static size_t components_by_attrib[OGX_VERTEX_ATTRIB_COUNT] = {
                    [OGX_VERTEX_ATTRIB_POSIITON ] = 3,
                    [OGX_VERTEX_ATTRIB_NORMAL   ] = 3,
                    [OGX_VERTEX_ATTRIB_TANGENT  ] = 3,
                    [OGX_VERTEX_ATTRIB_TEXCOORD0] = 2,
                };
                size_t comp = components_by_attrib[vertex_array->attrib];
                if (comp == 1) {
                    result = ogx_load_float_array(doc, &vertex_array->values.as_float, 0, &doc->value_pool[field->value_idx]);
                } else if (comp == 2) {
                    result = ogx_load_vec2_array(doc, &vertex_array->values.as_vec2, &doc->value_pool[field->value_idx]);
                } else if (comp == 3) {
                    result = ogx_load_vec3_array(doc, &vertex_array->values.as_vec3, &doc->value_pool[field->value_idx]);
                } else {
#if _DEBUG
                    ta_log_write(&tg_debug_log, SRC_OGX,
                        "[%s:%zu:%zu] expected 'attrib' field, '%s' field cannot be loaded before attribute type is known\n",
                        value->dbg_symbol.filename, value->dbg_symbol.line, value->dbg_symbol.column, field->name);
#endif
                    result = OGX_UNKNOWN_TYPE;
                }
            } else if (field->name == ogxs_morph) {
                result = ogx_load_float(&vertex_array->morph, &doc->value_pool[field->value_idx]);
            } else {
#if _DEBUG
                ta_log_write(&tg_debug_log, SRC_OGX, "[%s:%zu:%zu] unexpected field '%s'\n", value->dbg_symbol.filename,
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

static ogx_result ogx_load_index_array(dml_document *doc, ogx_index_array *index_array, dml_value *value)
{
    assert(index_array);
    assert(value);

    ogx_result result = OGX_SUCCESS;
    if (value->type != DML_VALUE_OBJECT) {
#if _DEBUG
        ta_log_write(&tg_debug_log, SRC_OGX, "[%s:%zu:%zu] expected object, found %s\n", value->dbg_symbol.filename,
            value->dbg_symbol.line, value->dbg_symbol.column, dml_value_type_str[value->type]);
#endif
        result = OGX_EXPECTED_OBJECT;
    } else {
        dlb_vec_each(size_t *, field_idx, value->data.as_object.fields) {
            dml_field *field = &doc->field_pool[*field_idx];
            if (field->name == ogxs_material) {
                result = ogx_load_string(&index_array->material, &doc->value_pool[field->value_idx]);
            } else if (field->name == ogxs_data) {
                result = ogx_load_float_array(doc, &index_array->values.as_float, 0, &doc->value_pool[field->value_idx]);
            } else {
#if _DEBUG
                ta_log_write(&tg_debug_log, SRC_OGX, "[%s:%zu:%zu] unexpected field '%s'\n", value->dbg_symbol.filename,
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

static ogx_result ogx_load_skeleton(dml_document *doc, ogx_skeleton *skeleton, dml_value *value)
{
    assert(skeleton);
    assert(value);

    ogx_result result = OGX_SUCCESS;
    if (value->type != DML_VALUE_OBJECT) {
#if _DEBUG
        ta_log_write(&tg_debug_log, SRC_OGX, "[%s:%zu:%zu] expected object, found %s\n", value->dbg_symbol.filename,
            value->dbg_symbol.line, value->dbg_symbol.column, dml_value_type_str[value->type]);
#endif
        result = OGX_EXPECTED_OBJECT;
    } else {
        dlb_vec_each(size_t *, field_idx, value->data.as_object.fields) {
            dml_field *field = &doc->field_pool[*field_idx];
            if (field->name == ogxs_bones) {
                result = ogx_load_string_array(doc, &skeleton->bones, &doc->value_pool[field->value_idx]);
            } else if (field->name == ogxs_bind_poses) {
                // TODO: Is this a safe cast? Do we even need ogx_mat4 type?
                result = ogx_load_mat4_array(doc, &skeleton->bind_poses, &doc->value_pool[field->value_idx]);
            } else {
#if _DEBUG
                ta_log_write(&tg_debug_log, SRC_OGX, "[%s:%zu:%zu] unexpected field '%s'\n", value->dbg_symbol.filename,
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

static ogx_result ogx_load_skin(dml_document *doc, ogx_skin *skin, dml_value *value)
{
    assert(skin);
    assert(value);

    ogx_result result = OGX_SUCCESS;
    if (value->type != DML_VALUE_OBJECT) {
#if _DEBUG
        ta_log_write(&tg_debug_log, SRC_OGX, "[%s:%zu:%zu] expected object, found %s\n", value->dbg_symbol.filename,
            value->dbg_symbol.line, value->dbg_symbol.column, dml_value_type_str[value->type]);
#endif
        result = OGX_EXPECTED_OBJECT;
    } else {
        dlb_vec_each(size_t *, field_idx, value->data.as_object.fields) {
            dml_field *field = &doc->field_pool[*field_idx];
            if (field->name == ogxs_transform) {
                result = ogx_load_transform(doc, &skin->transform, &doc->value_pool[field->value_idx]);
            } else if (field->name == ogxs_skeleton) {
                result = ogx_load_skeleton(doc, &skin->skeleton, &doc->value_pool[field->value_idx]);
            } else if (field->name == ogxs_bone_count_array) {
                result = ogx_load_float_array(doc, &skin->bone_count_array, 0, &doc->value_pool[field->value_idx]);
            } else if (field->name == ogxs_bone_index_array) {
                result = ogx_load_float_array(doc, &skin->bone_index_array, 0, &doc->value_pool[field->value_idx]);
            } else if (field->name == ogxs_bone_weight_array) {
                result = ogx_load_float_array(doc, &skin->bone_weight_array, 0, &doc->value_pool[field->value_idx]);
            } else {
#if _DEBUG
                ta_log_write(&tg_debug_log, SRC_OGX, "[%s:%zu:%zu] unexpected field '%s'\n", value->dbg_symbol.filename,
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

static ogx_result ogx_load_morph(dml_document *doc, ogx_morph *morph, dml_value *value)
{
    assert(morph);
    assert(value);

    ogx_result result = OGX_SUCCESS;
    if (value->type != DML_VALUE_OBJECT) {
#if _DEBUG
        ta_log_write(&tg_debug_log, SRC_OGX, "[%s:%zu:%zu] expected object, found %s\n", value->dbg_symbol.filename,
            value->dbg_symbol.line, value->dbg_symbol.column, dml_value_type_str[value->type]);
#endif
        result = OGX_EXPECTED_OBJECT;
    } else {
        dlb_vec_each(size_t *, field_idx, value->data.as_object.fields) {
            dml_field *field = &doc->field_pool[*field_idx];
            if (field->name == ogxs_name) {
                result = ogx_load_string(&morph->name, &doc->value_pool[field->value_idx]);
            } else if (field->name == ogxs_base) {
                result = ogx_load_float(&morph->base, &doc->value_pool[field->value_idx]);
            } else {
#if _DEBUG
                ta_log_write(&tg_debug_log, SRC_OGX, "[%s:%zu:%zu] unexpected field '%s'\n", value->dbg_symbol.filename,
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

static ogx_result ogx_load_morph_array(dml_document *doc, ogx_morph **array, dml_value *value)
{
    assert(array);
    assert(value);

    ogx_result result = OGX_SUCCESS;
    if (value->type != DML_VALUE_ARRAY) {
        result = OGX_EXPECTED_ARRAY;
    } else {
        size_t len = dlb_vec_len(value->data.as_array.values);
        dlb_vec_reserve(*array, len);
        dlb_vec_each(size_t *, value_idx, value->data.as_array.values) {
            dml_value *arr_value = &doc->value_pool[*value_idx];
            ogx_morph *morph = dlb_vec_alloc(*array);
            result = ogx_load_morph(doc, morph, arr_value);
            if (result != OGX_SUCCESS) {
                dlb_vec_free(*array);
                break;
            }
        }
    }
    return result;
}

static ogx_result ogx_load_mesh(dml_document *doc, ogx_mesh *mesh, dml_value *value)
{
    assert(mesh);
    assert(value);

    ogx_result result = OGX_SUCCESS;
    if (value->type != DML_VALUE_OBJECT) {
#if _DEBUG
        ta_log_write(&tg_debug_log, SRC_OGX, "[%s:%zu:%zu] expected object, found %s\n", value->dbg_symbol.filename,
            value->dbg_symbol.line, value->dbg_symbol.column, dml_value_type_str[value->type]);
#endif
        result = OGX_EXPECTED_OBJECT;
    } else {
        dlb_vec_each(size_t *, field_idx, value->data.as_object.fields) {
            dml_field *field = &doc->field_pool[*field_idx];
            if (field->name == ogxs_vertex_array) {
                ogx_vertex_array *vertex_array = dlb_vec_alloc(mesh->vertex_arrays);
                result = ogx_load_vertex_array(doc, vertex_array, &doc->value_pool[field->value_idx]);
            } else if (field->name == ogxs_index_array) {
                ogx_index_array *index_array = dlb_vec_alloc(mesh->index_arrays);
                result = ogx_load_index_array(doc, index_array, &doc->value_pool[field->value_idx]);
            } else if (field->name == ogxs_skin) {
                result = ogx_load_skin(doc, &mesh->skin, &doc->value_pool[field->value_idx]);
            } else {
#if _DEBUG
                ta_log_write(&tg_debug_log, SRC_OGX, "[%s:%zu:%zu] unexpected field '%s'\n", value->dbg_symbol.filename,
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

static ogx_result ogx_load_geometry(dml_document *doc, ogx_geometry *geometry, dml_value *value)
{
    assert(geometry);
    assert(value);

    ogx_result result = OGX_SUCCESS;
    if (value->type != DML_VALUE_OBJECT) {
#if _DEBUG
        ta_log_write(&tg_debug_log, SRC_OGX, "[%s:%zu:%zu] expected object, found %s\n", value->dbg_symbol.filename,
            value->dbg_symbol.line, value->dbg_symbol.column, dml_value_type_str[value->type]);
#endif
        result = OGX_EXPECTED_OBJECT;
    } else {
        dlb_vec_each(size_t *, field_idx, value->data.as_object.fields) {
            dml_field *field = &doc->field_pool[*field_idx];
            if (field->name == ogxs_name) {
                result = ogx_load_string(&geometry->name, &doc->value_pool[field->value_idx]);
            } else if (field->name == ogxs_morphs) {
                result = ogx_load_morph_array(doc, &geometry->morphs, &doc->value_pool[field->value_idx]);
            } else if (field->name == ogxs_mesh) {
                ogx_mesh *mesh = dlb_vec_alloc(geometry->meshes);
                result = ogx_load_mesh(doc, mesh, &doc->value_pool[field->value_idx]);
            } else {
#if _DEBUG
                ta_log_write(&tg_debug_log, SRC_OGX, "[%s:%zu:%zu] unexpected field '%s'\n", value->dbg_symbol.filename,
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

static ogx_result ogx_load_light_atten(dml_document *doc, ogx_light_atten *atten, dml_value *value)
{
    assert(atten);
    assert(value);

    ogx_result result = OGX_SUCCESS;
    if (value->type != DML_VALUE_OBJECT) {
#if _DEBUG
        ta_log_write(&tg_debug_log, SRC_OGX, "[%s:%zu:%zu] expected object, found %s\n", value->dbg_symbol.filename,
            value->dbg_symbol.line, value->dbg_symbol.column, dml_value_type_str[value->type]);
#endif
        result = OGX_EXPECTED_OBJECT;
    } else {
        dlb_vec_each(size_t *, field_idx, value->data.as_object.fields) {
            dml_field *field = &doc->field_pool[*field_idx];
            if (field->name == ogxs_kind) {
                const char *atten_kind = 0;
                result = ogx_load_string(&atten_kind, &doc->value_pool[field->value_idx]);
                if (result == OGX_SUCCESS) {
                    if (!strcmp(atten_kind, "distance")) {
                        atten->kind = OGX_LIGHT_ATTEN_KIND_DISTANCE;
                    } else {
#if _DEBUG
                        ta_log_write(&tg_debug_log, SRC_OGX,
                            "[%s:%zu:%zu] unexpected atten kind, expected 'distance', found '%s'\n",
                            value->dbg_symbol.filename, value->dbg_symbol.line, value->dbg_symbol.column, atten_kind);
#endif
                        result = OGX_UNEXPECTED_VALUE;
                    }
                }
            } else if (field->name == ogxs_curve) {
                const char *atten_curve = 0;
                result = ogx_load_string(&atten_curve, &doc->value_pool[field->value_idx]);
                if (result == OGX_SUCCESS) {
                    if (!strcmp(atten_curve, "inverse_square")) {
                        atten->curve = OGX_LIGHT_ATTEN_CURVE_INVERSE_SQUARE;
                    } else {
#if _DEBUG
                        ta_log_write(&tg_debug_log, SRC_OGX,
                            "[%s:%zu:%zu] unexpected atten curve, expected 'inverse_square', found '%s'\n",
                            value->dbg_symbol.filename, value->dbg_symbol.line, value->dbg_symbol.column, atten_curve);
#endif
                        result = OGX_UNEXPECTED_VALUE;
                    }
                }
            } else if (field->name == ogxs_scale) {
                result = ogx_load_float(&atten->scale, &doc->value_pool[field->value_idx]);
            } else {
#if _DEBUG
                ta_log_write(&tg_debug_log, SRC_OGX, "[%s:%zu:%zu] unexpected field '%s'\n", value->dbg_symbol.filename,
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

static ogx_result ogx_load_light(dml_document *doc, ogx_light *light, dml_value *value)
{
    assert(light);
    assert(value);

    ogx_result result = OGX_SUCCESS;
    if (value->type != DML_VALUE_OBJECT) {
#if _DEBUG
        ta_log_write(&tg_debug_log, SRC_OGX, "[%s:%zu:%zu] expected object, found %s\n", value->dbg_symbol.filename,
            value->dbg_symbol.line, value->dbg_symbol.column, dml_value_type_str[value->type]);
#endif
        result = OGX_EXPECTED_OBJECT;
    } else {
        dlb_vec_each(size_t *, field_idx, value->data.as_object.fields) {
            dml_field *field = &doc->field_pool[*field_idx];
            if (field->name == ogxs_name) {
                result = ogx_load_string(&light->name, &doc->value_pool[field->value_idx]);
            } else if (field->name == ogxs_type) {
                const char *light_type = 0;
                result = ogx_load_string(&light_type, &doc->value_pool[field->value_idx]);
                if (result == OGX_SUCCESS) {
                    if (!strcmp(light_type, "point")) {
                        light->type = OGX_LIGHT_TYPE_POINT;
                    } else {
#if _DEBUG
                        ta_log_write(&tg_debug_log, SRC_OGX,
                            "[%s:%zu:%zu] unexpected light type, expected 'point', found '%s'\n",
                            value->dbg_symbol.filename, value->dbg_symbol.line, value->dbg_symbol.column, light_type);
#endif
                        result = OGX_UNEXPECTED_VALUE;
                    }
                }
            } else if (field->name == ogxs_shadow) {
                result = ogx_load_bool(&light->shadow, &doc->value_pool[field->value_idx]);
            } else if (field->name == ogxs_color) {
                result = ogx_load_vec3(doc, &light->color, &doc->value_pool[field->value_idx]);
            } else if (field->name == ogxs_intensity) {
                result = ogx_load_float(&light->intensity, &doc->value_pool[field->value_idx]);
            } else if (field->name == ogxs_atten) {
                //result = ogx_load_float(&light->atten, &field->value);
                ogx_light_atten *atten = dlb_vec_alloc(light->attens);
                result = ogx_load_light_atten(doc, atten, &doc->value_pool[field->value_idx]);
            } else {
#if _DEBUG
                ta_log_write(&tg_debug_log, SRC_OGX, "[%s:%zu:%zu] unexpected field '%s'\n", value->dbg_symbol.filename,
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

static ogx_result ogx_load_texture(dml_document *doc, ogx_texture *texture, dml_value *value)
{
    assert(texture);
    assert(value);

    ogx_result result = OGX_SUCCESS;
    if (value->type != DML_VALUE_OBJECT) {
#if _DEBUG
        ta_log_write(&tg_debug_log, SRC_OGX, "[%s:%zu:%zu] expected object, found %s\n", value->dbg_symbol.filename,
            value->dbg_symbol.line, value->dbg_symbol.column, dml_value_type_str[value->type]);
#endif
        result = OGX_EXPECTED_OBJECT;
    } else {
        dlb_vec_each(size_t *, field_idx, value->data.as_object.fields) {
            dml_field *field = &doc->field_pool[*field_idx];
            if (field->name == ogxs_name) {
                result = ogx_load_string(&texture->name, &doc->value_pool[field->value_idx]);
            } else if (field->name == ogxs_path) {
                result = ogx_load_string(&texture->path, &doc->value_pool[field->value_idx]);
            } else {
#if _DEBUG
                ta_log_write(&tg_debug_log, SRC_OGX, "[%s:%zu:%zu] unexpected field '%s'\n", value->dbg_symbol.filename,
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

static ogx_result ogx_load_material(dml_document *doc, ogx_material *material, dml_value *value)
{
    assert(material);
    assert(value);

    ogx_result result = OGX_SUCCESS;
    if (value->type != DML_VALUE_OBJECT) {
#if _DEBUG
        ta_log_write(&tg_debug_log, SRC_OGX, "[%s:%zu:%zu] expected object, found %s\n", value->dbg_symbol.filename,
            value->dbg_symbol.line, value->dbg_symbol.column, dml_value_type_str[value->type]);
#endif
        result = OGX_EXPECTED_OBJECT;
    } else {
        dlb_vec_each(size_t *, field_idx, value->data.as_object.fields) {
            dml_field *field = &doc->field_pool[*field_idx];
            if (field->name == ogxs_name) {
                result = ogx_load_string(&material->name, &doc->value_pool[field->value_idx]);
            } else if (field->name == ogxs_alpha_factor) {
                result = ogx_load_float(&material->alpha_factor, &doc->value_pool[field->value_idx]);
            } else if (field->name == ogxs_alpha_texture) {
                result = ogx_load_string(&material->alpha_texture, &doc->value_pool[field->value_idx]);
            } else if (field->name == ogxs_albedo_factor) {
                result = ogx_load_vec3(doc, &material->albedo_factor, &doc->value_pool[field->value_idx]);
            } else if (field->name == ogxs_albedo_texture) {
                result = ogx_load_string(&material->albedo_texture, &doc->value_pool[field->value_idx]);
            } else if (field->name == ogxs_emissive_factor) {
                result = ogx_load_vec3(doc, &material->emissive_factor, &doc->value_pool[field->value_idx]);
            } else if (field->name == ogxs_emissive_texture) {
                result = ogx_load_string(&material->emissive_texture, &doc->value_pool[field->value_idx]);
            } else if (field->name == ogxs_metallic_factor) {
                result = ogx_load_float(&material->metallic_factor, &doc->value_pool[field->value_idx]);
            } else if (field->name == ogxs_metallic_texture) {
                result = ogx_load_string(&material->metallic_texture, &doc->value_pool[field->value_idx]);
            } else if (field->name == ogxs_normal_factor) {
                result = ogx_load_vec3(doc, &material->normal_factor, &doc->value_pool[field->value_idx]);
            } else if (field->name == ogxs_normal_texture) {
                result = ogx_load_string(&material->normal_texture, &doc->value_pool[field->value_idx]);
            } else if (field->name == ogxs_roughness_factor) {
                result = ogx_load_float(&material->roughness_factor, &doc->value_pool[field->value_idx]);
            } else if (field->name == ogxs_roughness_texture) {
                result = ogx_load_string(&material->roughness_texture, &doc->value_pool[field->value_idx]);
            } else {
#if _DEBUG
                ta_log_write(&tg_debug_log, SRC_OGX, "[%s:%zu:%zu] unexpected field '%s'\n", value->dbg_symbol.filename,
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

static ogx_result ogx_load_scene(dml_document *doc, ogx_scene *scene)
{
    dml_value *root_value = &doc->value_pool[doc->root_value_idx];
    if (root_value->type != DML_VALUE_OBJECT) {
#if _DEBUG
        ta_log_write(&tg_debug_log, SRC_OGX, "[%s:%zu:%zu] unexpected root value type '%s', expected object\n",
            root_value->dbg_symbol.filename, root_value->dbg_symbol.line, root_value->dbg_symbol.column,
            dml_value_type_str[root_value->type]);
#endif
        return OGX_UNEXPECTED_VALUE;
    }

    ogx_result result = OGX_EMPTY_DOCUMENT;
    dml_object *root_object = &root_value->data.as_object;
    dlb_vec_each(size_t *, field_idx, root_object->fields) {
        dml_field *field = &doc->field_pool[*field_idx];
        if (field->name == ogxs_bone_node) {
            ogx_node *node = dlb_vec_alloc(scene->nodes);
            node->basic_node.type = OGX_BONE_NODE;
            result = ogx_load_node(doc, node, &doc->value_pool[field->value_idx]);
        } else if (field->name == ogxs_camera_node) {
            ogx_node *node = dlb_vec_alloc(scene->nodes);
            node->basic_node.type = OGX_CAMERA_NODE;
            result = ogx_load_node(doc, node, &doc->value_pool[field->value_idx]);
        } else if (field->name == ogxs_geometry_node) {
            ogx_node *node = dlb_vec_alloc(scene->nodes);
            node->basic_node.type = OGX_GEOMETRY_NODE;
            result = ogx_load_node(doc, node, &doc->value_pool[field->value_idx]);
        } else if (field->name == ogxs_light_node) {
            ogx_node *node = dlb_vec_alloc(scene->nodes);
            node->basic_node.type = OGX_LIGHT_NODE;
            result = ogx_load_node(doc, node, &doc->value_pool[field->value_idx]);
        } else if (field->name == ogxs_node) {
            ogx_node *node = dlb_vec_alloc(scene->nodes);
            node->basic_node.type = OGX_BASIC_NODE;
            result = ogx_load_node(doc, node, &doc->value_pool[field->value_idx]);
        } else if (field->name == ogxs_camera) {
            ogx_camera *camera = dlb_vec_alloc(scene->cameras);
            result = ogx_load_camera(doc, camera, &doc->value_pool[field->value_idx]);
        } else if (field->name == ogxs_geometry) {
            ogx_geometry *geometry = dlb_vec_alloc(scene->geometry);
            result = ogx_load_geometry(doc, geometry, &doc->value_pool[field->value_idx]);
        } else if (field->name == ogxs_light) {
            ogx_light *light = dlb_vec_alloc(scene->lights);
            result = ogx_load_light(doc, light, &doc->value_pool[field->value_idx]);
        } else if (field->name == ogxs_texture) {
            ogx_texture *texture = dlb_vec_alloc(scene->textures);
            result = ogx_load_texture(doc, texture, &doc->value_pool[field->value_idx]);
        } else if (field->name == ogxs_material) {
            ogx_material *material = dlb_vec_alloc(scene->materials);
            result = ogx_load_material(doc, material, &doc->value_pool[field->value_idx]);
        } else {
#if _DEBUG
            ta_log_write(&tg_debug_log, SRC_OGX, "[%s:%zu:%zu] unexpected field '%s'\n", field->dbg_symbol.filename,
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

ogx_result ogx_scene_from_file(ogx_scene *scene, const char *filename)
{
    ogx_result result = OGX_SUCCESS;
    bool echo = tg_debug_log.echo_stdout;
    tg_debug_log.echo_stdout = true;
    ta_log_timed_region_start(&tg_debug_log, SRC_OGX, CSTR("ogx_parse"));

    if (!ogxs_light_node) {
        OGX_SYMBOLS(OGX_SYMBOL_DEFINE);
    }

    dml_document document = { 0 };
    dml_result doc_result = dml_document_from_file(&document, filename);
    if (doc_result == DML_SUCCESS) {
        ta_log_write(&tg_debug_log, SRC_OGX, "Parsing...\n");
        result = ogx_load_scene(&document, scene);
    } else {
        switch (doc_result) {
            case DML_FILE_INVALID:
                result = OGX_FILE_INVALID;
                break;
            case DML_SYNTAX_ERROR:
                result = OGX_SYNTAX_ERROR;
                break;
            default:
                DLB_ASSERT(!"Unknown DML error, make explicit OGX error code for this case");
                result = OGX_UNKNOWN_DML_ERROR;
                break;
        }
    }

    if (result != OGX_SUCCESS) {
        ta_log_write(&tg_debug_log, SRC_OGX, "Parse failed: %s.\n", ogx_result_str[result]);
        goto cleanup;
    }

cleanup:
    ogx_free(&document);

    ta_log_timed_region_end(&tg_debug_log, CSTR("ogx_parse"));
    tg_debug_log.echo_stdout = echo;
    return result;
}

void ogx_free(dml_document *document)
{
    dlb_vec_free(document->field_pool);
    dlb_vec_free(document->value_pool);
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