#ifndef L_DEFS_H
#define L_DEFS_H

#include <stdint.h>
#include "../lib/dstuff/ds_vector.h"
#include "../lib/dstuff/ds_matrix.h"

#include "p_defs.h"
#include "e_defs.h"
#include "../engine/g_defs.h"

#define L_LEVEL_HEADER_MAGIC0 0x4749454e    /* NEIG */
#define L_LEVEL_HEADER_MAGIC1 0x524f4248    /* HBOR */

enum L_HEADER_VERSIONS
{
    L_HEADER_VERSION_CURRENT
};
//
//struct l_header_v0_t
//{
//    ptrdiff_t level_editor_start;
//    ptrdiff_t level_editor_size;
//
//    ptrdiff_t light_section_start;
//    ptrdiff_t light_section_size;
//
//    ptrdiff_t entity_section_start;
//    ptrdiff_t entity_section_size;
//
//    ptrdiff_t ent_def_section_start;
//    ptrdiff_t ent_def_section_size;
//
//    ptrdiff_t material_section_start;
//    ptrdiff_t material_section_size;
//
//    ptrdiff_t world_section_start;
//    ptrdiff_t world_section_size;
//
//    ptrdiff_t waypoint_section_start;
//    ptrdiff_t waypoint_section_size;
//
//    ptrdiff_t game_section_start;
//    ptrdiff_t game_section_size;
//};

struct l_level_header_t
{
    uint32_t magic0;
    uint32_t magic1;
    uint64_t version;

    uint64_t level_editor_start;
    uint64_t level_editor_size;

    uint64_t light_section_start;
    uint64_t light_section_size;

    uint64_t entity_section_start;
    uint64_t entity_section_size;

    uint64_t ent_def_section_start;
    uint64_t ent_def_section_size;

    uint64_t material_section_start;
    uint64_t material_section_size;

    uint64_t world_section_start;
    uint64_t world_section_size;

    uint64_t waypoint_section_start;
    uint64_t waypoint_section_size;

    uint64_t game_section_start;
    uint64_t game_section_size;

    size_t reserved[32];
};

struct l_light_section_t
{
    uint64_t record_start;
    uint64_t record_count;
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
    uint64_t vert_start;
    uint64_t vert_count;
};

struct l_ent_def_section_t
{
    uint64_t record_start;
    uint64_t record_count;
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
    uint64_t record_start;
    uint64_t record_count;
    uint64_t reserved[32];
};

struct l_entity_record_t
{
    mat3_t orientation;
    vec3_t position;
    vec3_t scale;

    uint64_t ent_def;
    uint32_t s_index;
    uint32_t d_index;

    uint64_t child_start;
    uint64_t child_count;

    uint64_t prop_start;
    uint64_t prop_count;
};

struct l_material_section_t
{
    uint64_t record_start;
    uint64_t record_count;
    uint64_t reserved[32];
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
    uint64_t vert_start;
    uint64_t vert_count;
    uint64_t index_start;
    uint64_t index_count;
    uint64_t batch_start;
    uint64_t batch_count;
    uint64_t bsp_start;
    uint64_t bsp_count;

    uint64_t reserved[32];
};

struct l_bspn_record_t
{
    vec3_t normal;
    float dist;
    uint64_t batch_start;
    uint64_t batch_count;
    uint64_t front;
    uint64_t back;
};

struct l_batch_record_t
{
    uint64_t start;
    uint64_t count;
    size_t material;
};

//struct l_game_section_t
//{
//    uint64_t enemy_start;
//    uint64_t enemy_count;
//
//    uint64_t player_start;
//    uint64_t player_size;
//
//    uint64_t entity_start;
//    uint64_t entity_size;
//};

//struct l_enemy_record_t
//{
//    uint64_t type;
//    vec3_t position;
//    mat3_t orientation;
//    uint32_t d_index;
//    uint32_t s_index;
//
//    union
//    {
//        struct g_camera_fields_t camera_fields;
//        struct { uint64_t extra[32]; };
//    };
//};


//struct l_player_section_t
//{
//
//};
//
//struct l_player_record_t
//{
//    vec3_t position;
//    float pitch;
//    float yaw;
//};

struct l_level_t
{
    uint32_t index;
};

#endif







