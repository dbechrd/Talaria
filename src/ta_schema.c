#include "ta_schema.h"
#include "ta_symbol.h"
#include "ta_math.h"
#include "ta_file.h"
#include "ta_camera.h"
#include "ta_mesh.h"
#include "ta_texture.h"
#include "ta_shader.h"
#include "ta_audio.h"
#include "ta_entity.h"
#include "ta_ent_button.h"
#include "ta_rigid_body.h"
#include "ta_light.h"
#include "dlb_types.h"
#include "dlb_vector.h"
#include "dlb_hash.h"
#include <stdlib.h>

static ta_schema tg_schemas[F_TA_COUNT];
static dlb_hash tg_schemas_by_name;

const char *ta_schema_field_type_str(ta_schema_field_type type) {
    switch(type) {
        case F_TA_NULL:                 return "NULL";
        // Compound types
        case F_TA_VEC2:                 return "TA_VEC2";
        case F_TA_VEC3:                 return "TA_VEC3";
        case F_TA_VEC4:                 return "TA_VEC4";
        case F_TA_MAT3:                 return "TA_MAT3";
        case F_TA_MAT4:                 return "TA_MAT4";
        case F_TA_RGB:                  return "TA_COLOR3";
        case F_TA_RGBA:                 return "TA_COLOR4";
        case F_TA_TRANSFORM:            return "TA_TRANSFORM";
        case F_TA_CAMERA:               return "TA_CAMERA";
        case F_TA_LIGHT:                return "TA_LIGHT";
        case F_TA_DIRECTIONAL_LIGHT:    return "TA_SUN_LIGHT";
        case F_TA_POINT_LIGHT:          return "TA_POINT_LIGHT";
        case F_TA_MATERIAL:             return "TA_MATERIAL";
        case F_TA_MESH_GROUP:           return "TA_MESH_GROUP";
        case F_TA_SHADER:               return "TA_SHADER";
        case F_TA_SHADER_ATTRIBUTE:     return "TA_SHADER_ATTRIBUTE";
        case F_TA_SHADER_UNIFORM:       return "TA_SHADER_UNIFORM";
        case F_TA_TEXTURE:              return "TA_TEXTURE";
        case F_TA_AUDIO_BUFFER:         return "TA_AUDIO_BUFFER";
        case F_TA_AUDIO_SOURCE:         return "TA_AUDIO_SOURCE";
        case F_TA_ENTITY:               return "TA_ENTITY";
        case F_TA_ENT_BUTTON:           return "TA_ENT_BUTTON";
        case F_TA_PLANE:                return "TA_PLANE";
        case F_TA_SPHERE:               return "TA_SPHERE";
        case F_TA_AABB:                 return "TA_AABB";
        case F_TA_OBB:                  return "TA_OBB";
        case F_TA_COLLIDER:             return "TA_COLLIDER";
        case F_TA_RIGID_BODY:           return "TA_RIGID_BODY";
        // Atomic types
        case F_ATOM_BOOL:               return "ATOM_BOOL";
        case F_ATOM_INT:                return "ATOM_INT";
        case F_ATOM_UINT:               return "ATOM_UINT";
        case F_ATOM_FLOAT:              return "ATOM_FLOAT";
        case F_ATOM_STRING:             return "ATOM_STRING";
        case F_ATOM_ENUM:               return "ATOM_ENUM";
        default:
            DLB_ASSERT(!"<UNKNOWN_TA_FIELD_TYPE>");
            return 0;
    }
}

static void type_field_add(ta_schema *schema, ta_schema_field_type type,
    const char *name, u32 offset, u32 size, u32 array_len, bool is_alias,
    enum_to_str *enum_converter, bool is_union_type, bool in_union,
    int union_type)
{
    ta_schema_field *field = dlb_vec_alloc(schema->fields);
    field->type = type;
    field->name = name;
    field->offset = offset;
    field->size = size;
    field->array_len = array_len;
    field->is_alias = is_alias;
    field->enum_converter = enum_converter;
    field->is_union_type = is_union_type;
    field->in_union = in_union;
    field->union_type = union_type;
}

