#include "ta_schema.h"
#include "ta_symbol.h"
#include "ta_math.h"
#include "ta_file.h"
#include "ta_camera.h"
#include "ta_mesh.h"
#include "ta_texture.h"
#include "ta_entity.h"
#include "dlb_types.h"
#include "dlb_vector.h"
#include "dlb_hash.h"
#include <stdlib.h>

static ta_schema tg_schemas[F_COUNT];
static dlb_hash tg_schemas_by_name;

const char *ta_schema_field_type_str(ta_schema_field_type type) {
    switch(type) {
        case F_ATOM_INT:       return "ATOM_INT";
        case F_ATOM_UINT:      return "ATOM_UINT";
        case F_ATOM_FLOAT:     return "ATOM_FLOAT";
        case F_ATOM_STRING:    return "ATOM_STRING";
        case F_TA_VEC3:        return "TA_VEC3";
        case F_TA_VEC4:        return "TA_VEC4";
        case F_TA_RGB:         return "TA_COLOR3";
        case F_TA_RGBA:        return "TA_COLOR4";
        case F_TA_TRANSFORM:   return "TA_TRANSFORM";
        case F_TA_CAMERA:      return "TA_CAMERA";
        case F_TA_SUN_LIGHT:   return "TA_SUN_LIGHT";
        case F_TA_POINT_LIGHT: return "TA_POINT_LIGHT";
        case F_TA_MATERIAL:    return "TA_MATERIAL";
        case F_TA_MESH:        return "TA_MESH";
        case F_TA_SHADER:      return "TA_SHADER";
        case F_TA_TEXTURE:     return "TA_TEXTURE";
        case F_TA_ENTITY:      return "TA_ENTITY";
        default: return "<UNKNOWN_TA_FIELD_TYPE>";
    }
};

static void obj_field_add(ta_schema *obj, ta_schema_field_type type, const char *name,
    u32 offset, bool alias)
{
    ta_schema_field *field = dlb_vec_alloc(obj->fields);
    field->type = type;
    field->name = name;
    field->offset = offset;
    field->alias = alias;
}

#define OBJ_START(_type, field_type) \
    obj = &tg_schemas[field_type]; \
    obj->type = field_type; \
    obj->name = INTERN(STRING(_type));

