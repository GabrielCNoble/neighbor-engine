#include <float.h>
#include "physics.h"
#include "dstuff/ds_slist.h"
#include "dstuff/ds_list.h"
#include "dstuff/ds_mem.h"
#include "dstuff/ds_dbvt.h"
#include "r_draw.h"


struct ds_slist_t p_colliders[P_COLLIDER_TYPE_LAST];
struct ds_slist_t p_col_shapes[P_COL_SHAPE_TYPE_LAST];
struct ds_list_t p_dbvt_contents;
//struct ds_slist_t p_col_planes;
struct ds_list_t p_collisions;
struct ds_list_t p_collision_pairs;
//struct p_col_plane_t *p_pair_col_planes;
struct ds_dbvt_t p_main_dbvt;
struct ds_dbvt_t p_trigger_dbvt;
uint32_t p_frame = 0;

vec3_t p_col_normals[] =
{
    vec3_t_c( 1.0, 0.0, 0.0),
    vec3_t_c(-1.0, 0.0, 0.0),
    vec3_t_c( 0.0, 1.0, 0.0),
    vec3_t_c( 0.0,-1.0, 0.0),
    vec3_t_c( 0.0, 0.0, 1.0),
    vec3_t_c( 0.0, 0.0,-1.0),
};

void p_Init()
{
    p_colliders[P_COLLIDER_TYPE_MOVABLE] = ds_slist_create(sizeof(struct p_movable_collider_t), 512);
    p_colliders[P_COLLIDER_TYPE_STATIC] = ds_slist_create(sizeof(struct p_static_collider_t), 512);
    p_colliders[P_COLLIDER_TYPE_TRIGGER] = ds_slist_create(sizeof(struct p_trigger_collider_t), 128);
    p_dbvt_contents = ds_list_create(sizeof(struct p_collider_t *), 256);
    p_collision_pairs = ds_list_create(sizeof(struct p_col_pair_t), 512);
    p_collisions = ds_list_create(sizeof(struct p_collider_t *), 4096);
//    p_col_planes = ds_slist_create(sizeof(struct p_col_plane_t) * 6, 512);
//    p_pair_col_planes = mem_Calloc(32, sizeof(struct p_col_plane_t));
    p_main_dbvt = ds_dbvt_create(0);
    p_trigger_dbvt = ds_dbvt_create(0);

    p_col_shapes[P_COL_SHAPE_TYPE_CAPSULE] = ds_slist_create(sizeof(struct p_capsule_shape_t), 512);
    p_col_shapes[P_COL_SHAPE_TYPE_TMESH] = ds_slist_create(sizeof(struct p_tmesh_shape_t), 512);
}

void p_Shutdown()
{

}

struct p_collider_t *p_CreateCollider(uint32_t type, vec3_t *position, mat3_t *orientation, struct p_col_shape_t *col_shape)
{
    uint32_t collider_index;
    struct p_collider_t *collider;

    collider_index = ds_slist_add_element(&p_colliders[type], NULL);
    collider = ds_slist_get_element(&p_colliders[type], collider_index);

    collider->index = collider_index;

    if(!orientation)
    {
        mat3_t_identity(&collider->orientation);
    }
    else
    {
        collider->orientation = *orientation;
    }

    collider->position = *position;
    collider->type = type;
    collider->shape = col_shape;
    collider->user_data = NULL;

    struct ds_dbvt_t *dbvh;
    if(type == P_COLLIDER_TYPE_TRIGGER)
    {
        dbvh = &p_trigger_dbvt;
    }
    else
    {
        dbvh = &p_main_dbvt;
    }

    collider->node_index = ds_dbvt_alloc_node(dbvh);
    struct ds_dbvn_t *node = ds_dbvt_get_node_pointer(dbvh, collider->node_index);
    p_CollisionShapeBounds(col_shape, &node->max, &node->min);
    vec3_t_add(&node->min, &node->min, &collider->position);
    vec3_t_add(&node->max, &node->max, &collider->position);
    node->contents = collider;
    ds_dbvt_insert_node(dbvh, collider->node_index);

    return collider;
}

