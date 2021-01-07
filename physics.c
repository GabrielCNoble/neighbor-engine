#include "physics.h"
#include "dstuff/ds_stack_list.h"
#include "dstuff/ds_list.h"
#include "dstuff/ds_mem.h"
#include "dstuff/ds_dbvh.h"


struct stack_list_t p_colliders[P_COLLIDER_TYPE_LAST];
struct list_t p_collisions;
struct list_t p_collision_pairs;
struct p_col_plane_t *p_col_planes;
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
    p_col_planes = mem_Calloc(32, sizeof(struct p_col_plane_t));
    p_main_dbvh = create_dbvh_tree(0);
    p_trigger_dbvh = create_dbvh_tree(0);
}

void p_Shutdown()
{
    
}

struct p_collider_t *p_CreateCollider(uint32_t type, vec3_t *position, vec3_t *size)
{
    uint32_t collider_index;
    struct p_collider_t *collider;
    
    collider_index = add_stack_list_element(&p_colliders[type], NULL);
    collider = get_stack_list_element(&p_colliders[type], collider_index);
    
    collider->index = collider_index;
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
        
        for(uint32_t bump_index = 0; bump_index < 8; bump_index++)
        {
            /* static colliders */
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
                    p_ComputeCollision((struct p_collider_t *)collider_a, collider_b, &closest_trace);
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
        }
        
        collider_a->collision_count = p_collisions.cursor - collider_a->first_collision;
        
//        for(uint32_t trigger_index = 0; trigger_index < p_colliders[P_COLLIDER_TYPE_TRIGGER].cursor; trigger_index++)
//        {
//            struct p_trigger_collider_t *trigger_collider = get_stack_list_element(&p_colliders[P_COLLIDER_TYPE_TRIGGER], trigger_index);
//            if(trigger_collider->index != 0xffffffff)
//            {
//                trigger_collider->collisions.cursor = 0;
//            }
//        }
        
//        /* triggers */
//        struct p_trace_t closest_trace = {};
//        closest_trace.time = 1.0;
//        vec3_t min;
//        vec3_t max;
//        
//        p_ComputeMoveBox((struct p_collider_t *)collider_a, &min, &max);
//        
//        struct list_t *contents = box_on_dbvh_contents(&p_trigger_dbvh, &max, &min);
//        
//        for(uint32_t collider_index = 0; collider_index < contents->cursor; collider_index++)
//        {
//            struct p_trigger_collider_t *trigger = *(struct p_trigger_collider_t **)get_list_element(contents, collider_index);
//            
//            if(p_ComputeCollision((struct p_collider_t *)collider_a, (struct p_collider_t *)trigger, &closest_trace))
//            {
//                add_list_element(&trigger->collisions, &collider_a);
//            }
//        }
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

void p_ComputeCollisionPlanes(struct p_collider_t *collider_a, struct p_collider_t *collider_b, struct p_col_plane_t **planes, uint32_t *plane_count)
{
    uint32_t plane_index;
    for(plane_index = 0; plane_index < 6; plane_index++)
    {
        struct p_col_plane_t *plane = p_col_planes + plane_index;
        plane->normal = p_col_normals[plane_index];
        float size;
        
        switch(plane_index)
        {
            case 0: case 1:
                size = collider_a->size.x + collider_b->size.x;
            break;
            
            case 2: case 3:
                size = collider_a->size.y + collider_b->size.y;
            break;
            
            case 4: case 5:
                size = (collider_a->size.z + collider_b->size.z);
            break;
        }
        
        vec3_t dir;
        vec3_t_mul(&dir, &plane->normal, size * 0.5);
        vec3_t_add(&plane->point, &collider_b->position, &dir);
    }
    
    *planes = p_col_planes;
    *plane_count = plane_index;
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
    p_ComputeCollisionPlanes(collider_a, collider_b, &planes, &plane_count);
    
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
uint32_t p_TracePlanesRecursive(vec3_t *start, vec3_t *end, float start_t, float end_t, struct p_trace_t *trace, struct p_col_plane_t *planes, uint32_t plane_count)
{   
    vec3_t trace_vec;
    vec3_t vec;
    vec3_t_sub(&trace_vec, end, start);
    
    struct p_col_plane_t *plane = planes;
    planes++;
    plane_count--;
    
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
            if(!p_TracePlanesRecursive(start, &middle, start_t, middle_t, trace, planes, plane_count))
            {
                /* if we get here, it means a recursive call has found the intersection, and so there's nothing
                else to do */
                return 0;
            }
        }
        
        if(!p_SolidPoint(&middle, planes, plane_count))
        {
            /* the middle point (start of second split) is not in solid space, which
            means the second split can possibly straddle other planes */
            return p_TracePlanesRecursive(&middle, end, middle_t, end_t, trace, planes, plane_count);
        }
        
        trace->time = middle_t;
        trace->normal = plane->normal;
        vec3_t_fmadd(&trace->point, &trace->start, &trace->dir, trace->time);
        /* tell previous recursive calls that we found the point, and should stop doing work */
        return 0;
    }
    else if(dist_a < 0.0)
    {
        if(plane_count > 1)
        {
            /* the whole trace is behind the current plane, so just pass it along. No need to test
            for intersections in front of the plane, either. */
            return p_TracePlanesRecursive(start, end, start_t, end_t, trace, planes, plane_count);
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
    p_TracePlanesRecursive(start, &end, 0.0, 1.0, trace, planes, plane_count);
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
    uint32_t misses = p_TracePlanesRecursive(start, end, 0.0, 1.0, &temp_trace, planes, 6);
    
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
        p_col_planes[plane_index].normal = p_col_normals[plane_index];
    }
    
    if(p_RaycastRecursive(p_main_dbvh.root, start, end, p_col_planes, &temp_trace))
    {
        *trace = temp_trace;
        return 1;
    }
        
    return 0;
}




