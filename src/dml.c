#include "dml.h"
#include "dml_parser.h"
#include "dml_scanner.h"
#include "dml_token.h"
#include "ta_file.h"
#include "dlb/dlb_vector.h"

const char *dml_result_str(dml_result result)
{
    switch (result) {
        case DML_SUCCESS     : return "DML_SUCCESS"     ;
        case DML_FILE_INVALID: return "DML_FILE_INVALID";
        case DML_SYNTAX_ERROR: return "DML_SYNTAX_ERROR";
        default: DLB_ASSERT(!"Invalid enum value"); return "<DML_RESULT_UNKNOWN>";
    }
};

const char *dml_literal_type_str(dml_literal_type literal_type)
{
    switch (literal_type) {
        case DML_LITERAL_NULL  : return "DML_LITERAL_NULL  ";
        case DML_LITERAL_BOOL  : return "DML_LITERAL_BOOL  ";
        case DML_LITERAL_FLOAT : return "DML_LITERAL_FLOAT ";
        case DML_LITERAL_STRING: return "DML_LITERAL_STRING";
        default: DLB_ASSERT(!"Invalid enum value"); return "<DML_LITERAL_UNKNOWN>";
    }
};

const char *dml_value_type_str(dml_value_type value_type)
{
    switch (value_type) {
        case DML_VALUE_OBJECT : return "DML_VALUE_OBJECT ";
        case DML_VALUE_ARRAY  : return "DML_VALUE_ARRAY  ";
        case DML_VALUE_LITERAL: return "DML_VALUE_LITERAL";
        default: DLB_ASSERT(!"Invalid enum value"); return "<DML_VALUE_UNKNOWN>";
    }
};

dml_result dml_document_from_file(dml_document *document, const char *filename)
{
    dml_result result = DML_SUCCESS;

    dml_scanner scanner = { 0 };
    dml_token *tokens = 0;
    dml_parser parser = { 0 };

    ta_log_timed_region_start(&tg_debug_log, SRC_DML, CSTR("dml_load"));
    ta_log_write(&tg_debug_log, SRC_DML, "Reading file %s\n", filename);

    document->filename = filename;

    char *source = ta_file_read_all(filename);
    size_t source_len = dlb_vec_len(source) - 1;
    if (!source) {
        ta_log_write(&tg_debug_log, SRC_DML, "[%s] failed to open file\n", filename);
        result = DML_FILE_INVALID;
        goto cleanup;
    }
    if (!source_len) {
        ta_log_write(&tg_debug_log, SRC_DML, "[%s] empty file\n", filename);
        result = DML_FILE_INVALID;
        goto cleanup;
    }

    ta_log_write(&tg_debug_log, SRC_DML, "Scanning...\n");

    dml_scanner_init(&scanner, source, source_len);
    if (!dml_scanner_scan_tokens(&scanner, &tokens)) {
        ta_log_write(&tg_debug_log, SRC_DML, "Scanner produced errors, skipping parse stage.\n");
#if 0
        ta_log_write(&tg_debug_log, SRC_DML, "Token stream:\n");
        dlb_vec_each(dml_token *, token, tokens) {
            ta_log_write(&tg_debug_log, SRC_DML, "[%04d:%04d] %18s %s", token->line, token->column,
                DMLTokenTypeToString(token->type), token->lexeme);
            if (token->type == TOK_NUMBER) {
                ta_log_write(&tg_debug_log, SRC_DML, " (%f)\n", token->literal.as_float);
            } else {
                ta_log_write(&tg_debug_log, SRC_DML, "\n");
            }
        }
#endif
        result = DML_SYNTAX_ERROR;
        goto cleanup;
    }

    ta_log_write(&tg_debug_log, SRC_DML, "Parsing...\n");

    dml_parser_init(&parser, tokens, filename, source, source_len);
    dml_parser_parse(&parser, document);

#if 0
    fputs("Document:\n", stdout);
    DMLPrintObject(&document, 0);
    fputc('\n', stdout);
#endif

cleanup:
    dlb_vec_free(source);

    ta_log_timed_region_end(&tg_debug_log, CSTR("dml_load"));
    return result;
}

void dml_document_free(dml_document *document)
{
    if (document) {
        dlb_vec_free(document->field_pool);
        dlb_vec_free(document->value_pool);
    }
}

//static void PrintIndent(int indent)
//{
//    for (int i = 0; i < indent; ++i) {
//        fputs("  ", stdout);
//    }
//}
//
//void DMLPrintObject(dml_object *object, int indent)
//{
//    fputs("{\n", stdout);
//    dlb_vec_each(size_t, field_idx, object->fields) {
//        PrintIndent(indent + 1);
//        DMLPrintField(*field, indent + 1);
//    }
//    PrintIndent(indent);
//    fputc('}', stdout);
//}
//
//void DMLPrintArray(dml_array *array, int indent)
//{
//    fputs("[\n", stdout);
//
//    size_t values_len = dlb_vec_len(array->values);
//
//    // Print first element
//    if (values_len) {
//        PrintIndent(indent + 1);
//        DMLPrintValue(array->values[0], indent + 1);
//    }
//
//    // Print remaining elements (w/ comma separators)
//    for (size_t i = 1; i < values_len; ++i) {
//        fputc(',', stdout);
//        if (i % 8 == 0) {
//            fputc('\n', stdout);
//            PrintIndent(indent + 1);
//        }
//        DMLPrintValue(array->values[i], indent + 1);
//    }
//
//    fputc('\n', stdout);
//    PrintIndent(indent);
//    fputc(']', stdout);
//}
//
//void DMLPrintLiteral(dml_literal *literal)
//{
//    switch (literal->type) {
//        case DML_LITERAL_NULL:
//            fputs("null", stdout);
//            break;
//        case DML_LITERAL_BOOL:
//            if (literal->data.as_bool) {
//                fputs("true", stdout);
//            } else {
//                fputs("false", stdout);
//            }
//            break;
//        case DML_LITERAL_FLOAT:
//            fprintf(stdout, "0x%08x", (int)literal->data.as_float);
//            break;
//        case DML_LITERAL_STRING:
//            fprintf(stdout, "\"%s\"", literal->data.as_string);
//            break;
//    }
//}
//
//void DMLPrintValue(dml_value *value, int indent)
//{
//    switch (value->type) {
//        case DML_VALUE_OBJECT:
//            DMLPrintObject(&value->data.as_object, indent);
//            break;
//        case DML_VALUE_ARRAY:
//            DMLPrintArray(&value->data.as_array, indent);
//            break;
//        case DML_VALUE_LITERAL:
//            DMLPrintLiteral(&value->data.as_literal);
//            break;
//    }
//}
//
//void DMLPrintField(dml_field *field, int indent)
//{
//    fprintf(stdout, "%s: ", field->name);
//    DMLPrintValue(&field->value, indent);
//    fputc('\n', stdout);
//}