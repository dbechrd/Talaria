#include "ta_schema.h"
#include "ta_symbol.h"
#include "ta_math.h"
#include "ta_file.h"
#include "ta_camera.h"
#include "ta_mesh.h"
#include "ta_texture.h"
#include "ta_shader.h"
#include "ta_entity.h"
#include "dlb_types.h"
#include "dlb_vector.h"
#include "dlb_hash.h"
#include <stdlib.h>

static ta_schema tg_schemas[F_TA_COUNT];
static dlb_hash tg_schemas_by_name;

const char *ta_schema_field_type_str(ta_schema_field_type type) {
    switch(type) {
        case F_TA_NULL:        return "NULL";

        // Compound types
        case F_TA_VEC3:        return "TA_VEC3";
        case F_TA_VEC4:        return "TA_VEC4";
        case F_TA_RGB:         return "TA_COLOR3";
        case F_TA_RGBA:        return "TA_COLOR4";
        case F_TA_TRANSFORM:   return "TA_TRANSFORM";
        case F_TA_CAMERA:      return "TA_CAMERA";
        case F_TA_SUN_LIGHT:   return "TA_SUN_LIGHT";
        case F_TA_POINT_LIGHT: return "TA_POINT_LIGHT";
        case F_TA_MATERIAL:    return "TA_MATERIAL";
        case F_TA_MESH_GROUP:        return "TA_MESH";
        case F_TA_SHADER:      return "TA_SHADER";
        case F_TA_TEXTURE:     return "TA_TEXTURE";
        case F_TA_ENTITY:      return "TA_ENTITY";

        // Atomic types
        case F_ATOM_INT:       return "ATOM_INT";
        case F_ATOM_UINT:      return "ATOM_UINT";
        case F_ATOM_FLOAT:     return "ATOM_FLOAT";
        case F_ATOM_STRING:    return "ATOM_STRING";

        default: return "<UNKNOWN_TA_FIELD_TYPE>";
    }
};

static void type_field_add(ta_schema *schema, ta_schema_field_type type,
    const char *name, u32 offset, u32 size, bool array, bool alias)
{
    ta_schema_field *field = dlb_vec_alloc(schema->fields);
    field->type = type;
    field->name = name;
    field->offset = offset;
    field->size = size;
    field->array = array;
    field->alias = alias;
}

#define TYPE_START(_type, field_type) \
    schema = &tg_schemas[field_type]; \
    schema->type = field_type; \
    schema->name = INTERN(STRING(_type)); \
    schema->size = sizeof(_type);

