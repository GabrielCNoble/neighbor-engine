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
    P_COL_SHAPE_TYPE_BOX,
    P_COL_SHAPE_TYPE_LAST,
};

struct p_shape_def_t
{
    uint32_t type;
    vec3_t position;
    mat3_t orientation;

    union
    {
        struct { vec3_t size; } box;
        struct { float height; float radius; } capsule;
        struct { vec3_t *verts; uint32_t count; }tri_mesh;
    };
};

enum P_COLLIDER_TYPE
{
    P_COLLIDER_TYPE_STATIC = 0,
    P_COLLIDER_TYPE_KINEMATIC,
    P_COLLIDER_TYPE_DYNAMIC,
    P_COLLIDER_TYPE_TRIGGER,
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

struct p_col_def_t
{
    float mass;
    uint32_t type;
    uint32_t shape_count;
    struct p_shape_def_t shape[1];
};

struct p_col_section_t
{
    size_t record_start;
    size_t record_count;
};

struct p_col_record_t
{
    mat3_t orientation;
    vec3_t position;
    struct p_col_def_t def;
};

#endif
