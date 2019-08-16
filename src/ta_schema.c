#include "ta_schema.h"
#include "ta_symbol.h"
#include "ta_math.h"
#include "ta_file.h"
#include "ta_camera.h"
#include "ta_mesh.h"
#include "ta_texture.h"
#include "ta_shader.h"
#include "ta_audio.h"
#include "ta_node.h"
#include "ta_button.h"
#include "ta_rigid_body.h"
#include "ta_light.h"
#include "ta_material.h"
#include "ta_mesh_group.h"
#include "ta_font.h"
#include "dlb_types.h"
#include "dlb_vector.h"
#include "dlb_hash.h"
#include <stdlib.h>

static ta_schema tg_schemas[TA_COUNT];
static dlb_hash tg_schemas_by_name;

const char *ta_schema_field_type_str(ta_schema_field_type type) {
    switch(type) {
        case TA_NULL:               return "TA_NULL";

		// Scene-level compound types
		case TA_CAMERA:				return "TA_CAMERA";
		case TA_LIGHT:              return "TA_LIGHT";
		case TA_MATERIAL:           return "TA_MATERIAL";
		case TA_MESH_GROUP:         return "TA_MESH_GROUP";
		case TA_SHADER:             return "TA_SHADER";
		case TA_TEXTURE:            return "TA_TEXTURE";
		case TA_NODE:				return "TA_NODE";
		case TA_AUDIO_BUFFER:       return "TA_AUDIO_BUFFER";
		case TA_AUDIO_SOURCE:       return "TA_AUDIO_SOURCE";
		case TA_RIGID_BODY:         return "TA_RIGID_BODY";
        case TA_BUTTON:			    return "TA_BUTTON";
        case TA_FONT:			    return "TA_FONT";

		// Other compound types
        case TA_VEC2:               return "TA_VEC2";
        case TA_VEC3:               return "TA_VEC3";
        case TA_VEC4:               return "TA_VEC4";
        case TA_MAT3:               return "TA_MAT3";
        case TA_MAT4:               return "TA_MAT4";
        case TA_RGB:                return "TA_RGB";
        case TA_RGBA:               return "TA_RGBA";
        case TA_TRANSFORM:          return "TA_TRANSFORM";
        case TA_DIRECTIONAL_LIGHT:  return "TA_DIRECTIONAL_LIGHT";
        case TA_POINT_LIGHT:        return "TA_POINT_LIGHT";
        case TA_SHADER_ATTRIBUTE:   return "TA_SHADER_ATTRIBUTE";
        case TA_SHADER_UNIFORM:     return "TA_SHADER_UNIFORM";
        case TA_PLANE:              return "TA_PLANE";
        case TA_SPHERE:             return "TA_SPHERE";
        case TA_AABB:               return "TA_AABB";
        case TA_OBB:                return "TA_OBB";
        case TA_COLLIDER:           return "TA_COLLIDER";

        // Atomic types
        case ATOM_BOOL:             return "ATOM_BOOL";
        case ATOM_INT:              return "ATOM_INT";
        case ATOM_UINT:             return "ATOM_UINT";
        case ATOM_FLOAT:            return "ATOM_FLOAT";
        case ATOM_STRING:           return "ATOM_STRING";
        case ATOM_ENUM:             return "ATOM_ENUM";

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

    TYPE_START(ta_vec2, TA_VEC2);
    TYPE_FIELD(ta_vec2, x, ATOM_FLOAT);
    TYPE_FIELD(ta_vec2, y, ATOM_FLOAT);
    TYPE_END(ta_vec2);

    TYPE_START(ta_vec3, TA_VEC3);
    TYPE_FIELD(ta_vec3, x, ATOM_FLOAT);
    TYPE_FIELD(ta_vec3, y, ATOM_FLOAT);
    TYPE_FIELD(ta_vec3, z, ATOM_FLOAT);
    TYPE_END(ta_vec3);

    TYPE_START(ta_vec4, TA_VEC4);
    TYPE_FIELD(ta_vec4, x, ATOM_FLOAT);
    TYPE_FIELD(ta_vec4, y, ATOM_FLOAT);
    TYPE_FIELD(ta_vec4, z, ATOM_FLOAT);
    TYPE_FIELD(ta_vec4, w, ATOM_FLOAT);
    TYPE_END(ta_vec4);

    TYPE_START(ta_quat, TA_QUAT);
    TYPE_FIELD(ta_quat, x, ATOM_FLOAT);
    TYPE_FIELD(ta_quat, y, ATOM_FLOAT);
    TYPE_FIELD(ta_quat, z, ATOM_FLOAT);
    TYPE_FIELD(ta_quat, w, ATOM_FLOAT);
    TYPE_END(ta_quat);

    TYPE_START(ta_mat3, TA_MAT3);
    TYPE_UNION_TYPE(ta_mat3, data, ATOM_ENUM, 0);
    TYPE_UNION_ARRAY(ta_mat3, arr, ATOM_FLOAT, 9, data, 0);
    TYPE_END(ta_mat3);

    TYPE_START(ta_mat4, TA_MAT4);
    TYPE_UNION_TYPE(ta_mat4, data, ATOM_ENUM, 0);
    TYPE_UNION_ARRAY(ta_mat4, arr, ATOM_FLOAT, 16, data, 0);
    TYPE_END(ta_mat4);

    TYPE_START(ta_rgb, TA_RGB);
    TYPE_FIELD(ta_rgb, r, ATOM_FLOAT);
    TYPE_FIELD(ta_rgb, g, ATOM_FLOAT);
    TYPE_FIELD(ta_rgb, b, ATOM_FLOAT);
    TYPE_END(ta_rgb);

    TYPE_START(ta_rgba, TA_RGBA);
    TYPE_FIELD(ta_rgba, r, ATOM_FLOAT);
    TYPE_FIELD(ta_rgba, g, ATOM_FLOAT);
    TYPE_FIELD(ta_rgba, b, ATOM_FLOAT);
    TYPE_FIELD(ta_rgba, a, ATOM_FLOAT);
    TYPE_END(ta_rgba);

    TYPE_START(ta_transform, TA_TRANSFORM);
    TYPE_FIELD(ta_transform, position,    TA_VEC3);
    TYPE_FIELD(ta_transform, orientation, TA_QUAT);
    TYPE_FIELD(ta_transform, scale,       TA_VEC3);
    TYPE_END(ta_transform);

    // Scene-level object types
    TYPE_START(ta_camera, TA_CAMERA);
    TYPE_FIELD(ta_camera, uid,                 ATOM_STRING);
    TYPE_FIELD(ta_camera, position,            TA_VEC3);
    TYPE_FIELD(ta_camera, position_smooth,     ATOM_FLOAT);
    TYPE_FIELD(ta_camera, position_target_vel, ATOM_FLOAT);
    TYPE_FIELD(ta_camera, yaw,                 ATOM_FLOAT);
    TYPE_FIELD(ta_camera, yaw_smooth,          ATOM_FLOAT);
    TYPE_FIELD(ta_camera, pitch,               ATOM_FLOAT);
    TYPE_FIELD(ta_camera, pitch_smooth,        ATOM_FLOAT);
    TYPE_FIELD(ta_camera, pitch_min,           ATOM_FLOAT);
    TYPE_FIELD(ta_camera, pitch_max,           ATOM_FLOAT);
    TYPE_FIELD(ta_camera, fov,                 ATOM_FLOAT);
    TYPE_FIELD(ta_camera, znear,               ATOM_FLOAT);
    TYPE_FIELD(ta_camera, focal_point,         TA_VEC3);
    TYPE_FIELD(ta_camera, up,                  TA_VEC3);
    TYPE_FIELD(ta_camera, ortho,               ATOM_BOOL);
    TYPE_END(ta_camera);

    TYPE_START(ta_light_directional, TA_DIRECTIONAL_LIGHT);
    TYPE_FIELD(ta_light_directional, direction, TA_VEC3);
    TYPE_END(ta_light_directional);

    TYPE_START(ta_light_point, TA_POINT_LIGHT);
    TYPE_END(ta_light_point);

    TYPE_START(ta_light_spot, TA_SPOT_LIGHT);
    TYPE_FIELD(ta_light_spot, direction,     TA_VEC3);
    TYPE_FIELD(ta_light_spot, theta_cone,    ATOM_FLOAT);
    TYPE_FIELD(ta_light_spot, theta_falloff, ATOM_FLOAT);
    TYPE_END(ta_light_spot);

    TYPE_START(ta_light, TA_LIGHT);
    TYPE_FIELD(ta_light, uid,       ATOM_STRING);
    TYPE_FIELD(ta_light, disabled,  ATOM_BOOL);
    TYPE_FIELD(ta_light, intensity, ATOM_FLOAT);
    TYPE_FIELD(ta_light, position,  TA_VEC3);
    TYPE_FIELD(ta_light, color,     TA_RGB);
    TYPE_UNION_TYPE(ta_light,  type,        ATOM_ENUM,            ta_light_type_str);
    TYPE_UNION_FIELD(ta_light, ambient,     TA_AMBIENT_LIGHT,     data, TA_LIGHT_AMBIENT);
    TYPE_UNION_FIELD(ta_light, directional, TA_DIRECTIONAL_LIGHT, data, TA_LIGHT_DIRECTIONAL);
    TYPE_UNION_FIELD(ta_light, point,       TA_POINT_LIGHT,       data, TA_LIGHT_POINT);
    TYPE_UNION_FIELD(ta_light, spot,        TA_SPOT_LIGHT,        data, TA_LIGHT_SPOT);
    TYPE_END(ta_light);

    TYPE_START(ta_material, TA_MATERIAL);
    TYPE_FIELD(ta_material, uid,                  ATOM_STRING);
    TYPE_FIELD(ta_material, shader_uid,           ATOM_STRING);
    TYPE_FIELD(ta_material, texture_albedo_uid,   ATOM_STRING);
    TYPE_FIELD(ta_material, texture_metallic_uid, ATOM_STRING);
    TYPE_END(ta_material);

    TYPE_START(ta_mesh_group, TA_MESH_GROUP);
    TYPE_FIELD(ta_mesh_group, uid,  ATOM_STRING);
    TYPE_FIELD(ta_mesh_group, path, ATOM_STRING);
    TYPE_END(ta_mesh_group);

    TYPE_START(ta_shader_attribute, TA_SHADER_ATTRIBUTE);
    TYPE_FIELD(ta_shader_attribute, name, ATOM_STRING);
    TYPE_ENUM(ta_shader_attribute, type, ATOM_ENUM, ta_glsl_type_str);
    TYPE_END(ta_shader_attribute);

    TYPE_START(ta_shader_uniform, TA_SHADER_UNIFORM);
    TYPE_FIELD(ta_shader_uniform, name, ATOM_STRING);
    TYPE_UNION_TYPE(ta_shader_uniform, type, ATOM_ENUM, ta_glsl_type_str);
    //TYPE_UNION_FIELD(ta_shader_uniform, glint,     ATOM_INT,  value, TA_GLSL_INT);
    //TYPE_UNION_FIELD(ta_shader_uniform, gluint,    ATOM_UINT, value, TA_GLSL_UINT);
    //TYPE_UNION_FIELD(ta_shader_uniform, sampler2d, ATOM_UINT, value, TA_GLSL_SAMPLER2D);
    //TYPE_UNION_FIELD(ta_shader_uniform, vec2,      TA_VEC2,   value, TA_GLSL_VEC2);
    //TYPE_UNION_FIELD(ta_shader_uniform, vec3,      TA_VEC3,   value, TA_GLSL_VEC3);
    //TYPE_UNION_FIELD(ta_shader_uniform, vec4,      TA_VEC4,   value, TA_GLSL_VEC4);
    //TYPE_UNION_FIELD(ta_shader_uniform, mat3,      TA_MAT3,   value, TA_GLSL_MAT3);
    //TYPE_UNION_FIELD(ta_shader_uniform, mat4,      TA_MAT4,   value, TA_GLSL_MAT4);
    TYPE_UNION_VECTOR(ta_shader_uniform, properties, TA_SHADER_UNIFORM, value, TA_GLSL_STRUCT);
    TYPE_END(ta_shader_uniform);

    TYPE_START(ta_shader, TA_SHADER);
    TYPE_FIELD(ta_shader, uid,         ATOM_STRING);
    TYPE_FIELD(ta_shader, path_vert,   ATOM_STRING);
    TYPE_FIELD(ta_shader, path_frag,   ATOM_STRING);
    TYPE_VECTOR(ta_shader, attributes, TA_SHADER_ATTRIBUTE);
    TYPE_VECTOR(ta_shader, uniforms,   TA_SHADER_UNIFORM);
    TYPE_END(ta_shader);

    TYPE_START(ta_texture, TA_TEXTURE);
    TYPE_FIELD(ta_texture, uid,      ATOM_STRING);
    TYPE_FIELD(ta_texture, path,     ATOM_STRING);
    TYPE_FIELD(ta_texture, channels, ATOM_INT);
    TYPE_FIELD(ta_texture, linear,   ATOM_BOOL);
    TYPE_END(ta_texture);

    TYPE_START(ta_audio_buffer, TA_AUDIO_BUFFER);
    TYPE_FIELD(ta_audio_buffer, uid,  ATOM_STRING);
    TYPE_FIELD(ta_audio_buffer, path, ATOM_STRING);
    TYPE_END(ta_audio_buffer);

    TYPE_START(ta_audio_source, TA_AUDIO_SOURCE);
    TYPE_FIELD(ta_audio_source, uid,              ATOM_STRING);
    TYPE_FIELD(ta_audio_source, pitch,            ATOM_FLOAT);
    TYPE_FIELD(ta_audio_source, gain,             ATOM_FLOAT);
    TYPE_FIELD(ta_audio_source, loop,             ATOM_BOOL);
    TYPE_FIELD(ta_audio_source, audio_buffer_uid, ATOM_STRING);
    TYPE_END(ta_audio_source);

    TYPE_START(ta_node, TA_NODE);
    TYPE_FIELD(ta_node, uid,             ATOM_STRING);
    TYPE_FIELD(ta_node, transform,       TA_TRANSFORM);
    TYPE_FIELD(ta_node, material_uid,    ATOM_STRING);
    TYPE_FIELD(ta_node, mesh_group_uid,  ATOM_STRING);
    TYPE_FIELD(ta_node, rigid_body_uid,  ATOM_STRING);
    TYPE_FIELD(ta_node, button_uid,      ATOM_STRING);
    TYPE_FIELD(ta_node, invisible,       ATOM_BOOL);
    TYPE_FIELD(ta_node, cast_shadows,    ATOM_BOOL);
    TYPE_FIELD(ta_node, receive_shadows, ATOM_BOOL);
    TYPE_END(ta_node);

    TYPE_START(e_button, TA_BUTTON);
	TYPE_FIELD(e_button, uid,                 ATOM_STRING);
	TYPE_FIELD(e_button, audio_source_uid,    ATOM_STRING);
    TYPE_FIELD(e_button, sfx_activated_uid,   ATOM_STRING);
    TYPE_FIELD(e_button, sfx_active_uid,      ATOM_STRING);
    TYPE_FIELD(e_button, sfx_deactivated_uid, ATOM_STRING);
    TYPE_END(e_button);

    TYPE_START(ta_plane, TA_PLANE);
    TYPE_FIELD(ta_plane, center, TA_VEC3);
    TYPE_FIELD(ta_plane, normal, TA_VEC3);
    TYPE_END(ta_plane);

    TYPE_START(ta_sphere, TA_SPHERE);
    TYPE_FIELD(ta_sphere, center, TA_VEC3);
    TYPE_FIELD(ta_sphere, radius, ATOM_FLOAT);
    TYPE_END(ta_sphere);

    TYPE_START(ta_aabb, TA_AABB);
    TYPE_FIELD(ta_aabb, center,  TA_VEC3);
    TYPE_FIELD(ta_aabb, extents, TA_VEC3);
    TYPE_END(ta_aabb);

    TYPE_START(ta_obb, TA_OBB);
    TYPE_FIELD(ta_obb, center,  TA_VEC3);
    TYPE_FIELD(ta_obb, extents, TA_VEC3);
    TYPE_ARRAY(ta_obb, axes,    TA_VEC3, 3);
    TYPE_END(ta_obb);

    TYPE_START(ta_collider, TA_COLLIDER);
    TYPE_UNION_TYPE(ta_collider,  type,   ATOM_ENUM, ta_collider_type_str);
    TYPE_UNION_FIELD(ta_collider, plane,  TA_PLANE,  data, TA_COLLIDER_PLANE);
    TYPE_UNION_FIELD(ta_collider, sphere, TA_SPHERE, data, TA_COLLIDER_SPHERE);
    TYPE_UNION_FIELD(ta_collider, aabb,   TA_AABB,   data, TA_COLLIDER_AABB);
    TYPE_UNION_FIELD(ta_collider, obb,    TA_OBB,    data, TA_COLLIDER_OBB);
    TYPE_END(ta_collider);

    TYPE_START(ta_rigid_body, TA_RIGID_BODY);
    TYPE_FIELD(ta_rigid_body, uid,         ATOM_STRING);
    TYPE_FIELD(ta_rigid_body, collider,    TA_COLLIDER);
    TYPE_FIELD(ta_rigid_body, position,    TA_VEC3);
    TYPE_FIELD(ta_rigid_body, orientation, TA_QUAT);
    TYPE_FIELD(ta_rigid_body, mass,        ATOM_FLOAT);
    TYPE_FIELD(ta_rigid_body, trigger,     ATOM_BOOL);
    TYPE_END(ta_rigid_body);

    TYPE_START(ta_font, TA_FONT);
    TYPE_FIELD(ta_font, uid,          ATOM_STRING);
    TYPE_FIELD(ta_font, path,         ATOM_STRING);
    TYPE_FIELD(ta_font, pixel_height, ATOM_FLOAT);
    TYPE_FIELD(ta_font, shader_uid,   ATOM_STRING);
    TYPE_END(ta_font);
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
    ta_schema *schema = dlb_hash_search(&tg_schemas_by_name, name, len, 0);
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
		case ATOM_BOOL: {
			bool *val = ptr;
			fprintf(f, "%s", *val ? "true" : "false");
			break;
		} case ATOM_INT: {
            int *val = ptr;
            fprintf(f, "%d", *val);
            break;
        } case ATOM_UINT: {
            u32 *val = ptr;
            fprintf(f, "%u", *val);
            break;
        } case ATOM_FLOAT: {
            float *val = ptr;
            fprintf(f, "0x%08X (%f)", *(u32 *)val, *val);
            break;
        } case ATOM_STRING: {
            const char **val = ptr;
            //fprintf(f, "\"%s\"  # %08X\n", IFNULL(*val, ""), (u32)*val);
            fprintf(f, "\"%s\"", IFNULL(*val, ""));
            break;
        } case ATOM_ENUM: {
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
    int in_array)
{
    ta_schema *schema = &tg_schemas[type];
    if (!in_array) {
        if (level == 0) {
            fprintf(f, "%s:\n", schema->name);
        }
    }

    // NOTE: Defaults to a value unlikely to be used in any legitimate enum to
    //       detect errors more easily.
    int union_type = -9001;

    dlb_vec_each(ta_schema_field *, field, schema->fields) {
        if (field->is_alias) {
            continue;
        }
        if (field->in_union && field->union_type != union_type) {
            continue;
        }

        if (field->array_len) {
            //DLB_ASSERT(!in_array && "Don't know how to print nested arrays");
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
                if (field->type < TA_COUNT) {
                    indent(f, level + 2);
                    fprintf(f, "{\n");
                    ta_schema_print(f, field->type, p, level + 2, in_array + 1);
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
            if (field->type < TA_COUNT) {
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
                if (field->type == ATOM_ENUM && field->enum_converter) {
					int enum_type = *(int *)(ptr + field->offset);
                    const char *enum_str = field->enum_converter(enum_type);
                    fprintf(f, "  # %s", enum_str);
                }
                fprintf(f, "\n");
            }
        }
    }
}