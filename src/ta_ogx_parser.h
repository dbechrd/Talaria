#pragma once
#include <stdint.h>

typedef enum ogx_result {
    OGX_SUCCESS,
    OGX_FILE_INVALID,
    OGX_SYNTAX_ERROR,
    OGX_EMPTY_DOCUMENT,
    OGX_UNEXPECTED_VALUE,
    OGX_UNEXPECTED_TYPE,
    OGX_UNEXPECTED_FIELD,
    OGX_EXPECTED_LITERAL,
    OGX_EXPECTED_BOOL,
    OGX_EXPECTED_STRING,
    OGX_EXPECTED_FLOAT,
    OGX_EXPECTED_ARRAY,
    OGX_EXPECTED_OBJECT,
    OGX_INVALID_ARRAY_LENGTH,
    OGX_NOT_IMPLEMENTED,
    OGX_UNKNOWN_DML_ERROR,

    OGX_RESULT_COUNT
} ogx_result;

const char *ogx_result_str[OGX_RESULT_COUNT];

ogx_result ogx_scene_from_file(struct ogx_scene *scene, const char *filename);
void ogx_free(struct dml_document *document);