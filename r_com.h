#ifndef R_COM_H
#define R_COM_H

#include "dstuff/ds_vector.h"
#include "dstuff/ds_matrix.h"
#include "dstuff/ds_alloc.h"
#include <stdint.h>

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
    R_UNIFORM_MVP,
    R_UNIFORM_TEX0,
    R_UNIFORM_TEX1,
    R_UNIFORM_LAST,
};

enum R_ATTRIBS
{
    R_ATTRIB_POSITION = 1,
    R_ATTRIB_NORMAL = 1 << 1,
    R_ATTRIB_COLOR = R_ATTRIB_NORMAL,
    R_ATTRIB_TEX_COORDS = 1 << 2,
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
        vec3_t normal;
        vec3_t color;
    };
    vec3_t tangent;
    vec2_t tex_coords;
};

struct r_vert_section_t
{
    uint32_t vert_count;
    struct r_vert_t verts[];
};

struct r_batch_t
{
    uint32_t start;
    uint32_t count;
    struct r_material_t *material;
};

struct r_immediate_batch_t
{
    uint32_t start;
    uint32_t count;
    uint32_t mode;
    float size;
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
    uint32_t vert_count;
    struct r_vert_t *verts;
    uint32_t indice_count;
    uint32_t *indices;
    uint32_t batch_count;
    struct r_batch_t *batches;
    struct a_skeleton_t *skeleton;
    uint32_t weight_range_count;
    struct a_weight_range_t *weight_ranges;
    uint32_t weight_count;
    struct a_weight_t *weights;
    struct r_model_t *base; 
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
};

struct r_draw_batch_t 
{
    mat4_t model_view_projection_matrix;
    struct r_batch_t batch;
};


#define R_POSITION_LOCATION 0
#define R_NORMAL_LOCATION 1
#define R_COLOR_LOCATION R_NORMAL_LOCATION
#define R_TEX_COORDS_LOCATION 2
#define R_VERTEX_BUFFER_SIZE (sizeof(struct r_vert_t) * 1000000)
#define R_INDEX_BUFFER_SIZE (sizeof(uint32_t) * 1000000)
#define R_IMMEDIATE_BUFFER_SIZE (sizeof(struct r_vert_t) * 10000)
#define R_MAX_VERTEX_WEIGHTS 16


#endif // R_COM_H
