#ifndef R_COM_H
#define R_COM_H

#include "dstuff/ds_vector.h"
#include "dstuff/ds_matrix.h"
#include "dstuff/ds_alloc.h"
#include "dstuff/ds_buffer.h"
#include <stdint.h>
#include <stddef.h>
#include <stdalign.h>

struct r_texture_t
{
    uint32_t handle;
    uint32_t index;
    char *name;
};

struct r_material_t
{
    struct r_texture_t *diffuse_texture;
    struct r_texture_t *normal_texture;
    struct r_texture_t *roughness_texture;
    struct r_texture_t *height_texture;
    uint32_t index;
    char *name;
};

struct r_material_record_t
{
    char name[64];
    char diffuse_texture[64];
    char normal_texture[64];
};

struct r_material_section_t
{
    uint32_t material_count;
    struct r_material_record_t materials[];
};


enum R_UNIFORM
{
    R_UNIFORM_MODEL_VIEW_PROJECTION_MATRIX,
    R_UNIFORM_MODEL_VIEW_MATRIX,
    R_UNIFORM_VIEW_MATRIX,
    R_UNIFORM_CAMERA_MATRIX,
    R_UNIFORM_POINT_PROJ_PARAMS,
//    R_UNIFORM_POINT_PROJ_MATRIX,
//    R_UNIFORM_POINT_VIEW_MATRIX,
    R_UNIFORM_TEX0,
    R_UNIFORM_TEX1,
    R_UNIFORM_TEX2,
    R_UNIFORM_TEX3,
    R_UNIFORM_TEX4,
    R_UNIFORM_TEX_ALBEDO,
    R_UNIFORM_TEX_NORMAL,
    R_UNIFORM_TEX_METALNESS,
    R_UNIFORM_TEX_ROUGHNESS,
    R_UNIFORM_TEX_HEIGHT,
    R_UNIFORM_TEX_CLUSTERS,
    R_UNIFORM_TEX_SHADOW_ATLAS,
    R_UNIFORM_TEX_INDIRECT,
    R_UNIFORM_CLUSTER_DENOM,
    R_UNIFORM_Z_NEAR,
    R_UNIFORM_Z_FAR,
    R_UNIFORM_WIDTH,
    R_UNIFORM_HEIGHT,
    R_UNIFORM_LAST,
};

#define R_CLUSTERS_TEX_UNIT 0
#define R_ALBEDO_TEX_UNIT 1
#define R_NORMAL_TEX_UNIT 2
#define R_METALNESS_TEX_UNIT 3
#define R_ROUGHNESS_TEX_UNIT 4
#define R_HEIGHT_TEX_UNIT 5
#define R_SHADOW_ATLAS_TEX_UNIT 6
#define R_INDIRECT_TEX_UNIT 7

enum R_ATTRIBS
{
    R_ATTRIB_POSITION = 1,
    R_ATTRIB_NORMAL = 1 << 1,
    R_ATTRIB_TANGENT = 1 << 2,
    R_ATTRIB_TEX_COORDS = 1 << 3,
    R_ATTRIB_COLOR = R_ATTRIB_NORMAL,
};

struct r_shader_t
{
    uint32_t handle;
    uint32_t index;
    uint32_t attribs;
    uint32_t uniforms[R_UNIFORM_LAST];
};

struct r_vert_t
{
    vec3_t pos;
    union
    {
        vec4_t normal;
        vec4_t color;
    };
    vec3_t tangent;
    vec2_t tex_coords;
};

struct r_vert_section_t
{
    vec3_t min;
    vec3_t max;
    uint32_t vert_count;
    struct r_vert_t verts[];
};

struct r_batch_t
{
    uint32_t start;
    uint32_t count;
    struct r_material_t *material;
};

struct r_batch_record_t
{
    uint32_t start;
    uint32_t count;
    char material[64];
};

struct r_batch_section_t
{
    uint32_t batch_count;
    struct r_batch_record_t batches[];
};

struct r_index_section_t
{
    uint32_t index_count;
    uint32_t indexes[];
};

struct r_model_t
{
    uint32_t index;
    struct ds_chunk_h vert_chunk;
    struct ds_chunk_h index_chunk;
    struct ds_buffer_t verts;
    struct ds_buffer_t indices;
    struct ds_buffer_t batches;
//    uint32_t vert_count;
//    struct r_vert_t *verts;
//    uint32_t indice_count;
//    uint32_t *indices;
//    uint32_t batch_count;
//    struct r_batch_t *batches;
    struct a_skeleton_t *skeleton;
//    uint32_t weight_range_count;
    struct ds_buffer_t weight_ranges;
//    uint32_t weight_count;
    struct ds_buffer_t weights;
    struct r_model_t *base;
    vec3_t min;
    vec3_t max;
};