void p_DestroyCollider(struct p_collider_t *collider)
{
    if(collider && collider->index != 0xffffffff)
    {
        ds_dbvt_remove_node(&p_main_dbvt, collider->node_index);
        ds_slist_remove_element(&p_colliders[collider->type], collider->index);
        collider->index = 0xffffffff;
    }
}

struct p_collider_t *p_GetCollider(uint32_t type, uint32_t index)
{
    struct p_collider_t *collider;

    collider = ds_slist_get_element(&p_colliders[type], index);

    if(collider && collider->index == 0xffffffff)
    {
        collider = NULL;
    }

    return collider;
}

void p_DisplaceCollider(struct p_collider_t *collider, vec3_t *disp)
{
//    struct p_col_plane_t *planes;
//
//    switch(collider->type)
//    {
//        case P_COLLIDER_TYPE_STATIC:
//        {
//            vec3_t_add(&collider->position, &collider->position, disp);
//            struct ds_dbvn_t *node = ds_dbvt_get_node_pointer(&p_main_dbvt, collider->node_index);
//            vec3_t_add(&node->max, &node->max, disp);
//            vec3_t_add(&node->min, &node->min, disp);
//            uint32_t node_index = ds_dbvt_nodes_smallest_volume(&p_main_dbvt, collider->node_index);
//            ds_dbvt_pair_nodes(&p_main_dbvt, collider->node_index, node_index);
//
//            planes = ds_slist_get_element(&p_col_planes, collider->planes_index);
//
//            for(uint32_t plane_index = 0; plane_index < 6; plane_index++)
//            {
//                struct p_col_plane_t *plane = planes + plane_index;
//                vec3_t_add(&plane->point, &plane->point, disp);
//            }
//        }
//        break;
//
//        case P_COLLIDER_TYPE_MOVABLE:
//        {
//            struct p_movable_collider_t *movable_collider = (struct p_movable_collider_t *)collider;
//            vec3_t_add(&movable_collider->disp, &movable_collider->disp, disp);
//        }
//        break;
//    }
}

void p_RotateCollider(struct p_collider_t *collider, mat3_t *rot)
{
    mat3_t_mul(&collider->orientation, &collider->orientation, rot);
    p_UpdateColliderNode(collider);
}

//void p_RotateColliderX(struct p_collider_t *collider, float angle)
//{
//    mat3_t_rotate_x(&collider->orientation, angle);
//    p_UpdateColliderNode(collider);
////    p_GenColPlanes(collider);
//}
//
//void p_RotateColliderY(struct p_collider_t *collider, float angle)
//{
//    mat3_t_rotate_y(&collider->orientation, angle);
//    p_UpdateColliderNode(collider);
////    p_GenColPlanes(collider);
//}
//
//void p_RotateColliderZ(struct p_collider_t *collider, float angle)
//{
//    mat3_t_rotate_z(&collider->orientation, angle);
//    p_UpdateColliderNode(collider);
////    p_GenColPlanes(collider);
//}

uint32_t p_BoxIntersect(vec3_t *box_a0, vec3_t *box_a1, vec3_t *box_b0, vec3_t *box_b1)
{
    return box_a0->x < box_b1->x && box_a1->x > box_b0->x &&
           box_a0->y < box_b1->y && box_a1->y > box_b0->y &&
           box_a0->z < box_b1->z && box_a1->z > box_b0->z;
}

