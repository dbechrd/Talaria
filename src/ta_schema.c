#include "ta_schema.h"
#include "ta_symbol.h"
#include "ta_math.h"
#include "ta_file.h"
#include "ta_camera.h"
#include "ta_mesh.h"
#include "ta_texture.h"
#include "ta_shader.h"
#include "ta_audio.h"
#include "ta_button.h"
#include "ta_rigid_body.h"
#include "ta_light.h"
#include "ta_material.h"
#include "ta_font.h"
#include "ta_model.h"
#include "ta_transform.h"
#include "ta_player.h"
#include "ta_log.h"
#include "dlb/dlb_types.h"
#include "dlb/dlb_vector.h"
#include "dlb/dlb_hash.h"
#include <stdlib.h>
#include <stdio.h>

ta_schema tg_schemas[TYP_COUNT];
static dlb_hash schemas_by_name;

void ta_schema_field_type_str(ta_schema_field_type type, const char **str)
{
    switch(type) {
        case TYP_NULL:              *str = "TYP_NULL";               break;
        /// Misc. field types /////////////////////////////////////////////
        case TYP_VEC2:              *str = "TYP_VEC2";               break;
        case TYP_VEC3:              *str = "TYP_VEC3";               break;
        case TYP_VEC4:              *str = "TYP_VEC4";               break;
        case TYP_MAT3:              *str = "TYP_MAT3";               break;
        case TYP_MAT4:              *str = "TYP_MAT4";               break;
        case TYP_XFORM:             *str = "TYP_XFORM";              break;
        case TYP_RGB:               *str = "TYP_RGB";                break;
        case TYP_RGBA:              *str = "TYP_RGBA";               break;
        case TYP_RGBA_U8:           *str = "TYP_RGBA_U8";            break;
        case TYP_LIGHT_AMBIENT:     *str = "TYP_LIGHT_AMBIENT";      break;
        case TYP_LIGHT_DIRECTIONAL: *str = "TYP_LIGHT_DIRECTIONAL";  break;
        case TYP_LIGHT_POINT:       *str = "TYP_LIGHT_POINT";        break;
        case TYP_LIGHT_SPOT:        *str = "TYP_LIGHT_SPOT";         break;
        case TYP_LIGHT_SHADOWMAP:   *str = "TYP_LIGHT_SHADOWMAP";    break;
        case TYP_SHADER_ATTRIBUTE:  *str = "TYP_SHADER_ATTRIBUTE";   break;
        case TYP_SHADER_UNIFORM:    *str = "TYP_SHADER_UNIFORM";     break;
        case TYP_PLANE:             *str = "TYP_PLANE";              break;
        case TYP_SPHERE:            *str = "TYP_SPHERE";             break;
        case TYP_AABB:              *str = "TYP_AABB";               break;
        case TYP_OBB:               *str = "TYP_OBB";                break;
        case TYP_COLLIDER:          *str = "TYP_COLLIDER";           break;
        case TYP_PIECE:             *str = "TYP_PIECE";              break;
        /// Component types ///////////////////////////////////////////////
        case TYP_AUDIO_SOURCE:      *str = "TYP_AUDIO_SOURCE";       break;
        case TYP_BUTTON:            *str = "TYP_BUTTON";             break;
        case TYP_CAMERA:            *str = "TYP_CAMERA";             break;
        case TYP_GUN:               *str = "TYP_GUN";                break;
        case TYP_LIGHT:             *str = "TYP_LIGHT";              break;
        case TYP_MODEL:             *str = "TYP_MODEL";              break;
        case TYP_PLAYER:            *str = "TYP_PLAYER";             break;
        case TYP_TRANSFORM:         *str = "TYP_TRANSFORM";          break;
        case TYP_RIGID_BODY:        *str = "TYP_RIGID_BODY";         break;
        /// Resource types ////////////////////////////////////////////////
        case TYP_AUDIO_BUFFER:      *str = "TYP_AUDIO_BUFFER";       break;
        case TYP_FONT:              *str = "TYP_FONT";               break;
        case TYP_MATERIAL:          *str = "TYP_MATERIAL";           break;
        case TYP_MESH:              *str = "TYP_MESH";               break;
        case TYP_SHADER:            *str = "TYP_SHADER";             break;
        case TYP_TEXTURE:           *str = "TYP_TEXTURE";            break;
        /// Atomic types //////////////////////////////////////////////////
        case ATOM_BOOL:             *str = "ATOM_BOOL";              break;
        case ATOM_UINT8:            *str = "ATOM_UINT8";             break;
        case ATOM_INT:              *str = "ATOM_INT";               break;
        case ATOM_UINT:             *str = "ATOM_UINT";              break;
        case ATOM_FLOAT:            *str = "ATOM_FLOAT";             break;
        case ATOM_STRING:           *str = "ATOM_STRING";            break;
        case ATOM_ENUM:             *str = "ATOM_ENUM";              break;
        default:
            DLB_ASSERT(!"<UNKNOWN_TA_FIELD_TYPE>");
            return;
    }
}

