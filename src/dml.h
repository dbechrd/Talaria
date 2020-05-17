#pragma once

#define DML_ERROR_CONTEXT_TAB_WIDTH 2

typedef struct DMLObject {
    struct DMLField *fields;
} DMLObject;

typedef struct DMLArray {
    struct DMLValue *values;
} DMLArray;

typedef enum DMLLiteralType {
    DML_LITERAL_NULL,
    DML_LITERAL_BOOL,
    DML_LITERAL_DOUBLE,
    DML_LITERAL_STRING,
} DMLLiteralType;

typedef struct DMLLiteral {
    DMLLiteralType type;
    union {
        float as_float;
        const char *as_string;
        int as_bool;
    } data;
} DMLLiteral;

typedef enum DMLValueType {
    DML_VALUE_OBJECT,
    DML_VALUE_ARRAY,
    DML_VALUE_LITERAL,
} DMLValueType;

typedef struct DMLValue {
    DMLValueType type;
    union {
        DMLObject as_object;
        DMLArray as_array;
        DMLLiteral as_literal;
    } data;
} DMLValue;

typedef struct DMLField {
    const char *name;
    DMLValue value;
} DMLField;

void DMLPrintObject(DMLObject *object, int indent);
void DMLPrintArray(DMLArray *array, int indent);
void DMLPrintLiteral(DMLLiteral *literal);
void DMLPrintValue(DMLValue *value, int indent);
void DMLPrintField(DMLField *field, int indent);