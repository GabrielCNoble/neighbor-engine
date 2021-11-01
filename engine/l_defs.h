#ifndef L_DEFS_H
#define L_DEFS_H

#include <stdint.h>
#include "../lib/dstuff/ds_vector.h"
#include "../lib/dstuff/ds_matrix.h"

struct l_light_section_t
{
    size_t light_record_start;
    size_t light_record_count;
    size_t reserved[32];
};

struct l_light_record_t
{
    mat3_t orientation;
    vec3_t position;
    vec3_t color;
    vec2_t size;
    float energy;
    float radius;
    uint32_t type;
    uint32_t uuid;
    size_t vert_start;
    size_t vert_count;
};

struct l_material_section_t
{
    size_t material_record_start;
    size_t material_record_count;
    size_t reserved[32];
};

struct l_material_record_t
{
    char name[32];
    char diffuse_texture[32];
    char normal_texture[32];
    char roughness_texture[32];
    char height_texture[32];
    char metalness_texture[32];
};


struct l_entity_section_t
{

};

struct l_entity_record_t
{

};

#endif
