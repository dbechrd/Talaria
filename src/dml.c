#include "dml.h"
#include "dlb/dlb_vector.h"

const char *DMLLiteralTypeStr[DML_LITERAL_COUNT] = {
    [DML_LITERAL_NULL  ] = "DML_LITERAL_NULL  ",
    [DML_LITERAL_BOOL  ] = "DML_LITERAL_BOOL  ",
    [DML_LITERAL_FLOAT] = "DML_LITERAL_DOUBLE",
    [DML_LITERAL_STRING] = "DML_LITERAL_STRING",
};
const char *DMLValueTypeStr[DML_VALUE_COUNT] = {
    [DML_VALUE_OBJECT ] = "DML_VALUE_OBJECT ",
    [DML_VALUE_ARRAY  ] = "DML_VALUE_ARRAY  ",
    [DML_VALUE_LITERAL] = "DML_VALUE_LITERAL",
};

static void PrintIndent(int indent)
{
    for (int i = 0; i < indent; ++i) {
        fputs("  ", stdout);
    }
}

void DMLPrintObject(DMLObject *object, int indent)
{
    fputs("{\n", stdout);
    dlb_vec_each(DMLField *, field, object->fields) {
        PrintIndent(indent + 1);
        DMLPrintField(field, indent + 1);
    }
    PrintIndent(indent);
    fputc('}', stdout);
}

void DMLPrintArray(DMLArray *array, int indent)
{
    fputs("[\n", stdout);

    size_t values_len = dlb_vec_len(array->values);

    // Print first element
    if (values_len) {
        PrintIndent(indent + 1);
        DMLPrintValue(array->values, indent + 1);
    }

    // Print remaining elements (w/ comma separators)
    for (size_t i = 1; i < values_len; ++i) {
        fputc(',', stdout);
        if (i % 8 == 0) {
            fputc('\n', stdout);
            PrintIndent(indent + 1);
        }
        DMLPrintValue(array->values + i, indent + 1);
    }

    fputc('\n', stdout);
    PrintIndent(indent);
    fputc(']', stdout);
}

void DMLPrintLiteral(DMLLiteral *literal)
{
    switch (literal->type) {
        case DML_LITERAL_NULL:
            fputs("null", stdout);
            break;
        case DML_LITERAL_BOOL:
            if (literal->data.as_bool) {
                fputs("true", stdout);
            } else {
                fputs("false", stdout);
            }
            break;
        case DML_LITERAL_FLOAT:
            fprintf(stdout, "0x%08x", (int)literal->data.as_float);
            break;
        case DML_LITERAL_STRING:
            fprintf(stdout, "\"%s\"", literal->data.as_string);
            break;
    }
}

void DMLPrintValue(DMLValue *value, int indent)
{
    switch (value->type) {
        case DML_VALUE_OBJECT:
            DMLPrintObject(&value->data.as_object, indent);
            break;
        case DML_VALUE_ARRAY:
            DMLPrintArray(&value->data.as_array, indent);
            break;
        case DML_VALUE_LITERAL:
            DMLPrintLiteral(&value->data.as_literal);
            break;
    }
}

void DMLPrintField(DMLField *field, int indent)
{
    fprintf(stdout, "%s: ", field->name);
    DMLPrintValue(&field->value, indent);
    fputc('\n', stdout);
}