struct r_model_geometry_t
{
    uint32_t vert_count;
    struct r_vert_t *verts;
    uint32_t batch_count;
    struct r_batch_t *batches;
    uint32_t index_count;
    uint32_t *indices;
    vec3_t min;
    vec3_t max;
};

struct r_model_skeleton_t
{
    struct a_skeleton_t *skeleton;
    uint32_t weight_range_count;
    struct a_weight_range_t *weight_ranges;
    uint32_t weight_count;
    struct a_weight_t *weights;
};

struct r_model_create_info_t
{
    uint32_t vert_count;
    struct r_vert_t *verts;
    uint32_t batch_count;
    struct r_batch_t *batches;
    uint32_t index_count;
    uint32_t *indices;
    struct a_skeleton_t *skeleton;
    uint32_t weight_range_count;
    struct a_weight_range_t *weight_ranges;
    uint32_t weight_count;
    struct a_weight_t *weights;
    vec3_t min;
    vec3_t max;
};

struct r_draw_batch_t
{
    mat4_t model_view_matrix;
    struct r_batch_t batch;
};

enum R_LIGHT_TYPES
{
    R_LIGHT_TYPE_POINT = 0,
    R_LIGHT_TYPE_SPOT,
    R_LIGHT_TYPE_AREA,
};

struct r_l_data_t
{
    vec4_t pos_rad;
    vec4_t color_res;
};

struct r_light_t
{
    struct r_l_data_t data;
    float energy;

    uint32_t min_x : 7;
    uint32_t min_y : 7;
    uint32_t min_z : 5;

    uint32_t max_x : 7;
    uint32_t max_y : 7;
    uint32_t max_z : 5;

    uint32_t gpu_index;
    uint32_t first_shadow_map;
    uint32_t index;
};

struct r_cluster_t
{
    uint32_t start;
    uint32_t count;
};

#define R_SHADOW_MAP_X_COORD_SHIFT 0
#define R_SHADOW_MAP_Y_COORD_SHIFT 8
#define R_SHADOW_MAP_RES_SHIFT 16
#define R_SHADOW_MAP_FACE_INDEX_SHIFT 0
#define R_SHADOW_MAP_FACE_INDEX_MASK 0x07
#define R_SHADOW_MAP_OFFSET_PACK_SHIFT 3
//#define R_SHADOW_MAP_COORD_OFFSET_MASK 0x03
//#define R_SHADOW_MAP_X_COORD_OFFSET_SHIFT 4
//#define R_SHADOW_MAP_Y_COORD_OFFSET_SHIFT 6

struct r_shadow_map_t
{
    uint32_t shadow_map;
};

struct r_vis_item_t
{
    mat4_t *transform;
    uint32_t index;
    struct r_model_t *model;
};

struct r_world_cmd_t
{
    struct r_material_t *material;
    uint32_t start;
    uint32_t count;
};

struct r_entity_cmd_t
{
    mat4_t model_view_matrix;
    struct r_material_t *material;
    uint32_t start;
    uint32_t count;
};

struct r_shadow_cmd_t
{
    mat4_t model_view_projection_matrix;
    uint32_t start;
    uint32_t count;
    uint32_t shadow_map;
//    uint32_t shadow_res;
};

enum R_IMMEDIATE_DATA_FLAGS
{
    R_IMMEDIATE_DATA_FLAG_BIG = 1,
    R_IMMEDIATE_DATA_FLAG_EXTERN = 1 << 1
};

struct r_immediate_data_header_t
{
    uint32_t flags;
};

enum R_I_CMDS
{
    R_I_CMD_DRAW = 0,
    R_I_CMD_SET_STATE,
    R_I_CMD_SET_MATRIX,
    R_I_CMD_SET_BUFFERS,
};

enum R_I_DRAW_CMDS
{
    R_I_DRAW_CMD_POINT_LIST = 0,
    R_I_DRAW_CMD_LINE_LIST,
    R_I_DRAW_CMD_LINE_STRIP,
    R_I_DRAW_CMD_TRIANGLE_LIST
};

enum R_I_SET_STATE_CMDS
{
    R_I_SET_STATE_CMD_SHADER,
    R_I_SET_STATE_CMD_TEXTURE,
    R_I_SET_STATE_CMD_BLENDING,
};

enum R_I_SET_MATRIX_CMDS
{
    R_I_SET_MATRIX_CMD_MODEL_MATRIX = 0,
    R_I_SET_MATRIX_CMD_VIEW_PROJECTION_MATRIX
};


#define R_IMMEDIATE_DATA_SLOT_SIZE (alignof(max_align_t))
/* sizeof(struct r_immediate_data_t) has to be <= R_IMMEDIATE_DATA_SLOT_SIZE,
and R_IMMEDIATE_DATA_SLOT_SIZE has to be what it is. This is to guarantee that
the address right after this struct is suitably aligned for any kind of type*/
struct r_i_data_t
{
    uint32_t flags;
    void *data;
};