#define TYPE_FIELD(type, field, field_type) \
    type_field_add(schema, field_type, INTERN(#field), OFFSETOF(type, field), SIZEOF_MEMBER(type, field), 0, 0)

#define TYPE_ARRAY(type, field, field_type) \
    type_field_add(schema, field_type, INTERN(#field), OFFSETOF(type, field), SIZEOF_MEMBER_ARRAY(type, field), 1, 0)

//#define TYPE_FIELD_ALIAS(type, field, field_type, array) \
//    type_field_add(schema, field_type, INTERN(#field), OFFSETOF(type, field), array, 1)

#define TYPE_END(type) \
    dlb_hash_insert(&tg_schemas_by_name, CSTR(STRING(type)), schema);

void ta_schema_register()
{
    DLB_ASSERT(!tg_schemas_by_name.size);
    dlb_hash_init(&tg_schemas_by_name, DLB_HASH_STRING, "[schema_register]", 32);
    ta_schema *schema;

#if 0
    // Atomic types
    TYPE_START(int, F_ATOM_INT);
    TYPE_END(int);

    TYPE_START(unsigned int, F_ATOM_UINT);
    TYPE_END(unsigned int);

    TYPE_START(float, F_ATOM_FLOAT);
    TYPE_END(float);

    TYPE_START(const char *, F_ATOM_STRING);
    TYPE_END(const char *);
#endif

    // Compound types
    TYPE_START(ta_vec3, F_TA_VEC3);
    TYPE_FIELD(ta_vec3, x, F_ATOM_FLOAT);
    TYPE_FIELD(ta_vec3, y, F_ATOM_FLOAT);
    TYPE_FIELD(ta_vec3, z, F_ATOM_FLOAT);
    TYPE_END(ta_vec3);

    TYPE_START(ta_vec4, F_TA_VEC4);
    TYPE_FIELD(ta_vec4, x, F_ATOM_FLOAT);
    TYPE_FIELD(ta_vec4, y, F_ATOM_FLOAT);
    TYPE_FIELD(ta_vec4, z, F_ATOM_FLOAT);
    TYPE_FIELD(ta_vec4, w, F_ATOM_FLOAT);
    TYPE_END(ta_vec4);

    TYPE_START(ta_rgb, F_TA_RGB);
    TYPE_FIELD(ta_rgb, r, F_ATOM_FLOAT);
    TYPE_FIELD(ta_rgb, g, F_ATOM_FLOAT);
    TYPE_FIELD(ta_rgb, b, F_ATOM_FLOAT);
    TYPE_END(ta_rgb);

    TYPE_START(ta_rgba, F_TA_RGBA);
    TYPE_FIELD(ta_rgba, r, F_ATOM_FLOAT);
    TYPE_FIELD(ta_rgba, g, F_ATOM_FLOAT);
    TYPE_FIELD(ta_rgba, b, F_ATOM_FLOAT);
    TYPE_FIELD(ta_rgba, a, F_ATOM_FLOAT);
    TYPE_END(ta_rgba);

    TYPE_START(ta_transform, F_TA_TRANSFORM);
    TYPE_FIELD(ta_transform, position, F_TA_VEC3);
    TYPE_FIELD(ta_transform, rotation, F_TA_VEC4);
    TYPE_FIELD(ta_transform, scale,    F_TA_VEC3);
    TYPE_END(ta_transform);

    // Scene-level object types
    TYPE_START(ta_camera, F_TA_CAMERA);
    TYPE_FIELD(ta_camera, uid,         F_ATOM_STRING);
    TYPE_FIELD(ta_camera, position,    F_TA_VEC3);
    TYPE_FIELD(ta_camera, velocity,    F_ATOM_FLOAT);
    TYPE_FIELD(ta_camera, mode,        F_ATOM_INT);
    TYPE_FIELD(ta_camera, target,      F_TA_VEC3);
    TYPE_FIELD(ta_camera, yaw,         F_ATOM_FLOAT);
    TYPE_FIELD(ta_camera, yaw_accel,   F_ATOM_FLOAT);
    TYPE_FIELD(ta_camera, pitch,       F_ATOM_FLOAT);
    TYPE_FIELD(ta_camera, pitch_min,   F_ATOM_FLOAT);
    TYPE_FIELD(ta_camera, pitch_max,   F_ATOM_FLOAT);
    TYPE_FIELD(ta_camera, pitch_accel, F_ATOM_FLOAT);
    TYPE_END(ta_camera);

    TYPE_START(ta_sun_light, F_TA_SUN_LIGHT);
    TYPE_FIELD(ta_sun_light, uid,       F_ATOM_STRING);
    TYPE_FIELD(ta_sun_light, direction, F_TA_VEC3);
    TYPE_FIELD(ta_sun_light, color,     F_TA_RGB);
    TYPE_END(ta_sun_light);

    TYPE_START(ta_point_light, F_TA_POINT_LIGHT);
    TYPE_FIELD(ta_point_light, uid,      F_ATOM_STRING);
    TYPE_FIELD(ta_point_light, position, F_TA_VEC3);
    TYPE_FIELD(ta_point_light, color,    F_TA_RGB);
    TYPE_END(ta_point_light);

    TYPE_START(ta_material, F_TA_MATERIAL);
    TYPE_FIELD(ta_material, uid,         F_ATOM_STRING);
    TYPE_FIELD(ta_material, shader_uid,  F_ATOM_STRING);
    TYPE_FIELD(ta_material, texture_uid, F_ATOM_STRING);
    TYPE_END(ta_material);

    TYPE_START(ta_mesh_group, F_TA_MESH_GROUP);
    TYPE_FIELD(ta_mesh_group, uid,  F_ATOM_STRING);
    TYPE_FIELD(ta_mesh_group, path, F_ATOM_STRING);
    TYPE_END(ta_mesh_group);

    TYPE_START(ta_shader, F_TA_SHADER);
    TYPE_FIELD(ta_shader, uid,        F_ATOM_STRING);
    TYPE_FIELD(ta_shader, path_vert,  F_ATOM_STRING);
    TYPE_FIELD(ta_shader, path_frag,  F_ATOM_STRING);
    TYPE_ARRAY(ta_shader, attributes, F_TA_SHADER_ATTRIBUTE);
    TYPE_ARRAY(ta_shader, uniforms,   F_TA_SHADER_UNIFORM);
    TYPE_END(ta_shader);

    TYPE_START(ta_shader_attribute, F_TA_SHADER_ATTRIBUTE);
    TYPE_FIELD(ta_shader_attribute, name, F_ATOM_STRING);
    TYPE_FIELD(ta_shader_attribute, type, F_ATOM_STRING);
    TYPE_END(ta_shader_attribute);

    TYPE_START(ta_shader_uniform, F_TA_SHADER_UNIFORM);
    TYPE_FIELD(ta_shader_uniform, name, F_ATOM_STRING);
    TYPE_FIELD(ta_shader_uniform, type, F_ATOM_STRING);
    TYPE_END(ta_shader_uniform);

    TYPE_START(ta_texture, F_TA_TEXTURE);
    TYPE_FIELD(ta_texture, uid,  F_ATOM_STRING);
    TYPE_FIELD(ta_texture, path, F_ATOM_STRING);
    TYPE_END(ta_texture);

    TYPE_START(ta_entity, F_TA_ENTITY);
    TYPE_FIELD(ta_entity, uid,          F_ATOM_STRING);
    TYPE_FIELD(ta_entity, material_uid, F_ATOM_STRING);
    TYPE_FIELD(ta_entity, mesh_uid,     F_ATOM_STRING);
    TYPE_FIELD(ta_entity, shader_uid,   F_ATOM_STRING);
    TYPE_FIELD(ta_entity, texture_uid,  F_ATOM_STRING);
    TYPE_FIELD(ta_entity, transform,    F_TA_TRANSFORM);
    TYPE_END(ta_entity);
}

#undef TYPE_START
#undef TYPE_FIELD
//#undef TYPE_FIELD_ALIAS
#undef TYPE_END

ta_schema *ta_schema_find(const char *name, int len)
{
    ta_schema *schema = dlb_hash_search(&tg_schemas_by_name, name, len);
    return schema;
}

ta_schema_field *ta_schema_field_find(ta_schema_field_type type, const char *name)
{
    ta_schema_field *field = 0;
    ta_schema *obj = &tg_schemas[type];
    for (ta_schema_field *f = obj->fields; f != dlb_vec_end(obj->fields); f++) {
        if (f->name == name) {
            field = f;
            break;
        }
    }
    return field;
}

void ta_schema_print_atom(FILE *f, ta_schema_field *field, void *ptr)
{
    fprintf(f, "%s: ", field->name);
    switch (field->type) {
        case F_ATOM_INT: {
            int *val = ptr;
            fprintf(f, "%d", *val);
            break;
        } case F_ATOM_UINT: {
            u32 *val = ptr;
            fprintf(f, "%u", *val);
            break;
        } case F_ATOM_FLOAT: {
            float *val = ptr;
            fprintf(f, "0x%08X (%f)", *(u32 *)val, *val);
            break;
        } case F_ATOM_STRING: {
            const char **val = ptr;
            //fprintf(f, "\"%s\"  # %08X\n", IFNULL(*val, ""), (u32)*val);
            fprintf(f, "\"%s\"", IFNULL(*val, ""));
            break;
        } default: {
            PANIC("Unexpected field type, don't know how to print");
        }
    }
}

static inline void indent(FILE *f, int count)
{
    for (int i = 0; i < count; i++) {
        fprintf(f, "  ");
    }
}

void ta_schema_print(FILE *f, ta_schema_field_type type, u8 *ptr, int level, bool in_array)
{
    ta_schema *schema = &tg_schemas[type];
    if (!in_array) {
        if (level == 0) {
            fprintf(f, "%s:\n", schema->name);
        }
    }

    for (ta_schema_field *field = schema->fields; field != dlb_vec_end(schema->fields); field++)
    {
        if (field->alias) {
            continue;
        }

        if (field->array) {
            DLB_ASSERT(!in_array && "Don't know how to print nested arrays");
            indent(f, level + 1);
            fprintf(f, "%s: [\n", field->name);

            u8 *arr = *(void **)(ptr + field->offset);
            u32 arr_len = dlb_vec_len(arr);
            u8 *arr_end = arr + (arr_len * field->size);
            for (u8 *p = arr; p != arr_end; p += field->size) {
                if (field->type < F_TA_COUNT) {
                    indent(f, level + 2);
                    fprintf(f, "{\n");
                    ta_schema_print(f, field->type, p, level + 2, 1);
                    indent(f, level + 2);
                    fprintf(f, "},\n");
                } else {
                    ta_schema_print_atom(f, field, p);
                }
            }

            indent(f, level + 1);
            fprintf(f, "]\n");
        } else {
            indent(f, level + 1);
            if (field->type < F_TA_COUNT) {
                fprintf(f, "%s:\n", field->name);
                ta_schema_print(f, field->type, ptr + field->offset, level + 1, 0);
            } else {
                ta_schema_print_atom(f, field, ptr + field->offset);
            }
            if (in_array) {
                fprintf(f, ",");
            }
            fprintf(f, "\n");
        }
    }
}