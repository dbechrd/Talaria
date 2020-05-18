#pragma once

#define DML_ERROR_CONTEXT_TAB_WIDTH 2

#if _DEBUG
    typedef struct DMLDebugSymbol {
        const char *filename;
        size_t line;
        size_t column;
    } DMLDebugSymbol;
    #define DML_DEBUG_SYMBOL DMLDebugSymbol dbg_symbol
#else
    #define DML_DEBUG_SYMBOL
#endif

// TODO: DMLField pool
typedef struct DMLObject {
    struct DMLField *fields;
    DML_DEBUG_SYMBOL;
} DMLObject;

// TODO: DMLValue pool
typedef struct DMLArray {
    struct DMLValue *values;
    DML_DEBUG_SYMBOL;
} DMLArray;

typedef enum DMLLiteralType {
    DML_LITERAL_NULL,
    DML_LITERAL_BOOL,
    DML_LITERAL_FLOAT,
    DML_LITERAL_STRING,
    DML_LITERAL_COUNT
} DMLLiteralType;

extern const char *DMLLiteralTypeStr[DML_LITERAL_COUNT];

typedef struct DMLLiteral {
    DMLLiteralType type;
    union {
        float as_float;
        const char *as_string;
        int as_bool;
    } data;
    DML_DEBUG_SYMBOL;
} DMLLiteral;

typedef enum DMLValueType {
    DML_VALUE_OBJECT,
    DML_VALUE_ARRAY,
    DML_VALUE_LITERAL,
    DML_VALUE_COUNT
} DMLValueType;

extern const char *DMLValueTypeStr[DML_VALUE_COUNT];

typedef struct DMLValue {
    DMLValueType type;
    union {
        DMLObject as_object;
        DMLArray as_array;
        DMLLiteral as_literal;
    } data;
    DML_DEBUG_SYMBOL;
} DMLValue;

typedef struct DMLField {
    const char *name;  // symbol
    DMLValue value;
    DML_DEBUG_SYMBOL;
} DMLField;

void DMLPrintObject(DMLObject *object, int indent);
void DMLPrintArray(DMLArray *array, int indent);
void DMLPrintLiteral(DMLLiteral *literal);
void DMLPrintValue(DMLValue *value, int indent);
void DMLPrintField(DMLField *field, int indent);