void p_UpdateColliders(float delta_time)
{
//    p_collisions.cursor = 0;
    p_frame++;
    p_collision_pairs.cursor = 0;

    r_i_SetViewProjectionMatrix(NULL);
    r_i_SetModelMatrix(NULL);
    r_i_SetShader(NULL);
    vec3_t corners[8];

    for(uint32_t collider_index = 0; collider_index < p_colliders[P_COLLIDER_TYPE_STATIC].cursor; collider_index++)
    {
        struct p_collider_t *collider = p_GetCollider(P_COLLIDER_TYPE_STATIC, collider_index);
        if(collider)
        {
            struct ds_dbvn_t *node = ds_dbvt_get_node_pointer(&p_main_dbvt, collider->node_index);

            corners[0] = vec3_t_c(node->min.x, node->max.y, node->min.z);
            corners[1] = vec3_t_c(node->min.x, node->min.y, node->min.z);
            corners[2] = vec3_t_c(node->max.x, node->min.y, node->min.z);
            corners[3] = vec3_t_c(node->max.x, node->max.y, node->min.z);

            corners[4] = vec3_t_c(node->min.x, node->max.y, node->max.z);
            corners[5] = vec3_t_c(node->min.x, node->min.y, node->max.z);
            corners[6] = vec3_t_c(node->max.x, node->min.y, node->max.z);
            corners[7] = vec3_t_c(node->max.x, node->max.y, node->max.z);

            r_i_DrawLine(&corners[0], &corners[1], &vec4_t_c(0.0, 1.0, 0.0, 1.0), 1.0);
            r_i_DrawLine(&corners[1], &corners[2], &vec4_t_c(0.0, 1.0, 0.0, 1.0), 1.0);
            r_i_DrawLine(&corners[2], &corners[3], &vec4_t_c(0.0, 1.0, 0.0, 1.0), 1.0);
            r_i_DrawLine(&corners[3], &corners[0], &vec4_t_c(0.0, 1.0, 0.0, 1.0), 1.0);

            r_i_DrawLine(&corners[4], &corners[5], &vec4_t_c(0.0, 1.0, 0.0, 1.0), 1.0);
            r_i_DrawLine(&corners[5], &corners[6], &vec4_t_c(0.0, 1.0, 0.0, 1.0), 1.0);
            r_i_DrawLine(&corners[6], &corners[7], &vec4_t_c(0.0, 1.0, 0.0, 1.0), 1.0);
            r_i_DrawLine(&corners[7], &corners[4], &vec4_t_c(0.0, 1.0, 0.0, 1.0), 1.0);

            r_i_DrawLine(&corners[0], &corners[4], &vec4_t_c(0.0, 1.0, 0.0, 1.0), 1.0);
            r_i_DrawLine(&corners[1], &corners[5], &vec4_t_c(0.0, 1.0, 0.0, 1.0), 1.0);
            r_i_DrawLine(&corners[2], &corners[6], &vec4_t_c(0.0, 1.0, 0.0, 1.0), 1.0);
            r_i_DrawLine(&corners[3], &corners[7], &vec4_t_c(0.0, 1.0, 0.0, 1.0), 1.0);
        }
    }

    /* broadphase */
    for(uint32_t col_a_index = 0; col_a_index < p_colliders[P_COLLIDER_TYPE_MOVABLE].cursor; col_a_index++)
    {
        struct p_movable_collider_t *collider_a = (struct p_movable_collider_t *)p_GetCollider(P_COLLIDER_TYPE_MOVABLE, col_a_index);

        if(collider_a)
        {
            collider_a->flags &= ~(P_COLLIDER_FLAG_ON_GROUND | P_COLLIDER_FLAG_TOP_COLLIDED);
            collider_a->first_collision = p_collisions.cursor;

            struct ds_dbvn_t *node = ds_dbvt_get_node_pointer(&p_main_dbvt, collider_a->node_index);
//            vec3_t move_max = node->max;
//            vec3_t move_min = node->min;

//            if(collider_a->disp.x > 0.0) move_max.x += collider_a->disp.x;
//            else move_min.x += collider_a->disp.x;
//
//            if(collider_a->disp.y > 0.0) move_max.y += collider_a->disp.y;
//            else move_min.y += collider_a->disp.y;
//
//            if(collider_a->disp.z > 0.0) move_max.z += collider_a->disp.z;
//            else move_min.z += collider_a->disp.z;

            node->ignore = 1;
            ds_dbvt_box_contents(&p_dbvt_contents, &p_main_dbvt, &node->max, &node->min);
            /* if node A overlaps node B, then node B overlaps node A. To avoid the creation of
            duplicate collision pairs, we mark this node (node A) to be ignored when an overlap
            query for node B gets executed. */
            node->ignore = p_dbvt_contents.cursor > 0;

            if(p_dbvt_contents.cursor)
            {
                for(uint32_t content_index = 0; content_index < p_dbvt_contents.cursor; content_index++)
                {
                    struct p_collider_t *collider_b = *(struct p_collider_t **)ds_list_get_element(&p_dbvt_contents, content_index);
                    uint32_t pair_index = ds_list_add_element(&p_collision_pairs, NULL);
                    struct p_col_pair_t *pair = ds_list_get_element(&p_collision_pairs, pair_index);
                    pair->collider_a = collider_a;
                    pair->collider_b = collider_b;
                }
            }
        }
    }

    for(uint32_t pair_index = 0; pair_index < p_collision_pairs.cursor; pair_index++)
    {
        struct p_col_pair_t *pair = ds_list_get_element(&p_collision_pairs, pair_index);

    }

//    for(uint32_t trigger_index = 0; trigger_index < p_colliders[P_COLLIDER_TYPE_TRIGGER].cursor; trigger_index++)
//    {
//        struct p_trigger_collider_t *trigger_collider = (struct p_trigger_collider_t *)p_GetCollider(P_COLLIDER_TYPE_TRIGGER, trigger_index);
//
//        if(trigger_collider)
//        {
//            trigger_collider->first_collision = p_collisions.cursor;
//
//            struct ds_dbvn_t *node = ds_dbvt_get_node_pointer(&p_trigger_dbvt, trigger_collider->node_index);
//            struct ds_list_t *contents = ds_dbvt_box_contents(&p_main_dbvt, &node->max, &node->min);
//
//            for(uint32_t collider_index = 0; collider_index < contents->cursor; collider_index++)
//            {
//                struct p_collider_t *collider = *(struct p_collider_t **)ds_list_get_element(contents, collider_index);
//                struct p_trace_t trace = {};
//                if(p_ComputeCollision((struct p_collider_t *)trigger_collider, collider, &trace))
//                {
//                    ds_list_add_element(&p_collisions, &collider);
//                }
//            }
//
//            trigger_collider->collision_count = p_collisions.cursor - trigger_collider->first_collision;
//        }
//    }
}