#define OBJ_FIELD(type, field, field_type) \
    obj_field_add(obj, field_type, INTERN(#field), OFFSETOF(type, field), 0)

#define OBJ_END(type) \
    dlb_hash_insert(&tg_schemas_by_name, CSTR(STRING(type)), obj);

void ta_schema_register()
{
    DLB_ASSERT(!tg_schemas_by_name.size);
    dlb_hash_init(&tg_schemas_by_name, DLB_HASH_STRING, "[schema_register]", 32);
    ta_schema *obj;

    // Atomic types
    OBJ_START(int, F_ATOM_INT);
    OBJ_END(int);

    OBJ_START(unsigned int, F_ATOM_UINT);
    OBJ_END(unsigned int);

    OBJ_START(float, F_ATOM_FLOAT);
    OBJ_END(float);

    OBJ_START(const char *, F_ATOM_STRING);
    OBJ_END(const char *);

    // Compound types
    OBJ_START(ta_vec3, F_TA_VEC3);
    OBJ_FIELD(ta_vec3, x, F_ATOM_FLOAT);
    OBJ_FIELD(ta_vec3, y, F_ATOM_FLOAT);
    OBJ_FIELD(ta_vec3, z, F_ATOM_FLOAT);
    OBJ_END(ta_vec3);

    OBJ_START(ta_vec4, F_TA_VEC4);
    OBJ_FIELD(ta_vec4, x, F_ATOM_FLOAT);
    OBJ_FIELD(ta_vec4, y, F_ATOM_FLOAT);
    OBJ_FIELD(ta_vec4, z, F_ATOM_FLOAT);
    OBJ_FIELD(ta_vec4, w, F_ATOM_FLOAT);
    OBJ_END(ta_vec4);

    OBJ_START(ta_rgb, F_TA_RGB);
    OBJ_FIELD(ta_rgb, r, F_ATOM_FLOAT);
    OBJ_FIELD(ta_rgb, g, F_ATOM_FLOAT);
    OBJ_FIELD(ta_rgb, b, F_ATOM_FLOAT);
    OBJ_END(ta_rgb);

    OBJ_START(ta_rgba, F_TA_RGBA);
    OBJ_FIELD(ta_rgba, r, F_ATOM_FLOAT);
    OBJ_FIELD(ta_rgba, g, F_ATOM_FLOAT);
    OBJ_FIELD(ta_rgba, b, F_ATOM_FLOAT);
    OBJ_FIELD(ta_rgba, a, F_ATOM_FLOAT);
    OBJ_END(ta_rgba);

    OBJ_START(ta_transform, F_TA_TRANSFORM);
    OBJ_FIELD(ta_transform, position, F_TA_VEC3);
    OBJ_FIELD(ta_transform, rotation, F_TA_VEC4);
    OBJ_FIELD(ta_transform, scale,    F_TA_VEC3);
    OBJ_END(ta_transform);

    // Scene-level object types
    OBJ_START(ta_camera, F_TA_CAMERA);
    OBJ_FIELD(ta_camera, position,    F_TA_VEC3);
    OBJ_FIELD(ta_camera, velocity,    F_ATOM_FLOAT);
    OBJ_FIELD(ta_camera, mode,        F_ATOM_INT);
    OBJ_FIELD(ta_camera, target,      F_TA_VEC3);
    OBJ_FIELD(ta_camera, yaw,         F_ATOM_FLOAT);
    OBJ_FIELD(ta_camera, yaw_accel,   F_ATOM_FLOAT);
    OBJ_FIELD(ta_camera, pitch,       F_ATOM_FLOAT);
    OBJ_FIELD(ta_camera, pitch_min,   F_ATOM_FLOAT);
    OBJ_FIELD(ta_camera, pitch_max,   F_ATOM_FLOAT);
    OBJ_FIELD(ta_camera, pitch_accel, F_ATOM_FLOAT);
    OBJ_END(ta_camera);

    OBJ_START(ta_sun_light, F_TA_SUN_LIGHT);
    OBJ_FIELD(ta_sun_light, name,      F_ATOM_STRING);
    OBJ_FIELD(ta_sun_light, direction, F_TA_VEC3);
    OBJ_FIELD(ta_sun_light, color,     F_TA_RGB);
    OBJ_END(ta_sun_light);

    OBJ_START(ta_point_light, F_TA_POINT_LIGHT);
    OBJ_FIELD(ta_point_light, name,     F_ATOM_STRING);
    OBJ_FIELD(ta_point_light, position, F_TA_VEC3);
    OBJ_FIELD(ta_point_light, color,    F_TA_RGB);
    OBJ_END(ta_point_light);

    OBJ_START(ta_material, F_TA_MATERIAL);
    OBJ_FIELD(ta_material, name, F_ATOM_STRING);
    OBJ_END(ta_material);

    OBJ_START(ta_mesh, F_TA_MESH);
    OBJ_FIELD(ta_mesh, name, F_ATOM_STRING);
    OBJ_FIELD(ta_mesh, path, F_ATOM_STRING);
    OBJ_END(ta_mesh);

    OBJ_START(ta_shader, F_TA_SHADER);
    OBJ_FIELD(ta_shader, name, F_ATOM_STRING);
    OBJ_FIELD(ta_shader, path, F_ATOM_STRING);
    OBJ_END(ta_shader);

    OBJ_START(ta_texture_2d, F_TA_TEXTURE);
    OBJ_FIELD(ta_texture_2d, name, F_ATOM_STRING);
    OBJ_FIELD(ta_texture_2d, path, F_ATOM_STRING);
    OBJ_END(ta_texture_2d);

    OBJ_START(ta_entity, F_TA_ENTITY);
    OBJ_FIELD(ta_entity, name,      F_ATOM_STRING);
    OBJ_FIELD(ta_entity, material,  F_ATOM_STRING);
    OBJ_FIELD(ta_entity, mesh,      F_ATOM_STRING);
    OBJ_FIELD(ta_entity, shader,    F_ATOM_STRING);
    OBJ_FIELD(ta_entity, texture,   F_ATOM_STRING);
    OBJ_FIELD(ta_entity, transform, F_TA_TRANSFORM);
    OBJ_END(ta_entity);
}

#undef OBJ_START
#undef OBJ_FIELD
#undef OBJ_END

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

void ta_schema_print(FILE *f, ta_schema_field_type type, u8 *ptr, int level)
{
    ta_schema *schema = &tg_schemas[type];
    if (level == 0) {
        fprintf(f, "%s:\n", schema->name);
    }

    for (ta_schema_field *field = schema->fields; field != dlb_vec_end(schema->fields); field++)
    {
        if (field->alias) {
            continue;
        }

        for (int i = 0; i < level + 1; i++) {
            fprintf(f, "  ");
        }

        if (field->type > F_ATOM_END) {
            fprintf(f, "%s:\n", field->name);
            ta_schema_print(f, field->type, ptr + field->offset, level + 1);
        } else {
            fprintf(f, "%s: ", field->name);
            switch (field->type ) {
                case F_ATOM_INT: {
                    int *val = (int *)(ptr + field->offset);
                    fprintf(f, "%d\n", *val);
                    break;
                } case F_ATOM_UINT: {
                    u32 *val = (u32 *)(ptr + field->offset);
                    fprintf(f, "%u\n", *val);
                    break;
                } case F_ATOM_FLOAT: {
                    float *val = (float *)(ptr + field->offset);
                    fprintf(f, "0x%08X (%f)\n", *(u32 *)val, *val);
                    break;
                } case F_ATOM_STRING: {
                    const char **val = (const char **)(ptr + field->offset);
                    //fprintf(f, "\"%s\"  # %08X\n", IFNULL(*val, ""), (u32)*val);
                    fprintf(f, "\"%s\"\n", IFNULL(*val, ""));
                    break;
                } default: {
                    PANIC("Unexpected field type, don't know how to print");
                }
            }
        }
    }
}