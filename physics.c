#include <float.h>
#include "physics.h"
#include "dstuff/ds_stack_list.h"
#include "dstuff/ds_list.h"
#include "dstuff/ds_mem.h"
#include "dstuff/ds_dbvh.h"
#include "r_draw.h"


struct stack_list_t p_colliders[P_COLLIDER_TYPE_LAST];
struct stack_list_t p_col_planes;
struct list_t p_collisions;
struct list_t p_collision_pairs;
struct p_col_plane_t *p_pair_col_planes;
struct dbvh_tree_t p_main_dbvh;
struct dbvh_tree_t p_trigger_dbvh;
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
    p_colliders[P_COLLIDER_TYPE_MOVABLE] = create_stack_list(sizeof(struct p_movable_collider_t), 512);
    p_colliders[P_COLLIDER_TYPE_STATIC] = create_stack_list(sizeof(struct p_static_collider_t), 512);
    p_colliders[P_COLLIDER_TYPE_TRIGGER] = create_stack_list(sizeof(struct p_trigger_collider_t), 128);
    p_collisions = create_list(sizeof(struct p_collider_t *), 4096);
    p_col_planes = create_stack_list(sizeof(struct p_col_plane_t) * 6, 512);
    p_pair_col_planes = mem_Calloc(32, sizeof(struct p_col_plane_t));
    p_main_dbvh = create_dbvh_tree(0);
    p_trigger_dbvh = create_dbvh_tree(0);
}

void p_Shutdown()
{

}

struct p_collider_t *p_CreateCollider(uint32_t type, vec3_t *position, mat3_t *orientation, vec3_t *size)
{
    uint32_t collider_index;
    struct p_collider_t *collider;

    collider_index = add_stack_list_element(&p_colliders[type], NULL);
    collider = get_stack_list_element(&p_colliders[type], collider_index);

    collider->index = collider_index;
    collider->planes_index = add_stack_list_element(&p_col_planes, NULL);

    if(!orientation)
    {
        mat3_t_identity(&collider->orientation);
    }
    else
    {
        collider->orientation = *orientation;
    }

    collider->position = *position;
    collider->size = *size;
    collider->type = type;
    collider->user_data = NULL;

    struct dbvh_tree_t *dbvh;
    if(type == P_COLLIDER_TYPE_TRIGGER)
    {
        dbvh = &p_trigger_dbvh;
    }
    else
    {
        dbvh = &p_main_dbvh;
    }

    collider->node_index = alloc_dbvh_node(dbvh);
    struct dbvh_node_t *node = get_dbvh_node_pointer(dbvh, collider->node_index);
    vec3_t_fmadd(&node->min, &collider->position, &collider->size, -0.5);
    vec3_t_fmadd(&node->max, &collider->position, &collider->size, 0.5);


    node->contents = collider;
    insert_node_into_dbvh(dbvh, collider->node_index);

    p_GenColPlanes(collider);

    return collider;
}

struct p_collider_t *p_GetCollider(uint32_t type, uint32_t index)
{
    struct p_collider_t *collider;

    collider = get_stack_list_element(&p_colliders[type], index);

    if(collider && collider->index == 0xffffffff)
    {
        collider = NULL;
    }

    return collider;
}

struct p_collider_t *p_GetCollision(struct p_collider_t *collider, uint32_t collision_index)
{
    uint32_t index;
    uint32_t last;
    switch(collider->type)
    {
        case P_COLLIDER_TYPE_TRIGGER:
        {
            struct p_trigger_collider_t *trigger_collider = (struct p_trigger_collider_t *)collider;
            index = trigger_collider->first_collision + collision_index;
            last = index + trigger_collider->collision_count - 1;
        }
        break;
    }

    if(index > last)
    {
        return NULL;
    }

    return *(struct p_collider_t **)get_list_element(&p_collisions, index);
}

void p_DisplaceCollider(struct p_collider_t *collider, vec3_t *disp)
{
    struct p_col_plane_t *planes;

    switch(collider->type)
    {
        case P_COLLIDER_TYPE_STATIC:
        {
            vec3_t_add(&collider->position, &collider->position, disp);
            struct dbvh_node_t *node = get_dbvh_node_pointer(&p_main_dbvh, collider->node_index);
            vec3_t_add(&node->max, &node->max, disp);
            vec3_t_add(&node->min, &node->min, disp);
            uint32_t node_index = nodes_smallest_volume(&p_main_dbvh, collider->node_index);
            pair_dbvh_nodes(&p_main_dbvh, collider->node_index, node_index);

            planes = get_stack_list_element(&p_col_planes, collider->planes_index);

            for(uint32_t plane_index = 0; plane_index < 6; plane_index++)
            {
                struct p_col_plane_t *plane = planes + plane_index;
                vec3_t_add(&plane->point, &plane->point, disp);
            }
        }
        break;

        case P_COLLIDER_TYPE_MOVABLE:
        {
            struct p_movable_collider_t *movable_collider = (struct p_movable_collider_t *)collider;
            vec3_t_add(&movable_collider->disp, &movable_collider->disp, disp);
        }
        break;
    }
}

void p_RotateColliderX(struct p_collider_t *collider, float angle)
{
    mat3_t_rotate_x(&collider->orientation, angle);
    p_UpdateColliderNode(collider);
    p_GenColPlanes(collider);
}

