#ifndef XCHG_H
#define XCHG_H

#include "ds_list.h"
#include "ds_vector.h"

struct batch_data_t
{
    unsigned int start;
    unsigned int count;

    vec4_t base_color;
    char material[64];
    char diffuse_texture[64];
    char normal_texture[64];
};

#define DEFAULT_MATERIAL_NAME "default_material"
#define DEFAULT_BATCH (struct batch_data_t){0, 0, {1.0, 1.0, 1.0, 1.0}, DEFAULT_MATERIAL_NAME, "", ""}

struct geometry_data_t
{
    struct ds_list_t vertices;
    struct ds_list_t normals;
    struct ds_list_t tangents;
    struct ds_list_t tex_coords;
    struct ds_list_t batches;
};

#endif // XCHG_G









