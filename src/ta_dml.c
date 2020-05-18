#include "ta_dml.h"
#include "dml_scanner.h"
#include "dml_parser.h"
#include "dml.h"
#include "ta_log.h"
#include "ta_file.h"
#include "ta_symbol.h"

#undef near
#undef far

static const char *dml_result_str[OGX_RESULT_COUNT] = {
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

static dml_result dml_load_document(ogx_scene *scene, DMLObject *document);

dml_result dml_load(const char *filename)
{
    dml_result result = OGX_SUCCESS;

    if (!dmls_light_node) {
        DML_SYMBOLS(SYMBOL_DEFINE);
    }

    ta_log_timed_region_start(&tg_debug_log, SRC_DML, CSTR("dml_load"));
    ta_log_write(&tg_debug_log, SRC_DML, "Loading %s\n", filename);

    char *source = ta_file_read_all(filename);
    size_t source_len = dlb_vec_len(source) - 1;
    if (!source) {
        ta_log_write(&tg_debug_log, SRC_DML, "[FATAL] Unable to open file [%s].\n", filename);
        result = OGX_FILE_INVALID;
        goto cleanup;
    }
    if (!source_len) {
        ta_log_write(&tg_debug_log, SRC_DML, "[FATAL] File was empty [%s].\n", filename);
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
    DMLParserInit(&parser, tokens, source, source_len);

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
        ta_log_write(&tg_debug_log, SRC_DML, "Load failed: %s.\n", dml_result_str[result]);
        goto cleanup;
    }

    ta_log_write(&tg_debug_log, SRC_DML, "Loaded successfully.\n");

cleanup:
    dlb_vec_free(source);
    // TODO: DMLObjectFree(document);

    ta_log_timed_region_end(&tg_debug_log, CSTR("dml_load"));
    return result;
}

static dml_result dml_load_string(const char **string, DMLValue *value)
{
    assert(string);
    assert(value);

    dml_result result = OGX_SUCCESS;
    if (value->type != DML_VALUE_LITERAL) {
        result = OGX_EXPECTED_LITERAL;
    } else if (value->data.as_literal.type != DML_LITERAL_STRING) {
        result = OGX_EXPECTED_STRING;
    } else {
        *string = value->data.as_literal.data.as_string;
    }
    return result;
}

static dml_result dml_load_mat4(ogx_mat4 *matrix, DMLValue *value)
{
    assert(matrix);
    assert(value);

    dml_result result = OGX_SUCCESS;
    if (value->type != DML_VALUE_ARRAY) {
        result = OGX_EXPECTED_ARRAY;
    } else if (dlb_vec_len(value->data.as_array.values) != 16) {
        result = OGX_INVALID_ARRAY_LENGTH;
    } else {
        for (size_t i = 0; i < 16; i++) {
            DMLValue *arr_value = value->data.as_array.values + i;
            if (arr_value->type != DML_VALUE_LITERAL) {
                result = OGX_EXPECTED_LITERAL;
                break;
            } else if (arr_value->data.as_literal.type != DML_LITERAL_FLOAT) {
                result = OGX_EXPECTED_FLOAT;
                break;
            } else {
                (*matrix)[i] = arr_value->data.as_literal.data.as_float;
            }
        }
    }
    return result;
}

static dml_result dml_load_transform(ogx_transform *transform, DMLValue *value)
{
    assert(transform);
    assert(value);

    dml_result result = OGX_SUCCESS;
    if (value->type != DML_VALUE_OBJECT) {
        result = OGX_EXPECTED_OBJECT;
    } else {
        dlb_vec_each(DMLField *, field, value->data.as_object.fields) {
            if (field->name == dmls_type) {
                result = dml_load_string(&transform->type, &field->value);
            } else if (field->name == dmls_data) {
                result = dml_load_mat4(&transform->data, &field->value);
            } else {
                result = OGX_UNEXPECTED_FIELD;
            }

            if (result != OGX_SUCCESS) {
                break;
            }
        }
    }
    return result;
}

static dml_result dml_load_basic_node_field(ogx_basic_node *node, DMLField *field)
{
    assert(node);
    assert(field);

    dml_result result;
    if (field->name == dmls_name) {
        result = dml_load_string(&node->name, &field->value);
    } else if (field->name == dmls_transform) {
        result = dml_load_transform(&node->transform, &field->value);
    } else if (field->name == dmls_parent) {
        result = OGX_NOT_IMPLEMENTED;
    } else if (field->name == dmls_children) {
        result = OGX_NOT_IMPLEMENTED;
    } else {
        result = OGX_UNEXPECTED_FIELD;
    }
    return result;
}

static dml_result dml_load_camera_node_field(ogx_camera_node *node, DMLField *field)
{
    assert(node);
    assert(field);

    dml_result result;
    if (field->name == dmls_camera) {
        result = dml_load_string(&node->camera, &field->value);
    } else {
        result = OGX_UNEXPECTED_FIELD;
    }
    return result;
}

static dml_result dml_load_geometry_node_field(ogx_geometry_node *node, DMLField *field)
{
    assert(node);
    assert(field);

    dml_result result;
    if (field->name == dmls_mesh) {
        result = dml_load_string(&node->mesh, &field->value);
    } else if (field->name == dmls_materials) {
        // TODO: Load array of strings
        //result = dml_load_string(&node->mesh, &field->value);
        result = OGX_NOT_IMPLEMENTED;
    } else {
        result = OGX_UNEXPECTED_FIELD;
    }
    return result;
}

static dml_result dml_load_light_node_field(ogx_light_node *node, DMLField *field)
{
    assert(node);
    assert(field);

    dml_result result;
    if (field->name == dmls_light) {
        result = dml_load_string(&node->light, &field->value);
    } else {
        result = OGX_UNEXPECTED_FIELD;
    }
    return result;
}

static dml_result dml_load_node(ogx_node *node, DMLValue *value)
{
    assert(node);
    assert(value);

    dml_result result = OGX_SUCCESS;
    if (value->type != DML_VALUE_OBJECT) {
        result = OGX_EXPECTED_OBJECT;
    } else {
        dlb_vec_each(DMLField *, field, value->data.as_object.fields) {
            result = dml_load_basic_node_field(&node->basic_node, field);
            if (result == OGX_UNEXPECTED_FIELD) {
                switch (node->basic_node.type) {
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
                break;
            }
        }
    }
    return result;
}

static dml_result dml_load_document(ogx_scene *scene, DMLObject *document)
{
    dml_result result = OGX_EMPTY_DOCUMENT;
    dlb_vec_each(DMLField *, field, document->fields) {
        printf("name: %s ", field->name);
        printf("value: %s\n", DMLValueTypeStr[field->value.type]);

        if (field->name == dmls_camera_node) {
            ogx_node *node = dlb_vec_alloc(scene->nodes);
            node->basic_node.type = OGX_CAMERA_NODE;
            result = dml_load_node(node, &field->value);
        } else if (field->name == dmls_geometry) {
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
        } else if (field->name == dmls_light) {
            result = OGX_NOT_IMPLEMENTED;
        } else if (field->name == dmls_camera) {
            result = OGX_NOT_IMPLEMENTED;
        } else if (field->name == dmls_material) {
            result = OGX_NOT_IMPLEMENTED;
        } else {
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