void p_RotateColliderY(struct p_collider_t *collider, float angle)
{
    mat3_t_rotate_y(&collider->orientation, angle);
    p_UpdateColliderNode(collider);
    p_GenColPlanes(collider);
}

void p_RotateColliderZ(struct p_collider_t *collider, float angle)
{
    mat3_t_rotate_z(&collider->orientation, angle);
    p_UpdateColliderNode(collider);
    p_GenColPlanes(collider);
}

uint32_t p_BoxIntersect(vec3_t *box_a0, vec3_t *box_a1, vec3_t *box_b0, vec3_t *box_b1)
{
    return box_a0->x < box_b1->x && box_a1->x > box_b0->x &&
           box_a0->y < box_b1->y && box_a1->y > box_b0->y &&
           box_a0->z < box_b1->z && box_a1->z > box_b0->z;
}

void p_UpdateColliders()
{
    p_collisions.cursor = 0;
    p_frame++;

//    r_i_SetTransform(NULL);
//    r_i_SetPrimitiveType(GL_LINES);
    vec3_t corners[8];

//    static uint32_t skip = 0;
//    static struct p_trace_t skip_trace = {};

    for(uint32_t collider_index = 0; collider_index < p_colliders[P_COLLIDER_TYPE_STATIC].cursor; collider_index++)
    {
        struct p_collider_t *collider = p_GetCollider(P_COLLIDER_TYPE_STATIC, collider_index);
        if(collider)
        {
            struct dbvh_node_t *node = get_dbvh_node_pointer(&p_main_dbvh, collider->node_index);

            corners[0] = vec3_t_c(node->min.x, node->max.y, node->min.z);
            corners[1] = vec3_t_c(node->min.x, node->min.y, node->min.z);
            corners[2] = vec3_t_c(node->max.x, node->min.y, node->min.z);
            corners[3] = vec3_t_c(node->max.x, node->max.y, node->min.z);

            corners[4] = vec3_t_c(node->min.x, node->max.y, node->max.z);
            corners[5] = vec3_t_c(node->min.x, node->min.y, node->max.z);
            corners[6] = vec3_t_c(node->max.x, node->min.y, node->max.z);
            corners[7] = vec3_t_c(node->max.x, node->max.y, node->max.z);

//            r_i_DrawLine(&corners[0], &corners[1], &vec3_t_c(0.0, 1.0, 0.0), 1.0);
//            r_i_DrawLine(&corners[1], &corners[2], &vec3_t_c(0.0, 1.0, 0.0), 1.0);
//            r_i_DrawLine(&corners[2], &corners[3], &vec3_t_c(0.0, 1.0, 0.0), 1.0);
//            r_i_DrawLine(&corners[3], &corners[0], &vec3_t_c(0.0, 1.0, 0.0), 1.0);
//
//            r_i_DrawLine(&corners[4], &corners[5], &vec3_t_c(0.0, 1.0, 0.0), 1.0);
//            r_i_DrawLine(&corners[5], &corners[6], &vec3_t_c(0.0, 1.0, 0.0), 1.0);
//            r_i_DrawLine(&corners[6], &corners[7], &vec3_t_c(0.0, 1.0, 0.0), 1.0);
//            r_i_DrawLine(&corners[7], &corners[4], &vec3_t_c(0.0, 1.0, 0.0), 1.0);
//
//            r_i_DrawLine(&corners[0], &corners[4], &vec3_t_c(0.0, 1.0, 0.0), 1.0);
//            r_i_DrawLine(&corners[1], &corners[5], &vec3_t_c(0.0, 1.0, 0.0), 1.0);
//            r_i_DrawLine(&corners[2], &corners[6], &vec3_t_c(0.0, 1.0, 0.0), 1.0);
//            r_i_DrawLine(&corners[3], &corners[7], &vec3_t_c(0.0, 1.0, 0.0), 1.0);
        }
    }
//
//    if(skip)
//    {
//        return;
//    }

    for(uint32_t col_a_index = 0; col_a_index < p_colliders[P_COLLIDER_TYPE_MOVABLE].cursor; col_a_index++)
    {
        struct p_movable_collider_t *collider_a = (struct p_movable_collider_t *)p_GetCollider(P_COLLIDER_TYPE_MOVABLE, col_a_index);

        if(!collider_a)
        {
            continue;
        }

//        collider_a->disp.y -= 0.01;
        collider_a->flags &= ~(P_COLLIDER_FLAG_ON_GROUND | P_COLLIDER_FLAG_TOP_COLLIDED);
        collider_a->first_collision = p_collisions.cursor;

        p_GenColPlanes((struct p_collider_t *)collider_a);

        for(uint32_t bump_index = 0; bump_index < 8; bump_index++)
        {
            struct p_trace_t closest_trace = {};
            closest_trace.time = 1.0;
            vec3_t box_a[2];
            p_ComputeMoveBox((struct p_collider_t *)collider_a, &box_a[0], &box_a[1]);

            struct list_t *contents = box_on_dbvh_contents(&p_main_dbvh, &box_a[1], &box_a[0]);

            for(uint32_t collider_index = 0; collider_index < contents->cursor; collider_index++)
            {
                struct p_collider_t *collider_b = *(struct p_collider_t **)get_list_element(contents, collider_index);
                if((struct p_collider_t *)collider_b != (struct p_collider_t *)collider_a)
                {
                    struct p_trace_t cur_trace = closest_trace;
                    p_ComputeCollision((struct p_collider_t *)collider_a, collider_b, &cur_trace);
                    if(!collider_b->index)
                    {
//                        r_i_SetTransform(NULL);

                        uint32_t plane_count;
                        struct p_col_plane_t *planes;
                        p_GenPairColPlanes((struct p_collider_t *)collider_a, (struct p_collider_t *)collider_b, &planes, &plane_count);

//                        r_i_SetPrimitiveType(GL_LINES);
                        for(uint32_t plane_index = 0; plane_index < plane_count; plane_index++)
                        {
                            vec3_t end;
                            struct p_col_plane_t *plane = planes + plane_index;
                            vec3_t_add(&end, &plane->point, &plane->normal);
//                            r_i_DrawLine(&plane->point, &end, &vec3_t_c(0.0, 1.0, 0.0), 1.0);

                            corners[0] = plane->point;
                            vec3_t_sub(&corners[0], &corners[0], &plane->t0);
                            vec3_t_add(&corners[0], &corners[0], &plane->t1);

                            corners[1] = plane->point;
                            vec3_t_sub(&corners[1], &corners[1], &plane->t0);
                            vec3_t_sub(&corners[1], &corners[1], &plane->t1);

                            corners[2] = plane->point;
                            vec3_t_add(&corners[2], &corners[2], &plane->t0);
                            vec3_t_sub(&corners[2], &corners[2], &plane->t1);

                            corners[3] = plane->point;
                            vec3_t_add(&corners[3], &corners[3], &plane->t0);
                            vec3_t_add(&corners[3], &corners[3], &plane->t1);

//                            r_i_DrawLine(&corners[0], &corners[1], &vec3_t_c(1.0, 1.0, 0.0), 1.0);
//                            r_i_DrawLine(&corners[1], &corners[2], &vec3_t_c(1.0, 1.0, 0.0), 1.0);
//                            r_i_DrawLine(&corners[2], &corners[3], &vec3_t_c(1.0, 1.0, 0.0), 1.0);
//                            r_i_DrawLine(&corners[3], &corners[0], &vec3_t_c(1.0, 1.0, 0.0), 1.0);
                        }



//                        r_i_SetPrimitiveType(GL_POINTS);
//                        for(uint32_t plane_index = 0; plane_index < plane_count; plane_index++)
//                        {
//                            struct p_col_plane_t *plane = planes + plane_index;
//                            r_i_DrawPoint(&plane->point, &vec3_t_c(1.0, 0.0, 0.0), 8.0);
//                        }
//
//
//                        r_i_SetPrimitiveType(GL_LINES);
//
//                        r_i_DrawPoint(&collider_a->position, &vec3_t_c(0.0, 0.0, 1.0), 8.0);
//
//                        struct dbvh_node_t *node = get_dbvh_node_pointer(&p_main_dbvh, collider_a->node_index);
//
//                        corners[0] = vec3_t_c(node->min.x, node->max.y, node->min.z);
//                        corners[1] = vec3_t_c(node->min.x, node->min.y, node->min.z);
//                        corners[2] = vec3_t_c(node->max.x, node->min.y, node->min.z);
//                        corners[3] = vec3_t_c(node->max.x, node->max.y, node->min.z);
//
//                        corners[4] = vec3_t_c(node->min.x, node->max.y, node->max.z);
//                        corners[5] = vec3_t_c(node->min.x, node->min.y, node->max.z);
//                        corners[6] = vec3_t_c(node->max.x, node->min.y, node->max.z);
//                        corners[7] = vec3_t_c(node->max.x, node->max.y, node->max.z);
//
//                        r_i_DrawLine(&corners[0], &corners[1], &vec3_t_c(1.0, 1.0, 0.0), 1.0);
//                        r_i_DrawLine(&corners[1], &corners[2], &vec3_t_c(1.0, 1.0, 0.0), 1.0);
//                        r_i_DrawLine(&corners[2], &corners[3], &vec3_t_c(1.0, 1.0, 0.0), 1.0);
//                        r_i_DrawLine(&corners[3], &corners[0], &vec3_t_c(1.0, 1.0, 0.0), 1.0);
//
//                        r_i_DrawLine(&corners[4], &corners[5], &vec3_t_c(1.0, 1.0, 0.0), 1.0);
//                        r_i_DrawLine(&corners[5], &corners[6], &vec3_t_c(1.0, 1.0, 0.0), 1.0);
//                        r_i_DrawLine(&corners[6], &corners[7], &vec3_t_c(1.0, 1.0, 0.0), 1.0);
//                        r_i_DrawLine(&corners[7], &corners[4], &vec3_t_c(1.0, 1.0, 0.0), 1.0);
//
//                        r_i_DrawLine(&corners[0], &corners[4], &vec3_t_c(1.0, 1.0, 0.0), 1.0);
//                        r_i_DrawLine(&corners[1], &corners[5], &vec3_t_c(1.0, 1.0, 0.0), 1.0);
//                        r_i_DrawLine(&corners[2], &corners[6], &vec3_t_c(1.0, 1.0, 0.0), 1.0);
//                        r_i_DrawLine(&corners[3], &corners[7], &vec3_t_c(1.0, 1.0, 0.0), 1.0);
                    }
//                    else
                    {
                        closest_trace = cur_trace;
                    }
                }
            }

            if(closest_trace.time < 1.0)
            {
                vec3_t_fmadd(&collider_a->position, &collider_a->position, &collider_a->disp, closest_trace.time);
                float proj = vec3_t_dot(&collider_a->disp, &closest_trace.normal);
                vec3_t_fmadd(&collider_a->disp, &collider_a->disp, &closest_trace.normal, -proj);
                proj = vec3_t_dot(&closest_trace.normal, &vec3_t_c(0.0, 1.0, 0.0));
                if(proj > 0.8)
                {
                    collider_a->flags |= P_COLLIDER_FLAG_ON_GROUND;
                }
                else if(proj < -0.6)
                {
                    collider_a->flags |= P_COLLIDER_FLAG_TOP_COLLIDED;
                }

                add_list_element(&p_collisions, &closest_trace.collider);
            }
            else
            {
                vec3_t_add(&collider_a->position, &collider_a->position, &collider_a->disp);

                struct dbvh_node_t *node = get_dbvh_node_pointer(&p_main_dbvh, collider_a->node_index);
                p_ComputeMoveBox((struct p_collider_t *)collider_a, &node->min, &node->max);
                uint32_t node_index = nodes_smallest_volume(&p_main_dbvh, collider_a->node_index);
                pair_dbvh_nodes(&p_main_dbvh, collider_a->node_index, node_index);
                break;
            }

            p_GenColPlanes((struct p_collider_t *)collider_a);
        }

        collider_a->collision_count = p_collisions.cursor - collider_a->first_collision;
    }

    for(uint32_t trigger_index = 0; trigger_index < p_colliders[P_COLLIDER_TYPE_TRIGGER].cursor; trigger_index++)
    {
        struct p_trigger_collider_t *trigger_collider = (struct p_trigger_collider_t *)p_GetCollider(P_COLLIDER_TYPE_TRIGGER, trigger_index);

        if(trigger_collider)
        {
            trigger_collider->first_collision = p_collisions.cursor;

            struct dbvh_node_t *node = get_dbvh_node_pointer(&p_trigger_dbvh, trigger_collider->node_index);
            struct list_t *contents = box_on_dbvh_contents(&p_main_dbvh, &node->max, &node->min);

            for(uint32_t collider_index = 0; collider_index < contents->cursor; collider_index++)
            {
                struct p_collider_t *collider = *(struct p_collider_t **)get_list_element(contents, collider_index);
                struct p_trace_t trace = {};
                if(p_ComputeCollision((struct p_collider_t *)trigger_collider, collider, &trace))
                {
                    add_list_element(&p_collisions, &collider);
                }
            }

            trigger_collider->collision_count = p_collisions.cursor - trigger_collider->first_collision;
        }
    }
}