#define TYPE_START(_type, field_type) \
    schema = &tg_schemas[field_type]; \
    schema->type = field_type; \
    schema->name = INTERN(STRING(_type)); \
    schema->size = sizeof(_type);

#define TYPE_FIELD(type, field, field_type) \
    type_field_add(schema, field_type, INTERN(#field), \
    OFFSETOF(type, field), SIZEOF_MEMBER(type, field), \
    0, false, 0, false, false, 0)

#define TYPE_FIELD_NAME(type, field, field_type, alias) \
    type_field_add(schema, field_type, INTERN(#alias), \
    OFFSETOF(type, field), SIZEOF_MEMBER(type, field), \
    0, false, 0, false, false, 0)

#define TYPE_ENUM(type, field, field_type, converter) \
    type_field_add(schema, field_type, INTERN(#field), \
    OFFSETOF(type, field), SIZEOF_MEMBER(type, field), \
    0, false, converter, false, false, 0)

#define TYPE_ARRAY(type, field, field_type, size) \
    type_field_add(schema, field_type, INTERN(#field), \
    OFFSETOF(type, field), SIZEOF_MEMBER_ARRAY(type, field), \
    size, false, 0, false, false, 0)

#define TYPE_VECTOR(type, field, field_type) \
    type_field_add(schema, field_type, INTERN(#field), \
    OFFSETOF(type, field), SIZEOF_MEMBER_ARRAY(type, field), \
    1, false, 0, false, false, 0)

#define TYPE_UNION_TYPE(type, field, field_type, converter) \
    type_field_add(schema, field_type, INTERN(#field), \
    OFFSETOF(type, field), SIZEOF_MEMBER(type, field), \
    0, false, converter, true, false, 0)

#define TYPE_UNION_FIELD(type, field, field_type, union_name, union_type) \
    type_field_add(schema, field_type, INTERN(#field), \
    OFFSETOF(type, union_name.field), SIZEOF_MEMBER(type, union_name.field), \
    0, false, 0, false, true, union_type)

#define TYPE_UNION_ARRAY(type, field, field_type, size, union_name, union_type) \
    type_field_add(schema, field_type, INTERN(#field), \
    OFFSETOF(type, union_name.field), SIZEOF_MEMBER_ARRAY(type, union_name.field), \
    size, false, 0, false, true, union_type)

#define TYPE_UNION_VECTOR(type, field, field_type, union_name, union_type) \
    type_field_add(schema, field_type, INTERN(#field), \
    OFFSETOF(type, union_name.field), SIZEOF_MEMBER_ARRAY(type, union_name.field), \
    1, false, 0, false, true, union_type)

#define TYPE_END(type) \
    dlb_hash_insert(&tg_schemas_by_name, CSTR(STRING(type)), schema);

void ta_schema_register()
{
    DLB_ASSERT(!tg_schemas_by_name.size);
    dlb_hash_init(&tg_schemas_by_name, DLB_HASH_STRING, "[schema_register]", 64);
    ta_schema *schema;

    TYPE_START(ta_vec2, F_TA_VEC2);
    TYPE_FIELD(ta_vec2, x, F_ATOM_FLOAT);
    TYPE_FIELD(ta_vec2, y, F_ATOM_FLOAT);
    TYPE_END(ta_vec2);

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

    TYPE_START(ta_quat, F_TA_QUAT);
    TYPE_FIELD(ta_quat, x, F_ATOM_FLOAT);
    TYPE_FIELD(ta_quat, y, F_ATOM_FLOAT);
    TYPE_FIELD(ta_quat, z, F_ATOM_FLOAT);
    TYPE_FIELD(ta_quat, w, F_ATOM_FLOAT);
    TYPE_END(ta_quat);

    TYPE_START(ta_mat3, F_TA_MAT3);
    TYPE_UNION_TYPE(ta_mat3, data, F_ATOM_ENUM, 0);
    TYPE_UNION_ARRAY(ta_mat3, arr, F_ATOM_FLOAT, 9, data, 0);
    TYPE_END(ta_mat3);

    TYPE_START(ta_mat4, F_TA_MAT4);
    TYPE_UNION_TYPE(ta_mat4, data, F_ATOM_ENUM, 0);
    TYPE_UNION_ARRAY(ta_mat4, arr, F_ATOM_FLOAT, 16, data, 0);
    TYPE_END(ta_mat4);

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
    TYPE_FIELD(ta_transform, position,    F_TA_VEC3);
    TYPE_FIELD(ta_transform, orientation, F_TA_QUAT);
    TYPE_FIELD(ta_transform, scale,       F_TA_VEC3);
    TYPE_END(ta_transform);

    // Scene-level object types
    TYPE_START(ta_camera, F_TA_CAMERA);
    TYPE_FIELD_NAME(ta_camera, ref.uid, F_ATOM_STRING, uid);
    TYPE_ENUM(ta_camera,  mode,                F_ATOM_ENUM, ta_camera_mode_str);
    TYPE_FIELD(ta_camera, position,            F_TA_VEC3);
    TYPE_FIELD(ta_camera, position_smooth,     F_ATOM_FLOAT);
    TYPE_FIELD(ta_camera, position_target_vel, F_ATOM_FLOAT);
    TYPE_FIELD(ta_camera, yaw,                 F_ATOM_FLOAT);
    TYPE_FIELD(ta_camera, yaw_smooth,          F_ATOM_FLOAT);
    TYPE_FIELD(ta_camera, pitch,               F_ATOM_FLOAT);
    TYPE_FIELD(ta_camera, pitch_smooth,        F_ATOM_FLOAT);
    TYPE_FIELD(ta_camera, pitch_min,           F_ATOM_FLOAT);
    TYPE_FIELD(ta_camera, pitch_max,           F_ATOM_FLOAT);
    TYPE_FIELD(ta_camera, fov,                 F_ATOM_FLOAT);
    TYPE_FIELD(ta_camera, nearz,               F_ATOM_FLOAT);
    TYPE_FIELD(ta_camera, up,                  F_TA_VEC3);
    TYPE_FIELD(ta_camera, ortho,               F_ATOM_UINT);
    TYPE_END(ta_camera);

    TYPE_START(ta_directional_light, F_TA_DIRECTIONAL_LIGHT);
    TYPE_FIELD(ta_directional_light, direction, F_TA_VEC3);
    TYPE_END(ta_directional_light);

    TYPE_START(ta_point_light, F_TA_POINT_LIGHT);
    TYPE_END(ta_point_light);

    TYPE_START(ta_spot_light, F_TA_SPOT_LIGHT);
    TYPE_FIELD(ta_spot_light, direction,     F_TA_VEC3);
    TYPE_FIELD(ta_spot_light, theta_cone,    F_ATOM_FLOAT);
    TYPE_FIELD(ta_spot_light, theta_falloff, F_ATOM_FLOAT);
    TYPE_END(ta_spot_light);

    TYPE_START(ta_light, F_TA_LIGHT);
    TYPE_FIELD_NAME(ta_light, ref.uid, F_ATOM_STRING, uid);
    TYPE_FIELD(ta_light, disabled,  F_ATOM_BOOL);
    TYPE_FIELD(ta_light, intensity, F_ATOM_FLOAT);
    TYPE_FIELD(ta_light, position,  F_TA_VEC3);
    TYPE_FIELD(ta_light, color,     F_TA_RGB);
    TYPE_UNION_TYPE(ta_light,  type,        F_ATOM_ENUM,            ta_light_type_str);
    TYPE_UNION_FIELD(ta_light, ambient,     F_TA_AMBIENT_LIGHT,     data, TA_LIGHT_AMBIENT);
    TYPE_UNION_FIELD(ta_light, directional, F_TA_DIRECTIONAL_LIGHT, data, TA_LIGHT_DIRECTIONAL);
    TYPE_UNION_FIELD(ta_light, point,       F_TA_POINT_LIGHT,       data, TA_LIGHT_POINT);
    TYPE_UNION_FIELD(ta_light, spot,        F_TA_SPOT_LIGHT,        data, TA_LIGHT_SPOT);
    TYPE_END(ta_light);

    TYPE_START(ta_material, F_TA_MATERIAL);
    TYPE_FIELD_NAME(ta_material, ref.uid, F_ATOM_STRING, uid);
    TYPE_FIELD(ta_material, shader_uid,           F_ATOM_STRING);
    TYPE_FIELD(ta_material, texture_albedo_uid,   F_ATOM_STRING);
    TYPE_FIELD(ta_material, texture_metallic_uid, F_ATOM_STRING);
    TYPE_END(ta_material);

    TYPE_START(ta_mesh_group, F_TA_MESH_GROUP);
    TYPE_FIELD_NAME(ta_mesh_group, ref.uid, F_ATOM_STRING, uid);
    TYPE_FIELD(ta_mesh_group, path, F_ATOM_STRING);
    TYPE_END(ta_mesh_group);

    TYPE_START(ta_shader_attribute, F_TA_SHADER_ATTRIBUTE);
    TYPE_FIELD(ta_shader_attribute, name, F_ATOM_STRING);
    TYPE_ENUM(ta_shader_attribute, type, F_ATOM_ENUM, ta_glsl_type_str);
    TYPE_END(ta_shader_attribute);

    TYPE_START(ta_shader_uniform, F_TA_SHADER_UNIFORM);
    TYPE_FIELD(ta_shader_uniform, name, F_ATOM_STRING);
    TYPE_UNION_TYPE(ta_shader_uniform,   type,       F_ATOM_ENUM,         ta_glsl_type_str);
    TYPE_UNION_FIELD(ta_shader_uniform,  glint,      F_ATOM_INT,          value, TA_GLSL_INT);
    TYPE_UNION_FIELD(ta_shader_uniform,  gluint,     F_ATOM_UINT,         value, TA_GLSL_UINT);
    TYPE_UNION_FIELD(ta_shader_uniform,  sampler2d,  F_ATOM_UINT,         value, TA_GLSL_SAMPLER2D);
    TYPE_UNION_FIELD(ta_shader_uniform,  vec2,       F_TA_VEC2,           value, TA_GLSL_VEC2);
    TYPE_UNION_FIELD(ta_shader_uniform,  vec3,       F_TA_VEC3,           value, TA_GLSL_VEC3);
    TYPE_UNION_FIELD(ta_shader_uniform,  vec4,       F_TA_VEC4,           value, TA_GLSL_VEC4);
    TYPE_UNION_FIELD(ta_shader_uniform,  mat3,       F_TA_MAT3,           value, TA_GLSL_MAT3);
    TYPE_UNION_FIELD(ta_shader_uniform,  mat4,       F_TA_MAT4,           value, TA_GLSL_MAT4);
    TYPE_UNION_VECTOR(ta_shader_uniform, properties, F_TA_SHADER_UNIFORM, value, TA_GLSL_STRUCT);
    TYPE_END(ta_shader_uniform);

    TYPE_START(ta_shader, F_TA_SHADER);
    TYPE_FIELD_NAME(ta_shader, ref.uid, F_ATOM_STRING, uid);
    TYPE_FIELD(ta_shader, path_vert,   F_ATOM_STRING);
    TYPE_FIELD(ta_shader, path_frag,   F_ATOM_STRING);
    TYPE_VECTOR(ta_shader, attributes, F_TA_SHADER_ATTRIBUTE);
    TYPE_VECTOR(ta_shader, uniforms,   F_TA_SHADER_UNIFORM);
    TYPE_END(ta_shader);

    TYPE_START(ta_texture, F_TA_TEXTURE);
    TYPE_FIELD_NAME(ta_texture, ref.uid, F_ATOM_STRING, uid);
    TYPE_FIELD(ta_texture, path,     F_ATOM_STRING);
    TYPE_FIELD(ta_texture, channels, F_ATOM_INT);
    TYPE_FIELD(ta_texture, linear,   F_ATOM_BOOL);
    TYPE_END(ta_texture);

    TYPE_START(ta_audio_buffer, F_TA_AUDIO_BUFFER);
    TYPE_FIELD_NAME(ta_audio_buffer, ref.uid, F_ATOM_STRING, uid);
    TYPE_FIELD(ta_audio_buffer, path, F_ATOM_STRING);
    TYPE_END(ta_audio_buffer);

    TYPE_START(ta_audio_source, F_TA_AUDIO_SOURCE);
    TYPE_FIELD_NAME(ta_audio_source, ref.uid, F_ATOM_STRING, uid);
    TYPE_FIELD(ta_audio_source, pitch,            F_ATOM_FLOAT);
    TYPE_FIELD(ta_audio_source, gain,             F_ATOM_FLOAT);
    TYPE_FIELD(ta_audio_source, loop,             F_ATOM_BOOL);
    TYPE_FIELD(ta_audio_source, audio_buffer_uid, F_ATOM_STRING);
    TYPE_END(ta_audio_source);

    TYPE_START(ta_entity, F_TA_ENTITY);
    TYPE_FIELD_NAME(ta_entity, ref.uid, F_ATOM_STRING, uid);
    TYPE_FIELD(ta_entity, type,           F_ATOM_INT);
    TYPE_FIELD(ta_entity, transform,      F_TA_TRANSFORM);
    TYPE_FIELD(ta_entity, material_uid,   F_ATOM_STRING);
    TYPE_FIELD(ta_entity, mesh_group_uid, F_ATOM_STRING);
    TYPE_FIELD(ta_entity, rigid_body_uid, F_ATOM_STRING);
    TYPE_FIELD(ta_entity, parent_uid,     F_ATOM_STRING);
    TYPE_FIELD(ta_entity, invisible,      F_ATOM_BOOL);
    TYPE_END(ta_entity);

    TYPE_START(ta_ent_button, F_TA_ENT_BUTTON);
    TYPE_FIELD(ta_ent_button, base,                F_TA_ENTITY);
    TYPE_FIELD(ta_ent_button, audio_source_uid,    F_ATOM_STRING);
    TYPE_FIELD(ta_ent_button, sfx_activated_uid,   F_ATOM_STRING);
    TYPE_FIELD(ta_ent_button, sfx_active_uid,      F_ATOM_STRING);
    TYPE_FIELD(ta_ent_button, sfx_deactivated_uid, F_ATOM_STRING);
    TYPE_END(ta_ent_button);

    TYPE_START(ta_plane, F_TA_PLANE);
    TYPE_FIELD(ta_plane, center, F_TA_VEC3);
    TYPE_FIELD(ta_plane, normal, F_TA_VEC3);
    TYPE_END(ta_plane);

    TYPE_START(ta_sphere, F_TA_SPHERE);
    TYPE_FIELD(ta_sphere, center, F_TA_VEC3);
    TYPE_FIELD(ta_sphere, radius, F_ATOM_FLOAT);
    TYPE_END(ta_sphere);

    TYPE_START(ta_aabb, F_TA_AABB);
    TYPE_FIELD(ta_aabb, center,  F_TA_VEC3);
    TYPE_FIELD(ta_aabb, extents, F_TA_VEC3);
    TYPE_END(ta_aabb);

    TYPE_START(ta_obb, F_TA_OBB);
    TYPE_FIELD(ta_obb, center,  F_TA_VEC3);
    TYPE_FIELD(ta_obb, extents, F_TA_VEC3);
    TYPE_ARRAY(ta_obb, axes,    F_TA_VEC3, 3);
    TYPE_END(ta_obb);

    TYPE_START(ta_collider, F_TA_COLLIDER);
    TYPE_UNION_TYPE(ta_collider,  type,   F_ATOM_ENUM, ta_collider_type_str);
    TYPE_UNION_FIELD(ta_collider, plane,  F_TA_PLANE,  data, TA_COLLIDER_PLANE);
    TYPE_UNION_FIELD(ta_collider, sphere, F_TA_SPHERE, data, TA_COLLIDER_SPHERE);
    TYPE_UNION_FIELD(ta_collider, aabb,   F_TA_AABB,   data, TA_COLLIDER_AABB);
    TYPE_UNION_FIELD(ta_collider, obb,    F_TA_OBB,    data, TA_COLLIDER_OBB);
    TYPE_END(ta_collider);

    TYPE_START(ta_rigid_body, F_TA_RIGID_BODY);
    TYPE_FIELD_NAME(ta_rigid_body, ref.uid, F_ATOM_STRING, uid);
    TYPE_FIELD(ta_rigid_body, collider,    F_TA_COLLIDER);
    TYPE_FIELD(ta_rigid_body, position,    F_TA_VEC3);
    TYPE_FIELD(ta_rigid_body, orientation, F_TA_QUAT);
    TYPE_FIELD(ta_rigid_body, mass,        F_ATOM_FLOAT);
    TYPE_FIELD(ta_rigid_body, trigger,     F_ATOM_BOOL);
    TYPE_END(ta_rigid_body);
}

#undef TYPE_START
#undef TYPE_FIELD
#undef TYPE_FIELD_NAME
#undef TYPE_ENUM
#undef TYPE_ARRAY
#undef TYPE_VECTOR
#undef TYPE_UNION_TYPE
#undef TYPE_UNION_FIELD
#undef TYPE_UNION_ARRAY
#undef TYPE_UNION_VECTOR
#undef TYPE_END

ta_schema *ta_schema_find_by_type(ta_schema_field_type type)
{
    ta_schema *schema = &tg_schemas[type];
    return schema;
}

ta_schema *ta_schema_find_by_name(const char *name, int len)
{
    ta_schema *schema = dlb_hash_search(&tg_schemas_by_name, name, len);
    return schema;
}

ta_schema_field *ta_schema_field_find(ta_schema_field_type type, const char *name)
{
    ta_schema_field *field = 0;
    ta_schema *schema = &tg_schemas[type];
    dlb_vec_each(ta_schema_field *, f, schema->fields) {
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
        } case F_ATOM_ENUM: {
            int *val = ptr;
            fprintf(f, "%d", *val);
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

void ta_schema_print(FILE *f, ta_schema_field_type type, u8 *ptr, int level,
    bool in_array)
{
    ta_schema *schema = &tg_schemas[type];
    if (!in_array) {
        if (level == 0) {
            fprintf(f, "%s:\n", schema->name);
        }
    }

    // NOTE: Defaults to a value unlikely to be used in any legitimate enum to
    //       detect errors more easily.
    int union_type = INT_MIN;

    dlb_vec_each(ta_schema_field *, field, schema->fields) {
        if (field->is_alias) {
            continue;
        }
        if (field->in_union && field->union_type != union_type) {
            continue;
        }

        if (field->array_len) {
            DLB_ASSERT(!in_array && "Don't know how to print nested arrays");
            indent(f, level + 1);
            fprintf(f, "%s: [\n", field->name);

            u8 *arr = (ptr + field->offset);
            u32 arr_len = field->array_len;
            if (arr_len == 1) {
                arr = *(void **)(ptr + field->offset);
                arr_len = dlb_vec_len(arr);
            }
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
                if (in_array) {
                    fprintf(f, ",");
                }
            } else {
                ta_schema_print_atom(f, field, ptr + field->offset);
                if (in_array) {
                    fprintf(f, ",");
                }
                if (field->is_union_type) {
                    union_type = *(int *)(ptr + field->offset);
                }
                if (field->type == F_ATOM_ENUM && field->enum_converter) {
                    const char *enum_str = field->enum_converter(union_type);
                    fprintf(f, "  # %s", enum_str);
                }
                fprintf(f, "\n");
            }
        }
    }
}