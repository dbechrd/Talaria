#pragma once
#include "dlb/dlb_types.h"

typedef struct _iobuf FILE;

// HACK: Need to support atomic vectors for e.g. texture->pixels
typedef struct ta_rgba_u8 {
    u8 r, g, b, a;
} ta_rgba_u8;

typedef enum ta_schema_field_type {
    TYP_NULL,

    // Compound types
    TYP_VEC2,
    TYP_VEC3,
    TYP_VEC4,
    TYP_QUAT,
    TYP_MAT3,
    TYP_MAT4,
    TYP_TRANSFORM,
    TYP_RGB,
    TYP_RGBA,
    TYP_RGBA_U8,
    TYP_LIGHT_AMBIENT,
    TYP_LIGHT_DIRECTIONAL,
    TYP_LIGHT_POINT,
    TYP_LIGHT_SPOT,
    TYP_LIGHT_SHADOWMAP,
    TYP_SHADER_ATTRIBUTE,
    TYP_SHADER_UNIFORM,
    TYP_PLANE,
    TYP_SPHERE,
    TYP_AABB,
    TYP_OBB,
    TYP_COLLIDER,

    // Resource types
    TYP_AUDIO_BUFFER,
    TYP_FONT,
    TYP_MATERIAL,
    TYP_MESH_GROUP,
    TYP_SHADER,
    TYP_TEXTURE,

    // Entity wrapper
    TYP_ENTITY,

    // Component types
    TYP_AUDIO_SOURCE,
    TYP_BUTTON,
    TYP_CAMERA,
    TYP_LIGHT,
    TYP_MODEL,
    TYP_POSITION,   // TODO: TYP_LERP_STATE or something..
    TYP_RIGID_BODY,
    TYP_COUNT,

    // Atomic types
    ATOM_BOOL,
    ATOM_UINT8,
    ATOM_INT,
    ATOM_UINT,
    ATOM_FLOAT,
    ATOM_STRING,
    ATOM_ENUM,
} ta_schema_field_type;

typedef enum ta_resource_type {
    RES_COMP_AUDIO_SOURCE,
    RES_COMP_BUTTON,
    RES_COMP_CAMERA,
    RES_COMP_LIGHT,
    RES_COMP_MODEL,
    RES_COMP_POSITION,
    RES_COMP_RIGID_BODY,
    RES_COMP_COUNT,

    RES_AUDIO_BUFFER = RES_COMP_COUNT,
    RES_ENTITY,
    RES_FONT,
    RES_MATERIAL,
    RES_MESH_GROUP,
    RES_SHADER,
    RES_TEXTURE,
    RES_COUNT,
} ta_resource_type;

typedef const char *(enum_to_str)(int);

extern ta_schema_field_type ta_resource_types[RES_COUNT];

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

typedef void (schema_init)(void *ptr);
typedef void (schema_free)(void *ptr);

typedef struct ta_schema {
    ta_schema_field_type type;
    const char *name;
    u32 size;
    schema_init *init;
    schema_free *free;
    ta_schema_field *fields;
} ta_schema;

extern ta_schema tg_schemas[TYP_COUNT];

const char *ta_schema_field_type_str(ta_schema_field_type type);
void ta_schema_register();
ta_schema *ta_schema_find_by_name(const char *name, int len);
ta_schema_field *ta_schema_field_find(ta_schema_field_type type, const char *name);
void ta_schema_print(FILE *f, ta_schema_field_type type, u8 *ptr, int level,
    int in_array);