void p_GenColPlanes(struct p_collider_t *collider)
{
    vec3_t size;
    vec3_t_mul(&size, &collider->size, 0.5);
    struct p_col_plane_t *planes = get_stack_list_element(&p_col_planes, collider->planes_index);

    for(uint32_t plane_index = 0; plane_index < 6; plane_index++)
    {
        struct p_col_plane_t *plane = planes + plane_index;
        uint32_t index = plane_index >> 1;
        uint32_t t0_index = (index + 1) % 3;
        uint32_t t1_index = (index + 2) % 3;
        plane->normal = collider->orientation.rows[index];

        if(plane_index & 1)
        {
            vec3_t_mul(&plane->normal, &plane->normal, -1.0);
        }

        vec3_t_mul(&plane->point, &plane->normal, size.comps[index]);
        vec3_t_add(&plane->point, &plane->point, &collider->position);
        vec3_t_mul(&plane->t0, &collider->orientation.rows[t0_index], size.comps[t0_index]);
        vec3_t_mul(&plane->t1, &collider->orientation.rows[t1_index], size.comps[t1_index]);
    }
}

void p_GenPairColPlanes(struct p_collider_t *collider_a, struct p_collider_t *collider_b, struct p_col_plane_t **planes, uint32_t *plane_count)
{
    float extents[3];
//    mat3_t extents;
    uint32_t plane_index = 0;
    struct p_col_plane_t *pair_planes = p_pair_col_planes;
    struct p_col_plane_t *a_planes = get_stack_list_element(&p_col_planes, collider_a->planes_index);
    struct p_col_plane_t *b_planes = get_stack_list_element(&p_col_planes, collider_b->planes_index);

    vec3_t size_a;
    vec3_t size_b;

    vec3_t_mul(&size_a, &collider_a->size, 0.5);
    vec3_t_mul(&size_b, &collider_b->size, 0.5);

    *planes = pair_planes;


//    orientation_a = &collider_a->orientation;

//    for(uint32_t axis_index = 0; axis_index < 3; axis_index++)
//    {
//        vec3_t *axis = collider_b->orientation.rows + axis_index;
//
//        extents.rows[axis_index].x = fabs(vec3_t_dot(&collider_a->orientation.rows[0], axis));
//        extents.rows[axis_index].y = fabs(vec3_t_dot(&collider_a->orientation.rows[1], axis));
//        extents.rows[axis_index].z = fabs(vec3_t_dot(&collider_a->orientation.rows[2], axis));
//    }

    for(uint32_t axis_index = 0; axis_index < 3; axis_index++)
    {
        vec3_t *axis = collider_b->orientation.rows + axis_index;

        extents[axis_index] = size_a.x * fabs(vec3_t_dot(&collider_a->orientation.rows[0], axis)) +
                              size_a.y * fabs(vec3_t_dot(&collider_a->orientation.rows[1], axis)) +
                              size_a.z * fabs(vec3_t_dot(&collider_a->orientation.rows[2], axis));
    }


//    for(plane_index = 0; plane_index < 6; plane_index++)
//    {
//        struct p_col_plane_t *plane = pair_planes + plane_index;
//        uint32_t index = plane_index >> 1;
//        plane->normal = collider_b->orientation.rows[index];
//
//        if(plane_index & 1)
//        {
//            vec3_t_mul(&plane->normal, &plane->normal, -1.0);
//        }
//
//        vec3_t_mul(&plane->point, &plane->normal, extents[index] + size_b.comps[index]);
//        vec3_t_add(&plane->point, &plane->point, &collider_b->position);
//    }
//
//    *plane_count = plane_index;
//    pair_planes += plane_index;


    for(uint32_t index = 0; index < 3; index++)
    {
        struct p_col_plane_t *pair_plane;
        struct p_col_plane_t *plane;

        pair_plane = pair_planes + plane_index;
        plane = b_planes + plane_index;
        *pair_plane = *plane;
        vec3_t_fmadd(&pair_plane->point, &pair_plane->point, &pair_plane->normal, extents[index]);
        plane_index++;

        pair_plane = pair_planes + plane_index;
        plane = b_planes + plane_index;
        *pair_plane = *plane;
        vec3_t_fmadd(&pair_plane->point, &pair_plane->point, &pair_plane->normal, extents[index]);
        plane_index++;
    }

    *plane_count = plane_index;
    pair_planes += plane_index;
    plane_index = 0;





    for(uint32_t axis_index = 0; axis_index < 3; axis_index++)
    {
        vec3_t *axis = collider_a->orientation.rows + axis_index;

        extents[axis_index] = size_b.x * fabs(vec3_t_dot(&collider_b->orientation.rows[0], axis)) +
                              size_b.y * fabs(vec3_t_dot(&collider_b->orientation.rows[1], axis)) +
                              size_b.z * fabs(vec3_t_dot(&collider_b->orientation.rows[2], axis));
    }

//    for(plane_index = 0; plane_index < 6; plane_index++)
//    {
//        struct p_col_plane_t *plane = pair_planes + plane_index;
//        uint32_t index = plane_index >> 1;
//        plane->normal = collider_a->orientation.rows[index];
//
//        if(plane_index & 1)
//        {
//            vec3_t_mul(&plane->normal, &plane->normal, -1.0);
//        }
//
//        vec3_t_mul(&plane->point, &plane->normal, extents[index] + size_a.comps[index]);
//        vec3_t_add(&plane->point, &plane->point, &collider_b->position);
//    }
//
//    (*plane_count) += plane_index;


    for(uint32_t index = 0; index < 3; index++)
    {
        struct p_col_plane_t *pair_plane;
        struct p_col_plane_t *plane;
        float extent = extents[index] + size_a.comps[index];

        pair_plane = pair_planes + plane_index;
        plane = a_planes + plane_index;
        pair_plane->normal = plane->normal;
        pair_plane->t0 = plane->t0;
        pair_plane->t1 = plane->t1;
        vec3_t_mul(&pair_plane->point, &pair_plane->normal, extent);
        vec3_t_add(&pair_plane->point, &pair_plane->point, &collider_b->position);
        plane_index++;

        pair_plane = pair_planes + plane_index;
        plane = a_planes + plane_index;
        pair_plane->normal = plane->normal;
        pair_plane->t0 = plane->t0;
        pair_plane->t1 = plane->t1;
        vec3_t_mul(&pair_plane->point, &pair_plane->normal, extent);
        vec3_t_add(&pair_plane->point, &pair_plane->point, &collider_b->position);
        plane_index++;
    }

    (*plane_count) += plane_index;
}