struct r_i_cmd_t
{
    void *data;
    uint16_t type;
    uint16_t sub_type;
};

struct r_i_verts_t
{
    float size;
    uint32_t count;
    struct r_vert_t verts[];
};

struct r_i_indices_t
{
    uint32_t count;
    uint32_t indices[];
};

struct r_i_draw_cmd_t
{
    uint32_t start;
    uint32_t count;
};

struct r_i_geometry_t
{
    struct r_i_verts_t *verts;
    struct r_i_indices_t *indices;
};

struct r_i_draw_list_t
{
    uint32_t indexed;
    struct r_i_draw_cmd_t *commands;
    uint32_t command_count;
};

struct r_i_transform_t
{
    mat4_t transform;
    uint32_t unset;
};

struct r_i_texture_t
{
    struct r_texture_t *texture;
    uint32_t tex_unit;
};

struct r_i_shader_t
{
    struct r_shader_t *shader;
};

struct r_i_blending_t
{
    uint16_t enable;
    uint16_t src_factor;
    uint16_t dst_factor;
};

struct r_i_depth_t
{
    uint16_t enable;
    uint16_t func;
};

struct r_i_stencil_t
{
    uint16_t enable;

    uint16_t stencil_fail;
    uint16_t depth_fail;
    uint16_t depth_pass;

    uint16_t operation;
    uint8_t mask;
    uint8_t ref;
};

struct r_i_cull_face_t
{
    uint16_t enable;
    uint16_t cull_face;
};

struct r_i_scissor_t
{
    uint16_t enable;
    uint16_t x;
    uint16_t y;
    uint16_t width;
    uint16_t height;
};

struct r_i_state_t
{
    struct r_i_shader_t *shader;
    struct r_i_blending_t *blending;
    struct r_i_depth_t *depth;
    struct r_i_stencil_t *stencil;
    struct r_i_cull_face_t *cull_face;
    struct r_i_scissor_t *scissor;
    struct r_i_texture_t *textures;
    uint32_t texture_count;
};

#define R_CLUSTER_ROW_WIDTH 32
#define R_CLUSTER_ROWS 16
#define R_CLUSTER_SLICES 16
#define R_CLUSTER_COUNT (R_CLUSTER_ROWS * R_CLUSTER_ROW_WIDTH * R_CLUSTER_SLICES)

#define R_CLUSTER_MAX_X (R_CLUSTER_ROW_WIDTH - 1)
#define R_CLUSTER_MAX_Y (R_CLUSTER_ROWS - 1)
#define R_CLUSTER_MAX_Z (R_CLUSTER_SLICES - 1)
#define R_CLUSTER_WIDTH 16
#define R_CLUSTER_HEIGHT 16
#define R_MAX_CLUSTER_LIGHTS 64
#define R_MAX_LIGHTS 0xffff

#define R_LIGHTS_UNIFORM_BUFFER_BINDING 0
#define R_LIGHT_INDICES_UNIFORM_BUFFER_BINDING 1
#define R_SHADOW_INDICES_BUFFER_BINDING 2

#define R_POSITION_LOCATION 0
#define R_NORMAL_LOCATION 1
#define R_TANGENT_LOCATION 2
#define R_TEX_COORDS_LOCATION 3
#define R_COLOR_LOCATION R_NORMAL_LOCATION

#define R_SHADOW_MAP_ATLAS_WIDTH 8192
#define R_SHADOW_MAP_ATLAS_HEIGHT 8192
#define R_SHADOW_MAP_MAX_RESOLUTION 1024
#define R_SHADOW_MAP_MIN_RESOLUTION 64
#define R_MAX_SHADOW_MAPS ((R_SHADOW_MAP_ATLAS_WIDTH * R_SHADOW_MAP_ATLAS_HEIGHT)/(R_SHADOW_MAP_MIN_RESOLUTION * R_SHADOW_MAP_MIN_RESOLUTION))

#define R_SHADOW_MAP_FACE_POS_X 0
#define R_SHADOW_MAP_FACE_NEG_X 1
#define R_SHADOW_MAP_FACE_POS_Y 2
#define R_SHADOW_MAP_FACE_NEG_Y 3
#define R_SHADOW_MAP_FACE_POS_Z 4
#define R_SHADOW_MAP_FACE_NEG_Z 5

#define R_VERTEX_BUFFER_SIZE (sizeof(struct r_vert_t) * 5000000)
#define R_INDEX_BUFFER_SIZE (sizeof(uint32_t) * 5000000)
#define R_IMMEDIATE_VERTEX_BUFFER_SIZE (sizeof(struct r_vert_t) * 100000)
#define R_IMMEDIATE_INDEX_BUFFER_SIZE (sizeof(uint32_t) * 100000)
#define R_MAX_VERTEX_WEIGHTS 16


#endif // R_COM_H
