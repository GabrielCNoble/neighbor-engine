#ifndef P_DEFS_H
#define P_DEFS_H

#include "../lib/dstuff/ds_vector.h"
#include "../lib/dstuff/ds_matrix.h"
#include "../lib/dstuff/ds_list.h"
#include "../lib/dstuff/ds_mem.h"
#include <stdint.h>
#include <float.h>

enum P_COL_SHAPE_TYPES
{
    P_COL_SHAPE_TYPE_CAPSULE = 0,
    P_COL_SHAPE_TYPE_CYLINDER,
    P_COL_SHAPE_TYPE_TRI_MESH,
    P_COL_SHAPE_TYPE_ITRI_MESH,
    P_COL_SHAPE_TYPE_BOX,
    P_COL_SHAPE_TYPE_LAST,
};

#define P_SHAPE_DEF_DATA                                                                                           \
    uint32_t type;                                                                                                 \
    vec3_t position;                                                                                               \
    mat3_t orientation;                                                                                            \
    union                                                                                                          \
    {                                                                                                              \
        struct { vec3_t size; } box;                                                                               \
        struct { float height; float radius; } capsule;                                                            \
        struct { vec3_t *verts; uint32_t count; } tri_mesh;                                                        \
        struct { vec3_t *verts; uint32_t vert_count; uint32_t *indices; uint32_t index_count; } itri_mesh;         \
    }

struct p_shape_data_t
{
    P_SHAPE_DEF_DATA;
};

struct p_col_def_record_t
{
    float mass;
    uint32_t type;
    uint32_t shape_count;
    struct p_shape_data_t shape[];
};

struct p_shape_def_t
{
    uint32_t index;
    struct p_shape_def_t *next;
    P_SHAPE_DEF_DATA;
};

struct p_col_def_t
{
    float mass;
    uint32_t type;
    uint32_t shape_count;
    struct p_shape_def_t *shape;
};

enum P_COLLIDER_TYPE
{
    P_COLLIDER_TYPE_STATIC = 0,
    P_COLLIDER_TYPE_KINEMATIC,
    P_COLLIDER_TYPE_DYNAMIC,
    P_COLLIDER_TYPE_TRIGGER,
    P_COLLIDER_TYPE_CHARACTER,
    P_COLLIDER_TYPE_CHILD,
    P_COLLIDER_TYPE_LAST
};

#define P_BASE_COLLIDER_FIELDS              \
    mat3_t orientation;                     \
    vec3_t position;                        \
    void *rigid_body;                       \
    uint32_t index;                         \
    uint32_t type


struct p_collider_t
{
    P_BASE_COLLIDER_FIELDS;
};

struct p_static_collider_t
{
    P_BASE_COLLIDER_FIELDS;
};

struct p_dynamic_collider_t
{
    P_BASE_COLLIDER_FIELDS;
    float mass;
};

enum P_CHARACTER_COLLIDER_FLAGS
{
    P_CHARACTER_COLLIDER_FLAG_ON_GROUND = 1,
    P_CHARACTER_COLLIDER_FLAG_JUMPED = 1 << 1
};

struct p_character_collider_t
{
    P_BASE_COLLIDER_FIELDS;
    float radius;
    float step_height;
    float height;
    float crouch_height;
    uint32_t flags;
};

struct p_child_collider_t
{
    P_BASE_COLLIDER_FIELDS;
    struct p_collider_t *parent;
    struct p_collider_t *children;
    struct p_collider_t *next;
    void *collision_shape;
};

#endif