ta_schema_field_type res_to_typ(ta_res_type type)
{
    ta_schema_field_type schema_type;
    switch (type) {
        // Component types
        case RES_COMP_AUDIO_SOURCE: schema_type = TYP_AUDIO_SOURCE ; break;
        case RES_COMP_BUTTON      : schema_type = TYP_BUTTON       ; break;
        case RES_COMP_CAMERA      : schema_type = TYP_CAMERA       ; break;
        case RES_COMP_GUN         : schema_type = TYP_GUN          ; break;
        case RES_COMP_LIGHT       : schema_type = TYP_LIGHT        ; break;
        case RES_COMP_MODEL       : schema_type = TYP_MODEL        ; break;
        case RES_COMP_PLAYER      : schema_type = TYP_PLAYER       ; break;
        case RES_COMP_TRANSFORM   : schema_type = TYP_TRANSFORM    ; break;
        case RES_COMP_RIGID_BODY  : schema_type = TYP_RIGID_BODY   ; break;
        // Resource types
        case RES_AUDIO_BUFFER     : schema_type = TYP_AUDIO_BUFFER ; break;
        case RES_FONT             : schema_type = TYP_FONT         ; break;
        case RES_MATERIAL         : schema_type = TYP_MATERIAL     ; break;
        case RES_MESH             : schema_type = TYP_MESH         ; break;
        case RES_SHADER           : schema_type = TYP_SHADER       ; break;
        case RES_TEXTURE          : schema_type = TYP_TEXTURE      ; break;
        default                   : schema_type = TYP_NULL         ; break;
    }
    return schema_type;
}

ta_res_type typ_to_res(ta_schema_field_type type)
{
    ta_res_type res_type;
    switch (type) {
        // Component types
        case TYP_AUDIO_SOURCE : res_type = RES_COMP_AUDIO_SOURCE; break;
        case TYP_BUTTON       : res_type = RES_COMP_BUTTON      ; break;
        case TYP_CAMERA       : res_type = RES_COMP_CAMERA      ; break;
        case TYP_GUN          : res_type = RES_COMP_GUN         ; break;
        case TYP_LIGHT        : res_type = RES_COMP_LIGHT       ; break;
        case TYP_MODEL        : res_type = RES_COMP_MODEL       ; break;
        case TYP_PLAYER       : res_type = RES_COMP_PLAYER      ; break;
        case TYP_TRANSFORM    : res_type = RES_COMP_TRANSFORM    ; break;
        case TYP_RIGID_BODY   : res_type = RES_COMP_RIGID_BODY  ; break;
        // Resource types
        case TYP_AUDIO_BUFFER : res_type = RES_AUDIO_BUFFER     ; break;
        case TYP_FONT         : res_type = RES_FONT             ; break;
        case TYP_MATERIAL     : res_type = RES_MATERIAL         ; break;
        case TYP_MESH         : res_type = RES_MESH             ; break;
        case TYP_SHADER       : res_type = RES_SHADER           ; break;
        case TYP_TEXTURE      : res_type = RES_TEXTURE          ; break;
        default               : res_type = RES_COUNT            ; break;
    }
    return res_type;
}

static void type_field_add(ta_schema *schema, ta_schema_field_type type,
    const char *name, size_t offset, size_t size, size_t array_len,
    bool is_alias, enum_to_str *enum_converter, bool is_union_type,
    bool in_union, int union_type)
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

#define TYPE_START(_type, field_type, _init, _free) \
    schema = &tg_schemas[field_type]; \
    schema->type = field_type; \
    schema->name = INTERN(STRING(_type)); \
    schema->size = sizeof(_type); \
    schema->init = _init; \
    schema->free = _free;

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
    dlb_hash_insert(&schemas_by_name, CSTR(STRING(type)), schema);

