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
    P_COL_SHAPE_TYPE_SPHERE,
    P_COL_SHAPE_TYPE_BOX,
    P_COL_SHAPE_TYPE_TRI_MESH,
    P_COL_SHAPE_TYPE_ITRI_MESH,
    P_COL_SHAPE_TYPE_LAST,
};

#define P_SHAPE_DEF_FIELDS                                                                                         \
    uint32_t type;                                                                                                 \
    vec3_t position;                                                                                               \
    mat3_t orientation;                                                                                            \
    union                                                                                                          \
    {                                                                                                              \
        struct { vec3_t size; } box;                                                                               \
        struct { float height; float radius; } capsule;                                                            \
        struct { float height; float radius; } cylinder;                                                           \
        struct { vec3_t *verts; uint32_t count; } tri_mesh;                                                        \
        struct { vec3_t *verts; uint32_t vert_count; uint32_t *indices; uint32_t index_count; } itri_mesh;         \
    }

struct p_shape_def_fields_t
{
    P_SHAPE_DEF_FIELDS;
};

//struct p_col_def_record_t
//{
//    float mass;
//    uint32_t type;
//    uint32_t shape_count;
//    struct p_shape_def_fields_t shape[];
//};

struct p_shape_def_t
{
    uint32_t index;
    struct p_shape_def_t *next;

    union
    {
        struct { P_SHAPE_DEF_FIELDS; };
        struct p_shape_def_fields_t fields;
    };
};

#define P_COL_DEF_FIELDS(SHAPE_LIST)                                                                \
    uint32_t type;                                                                                  \
    union                                                                                           \
    {                                                                                               \
        struct {float step_height; float crouch_height; float height; float radius;} character;     \
        struct {float mass; uint32_t shape_count; SHAPE_LIST;} passive;                             \
    }

struct p_col_def_t
{
    P_COL_DEF_FIELDS(struct p_shape_def_t *shape);
};

struct p_col_def_record_t
{
    size_t shape_start;
    P_COL_DEF_FIELDS(struct p_shape_def_fields_t shape[]);
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

enum P_COLLIDER_FLAGS
{
    P_COLLIDER_FLAG_FROZEN = 1,
};

#define P_BASE_COLLIDER_FIELDS              \
    struct p_constraint_t *constraints;     \
    mat3_t orientation;                     \
    vec3_t position;                        \
    void *rigid_body;                       \
    uint32_t index;                         \
    uint32_t type                           \
//    uint16_t flags


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


enum P_CONSTRAINT_TYPES
{
    P_CONSTRAINT_TYPE_HINGE = 0,
    P_CONSTRAINT_TYPE_POINT_TO_POINT,
    P_CONSTRAINT_TYPE_SLIDER,
    P_CONSTRAINT_TYPE_LAST,
};

#define P_CONSTRAINT_FIELDS                                                                                                 \
    uint32_t type;                                                                                                          \
    union                                                                                                                   \
    {                                                                                                                       \
        struct {float limit_low; float limit_high; vec3_t pivot_a; vec3_t pivot_b; vec3_t axis; } hinge;                    \
    }

struct p_constraint_fields_t
{
    P_CONSTRAINT_FIELDS;
};

struct p_collider_constraint_t
{
    struct p_constraint_t *next;
    struct p_constraint_t *prev;
    struct p_collider_t *collider;
    uint32_t transform_set;
};

struct p_constraint_t
{
    uint32_t index;
    void *constraint;
    struct p_collider_constraint_t colliders[2];

    union
    {
        struct { P_CONSTRAINT_FIELDS; };
        struct p_constraint_fields_t fields;
    };
};

struct p_constraint_def_t
{
    union
    {
        struct {P_CONSTRAINT_FIELDS; };
        struct p_constraint_fields_t fields;
    };
};



#endif
