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

extern const char *dml_literal_type_str[DML_LITERAL_COUNT];

typedef struct dml_literal {
    dml_literal_type type;
    union {
        float as_float;
        const char *as_string;
        int as_bool;
    } data;
    DML_DEBUG_SYMBOL
} dml_literal;

typedef enum dml_value_type {
    DML_VALUE_OBJECT,
    DML_VALUE_ARRAY,
    DML_VALUE_LITERAL,
    DML_VALUE_COUNT
} dml_value_type;

extern const char *dml_value_type_str[DML_VALUE_COUNT];

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

dml_result dml_document_from_file(dml_document *document, const char *filename);
void dml_document_free(dml_document *document);

//void DMLPrintObject(dml_object *object, int indent);
//void DMLPrintArray(dml_array *array, int indent);
//void DMLPrintLiteral(dml_literal *literal);
//void DMLPrintValue(dml_value *value, int indent);
//void DMLPrintField(dml_field *field, int indent);