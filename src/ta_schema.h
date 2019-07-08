#pragma once
#include "dlb_types.h"
#include <stdio.h>

typedef enum {
    TA_NULL,

    // Scene-level compound types
	TA_CAMERA,
	TA_LIGHT,
	TA_MATERIAL,
	TA_MESH_GROUP,
	TA_SHADER,
	TA_TEXTURE,
	TA_NODE,
	TA_AUDIO_BUFFER,
	TA_AUDIO_SOURCE,
	TA_RIGID_BODY,
	TA_BUTTON,
    TA_COUNT_POOLS,

    // Other compound types
    TA_VEC2,
    TA_VEC3,
    TA_VEC4,
    TA_QUAT,
    TA_MAT3,
    TA_MAT4,
    TA_RGB,
    TA_RGBA,
    TA_TRANSFORM,
    TA_AMBIENT_LIGHT,
    TA_DIRECTIONAL_LIGHT,
    TA_POINT_LIGHT,
    TA_SPOT_LIGHT,
    TA_SHADER_ATTRIBUTE,
    TA_SHADER_UNIFORM,
    TA_PLANE,
    TA_SPHERE,
    TA_AABB,
    TA_OBB,
    TA_COLLIDER,
    TA_COUNT,

    // Atomic types
    ATOM_BOOL					= 0x400,
    ATOM_INT,
    ATOM_UINT,
    ATOM_FLOAT,
    ATOM_STRING,
    ATOM_ENUM,
} ta_schema_field_type;

typedef const char *(enum_to_str)(int);

typedef struct {
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

typedef struct {
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