#ifndef PHYSICS_H
#define PHYSICS_H

#include "dstuff/ds_vector.h"
#include "dstuff/ds_matrix.h"
#include "dstuff/ds_list.h"
#include "dstuff/ds_dbvt.h"
#include <stdint.h>

struct p_box_shape_t
{
    vec3_t size;
};

struct p_capsule_shape_t
{
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
    struct ds_dbvt_t dbvh;
    struct p_col_tri_t *tris;
    uint32_t tri_count;
};

struct p_col_shape_t
{
    uint32_t index;
    uint32_t type;
    union
    {
        struct p_capsule_shape_t capsule_shape;
        struct p_tmesh_shape_t tmesh_shape;
    };
};

enum P_COL_SHAPE_TYPES
{
    P_COL_SHAPE_TYPE_CAPSULE = 0,
    P_COL_SHAPE_TYPE_TMESH
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
};

#define P_COLLIDER_FIELDS           \
    uint32_t type;                  \
    uint32_t index;                 \
    uint32_t node_index;            \
    uint32_t planes_index;          \
    mat3_t orientation;             \
    vec3_t position;                \
    vec3_t size;                    \
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
    uint32_t flags;
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

struct p_trace_t
{
    vec3_t start;
    vec3_t point;
    vec3_t normal;
    vec3_t dir;
    uint32_t solid_start;
    float time;
    struct p_collider_t *collider;
};

struct p_col_pair_t
{
    struct p_collider_t *collider_a;
    struct p_collider_t *collider_b;
};

void p_Init();

void p_Shutdown();

struct p_col_shape_t *p_CreateCollisionShape(uint32_t type);

struct p_col_shape_t *p_CreateCapsuleCollisionShape(float radius, float height);

struct p_col_shape_t *p_CreateTriMeshCollisionShape(vec3_t *verts, uint32_t vert_count);

struct p_collider_t *p_CreateCollider(uint32_t type, vec3_t *position, mat3_t *orientation, vec3_t *size);

struct p_collider_t *p_GetCollider(uint32_t type, uint32_t index);

struct p_collider_t *p_GetCollision(struct p_collider_t *collider, uint32_t collision_index);

void p_DisplaceCollider(struct p_collider_t *collider, vec3_t *disp);

void p_RotateColliderX(struct p_collider_t *collider, float angle);

void p_RotateColliderY(struct p_collider_t *collider, float angle);

void p_RotateColliderZ(struct p_collider_t *collider, float angle);

void p_UpdateColliders();

//void p_GenColPlanes(struct p_collider_t *collider);

//void p_GenPairColPlanes(struct p_collider_t *collider_a, struct p_collider_t *collider_b, struct p_col_plane_t **planes, uint32_t *plane_count);

void p_UpdateColliderNode(struct p_collider_t *collider);

void p_ComputeMoveBox(struct p_collider_t *collider, vec3_t *min, vec3_t *max);

uint32_t p_ComputeCollision(struct p_collider_t *collider_a, struct p_collider_t *collider_b, struct p_trace_t *trace);

uint32_t p_SolidPoint(vec3_t *point, struct p_col_plane_t *planes, uint32_t plane_count);

void p_TracePlanes(vec3_t *start, vec3_t *dir, struct p_col_plane_t *planes, uint32_t plane_count, struct p_trace_t *trace);

uint32_t p_Raycast(vec3_t *start, vec3_t *end, struct p_trace_t *trace);

#endif // PHYSICS_H





