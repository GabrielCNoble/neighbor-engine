#ifndef L_DEFS_H
#define L_DEFS_H

#include <stdint.h>
#include "../lib/dstuff/ds_vector.h"
#include "../lib/dstuff/ds_matrix.h"

#include "p_defs.h"
#include "e_defs.h"

enum L_LEVEL_DATAS
{
    L_LEVEL_DATA_LIGHTS = 1,
    L_LEVEL_DATA_ENTITIES = 1 << 1,
    L_LEVEL_DATA_WORLD = 1 << 2,
    L_LEVEL_DATA_WAYPOINTS = 1 << 3,
    L_LEVEL_DATA_MATERIALS = 1 << 4,
    L_LEVEL_DATA_ALL = L_LEVEL_DATA_LIGHTS |
                       L_LEVEL_DATA_ENTITIES |
                       L_LEVEL_DATA_WORLD |
                       L_LEVEL_DATA_WAYPOINTS |
                       L_LEVEL_DATA_MATERIALS,
};

struct l_light_section_t
{
    size_t record_start;
    size_t record_count;
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
    float softness;
    uint32_t angle;
    uint32_t type;
    uint32_t s_index;
    uint32_t d_index;
    size_t vert_start;
    size_t vert_count;
};

struct l_ent_def_section_t
{
    size_t record_start;
    size_t record_count;
};

struct l_ent_def_record_t
{
    char name[64];
    /* ent def loaded from this record, to speed up entity spawning
    during deserialization */
    struct e_ent_def_t *def;
};

struct l_entity_section_t
{
    size_t record_start;
    size_t record_count;
    size_t reserved[32];
};

struct l_entity_record_t
{
    mat3_t orientation;
    vec3_t position;
    vec3_t scale;

    size_t ent_def;
    uint32_t s_index;
    uint32_t d_index;

    size_t child_start;
    size_t child_count;

    size_t prop_start;
    size_t prop_count;
};

struct l_material_section_t
{
    size_t record_start;
    size_t record_count;
    size_t reserved[32];
};

struct l_material_record_t
{
    char name[64];
    char diffuse_texture[64];
    char normal_texture[64];
    char height_texture[64];
    char roughness_texture[64];
    char metalness_texture[64];
    struct r_material_t *material;
};

struct l_world_section_t
{
    size_t vert_start;
    size_t vert_count;
    size_t index_start;
    size_t index_count;
    size_t batch_start;
    size_t batch_count;
    size_t bsp_start;
    size_t bsp_count;

    size_t reserved[32];
};

struct l_bspn_record_t
{
    vec3_t normal;
    float dist;
    size_t batch_start;
    size_t batch_count;
    size_t front;
    size_t back;
};

struct l_batch_record_t
{
    uint32_t start;
    uint32_t count;
    size_t material;
};


struct l_player_section_t
{

};

struct l_player_record_t
{
    vec3_t position;
    float pitch;
    float yaw;
};


#endif