void p_UpdateColliderNode(struct p_collider_t *collider)
{
    vec3_t corners[8];
    vec3_t size = collider->size;
    vec3_t up;
    vec3_t max = vec3_t_c(-FLT_MAX, -FLT_MAX, -FLT_MAX);
    vec3_t min = vec3_t_c(FLT_MAX, FLT_MAX, FLT_MAX);
    vec3_t_mul(&size, &size, 0.5);

    corners[0] = vec3_t_c(-size.x, -size.y, -size.z);
    corners[1] = vec3_t_c( size.x, -size.y, -size.z);
    corners[2] = vec3_t_c( size.x, -size.y,  size.z);
    corners[3] = vec3_t_c(-size.x, -size.y,  size.z);

    corners[4] = vec3_t_c(-size.x,  size.y, -size.z);
    corners[5] = vec3_t_c( size.x,  size.y, -size.z);
    corners[6] = vec3_t_c( size.x,  size.y,  size.z);
    corners[7] = vec3_t_c(-size.x,  size.y,  size.z);

    for(uint32_t corner_index = 0; corner_index < 8; corner_index++)
    {
        vec3_t *corner = corners + corner_index;
        mat3_t_vec3_t_mul(corner, corner, &collider->orientation);
        vec3_t_max(&max, &max, corner);
        vec3_t_min(&min, &min, corner);
    }

//    vec3_t_mul(&up, &collider->orientation.rows[1], collider->size.y);
//
//    for(uint32_t corner_index = 4; corner_index < 8; corner_index++)
//    {
//        vec3_t *corner = corners + corner_index;
//        vec3_t_add(corner, corner - 4, &up);
//        vec3_t_max(&max, &max, corner);
//        vec3_t_min(&min, &min, corner);
//    }

    struct dbvh_node_t *node = get_dbvh_node_pointer(&p_main_dbvh, collider->node_index);
    vec3_t_add(&node->min, &min, &collider->position);
    vec3_t_add(&node->max, &max, &collider->position);

    uint32_t sibling_index = nodes_smallest_volume(&p_main_dbvh, collider->node_index);
    pair_dbvh_nodes(&p_main_dbvh, collider->node_index, sibling_index);
}

