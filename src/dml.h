#pragma once

typedef enum dml_result {
    DML_SUCCESS,
    DML_FILE_INVALID,
    DML_SYNTAX_ERROR,

    DML_RESULT_COUNT
} dml_result;

#define DML_ERROR_CONTEXT_TAB_WIDTH 2

#if _DEBUG
    typedef struct dml_debug_symbol {
        const char *filename;
        size_t line;
        size_t column;
    } dml_debug_symbol;
    #define DML_DEBUG_SYMBOL dml_debug_symbol dbg_symbol;
#else
    #define DML_DEBUG_SYMBOL
#endif

typedef struct dml_object {
    size_t *fields;  // index into field_pool
    DML_DEBUG_SYMBOL
} dml_object;

typedef struct dml_array {
    size_t *values;  // index into value_pool
    DML_DEBUG_SYMBOL
} dml_array;

typedef enum dml_literal_type {
    DML_LITERAL_NULL,
    DML_LITERAL_BOOL,
    DML_LITERAL_FLOAT,
    DML_LITERAL_STRING,
    DML_LITERAL_COUNT
} dml_literal_type;


// NOTE: Representing ints as floats in the parser. Maximum representable int is 2^24 (16,777,216).
// TODO: If we need 32-bit integers, we should parse into a double instead: 2^53 (9,007,199,254,740,992).
typedef struct dml_literal {
    dml_literal_type type;
    union {
        int as_bool;
        float as_float;
        const char *as_string;
    } data;
    DML_DEBUG_SYMBOL
} dml_literal;

typedef enum dml_value_type {
    DML_VALUE_OBJECT,
    DML_VALUE_ARRAY,
    DML_VALUE_LITERAL,
    DML_VALUE_COUNT
} dml_value_type;


typedef struct dml_value {
    dml_value_type type;
    union {
        dml_object as_object;
        dml_array as_array;
        dml_literal as_literal;
    } data;
    DML_DEBUG_SYMBOL
} dml_value;

typedef struct dml_field {
    const char *name;  // symbol
    size_t value_idx;
    DML_DEBUG_SYMBOL
} dml_field;

#undef DML_DEBUG_SYMBOL

typedef struct dml_document {
    const char *filename;
    dml_field *field_pool;   // vector
    dml_value *value_pool;   // vector
    size_t root_value_idx;
    struct ogx_scene *scene; // temp scene we're loading the document into
} dml_document;

const char *dml_result_str(dml_result result);
const char *dml_literal_type_str(dml_literal_type literal_type);
const char *dml_value_type_str(dml_value_type value_type);

dml_result dml_document_from_file(dml_document *document, const char *filename);
void dml_document_free(dml_document *document);

//void DMLPrintObject(dml_object *object, int indent);
//void DMLPrintArray(dml_array *array, int indent);
//void DMLPrintLiteral(dml_literal *literal);
//void DMLPrintValue(dml_value *value, int indent);
//void DMLPrintField(dml_field *field, int indent);