void p_UpdateColliderNode(struct p_collider_t *collider)
{
//    vec3_t corners[8];
//    vec3_t size = collider->size;
//    vec3_t up;
//    vec3_t max = vec3_t_c(-FLT_MAX, -FLT_MAX, -FLT_MAX);
//    vec3_t min = vec3_t_c(FLT_MAX, FLT_MAX, FLT_MAX);
//    vec3_t_mul(&size, &size, 0.5);
//
//    corners[0] = vec3_t_c(-size.x, -size.y, -size.z);
//    corners[1] = vec3_t_c( size.x, -size.y, -size.z);
//    corners[2] = vec3_t_c( size.x, -size.y,  size.z);
//    corners[3] = vec3_t_c(-size.x, -size.y,  size.z);
//
//    corners[4] = vec3_t_c(-size.x,  size.y, -size.z);
//    corners[5] = vec3_t_c( size.x,  size.y, -size.z);
//    corners[6] = vec3_t_c( size.x,  size.y,  size.z);
//    corners[7] = vec3_t_c(-size.x,  size.y,  size.z);
//
//    for(uint32_t corner_index = 0; corner_index < 8; corner_index++)
//    {
//        vec3_t *corner = corners + corner_index;
//        mat3_t_vec3_t_mul(corner, corner, &collider->orientation);
//        vec3_t_max(&max, &max, corner);
//        vec3_t_min(&min, &min, corner);
//    }
//
////    vec3_t_mul(&up, &collider->orientation.rows[1], collider->size.y);
////
////    for(uint32_t corner_index = 4; corner_index < 8; corner_index++)
////    {
////        vec3_t *corner = corners + corner_index;
////        vec3_t_add(corner, corner - 4, &up);
////        vec3_t_max(&max, &max, corner);
////        vec3_t_min(&min, &min, corner);
////    }
//
//    struct ds_dbvn_t *node = ds_dbvt_get_node_pointer(&p_main_dbvt, collider->node_index);
//    vec3_t_add(&node->min, &min, &collider->position);
//    vec3_t_add(&node->max, &max, &collider->position);
//
//    uint32_t sibling_index = ds_dbvt_nodes_smallest_volume(&p_main_dbvt, collider->node_index);
//    ds_dbvt_pair_nodes(&p_main_dbvt, collider->node_index, sibling_index);
}