void p_ComputeMoveBox(struct p_collider_t *collider, vec3_t *min, vec3_t *max)
{
    vec3_t_fmadd(min, &collider->position, &collider->size, -0.5);
    vec3_t_fmadd(max, &collider->position, &collider->size, 0.5);

    if(collider->type == P_COLLIDER_TYPE_MOVABLE)
    {
        struct p_movable_collider_t *movable_collider = (struct p_movable_collider_t *)collider;

        if(movable_collider->disp.x > 0.0) max->x += movable_collider->disp.x;
        else min->x += movable_collider->disp.x;

        if(movable_collider->disp.y > 0.0) max->y += movable_collider->disp.y;
        else min->y += movable_collider->disp.y;

        if(movable_collider->disp.z > 0.0) max->z += movable_collider->disp.z;
        else min->z += movable_collider->disp.z;
    }
}

uint32_t p_ComputeCollision(struct p_collider_t *collider_a, struct p_collider_t *collider_b, struct p_trace_t *trace)
{
    struct p_col_plane_t *planes;
    uint32_t plane_count;
    struct p_trace_t temp_trace = {};
    temp_trace.collider = collider_b;
    p_GenPairColPlanes(collider_a, collider_b, &planes, &plane_count);

    if(collider_a->type == P_COLLIDER_TYPE_MOVABLE)
    {
        struct p_movable_collider_t *movable_collider = (struct p_movable_collider_t *)collider_a;
        p_TracePlanes(&movable_collider->position, &movable_collider->disp, planes, plane_count, &temp_trace);
    }
    else
    {
        if(p_SolidPoint(&collider_a->position, planes, plane_count))
        {
            temp_trace.collider = collider_b;
            temp_trace.solid_start = 1;
        }
    }

    if(temp_trace.time < trace->time || temp_trace.solid_start)
    {
        *trace = temp_trace;
        return 1;
    }

    return 0;
}

