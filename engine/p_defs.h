#ifndef P_DEFS_H
#define P_DEFS_H

#include "../lib/dstuff/ds_vector.h"
#include "../lib/dstuff/ds_matrix.h"
#include "../lib/dstuff/ds_list.h"
#include "../lib/dstuff/ds_dbvt.h"
#include "../lib/dstuff/ds_mem.h"
#include <stdint.h>
#include <float.h>

#define P_COL_SHAPE_FIELDS  \
    uint32_t index;         \
    uint32_t type

struct p_col_shape_t
{
    P_COL_SHAPE_FIELDS;
};

struct p_box_shape_t
{
    P_COL_SHAPE_FIELDS;
    vec3_t size;
};

struct p_capsule_shape_t
{
    P_COL_SHAPE_FIELDS;
    float radius;
    float height;
};

struct p_col_tri_t
{
    vec3_t verts[3];
    vec3_t normal;
};

struct p_conv_hull_t
{
    vec3_t *verts;
    uint32_t vert_count;
};

struct p_tmesh_shape_t
{
    P_COL_SHAPE_FIELDS;
    struct ds_dbvt_t dbvh;
    struct p_col_tri_t *tris;
    uint32_t tri_count;
};

struct p_contact_t
{
    vec3_t point;
    vec3_t normal;
    float depth;
};

enum P_COL_SHAPE_TYPES
{
    P_COL_SHAPE_TYPE_CAPSULE = 0,
    P_COL_SHAPE_TYPE_TMESH,
    P_COL_SHAPE_TYPE_LAST,
};


struct p_col_plane_t
{
    vec3_t normal;
    vec3_t point;
    vec3_t t0;
    vec3_t t1;
};

enum P_COLLIDER_TYPE
{
    P_COLLIDER_TYPE_STATIC = 0,
    P_COLLIDER_TYPE_MOVABLE,
    P_COLLIDER_TYPE_TRIGGER,
    P_COLLIDER_TYPE_LAST
};

enum P_COLLIDER_FLAGS
{
    P_COLLIDER_FLAG_ON_GROUND = 1,
    P_COLLIDER_FLAG_TOP_COLLIDED = 1 << 1,
    P_COLLIDER_FLAG_BROADPHASE_DONE = 1 << 2,
};

#define P_COLLIDER_FIELDS           \
    uint32_t index;                 \
    uint32_t node_index;            \
    uint16_t type;                  \
    uint16_t flags;                 \
    struct p_col_shape_t *shape;    \
    mat3_t orientation;             \
    vec3_t position;                \
    void *user_data

struct p_collider_t
{
    P_COLLIDER_FIELDS;
};

struct p_static_collider_t
{
    P_COLLIDER_FIELDS;
};

struct p_movable_collider_t
{
    P_COLLIDER_FIELDS;
//    uint32_t flags;
    uint16_t first_collision;
    uint16_t collision_count;
    vec3_t disp;
};

struct p_trigger_collider_t
{
    P_COLLIDER_FIELDS;
    uint16_t first_collision;
    uint16_t collision_count;
//    struct list_t collisions;
};

//struct p_collider_t
//{
//    uint16_t type;
//    uint16_t flags;
//    uint32_t index;
//    uint32_t node_index;
//    uint16_t first_collision;
//    uint16_t collision_count;
//    vec3_t position;
//    vec3_t size;
//    vec3_t disp;
//};

//struct p_trigger_t
//{
//    uint32_t node_index;
//    uint32_t index;
//    uint32_t frame;
//    struct list_t collisions;
//    vec3_t position;
//    vec3_t size;
//};

//struct p_trace_t
//{
//    vec3_t start;
//    vec3_t point;
//    vec3_t normal;
//    vec3_t dir;
//    uint32_t solid_start;
//    float time;
//    struct p_collider_t *collider;
//};

struct p_col_pair_t
{
    struct p_collider_t *collider_a;
    struct p_collider_t *collider_b;
};

#endif