void ta_schema_register()
{
    DLB_ASSERT(!schemas_by_name.size);
    dlb_hash_init(&schemas_by_name, DLB_HASH_STRING, "[schema_register]", 64);
    ta_schema *schema;

    //--------------------------------------------------------------------------
    // Compound types
    //--------------------------------------------------------------------------
    TYPE_START  (ta_vec2, TYP_VEC2, 0, 0);
    TYPE_FIELD  (ta_vec2, x, ATOM_FLOAT);
    TYPE_FIELD  (ta_vec2, y, ATOM_FLOAT);
    TYPE_END    (ta_vec2);

    TYPE_START  (ta_vec3, TYP_VEC3, 0, 0);
    TYPE_FIELD  (ta_vec3, x, ATOM_FLOAT);
    TYPE_FIELD  (ta_vec3, y, ATOM_FLOAT);
    TYPE_FIELD  (ta_vec3, z, ATOM_FLOAT);
    TYPE_END    (ta_vec3);

    TYPE_START  (ta_vec4, TYP_VEC4, 0, 0);
    TYPE_FIELD  (ta_vec4, x, ATOM_FLOAT);
    TYPE_FIELD  (ta_vec4, y, ATOM_FLOAT);
    TYPE_FIELD  (ta_vec4, z, ATOM_FLOAT);
    TYPE_FIELD  (ta_vec4, w, ATOM_FLOAT);
    TYPE_END    (ta_vec4);

    TYPE_START      (ta_mat3, TYP_MAT3, 0, 0);
    TYPE_UNION_TYPE (ta_mat3,  data, ATOM_ENUM, 0);
    TYPE_UNION_ARRAY(ta_mat3, arr,  ATOM_FLOAT, 9, data, 0);
    TYPE_END        (ta_mat3);

    TYPE_START      (ta_mat4, TYP_MAT4, 0, 0);
    TYPE_UNION_TYPE (ta_mat4,  data, ATOM_ENUM, 0);
    TYPE_UNION_ARRAY(ta_mat4, arr,  ATOM_FLOAT, 16, data, 0);
    TYPE_END        (ta_mat4);

    TYPE_START  (ta_xform, TYP_XFORM, 0, 0);
    TYPE_FIELD  (ta_xform, position,    TYP_VEC3);
    TYPE_FIELD  (ta_xform, orientation, TYP_VEC4);
    TYPE_END    (ta_xform);

    TYPE_START  (ta_rgb, TYP_RGB, 0, 0);
    TYPE_FIELD  (ta_rgb, r, ATOM_FLOAT);
    TYPE_FIELD  (ta_rgb, g, ATOM_FLOAT);
    TYPE_FIELD  (ta_rgb, b, ATOM_FLOAT);
    TYPE_END    (ta_rgb);

    TYPE_START  (ta_rgba, TYP_RGBA, 0, 0);
    TYPE_FIELD  (ta_rgba, r, ATOM_FLOAT);
    TYPE_FIELD  (ta_rgba, g, ATOM_FLOAT);
    TYPE_FIELD  (ta_rgba, b, ATOM_FLOAT);
    TYPE_FIELD  (ta_rgba, a, ATOM_FLOAT);
    TYPE_END    (ta_rgba);

    TYPE_START  (ta_rgba_u8, TYP_RGBA_U8, 0, 0);
    TYPE_FIELD  (ta_rgba_u8, r, ATOM_UINT8);
    TYPE_FIELD  (ta_rgba_u8, g, ATOM_UINT8);
    TYPE_FIELD  (ta_rgba_u8, b, ATOM_UINT8);
    TYPE_FIELD  (ta_rgba_u8, a, ATOM_UINT8);
    TYPE_END    (ta_rgba_u8);

    TYPE_START  (ta_light_ambient, TYP_LIGHT_AMBIENT, 0, 0);
    TYPE_END    (ta_light_ambient);

    TYPE_START  (ta_light_directional, TYP_LIGHT_DIRECTIONAL, 0, 0);
    TYPE_END    (ta_light_directional);

    TYPE_START  (ta_light_point, TYP_LIGHT_POINT, 0, 0);
    TYPE_END    (ta_light_point);

    TYPE_START  (ta_light_spot, TYP_LIGHT_SPOT, 0, 0);
    TYPE_FIELD  (ta_light_spot, theta_cone,    ATOM_FLOAT);
    TYPE_FIELD  (ta_light_spot, theta_falloff, ATOM_FLOAT);
    TYPE_END    (ta_light_spot);

    TYPE_START  (ta_light_shadowmap, TYP_LIGHT_SHADOWMAP, 0, 0);
    TYPE_FIELD  (ta_light_shadowmap, shader,     ATOM_STRING);
    TYPE_FIELD  (ta_light_shadowmap, resolution, ATOM_UINT);
    TYPE_FIELD  (ta_light_shadowmap, znear,      ATOM_FLOAT);
    TYPE_FIELD  (ta_light_shadowmap, zfar,       ATOM_FLOAT);
    TYPE_END    (ta_light_shadowmap);

    TYPE_START  (ta_shader_attribute, TYP_SHADER_ATTRIBUTE, 0, 0);
    TYPE_FIELD  (ta_shader_attribute, name, ATOM_STRING);
    TYPE_ENUM   (ta_shader_attribute, type, ATOM_ENUM, ta_glsl_type_str);
    TYPE_END    (ta_shader_attribute);

    TYPE_START          (ta_shader_uniform, TYP_SHADER_UNIFORM, 0, 0);
    TYPE_FIELD          (ta_shader_uniform, name, ATOM_STRING);
    TYPE_UNION_TYPE     (ta_shader_uniform, type, ATOM_ENUM, ta_glsl_type_str);
    //TYPE_UNION_FIELD  (ta_shader_uniform, glint,     ATOM_INT,  value, TA_GLSL_INT);
    //TYPE_UNION_FIELD  (ta_shader_uniform, gluint,    ATOM_UINT, value, TA_GLSL_UINT);
    //TYPE_UNION_FIELD  (ta_shader_uniform, sampler2d, ATOM_UINT, value, TA_GLSL_SAMPLER2D);
    //TYPE_UNION_FIELD  (ta_shader_uniform, vec2,      TYP_VEC2,   value, TA_GLSL_VEC2);
    //TYPE_UNION_FIELD  (ta_shader_uniform, vec3,      TYP_VEC3,   value, TA_GLSL_VEC3);
    //TYPE_UNION_FIELD  (ta_shader_uniform, vec4,      TYP_VEC4,   value, TA_GLSL_VEC4);
    //TYPE_UNION_FIELD  (ta_shader_uniform, mat3,      TYP_MAT3,   value, TA_GLSL_MAT3);
    //TYPE_UNION_FIELD  (ta_shader_uniform, mat4,      TYP_MAT4,   value, TA_GLSL_MAT4);
    TYPE_UNION_VECTOR   (ta_shader_uniform, properties, TYP_SHADER_UNIFORM, value, TA_GLSL_STRUCT);
    TYPE_END            (ta_shader_uniform);

    TYPE_START  (ta_plane, TYP_PLANE, 0, 0);
    TYPE_FIELD  (ta_plane, center, TYP_VEC3);
    TYPE_FIELD  (ta_plane, normal, TYP_VEC3);
    TYPE_END    (ta_plane);

    TYPE_START  (ta_sphere, TYP_SPHERE, 0, 0);
    TYPE_FIELD  (ta_sphere, center, TYP_VEC3);
    TYPE_FIELD  (ta_sphere, radius, ATOM_FLOAT);
    TYPE_END    (ta_sphere);

    TYPE_START  (ta_aabb, TYP_AABB, 0, 0);
    TYPE_FIELD  (ta_aabb, center,  TYP_VEC3);
    TYPE_FIELD  (ta_aabb, extents, TYP_VEC3);
    TYPE_END    (ta_aabb);

    TYPE_START  (ta_obb, TYP_OBB, 0, 0);
    TYPE_FIELD  (ta_obb, center,      TYP_VEC3);
    TYPE_FIELD  (ta_obb, extents,     TYP_VEC3);
    TYPE_FIELD  (ta_obb, orientation, TYP_VEC4);
    TYPE_END    (ta_obb);

    TYPE_START      (ta_collider, TYP_COLLIDER, 0, 0);
    TYPE_UNION_TYPE (ta_collider, type,   ATOM_ENUM, ta_collider_type_str);
    TYPE_UNION_FIELD(ta_collider, plane,  TYP_PLANE,  data, TA_COLLIDER_PLANE);
    TYPE_UNION_FIELD(ta_collider, sphere, TYP_SPHERE, data, TA_COLLIDER_SPHERE);
    TYPE_UNION_FIELD(ta_collider, obb,    TYP_OBB,    data, TA_COLLIDER_OBB);
    TYPE_END        (ta_collider);

    TYPE_START  (ta_piece, TYP_PIECE, 0, 0);
    TYPE_FIELD  (ta_piece, mesh,     ATOM_STRING);
    TYPE_FIELD  (ta_piece, material, ATOM_STRING);
    TYPE_END    (ta_piece);

    //--------------------------------------------------------------------------
    // Resource types
    //--------------------------------------------------------------------------
    TYPE_START  (ta_audio_buffer, TYP_AUDIO_BUFFER, ta_audio_buffer_init, 0);
    TYPE_FIELD  (ta_audio_buffer, name, ATOM_STRING);
    TYPE_FIELD  (ta_audio_buffer, path, ATOM_STRING);
    TYPE_END    (ta_audio_buffer);

    TYPE_START  (ta_font, TYP_FONT, ta_font_init, 0);
    TYPE_FIELD  (ta_font, name,         ATOM_STRING);
    TYPE_FIELD  (ta_font, path,         ATOM_STRING);
    TYPE_FIELD  (ta_font, pixel_height, ATOM_FLOAT);
    TYPE_FIELD  (ta_font, shader,       ATOM_STRING);
    TYPE_END    (ta_font);

    TYPE_START  (ta_material, TYP_MATERIAL, ta_material_init, 0);
    TYPE_FIELD  (ta_material, name,              ATOM_STRING);
    TYPE_FIELD  (ta_material, shader,            ATOM_STRING);
    TYPE_FIELD  (ta_material, albedo_factor,     TYP_RGBA);
    TYPE_FIELD  (ta_material, albedo_texture,    ATOM_STRING);
    TYPE_FIELD  (ta_material, emission_factor,   TYP_RGB);
    TYPE_FIELD  (ta_material, emission_texture,  ATOM_STRING);
    TYPE_FIELD  (ta_material, metallic_factor,   ATOM_FLOAT);
    TYPE_FIELD  (ta_material, metallic_texture,  ATOM_STRING);
    TYPE_FIELD  (ta_material, roughness_factor,  ATOM_FLOAT);
    TYPE_FIELD  (ta_material, roughness_texture, ATOM_STRING);
    TYPE_FIELD  (ta_material, height_texture,    ATOM_STRING);
    TYPE_FIELD  (ta_material, normal_texture,    ATOM_STRING);
    TYPE_FIELD  (ta_material, occlusion_texture, ATOM_STRING);
    TYPE_END    (ta_material);

    TYPE_START  (ta_mesh, TYP_MESH, ta_mesh_init, ta_mesh_free);
    TYPE_FIELD  (ta_mesh, name,     ATOM_STRING);
    TYPE_FIELD  (ta_mesh, path,     ATOM_STRING);
    TYPE_FIELD  (ta_mesh, offset,   TYP_VEC3);
    TYPE_END    (ta_mesh);

    TYPE_START  (ta_shader, TYP_SHADER, ta_shader_init, ta_shader_free);
    TYPE_FIELD  (ta_shader, name,       ATOM_STRING);
    TYPE_FIELD  (ta_shader, path_vert,  ATOM_STRING);
    TYPE_FIELD  (ta_shader, path_frag,  ATOM_STRING);
    TYPE_VECTOR (ta_shader, attributes, TYP_SHADER_ATTRIBUTE);
    TYPE_VECTOR (ta_shader, uniforms,   TYP_SHADER_UNIFORM);
    TYPE_END    (ta_shader);

    TYPE_START      (ta_texture, TYP_TEXTURE, ta_texture_init, ta_texture_free);
    TYPE_FIELD      (ta_texture, name,          ATOM_STRING);
    TYPE_UNION_TYPE (ta_texture, type,          ATOM_ENUM, ta_texture_type_str);
    TYPE_UNION_FIELD(ta_texture, path,          ATOM_STRING, data, TA_TEXTURE_2D);
    TYPE_UNION_ARRAY(ta_texture, path_faces,    ATOM_STRING, 6, data, TA_TEXTURE_CUBEMAP);
    TYPE_VECTOR     (ta_texture, pixels,        ATOM_UINT8);
    TYPE_FIELD      (ta_texture, width,         ATOM_INT);
    TYPE_FIELD      (ta_texture, height,        ATOM_INT);
    TYPE_FIELD      (ta_texture, channels,      ATOM_INT);
    TYPE_FIELD      (ta_texture, linear,        ATOM_BOOL);
    TYPE_FIELD      (ta_texture, repeat,        ATOM_BOOL);
    TYPE_FIELD      (ta_texture, flip_y,        ATOM_BOOL);
    TYPE_FIELD      (ta_texture, gl_filter_min, ATOM_INT);
    TYPE_FIELD      (ta_texture, gl_filter_mag, ATOM_INT);
    TYPE_END        (ta_texture);

    //--------------------------------------------------------------------------
    // Component types
    //--------------------------------------------------------------------------
    TYPE_START  (ta_audio_source, TYP_AUDIO_SOURCE, ta_audio_source_init, 0);
    TYPE_FIELD  (ta_audio_source, name,         ATOM_STRING);
    TYPE_FIELD  (ta_audio_source, entity,       ATOM_STRING);
    TYPE_FIELD  (ta_audio_source, audio_buffer, ATOM_STRING);
    TYPE_FIELD  (ta_audio_source, pitch,        ATOM_FLOAT);
    TYPE_FIELD  (ta_audio_source, gain,         ATOM_FLOAT);
    TYPE_FIELD  (ta_audio_source, loop,         ATOM_BOOL);
    TYPE_END    (ta_audio_source);

    TYPE_START  (ta_e_button, TYP_BUTTON, e_button_init, 0);
    TYPE_FIELD  (ta_e_button, name,            ATOM_STRING);
    TYPE_FIELD  (ta_e_button, entity,          ATOM_STRING);
    TYPE_FIELD  (ta_e_button, sfx_activated,   ATOM_STRING);
    TYPE_FIELD  (ta_e_button, sfx_active,      ATOM_STRING);
    TYPE_FIELD  (ta_e_button, sfx_deactivated, ATOM_STRING);
    TYPE_END    (ta_e_button);

    TYPE_START  (ta_camera, TYP_CAMERA, ta_camera_init, 0);
    TYPE_FIELD  (ta_camera, name,                ATOM_STRING);
    TYPE_FIELD  (ta_camera, entity,              ATOM_STRING);
    TYPE_FIELD  (ta_camera, position_smooth,     ATOM_FLOAT);
    TYPE_FIELD  (ta_camera, position_target_vel, ATOM_FLOAT);
    TYPE_FIELD  (ta_camera, target_xform,        TYP_XFORM);
    TYPE_FIELD  (ta_camera, yaw,                 ATOM_FLOAT);
    TYPE_FIELD  (ta_camera, yaw_smooth,          ATOM_FLOAT);
    TYPE_FIELD  (ta_camera, pitch,               ATOM_FLOAT);
    TYPE_FIELD  (ta_camera, pitch_smooth,        ATOM_FLOAT);
    TYPE_FIELD  (ta_camera, pitch_min,           ATOM_FLOAT);
    TYPE_FIELD  (ta_camera, pitch_max,           ATOM_FLOAT);
    TYPE_FIELD  (ta_camera, fov,                 ATOM_FLOAT);
    TYPE_FIELD  (ta_camera, znear,               ATOM_FLOAT);
    TYPE_FIELD  (ta_camera, ortho,               ATOM_BOOL);
    TYPE_FIELD  (ta_camera, focal_point,         TYP_VEC3);
    TYPE_FIELD  (ta_camera, up,                  TYP_VEC3);
    TYPE_END    (ta_camera);

    TYPE_START  (ta_gun, TYP_GUN, 0, 0);
    TYPE_FIELD  (ta_gun, name,              ATOM_STRING);
    TYPE_FIELD  (ta_gun, entity,            ATOM_STRING);
    TYPE_FIELD  (ta_gun, carrying_ammo,     ATOM_UINT);
    TYPE_FIELD  (ta_gun, carrying_ammo_max, ATOM_UINT);
    TYPE_FIELD  (ta_gun, loaded_ammo,       ATOM_UINT);
    TYPE_FIELD  (ta_gun, loaded_ammo_max,   ATOM_UINT);
    TYPE_FIELD  (ta_gun, sfx_bang,          ATOM_STRING);
    TYPE_FIELD  (ta_gun, sfx_reload,        ATOM_STRING);
    TYPE_FIELD  (ta_gun, sfx_empty,         ATOM_STRING);
    TYPE_END    (ta_gun);

    TYPE_START      (ta_light, TYP_LIGHT, ta_light_init, 0);
    TYPE_FIELD      (ta_light, name,         ATOM_STRING);
    TYPE_FIELD      (ta_light, entity,       ATOM_STRING);
    TYPE_FIELD      (ta_light, intensity,    ATOM_FLOAT);
    TYPE_FIELD      (ta_light, color,        TYP_RGB);
    TYPE_UNION_TYPE (ta_light, type,         ATOM_ENUM, ta_light_type_str);
    //TYPE_UNION_FIELD(ta_light, ambient,      TYP_LIGHT_AMBIENT,     data, TA_LIGHT_AMBIENT);
    //TYPE_UNION_FIELD(ta_light, directional,  TYP_LIGHT_DIRECTIONAL, data, TA_LIGHT_DIRECTIONAL);
    //TYPE_UNION_FIELD(ta_light, point,        TYP_LIGHT_POINT,       data, TA_LIGHT_POINT);
    TYPE_UNION_FIELD(ta_light, spot,         TYP_LIGHT_SPOT,        data, TA_LIGHT_SPOT);
    TYPE_FIELD      (ta_light, shadowmap,    TYP_LIGHT_SHADOWMAP);
    TYPE_FIELD      (ta_light, disabled,     ATOM_BOOL);
    TYPE_FIELD      (ta_light, cast_shadows, ATOM_BOOL);
    TYPE_END        (ta_light);

    TYPE_START  (ta_model, TYP_MODEL, 0, ta_model_free);
    TYPE_FIELD  (ta_model, name,            ATOM_STRING);
    TYPE_FIELD  (ta_model, entity,          ATOM_STRING);
    TYPE_VECTOR (ta_model, pieces,          TYP_PIECE);
    TYPE_FIELD  (ta_model, invisible,       ATOM_BOOL);
    TYPE_FIELD  (ta_model, cast_shadows,    ATOM_BOOL);
    TYPE_FIELD  (ta_model, receive_shadows, ATOM_BOOL);
    TYPE_END    (ta_model);

    TYPE_START  (ta_player, TYP_PLAYER, 0, ta_player_free);
    TYPE_FIELD  (ta_player, name,   ATOM_STRING);
    TYPE_FIELD  (ta_player, entity, ATOM_STRING);
    TYPE_FIELD  (ta_player, e_gun,  ATOM_STRING);
    TYPE_VECTOR (ta_player, e_guns, ATOM_STRING);
    TYPE_END    (ta_player);

    TYPE_START  (ta_transform, TYP_TRANSFORM, ta_transform_init, ta_transform_free);
    TYPE_FIELD  (ta_transform, name,     ATOM_STRING);
    TYPE_FIELD  (ta_transform, entity,   ATOM_STRING);
    TYPE_FIELD  (ta_transform, xform,    TYP_XFORM);
    TYPE_FIELD  (ta_transform, parent,   ATOM_STRING);
    TYPE_VECTOR (ta_transform, children, ATOM_STRING);
    TYPE_END    (ta_transform);

    TYPE_START  (ta_rigid_body, TYP_RIGID_BODY, ta_rigid_body_init, ta_rigid_body_free);
    TYPE_FIELD  (ta_rigid_body, name,       ATOM_STRING);
    TYPE_FIELD  (ta_rigid_body, entity,     ATOM_STRING);
    TYPE_FIELD  (ta_rigid_body, collider,   TYP_COLLIDER);
    TYPE_FIELD  (ta_rigid_body, mass,       ATOM_FLOAT);
    TYPE_FIELD  (ta_rigid_body, trigger,    ATOM_BOOL);
    TYPE_FIELD  (ta_rigid_body, no_gravity, ATOM_BOOL);
    TYPE_END    (ta_rigid_body);
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

void ta_schema_find_by_name(const char *name, int len, ta_schema **schema)
{
    *schema = dlb_hash_search(&schemas_by_name, name, len, 0);
}

void ta_schema_field_find(ta_schema_field_type type, const char *name, ta_schema_field **field)
{
    ta_schema *schema = &tg_schemas[type];
    dlb_vec_each(ta_schema_field *, f, schema->fields) {
        if (f->name == name) {
            *field = f;
            break;
        }
    }
}

static bool schema_atom_present(ta_schema_field *field, void *ptr)
{
    bool present = false;
    switch (field->type) {
        case ATOM_BOOL: {
            bool *val = ptr;
            if (*val) present = true;
            break;
        } case ATOM_UINT8: {
            u8 *val = ptr;
            if (*val) present = true;
            break;
        } case ATOM_INT: {
            s32 *val = ptr;
            if (*val) present = true;
            break;
        } case ATOM_UINT: {
            u32 *val = ptr;
            if (*val) present = true;
            break;
        } case ATOM_FLOAT: {
            float *val = ptr;
            if (*val) present = true;
            break;
        } case ATOM_STRING: {
            const char **val = ptr;
            if (*val) present = true;
            break;
        } case ATOM_ENUM: {
            //int *val = ptr;
            present = true;
            break;
        } default: {
            PANIC("Unexpected field type, don't know how to print");
        }
    }
    return present;
}

static void schema_atom_print(FILE *f, ta_schema_field *field, void *ptr)
{
    switch (field->type) {
        case ATOM_BOOL: {
            bool *val = ptr;
            fprintf(f, "%s", *val ? "true" : "false");
            break;
        } case ATOM_UINT8: {
            u8 *val = ptr;
            fprintf(f, "%u", *val);
            break;
        } case ATOM_INT: {
            s32 *val = ptr;
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

            u8 *arr = (ptr + field->offset);
            size_t arr_len = field->array_len;
            if (arr_len == 1) {
                arr = *(void **)(ptr + field->offset);
                arr_len = dlb_vec_len(arr);
            }

            if (arr_len) {
                indent(f, level + 1);
                u8 *arr_end = arr + (arr_len * field->size);
                fprintf(f, "%s: [", field->name);
                if (field->type < TYP_COUNT) {
                    fprintf(f, "\n");
                    for (u8 *p = arr; p != arr_end; p += field->size) {
                        indent(f, level + 2);
                        fprintf(f, "{\n");
                        ta_schema_print(f, field->type, p, level + 2, in_array + 1);
                        indent(f, level + 2);
                        fprintf(f, "},\n");
                    }
                    indent(f, level + 1);
                } else {
                    for (u8 *p = arr; p != arr_end; p += field->size) {
                        schema_atom_print(f, field, p);
                        fprintf(f, ", ");
                    }
                }
                fprintf(f, "]\n");
            } else {
                // Don't print empty arrays?
                //indent(f, level + 1);
                //fprintf(f, "%s: []\n", field->name);
            }
        } else {
            if (field->type < TYP_COUNT) {
                indent(f, level + 1);
                fprintf(f, "%s:\n", field->name);
                ta_schema_print(f, field->type, ptr + field->offset, level + 1, 0);
                if (in_array) {
                    fprintf(f, ",");
                }
            } else {
                if (field->is_union_type) {
                    union_type = *(int *)(ptr + field->offset);
                }
                if (level || in_array || schema_atom_present(field, ptr + field->offset)) {
                    indent(f, level + 1);
                    fprintf(f, "%s: ", field->name);
                    schema_atom_print(f, field, ptr + field->offset);
                    if (in_array) {
                        fprintf(f, ",");
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
}
static void schema_atom_print_json(FILE *f, ta_schema_field *field, void *ptr)
{
    switch (field->type) {
        case ATOM_BOOL: {
            bool *val = ptr;
            fprintf(f, "%s", *val ? "true" : "false");
            break;
        } case ATOM_UINT8: {
            u8 *val = ptr;
            fprintf(f, "%u", *val);
            break;
        } case ATOM_INT: {
            s32 *val = ptr;
            fprintf(f, "%d", *val);
            break;
        } case ATOM_UINT: {
            u32 *val = ptr;
            fprintf(f, "%u", *val);
            break;
        } case ATOM_FLOAT: {
            float *val = ptr;
            fprintf(f, "\"0x%08X (%f)\"", *(u32 *)val, *val);
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
void ta_schema_print_json(FILE *f, ta_schema_field_type type, u8 *ptr, int level,
    int in_array)
{
    static int last_char_comma = false;
    static fpos_t last_char_comma_pos;

    ta_schema *schema = &tg_schemas[type];

    // NOTE: Defaults to a value unlikely to be used in any legitimate enum to
    //       detect errors more easily.
    int union_type = -9001;

    size_t field_count = dlb_vec_len(schema->fields);
    for (size_t field_idx = 0; field_idx < field_count; ++field_idx) {
        ta_schema_field *field = &schema->fields[field_idx];
        if (field->is_alias) {
            continue;
        }
        if (field->in_union && field->union_type != union_type) {
            continue;
        }

        u8 *field_ptr = ptr + field->offset;

        if (field->array_len) {
            u8 *arr = field_ptr;
            size_t arr_len = field->array_len;
            if (arr_len) {
                if (arr_len == 1) {
                    arr = *(void **)field_ptr;
                    arr_len = dlb_vec_len(arr);
                }
            }

            // Don't print empty arrays
            if (!arr_len) {
                //indent(f, level + 1);
                //fprintf(f, "\"%s\": []", field->name);
                continue;
            }

            indent(f, level + 1);
            fprintf(f, "\"%s\": [", field->name);
            for (size_t arr_idx = 0; arr_idx < arr_len; ++arr_idx) {
                if (field->type < TYP_COUNT) {
                    fprintf(f, "\n");
                    indent(f, level + 2);
                    fprintf(f, "{\n");
                    ta_schema_print_json(f, field->type, arr, level + 2, in_array + 1);
                    if (last_char_comma) {
                        DLB_ASSERT(!fsetpos(f, &last_char_comma_pos));
                        fprintf(f, "\n");
                    }
                    indent(f, level + 2);
                    fprintf(f, "}");
                } else {
                    schema_atom_print_json(f, field, arr);
                }
                if (arr_len && arr_idx < arr_len - 1) {
                    DLB_ASSERT(!fgetpos(f, &last_char_comma_pos));
                    fprintf(f, ",");
                    if (field->type > TYP_COUNT) {
                        fprintf(f, " ");
                    }
                    last_char_comma = true;
                } else {
                    last_char_comma = false;
                }
                if (field->type < TYP_COUNT) {
                    fprintf(f, "\n");
                }
                arr += field->size;
            }
            if (field->type < TYP_COUNT) {
                indent(f, level + 1);
            }
            DLB_ASSERT(!last_char_comma);
            fprintf(f, "]");
        } else {
            if (field->type < TYP_COUNT) {
                indent(f, level + 1);
                fprintf(f, "\"%s\": {\n", field->name);
                ta_schema_print_json(f, field->type, field_ptr, level + 1, 0);
                if (last_char_comma) {
                    DLB_ASSERT(!fsetpos(f, &last_char_comma_pos));
                    fprintf(f, "\n");
                }
                indent(f, level + 1);
                fprintf(f, "}");
            } else {
                if (field->is_union_type) {
                    union_type = *(int *)field_ptr;
                }
                if (level || in_array || schema_atom_present(field, field_ptr)) {
                    indent(f, level + 1);
                    fprintf(f, "\"%s\": ", field->name);
                    schema_atom_print_json(f, field, field_ptr);
                    fflush(f);
                    //if (field->type == ATOM_ENUM && field->enum_converter) {
                    //    int enum_type = *(int *)(ptr + field->offset);
                    //    const char *enum_str = field->enum_converter(enum_type);
                    //    fprintf(f, "  // %s", enum_str);
                    //}
                }
            }
        }

        if (field_count && field_idx < field_count - 1) {
            DLB_ASSERT(!fgetpos(f, &last_char_comma_pos));
            fprintf(f, ",");
            last_char_comma = true;
        } else {
            last_char_comma = false;
        }
        fprintf(f, "\n");
    }
}