uint32_t p_SolidPoint(vec3_t *point, struct p_col_plane_t *planes, uint32_t plane_count)
{
    for(uint32_t plane_index = 0; plane_index < plane_count; plane_index++)
    {
        struct p_col_plane_t *plane = planes + plane_index;
        vec3_t vec;
        vec3_t_sub(&vec, point, &plane->point);

        if(vec3_t_dot(&vec, &plane->normal) >= 0.0)
        {
            return 0;
        }
    }

    return 1;
}

/*
                 _______________
                |               |
                |               |
                |               |
                |               |
                |               |
                |               |
                |               |
                |               |
                |               |
                |               |
                |               |
                 ---------------


    colliders are boxes comprised of planes. In a way, they're simplier bsp
    trees, where each node has only the back node



        ________________A__________________
                |               |
                |               |
                |               |
                |               |
                |               B
                D               |
                |               |
                |               |
                |               |
                |               |
                |               |
        ----------------C-------|
                                |
                                |
                                |
                                |
                                |
                                |

    in this box, there are four planes (there will be at least 6 in 3d), and the structure of this bsp tree is

        A
       / \
      e   \
           B
          / \
         e   \
              C
             / \
            e   \
                 D
                / \
               e   s

    where nodes at the right side are the back nodes


        ________________A__________________
                |               |
                |               |
           X----|---------------|----->
                |               |
                |               B
                D               |
                |               |
                |               |
                |               |
                |               |
                |               |
        ----------------C-------|
                                |
                                |
                                |
                                |
                                |
                                |

    we have a ray, and want to  find where it intersects this bsp. In this implementation
    (which is somewhat similar to Quake's implementation), the ray has a start and end points,
    and two intersection time values, t0 and t1. Those intersection times are relative to the
    original ray, where t0 represents the start of the ray, and t1 represents the end. In the
    original ray, t0 == 0.0 and t1 == 1.0. If the ray happens to be split, say, exactly in half,
    the first split will be characterised by t0 == 0.0 and t1 == 0.5, and the second split will
    have t0 == 0.5 and t1 == 1.0. if it's not split in half, the value will be different.

    Finding the intersection is a matter of testing the ray against the plane of each node,
    splitting it if it straddles the plane, and calling the function recursively, ALWAYS
    processing the split that's closer to the ray origin first.


        ________________A__________________
                |               |
                |               |
           X----|---------------|----->
                |               |
                |               B
                D               |
                |               |
                |               |
                |               |
                |               |
                |               |
        ----------------C-------|
                                |
                                |
                                |
                                |
                                |
                                |

    In this example, we'll start at the root node, A. The ray is completely behind it, so
    we pass it down as is. That means start and end points are the same, and t0 and t1 are
    0.0 and 1.0, respectivelly.


        ________________A__________________
                |               |
                |               |
           X----|---------------0----->
                |               |
                |               B
                D               |
                |               |
                |               |
                |               |
                |               |
                |               |
        ----------------C-------|
                                |
                                |
                                |
                                |
                                |
                                |

    We now test A's back node, B. This time the ray straddels the plane, and we find time_b,
    which is the intersection time.


        ________________A__________________
                |               |
                |               |
           X----|---------------0----->
                |               |
                |               B
                D               |
                |               |
                |               |
                |               |
                |               |
                |               |
        ----------------C-------|
                                |
                                |
                                |
                                |
                                |
                                |

    Now we test both splits against B's back node, C, starting with the one that's closer to
    the ray origin. To the recursive call, we pass t0 == 0.0 and t1 == time_b.

        ________________A__________________
                |               |
                |               |
           X----0---------------0----->
                |               |
                |               B
                D               |
                |               |
                |               |
                |               |
                |               |
                |               |
        ----------------C-------|
                                |
                                |
                                |
                                |
                                |
                                |

    This split is completely behind C, so we make a recursive call, and pass the ray as is
    to it's back child, D. The ray straddels D, and so we have two new splits that have to
    be processed. Those splits are t0 == 0.0, t1 == time_d and t0 == time_d and t1 == time_b

    We start with the first split, and realize it's in front of D. Since those boxes are
    convex, and the bsp has no front nodes, there's nothing else to do here, since there's
    no planes this split can further straddle.

    The next step is the crucial one.

    Before processing the second split of this recursive call, we test to see if the point
    at the intersection would be in solid space (inside the box) if we were to recurse down.
    In this case, it would, because it'd end up behind every plane. So, this is a point where
    there's a transition between empty and solid space (or outside to inside), and since we
    processed first the split closer to the ray origin, this is the closest transition to the
    origin of the ray. Note, this is closest intersection *ever*, not necessarily the first one
    to go from empty to solid space. If the ray stars inside the box. this intersection will
    represent the transition from solid to empty space.

    If the point at the intersection wasn't in solid space, we'd take the back node of the current
    node, and repeat the process. Since that's not the case, the current recursive call saves the
    intersection time, time_d, and returns 0 to signal the intersection was found.




        ________________A__________________
                |               |
                |               |
                |               |
                |               |
   X            |               B
    \           D               |
     \          |               |
      \         |               |
       \        |               |
        \       |               |
         \      |               |
    ------\-------------C-------|
           \                    |
            \                   |
             \                  |
              \                 |
               \                |
                \               |

    Another case, but this time without intersection.


        ________________A__________________
                |               |
                |               |
                |               |
                |               |
   X            |               B
    \           D               |
     \          |               |
      \         |               |
       \        |               |
        \       |               |
         \      |               |
    ------\-------------C-------|
           \                    |
            \                   |
             \                  |
              \                 |
               \                |
                \               |

    Again, we start at A, and see the ray is completely behind the plane, so we recurse
    down, passing the whole ray.


        ________________A__________________
                |               |
                |               |
                |               |
                |               |
   X            |               B
    \           D               |
     \          |               |
      \         |               |
       \        |               |
        \       |               |
         \      |               |
    ------\-------------C-------|
           \                    |
            \                   |
             \                  |
              \                 |
               \                |
                \               |

    At B, we find that the ray, once again, is completely behind the plane, so we recurse
    down, passing the whole ray.


        ________________A__________________
                |               |
                |               |
                |               |
                |               |
   X            |               B
    \           D               |
     \          |               |
      \         |               |
       \        |               |
        \       |               |
         \      |               |
    ------0-------------C-------|
           \                    |
            \                   |
             \                  |
              \                 |
               \                |
                \               |

    At C, the ray straddles the plane, so we have two splits, and the intersection time is time_c.
    We start with the first split. Since it's behind C we recurse down, passing t0 == 0.0 and t1 == time_c.

        ________________A__________________
                |               |
                |               |
                |               |
                |               |
   X            |               B
    \           D               |
     \          |               |
      \         |               |
       \        |               |
        \       |               |
         \      |               |
    ------0-------------C-------|
           \                    |
            \                   |
             \                  |
              \                 |
               \                |
                \               |

    At D, we realize the whole ray is in front of it, so no further intersections are possible. We just
    return to the previous call.

        ________________A__________________
                |               |
                |               |
                |               |
                |               |
   X            |               B
    \           D               |
     \          |               |
      \         |               |
       \        |               |
        \       |               |
         \      |               |
    ------0-------------C-------|
           \                    |
            \                   |
             \                  |
              \                 |
               \                |
                \               |

    We get back at C, and then test the intersection point. We realize that the intersection point is in front of
    D, and D is the last plane, so the point is not in solid space, and we're not intersecting the box. In this case,
    we return 1, which actually serves to continue the recursion, but we also test if there's any plane left. If there
    isn't, we just return to the caller.
*/

