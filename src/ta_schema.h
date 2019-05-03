#pragma once
#include "dlb_types.h"
#include <stdio.h>

typedef enum {
    F_NULL,

    // Atomic types
    F_ATOM_INT,
    F_ATOM_UINT,
    F_ATOM_FLOAT,
    F_ATOM_STRING,
    F_ATOM_END = F_ATOM_STRING,

    // Object types
    F_TA_VEC3,
    F_TA_VEC4,
    F_TA_RGB,
    F_TA_RGBA,
    F_TA_TRANSFORM,
    F_TA_CAMERA,
    F_TA_SUN_LIGHT,
    F_TA_POINT_LIGHT,
    F_TA_MATERIAL,
    F_TA_MESH,
    F_TA_SHADER,
    F_TA_TEXTURE,
    F_TA_ENTITY,
    F_COUNT,
} ta_schema_field_type;

typedef struct {
    ta_schema_field_type type;
    const char *name;
    int offset;
    bool alias;
} ta_schema_field;

typedef struct {
    ta_schema_field_type type;
    const char *name;
    ta_schema_field *fields;
} ta_schema;

const char *ta_schema_field_type_str(ta_schema_field_type type);
void ta_schema_register();
ta_schema *ta_schema_find(const char *name, int len);
ta_schema_field *ta_schema_field_find(ta_schema_field_type type, const char *name);
void ta_schema_print(FILE *f, ta_schema_field_type type, u8 *ptr, int level);