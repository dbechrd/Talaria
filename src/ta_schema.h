#pragma once
#include "dlb/dlb_types.h"

typedef struct _iobuf FILE;

// TODO: OGX -> TA mapping
// [x] node -> ta_transform
// [ ] bode_node -> <3 options>:
//     1) nothing, transform being bone is implicit based on skeleton->skin->nodes containing its name
//     2) ta_transform with `node_type` flag of TA_NODE_BONE
//     3) ta_bone component, doesn't have any properties other than a name for now
// [ ] geometry_node -> ta_model (probably need this one..)

// [x] geometry -> ta_mesh
// [x] mesh -> ta_mesh
// [x] texture -> ta_texture
// [x] material -> ta_material


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
    TYP_MAT3,
    TYP_MAT4,
    TYP_XFORM,
    TYP_RGB,
    TYP_RGBA,
    TYP_RGBA_U8,
    TYP_LIGHT_AMBIENT,
    TYP_SHADOW_MAP_PROPERTIES,
    TYP_LIGHT_DIRECTIONAL,
    TYP_LIGHT_POINT,
    TYP_LIGHT_SPOT,
    TYP_SHADER_ATTRIBUTE,
    TYP_SHADER_UNIFORM,
    TYP_PLANE,
    TYP_SPHERE,
    TYP_AABB,
    TYP_OBB,
    TYP_CAPSULE,
    TYP_COLLIDER,
    TYP_ANIMATION_SAMPLER,
    TYP_ANIMATION_CHANNEL,
    TYP_SKELETON,
    TYP_SKIN,

    //TYP_OGX_TRANSFORM,
    //TYP_OGX_NODE_LIGHT,
    //TYP_OGX_NODE_CAMERA,
    //TYP_OGX_NODE_GEOMETRY,
    //TYP_OGX_NODE,
    //TYP_OGX_KEY,
    //TYP_OGX_TIME,
    //TYP_OGX_VALUE,
    //TYP_OGX_TRACK,
    //TYP_OGX_ANIMATION,
    //TYP_OGX_BONE_NODE,
    //TYP_OGX_VERTEX_ARRAY,
    //TYP_OGX_INDEX_ARRAY,
    //TYP_OGX_SKELETON,
    //TYP_OGX_SKIN,
    //TYP_OGX_MESH,
    //TYP_OGX_GEOMETRY,
    //TYP_OGX_ATTEN,
    //TYP_OGX_LIGHT,
    //TYP_OGX_CAMERA,
    //TYP_OGX_SCENE,

    // Component types
    TYP_AUDIO_SOURCE,
    TYP_BONE,
    TYP_BUTTON,
    TYP_CAMERA,
    TYP_GUN,
    TYP_LIGHT,
    TYP_MODEL,
    TYP_PLAYER,
    TYP_TRANSFORM,
    TYP_RIGID_BODY,

    // Resource types
    TYP_ANIMATION,
    TYP_AUDIO_BUFFER,
    TYP_CUBEMAP,
    TYP_FONT,
    TYP_MATERIAL,
    TYP_MESH,
    TYP_SHADER,
    TYP_TEXTURE,

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

typedef enum ta_res_type {
    // Component types
    RES_COMP_AUDIO_SOURCE,
    RES_COMP_BONE,
    RES_COMP_BUTTON,
    RES_COMP_CAMERA,
    RES_COMP_GUN,
    RES_COMP_LIGHT,
    RES_COMP_MODEL,
    RES_COMP_PLAYER,
    RES_COMP_TRANSFORM,
    RES_COMP_RIGID_BODY,
    RES_COMP_COUNT,
    // Resource types
    RES_ANIMATION = RES_COMP_COUNT,
    RES_AUDIO_BUFFER,
    RES_CUBEMAP,
    RES_FONT,
    RES_MATERIAL,
    RES_MESH,
    RES_SHADER,
    RES_TEXTURE,
    RES_COUNT,
} ta_res_type;

// All resource structs must start with this header
#define TA_RESOURCE_HEADER \
    ta_res_type res_type;   /* resource type      */ \
    size_t      index;      /* pool index         */ \
    const char  *name;      /* resource name      */

// All component structs must start with this header
#define TA_COMPONENT_HEADER  \
    TA_RESOURCE_HEADER       \
    const char *entity; /* owning entity name */

typedef struct ta_resource {
    TA_RESOURCE_HEADER
} ta_resource;

typedef struct ta_component {
    TA_COMPONENT_HEADER
} ta_component;

ta_schema_field_type res_to_typ(ta_res_type type);
ta_res_type typ_to_res(ta_schema_field_type type);

typedef const char *(enum_to_str)(int);

typedef struct ta_schema_field {
    ta_schema_field_type type;      // field type
    const char   *name;             // field name
    size_t       offset;            // field offset in the struct the parent schema represents
    size_t       size;              // field size in the struct
    size_t       array_len;         // 0 = not array, 1 = vector, >1 = fixed array size
    bool         is_alias;          // true if this field is just an alias for another field
    enum_to_str  *enum_converter;   // enum to string converter (for enum fields)
    bool         is_union_type;     // true if this field is a union
    bool         in_union;          // true if this field is in a union
    int          union_type;        // enum representing the type for which this union field is associated (used to
                                    // validate union fields are only present when they match the field type)
} ta_schema_field;

typedef void (schema_init)(void *ptr);
typedef void (schema_free)(void *ptr);

typedef struct ta_schema {
    ta_schema_field_type type;  // field type of the top-most node representing this schema
    const char      *name;      // field name of the top-most node (i.e. "schema name")
    size_t          size;       // size in bytes of the schema type and all of its children
    schema_init     *init;      // constructor
    schema_free     *free;      // destructor
    ta_schema_field *fields;    // array of fields in this schema type
} ta_schema;

extern ta_schema tg_schemas[TYP_COUNT];

void ta_schema_field_type_str   (ta_schema_field_type type, const char **str);
void ta_res_type_str            (ta_res_type type, const char **str);
void ta_schema_register         ();
void ta_schema_find_by_name     (const char *name, int len, ta_schema **schema);
void ta_schema_field_find       (ta_schema_field_type type, const char *name, ta_schema_field **field);
void ta_schema_print            (FILE *f, ta_schema_field_type type, u8 *ptr, int level, int in_array);
void ta_schema_print_json       (FILE *f, ta_schema_field_type type, u8 *ptr, int level, int in_array);