#define P_DELTA_NUDGE 0.01
uint32_t p_TracePlanesRecursive(vec3_t *start, vec3_t *end, float start_t, float end_t, struct p_trace_t *trace, struct p_col_plane_t *planes, int32_t plane_index, uint32_t plane_count)
{
    vec3_t trace_vec;
    vec3_t vec;
    vec3_t_sub(&trace_vec, end, start);

    struct p_col_plane_t *plane = planes + plane_index;
    plane_index++;

    vec3_t_sub(&vec, start, &plane->point);
    float dist_a = vec3_t_dot(&vec, &plane->normal);

    vec3_t_sub(&vec, end, &plane->point);
    float dist_b = vec3_t_dot(&vec, &plane->normal);

    if(dist_a * dist_b <= 0.0)
    {
        /* the trace is straddling this plane, so split it, recursively test the
        first split  */
        vec3_t middle;
        float middle_t;
        float frac;

        if(dist_a < 0.0)
        {
            frac = (dist_a + P_DELTA_NUDGE) / (dist_a - dist_b);
        }
        else
        {
            frac = (dist_a - P_DELTA_NUDGE) / (dist_a - dist_b);
        }

        if(frac > 1.0) frac = 1.0;
        else if(frac < 0.0) frac = 0.0;

        middle_t = start_t + (end_t - start_t) * frac;
        vec3_t_fmadd(&middle, start, &trace_vec, frac);

        if(dist_a < 0.0)
        {
            /* only test the first split if it's behind this plane. If it's in front, it'll never end inside
            the box */
            if(!p_TracePlanesRecursive(start, &middle, start_t, middle_t, trace, planes, plane_index, plane_count))
            {
                /* if we get here, it means a recursive call has found the intersection, and so there's nothing
                else to do */
                return 0;
            }
        }

        if(!p_SolidPoint(&middle, planes + plane_index, plane_count - plane_index))
        {
            /* the middle point (start of second split) is not in solid space, which
            means the second split can possibly straddle other planes */
            return p_TracePlanesRecursive(&middle, end, middle_t, end_t, trace, planes, plane_index, plane_count);
        }

        trace->time = middle_t;
        trace->normal = plane->normal;
        vec3_t_fmadd(&trace->point, &trace->start, &trace->dir, trace->time);
        /* tell previous recursive calls that we found the point, and should stop doing work */
        return 0;
    }
    else if(dist_a < 0.0)
    {
        if(plane_index < plane_count)
        {
            /* the whole trace is behind the current plane, so just pass it along. No need to test
            for intersections in front of the plane, either. */
            return p_TracePlanesRecursive(start, end, start_t, end_t, trace, planes, plane_index, plane_count);
        }

        trace->solid_start = 1;

    }

    /* we're still going... */
    return 1;
}

