#ifndef E_DEFS_H
#define E_DEFS_H

#include <stdint.h>
#include "../lib/dstuff/ds_list.h"
#include "r_defs.h"
#include "p_defs.h"

struct e_prop_t
{
    char *name;
    uint32_t size;
    void *data;
};

struct e_entity_t;

typedef void (thinker_t)(struct g_entity_t *this);

struct e_ent_def_t
{
    uint32_t index;
    char name[32];

    struct e_ent_def_t *next;
    struct e_ent_def_t *prev;
    struct e_ent_def_t *children;

    struct r_model_t *model;
    struct p_col_def_t *collider;

    mat3_t local_orientation;
    vec3_t local_position;
    vec3_t local_scale;
};

enum E_COMPONENT_TYPES
{
    E_COMPONENT_TYPE_LOCAL_TRANSFORM = 0,
    E_COMPONENT_TYPE_TRANSFORM,
    E_COMPONENT_TYPE_MODEL,
    E_COMPONENT_TYPE_PHYSICS,
    E_COMPONENT_TYPE_LAST,
};

#define E_ENT_BASE_COMPONENT_FIELDS   \
    struct e_entity_t *entity;        \
    uint32_t index;                   \
    uint32_t type;


struct e_component_t
{
    E_ENT_BASE_COMPONENT_FIELDS;
};

struct e_local_transform_component_t
{
    E_ENT_BASE_COMPONENT_FIELDS;
    struct e_local_transform_component_t *parent;
    struct e_local_transform_component_t *children;
    struct e_local_transform_component_t *next;
    struct e_local_transform_component_t *prev;

    mat3_t local_orientation;
    vec3_t local_position;
    vec3_t local_scale;
    uint32_t root_index;
};

struct e_physics_component_t
{
    E_ENT_BASE_COMPONENT_FIELDS;
    struct p_collider_t *collider;
};

struct e_transform_component_t
{
    E_ENT_BASE_COMPONENT_FIELDS;
    mat4_t transform;
    vec3_t extents;
};

struct e_model_component_t
{
    E_ENT_BASE_COMPONENT_FIELDS;
    struct r_model_t *model;
};

struct e_entity_t
{
    uint32_t index;

    union
    {
        struct
        {
            struct e_local_transform_component_t *local_transform_component;
            struct e_transform_component_t *transform_component;
            struct e_physics_component_t *physics_component;
            struct e_model_component_t *model_component;
        };

        struct e_component_t *components[E_COMPONENT_TYPE_LAST];
    };
};

//struct e_entity_t
//{
//    uint32_t index;
//    mat4_t transform;
//    mat3_t local_orientation;
//    vec3_t local_position;
//    vec3_t scale;
//    mat4_t *parent_transform; /* can be either other entity or skeleton attachment point */
//    struct ds_list_t props;
//    thinker_t *thinker;
//    struct p_collider_t *collider;
//    struct r_model_t *model;
//    struct a_mixer_t *mixer;
//    struct r_vis_item_t *item;
//    vec3_t extents;
//};

//struct e_handle_t
//{
//    uint32_t def   : 1;
//    uint32_t index : 31;
//};


#endif // E_DEFS_H
