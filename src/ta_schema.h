#pragma once
#include "dlb/dlb_types.h"
#include <stdio.h>

typedef enum ta_schema_field_type {
    TYP_NULL,

    // Scene-level compound types
	TYP_CAMERA,
	TYP_LIGHT,
	TYP_MATERIAL,
	TYP_MESH_GROUP,
	TYP_SHADER,
	TYP_TEXTURE,
	TYP_NODE,
	TYP_AUDIO_BUFFER,
	TYP_AUDIO_SOURCE,
	TYP_RIGID_BODY,
	TYP_BUTTON,
    TYP_FONT,
    TYP_COUNT_POOLS,

    // Other compound types
    TYP_VEC2,
    TYP_VEC3,
    TYP_VEC4,
    TYP_QUAT,
    TYP_MAT3,
    TYP_MAT4,
    TYP_RGB,
    TYP_RGBA,
    TYP_TRANSFORM,
    TYP_LIGHT_AMBIENT,
    TYP_LIGHT_DIRECTIONAL,
    TYP_LIGHT_POINT,
    TYP_LIGHT_SPOT,
    TYP_SHADER_ATTRIBUTE,
    TYP_SHADER_UNIFORM,
    TYP_PLANE,
    TYP_SPHERE,
    TYP_AABB,
    TYP_OBB,
    TYP_COLLIDER,
    TYP_COUNT,

    // Atomic types
    ATOM_BOOL           = 0x400,
    ATOM_INT,
    ATOM_UINT,
    ATOM_FLOAT,
    ATOM_STRING,
    ATOM_ENUM,
} ta_schema_field_type;

typedef const char *(enum_to_str)(int);

typedef struct ta_schema_field {
    ta_schema_field_type type;
    const char *name;
    u32 offset;
    u32 size;
    u32 array_len;  // Note: 0 = not array, 1 = vector, >1 = fixed array size
    bool is_alias;
    enum_to_str *enum_converter;
    bool is_union_type;
    bool in_union;
    int union_type;
} ta_schema_field;

typedef struct ta_schema {
    ta_schema_field_type type;
    const char *name;
    u32 size;
    ta_schema_field *fields;
} ta_schema;

const char *ta_schema_field_type_str(ta_schema_field_type type);
void ta_schema_register();
ta_schema *ta_schema_find_by_type(ta_schema_field_type type);
ta_schema *ta_schema_find_by_name(const char *name, int len);
ta_schema_field *ta_schema_field_find(ta_schema_field_type type, const char *name);
void ta_schema_print(FILE *f, ta_schema_field_type type, u8 *ptr, int level,
    int in_array);