void p_TracePlanes(vec3_t *start, vec3_t *dir, struct p_col_plane_t *planes, uint32_t plane_count, struct p_trace_t *trace)
{
    trace->time = 1.0;
    trace->dir = *dir;
    trace->start = *start;
    trace->solid_start = 0;

    vec3_t end;
    vec3_t_add(&end, start, dir);
    p_TracePlanesRecursive(start, &end, 0.0, 1.0, trace, planes, 0, plane_count);
}

uint32_t p_RaycastRecursive(uint32_t node_index, vec3_t *start, vec3_t *end, struct p_col_plane_t *planes, struct p_trace_t *trace)
{
    struct dbvh_node_t *node = get_dbvh_node_pointer(&p_main_dbvh, node_index);
    vec3_t extents[] = {node->max, node->min};

    for(uint32_t plane_index = 0; plane_index < 6; plane_index++)
    {
        planes[plane_index].point = extents[plane_index & 1];
    }

    struct p_trace_t temp_trace = *trace;
//    float prev_t = trace->time;
    uint32_t misses = p_TracePlanesRecursive(start, end, 0.0, 1.0, &temp_trace, planes, 0, 6);

    if(!misses || temp_trace.solid_start)
    {
        /* if we intersect the box, or if the ray is completely contained in it */
        if(node->children[0] != node->children[1])
        {
            /* not a leaf node */
            return p_RaycastRecursive(node->children[0], start, end, planes, trace) |
                   p_RaycastRecursive(node->children[1], start, end, planes, trace);
        }
        else if(temp_trace.time < trace->time)
        {
            *trace = temp_trace;
            trace->collider = node->contents;
            return 1;
        }
    }

    return 0;
}

uint32_t p_Raycast(vec3_t *start, vec3_t *end, struct p_trace_t *trace)
{
    struct p_trace_t temp_trace = {};
    temp_trace.start = *start;
    vec3_t_sub(&temp_trace.dir, end, start);
    temp_trace.time = 1.0;

    for(uint32_t plane_index = 0; plane_index < 6; plane_index++)
    {
        p_pair_col_planes[plane_index].normal = p_col_normals[plane_index];
    }

    if(p_RaycastRecursive(p_main_dbvh.root, start, end, p_pair_col_planes, &temp_trace))
    {
        *trace = temp_trace;
        return 1;
    }

    return 0;
}




