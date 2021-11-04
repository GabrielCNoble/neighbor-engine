#ifndef DS_DBVT_H
#define DS_DBVT_H

/*
    This is free and unencumbered software released into the public domain.

    Anyone is free to copy, modify, publish, use, compile, sell, or
    distribute this software, either in source code form or as a compiled
    binary, for any purpose, commercial or non-commercial, and by any
    means.

    In jurisdictions that recognize copyright laws, the author or authors
    of this software dedicate any and all copyright interest in the
    software to the public domain. We make this dedication for the benefit
    of the public at large and to the detriment of our heirs and
    successors. We intend this dedication to be an overt act of
    relinquishment in perpetuity of all present and future rights to this
    software under copyright law.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
    OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
    ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
    OTHER DEALINGS IN THE SOFTWARE.

    For more information, please refer to <http://unlicense.org/>
*/

/*
    ds_dbvt - simple dynamic bounding volume tree (DBVT)

    INTEGRATION:

        Integrating this library works similarly to the stb libraries. To create the
        implementations you do the following in ONE C or C++ file:

            #define DS_DBVT_IMPLEMENTATION
            #include "ds_dbvt.h"



    ABOUT:

        This file provides a simple dbvt implementation.

        The implementation allows insertions, removals and updates of the hierarchy without
        needing to rebuild it completely.

        Updates consist in pairing/unpairing nodes. Whenever a node is to be paired, the node it
        forms the smallest volume with will be located, those nodes will be paired (and the hierarchy
        will be accordingly adjusted), and the volumes will be recursively recomputed until reaching
        the root. Unpairing a node involves just adjusting the hierarchy and recomputing the volumes
        until the root.

        Insertion operations are just pairing operations with an additional step, which involves
        allocating a node.

        Removal operations are just unpairing operations.

        The heuristic used to compute the smallest aabb of two nodes is the smallest area heuristic.

        It's also possible to query which nodes fall within some specified aabb.


    BASIC USAGE:

        TODO
*/

#ifndef DS_STDDEF_H_INCLUDED
    #include <stddef.h>
    #define DS_STDDEF_H_INCLUDED
#endif

#ifndef DS_STDINT_H_INCLUDED
    #include <stdint.h>
    #define DS_STDINT_H_INCLUDED
#endif

#include "ds_vector.h"
#include "ds_list.h"
#include "ds_slist.h"

#define DS_DBVT_SLIST_CREATE ds_slist_create
#define DS_DBVT_SLIST_DESTROY ds_slist_destroy
#define DS_DBVT_SLIST_ADD_ELEMENT ds_slist_add_element
#define DS_DBVT_SLIST_GET_ELEMENT ds_slist_get_element
#define DS_DBVT_SLIST_REMOVE_ELEMENT ds_slist_remove_element


struct ds_dbvn_t
{
    uint32_t parent;
    uint32_t children[2];
    void *contents;
    vec3_t max;
    vec3_t min;
    vec3_t expanded_max;
    vec3_t expanded_min;
    void *user_data;
};

struct ds_dbvt_t
{
    uint32_t root;
    struct ds_slist_t node_pool;
};

#define INVALID_DBVH_NODE_INDEX 0xffffffff


struct ds_dbvt_t ds_dbvt_create(uint32_t user_data_size);

void ds_dbvt_build(struct ds_dbvt_t *dbvt, struct ds_dbvn_t *nodes, uint32_t node_count);

void ds_dbvt_destroy(struct ds_dbvt_t *tree);

uint32_t ds_dbvt_alloc_node(struct ds_dbvt_t *tree);

void ds_dbvt_dealloc_node(struct ds_dbvt_t *tree, int node_index);

void ds_dbvt_dealloc_all_nodes(struct ds_dbvt_t *tree);

void ds_dbvt_insert_node(struct ds_dbvt_t* tree, uint32_t node_index);

void ds_dbvt_pair_nodes(struct ds_dbvt_t *tree, uint32_t to_pair_index, uint32_t pair_to_index);

void ds_dbvt_remove_node(struct ds_dbvt_t *tree, uint32_t node_index);

struct ds_dbvn_t *ds_dbvt_get_node_pointer(struct ds_dbvt_t *tree, uint32_t node_index);

struct ds_dbvn_t *ds_dbvt_get_sibling_node_pointer(struct ds_dbvt_t *tree, uint32_t node_index);

uint32_t ds_dbvt_nodes_smallest_volume(struct ds_dbvt_t* tree, uint32_t node_index);

uint32_t ds_dbvt_box_overlap(vec3_t *a_max, vec3_t *a_min, vec3_t *b_max, vec3_t *b_min);

void ds_dbvt_pair_aabb(struct ds_dbvt_t *tree, uint32_t node_a, uint32_t node_b, vec3_t *max, vec3_t *min);

void ds_dbvt_recompute_volumes(struct ds_dbvt_t *tree, int start_node_index);

struct ds_list_t *ds_dbvt_box_contents(struct ds_dbvt_t *tree, vec3_t *aabb_max, vec3_t *aabb_min);


#ifdef DS_DBVT_IMPLEMENTATION

#include <stdio.h>
#include <float.h>

struct ds_dbvt_t ds_dbvt_create(uint32_t user_data_size)
{
    struct ds_dbvt_t tree;

    tree.root = INVALID_DBVH_NODE_INDEX;
    tree.node_pool = DS_DBVT_SLIST_CREATE(sizeof(struct ds_slist_t) + user_data_size, 128);

    return tree;
}

void ds_dbvt_destroy(struct ds_dbvt_t *tree)
{
    if(tree)
    {
        DS_DBVT_SLIST_DESTROY(&tree->node_pool);
    }
}

uint32_t ds_dbvt_alloc_node(struct ds_dbvt_t *tree)
{
    struct ds_dbvn_t *node;
    uint32_t node_index;
    node_index = DS_DBVT_SLIST_ADD_ELEMENT(&tree->node_pool, NULL);
    node = (struct ds_dbvn_t*)DS_DBVT_SLIST_GET_ELEMENT(&tree->node_pool, node_index);

    node->parent = INVALID_DBVH_NODE_INDEX;
    node->children[0] = INVALID_DBVH_NODE_INDEX;
    node->children[1] = INVALID_DBVH_NODE_INDEX;
    node->contents = (void*)0xdeadbeef;
    node->user_data = (char *)node + sizeof(struct ds_dbvn_t);

    return node_index;
}

void ds_dbvt_dealloc_node(struct ds_dbvt_t *tree, int node_index)
{
    struct ds_dbvn_t *node;
    node = ds_dbvt_get_node_pointer(tree, node_index);

    if(node)
    {
        node->parent = node_index;
        node->children[0] = node_index;
        node->children[1] = node_index;
        DS_DBVT_SLIST_REMOVE_ELEMENT(&tree->node_pool, node_index);
    }
}

void ds_dbvt_dealloc_all_nodes(struct ds_dbvt_t *tree)
{
    tree->root = INVALID_DBVH_NODE_INDEX;
    tree->node_pool.cursor = 0;
    tree->node_pool.free_stack_top = 0xffffffff;
}

void ds_dbvt_insert_node(struct ds_dbvt_t* tree, uint32_t node_index)
{
    struct ds_dbvn_t* node;
    uint32_t sibling_index;
    node = ds_dbvt_get_node_pointer(tree, node_index);
    if(node)
    {
        if(tree->root == INVALID_DBVH_NODE_INDEX)
        {
            tree->root = node_index;
            return;
        }

        sibling_index = ds_dbvt_nodes_smallest_volume(tree, node_index);

        if(sibling_index != node_index)
        {
            ds_dbvt_pair_nodes(tree, node_index, sibling_index);
        }
    }
}

void ds_dbvt_pair_nodes(struct ds_dbvt_t *tree, uint32_t to_pair_index, uint32_t pair_to_index)
{
    struct ds_dbvn_t *to_pair = ds_dbvt_get_node_pointer(tree, to_pair_index);
    struct ds_dbvn_t *pair_to = ds_dbvt_get_node_pointer(tree, pair_to_index);

    if(!(to_pair && pair_to))
    {
        return;
    }

    if(to_pair == pair_to)
    {
        /* 'to_pair' and 'pair_to' are the same node */
        return;
    }

    struct ds_dbvn_t *to_pair_parent = ds_dbvt_get_node_pointer(tree, to_pair->parent);

    if(to_pair->parent != pair_to_index && pair_to->parent != to_pair_index)
    {
        /* 'to_pair' and 'pair_to' must not be parent/child of one another */

        if(to_pair->parent != pair_to->parent || to_pair->parent == INVALID_DBVH_NODE_INDEX)
        {
            /* 'to_pair' and 'pair_to' must have different parents, or if they're the same, they must
            be the root node */

            if(!to_pair_parent || to_pair_parent->parent == INVALID_DBVH_NODE_INDEX)
            {
                /* 'to_pair' is either a new node, the root, or a direct child of the root node. So, we need to allocate
                a new node */
                to_pair->parent = ds_dbvt_alloc_node(tree);
                to_pair_parent = ds_dbvt_get_node_pointer(tree, to_pair->parent);
            }
            else
            {
                /* 'to_pair' is already in the hierarchy, and is not a direct child of the root node, just as its sibling
                node */
                to_pair_parent = ds_dbvt_get_node_pointer(tree, to_pair->parent);
                uint32_t sibling_index = to_pair_parent->children[to_pair_parent->children[0] == to_pair_index];

                struct ds_dbvn_t *to_pair_grandpa = ds_dbvt_get_node_pointer(tree, to_pair_parent->parent);
                struct ds_dbvn_t *sibling = ds_dbvt_get_node_pointer(tree, sibling_index);

                to_pair_grandpa->children[to_pair_grandpa->children[0] != to_pair->parent] = sibling_index;
                sibling->parent = to_pair_parent->parent;

                ds_dbvt_recompute_volumes(tree, sibling_index);
            }

            to_pair_parent->children[0] = to_pair_index;
            to_pair_parent->children[1] = pair_to_index;

            if(pair_to->parent == INVALID_DBVH_NODE_INDEX)
            {
                /* 'pair_to' is the root, so 'to_pair_parent' will become the new root */
                tree->root = to_pair->parent;
                to_pair_parent->parent = INVALID_DBVH_NODE_INDEX;
            }
            else
            {
                /* 'pair_to' is not the root, so 'to_pair_parent' will take its place as
                the child of its parent */
                struct ds_dbvn_t  *pair_to_parent = ds_dbvt_get_node_pointer(tree, pair_to->parent);
                pair_to_parent->children[pair_to_parent->children[0] != pair_to_index] = to_pair->parent;
                to_pair_parent->parent = pair_to->parent;
            }

            pair_to->parent = to_pair->parent;
        }
    }

    ds_dbvt_recompute_volumes(tree, to_pair_index);
}

void ds_dbvt_remove_node(struct ds_dbvt_t *tree, uint32_t node_index)
{
    struct ds_dbvn_t *node;
    struct ds_dbvn_t *sibling;
    struct ds_dbvn_t *parent;
    struct ds_dbvn_t *parent_parent;

    int sibling_index;

    node = ds_dbvt_get_node_pointer(tree, node_index);

    if(node)
    {
        if(node->parent == INVALID_DBVH_NODE_INDEX)
        {
            /* node is the current root... */


            /* setting the root to an invalid index is enough here, since
            only leaf nodes are accessible externally, and if a leaf node
            is also the root, it's the only node in the hierarchy... */
            tree->root = INVALID_DBVH_NODE_INDEX;
        }
        else
        {
            /* 'node' is not the root, so we'll remove it and 'parent', and
            then set 'node's sibling node as direct child node of 'parent' parent node... */

            parent = ds_dbvt_get_node_pointer(tree, node->parent);
            sibling_index = parent->children[parent->children[0] == node_index];
            sibling = ds_dbvt_get_node_pointer(tree, sibling_index);

            if(parent->parent != INVALID_DBVH_NODE_INDEX)
            {
                /* 'parent' is not the root...

                ############################################################################

                                                  (before)

                                                      .
                                                      .
                                                      .
                                                      |
                                              (parent's parent)
                                              ________|________
                                             |                 |
                                          (parent)      (parent's sibling)
                                        _____|_____
                                       |           |
                                    (node)      (sibling)

                ------------------------------------------------------------------------------

                                                   (after)

                                                      .
                                                      .
                                                      .
                                                      |
                                              (parent's parent)
                                              ________|________
                                             |                 |
                                        (sibling)      (parent's sibling)

                ############################################################################

                */



                parent_parent = ds_dbvt_get_node_pointer(tree, parent->parent);
                parent_parent->children[parent_parent->children[0] != node->parent] = sibling_index;
                sibling->parent = parent->parent;
            }
            else
            {
                /* parent is the root...

                ############################################################################

                                                  (before)

                                            (parent - the root)
                                          ___________|___________
                                         |                       |
                                      (node)                 (sibling)
                                    _____|______            _____|_____
                                   |            |          |           |
                                   .            .          .           .
                                   .            .          .           .
                                   .            .          .           .


                ------------------------------------------------------------------------------

                                                   (after)

                                           (sibling - new root)
                                               _____|_____
                                              |           |
                                              .           .
                                              .           .
                                              .           .

                ############################################################################
                 */
                tree->root = sibling_index;
            }

            ds_dbvt_recompute_volumes(tree, sibling_index);

            /* dealloc only 'parent', since 'node' is a leaf node, which is linked to
            a collider. Let external code decide whether it wants to get rid of 'node'... */
            ds_dbvt_dealloc_node(tree, node->parent);
            node->parent = INVALID_DBVH_NODE_INDEX;
        }
    }
}

struct ds_dbvn_t *ds_dbvt_get_node_pointer(struct ds_dbvt_t *tree, uint32_t node_index)
{
    struct ds_dbvn_t *node = (struct ds_dbvn_t*)ds_slist_get_element(&tree->node_pool, node_index);

    if(node)
    {
        if(node->children[0] == node_index && node->children[1] == node_index && node->parent == node_index)
        {
            node = NULL;
        }
    }

    return node;
}

struct ds_dbvn_t *ds_dbvt_get_sibling_node_pointer(struct ds_dbvt_t *tree, uint32_t node_index)
{
    struct ds_dbvn_t *node;
    struct ds_dbvn_t *parent;

    node = ds_dbvt_get_node_pointer(tree, node_index);
    parent = ds_dbvt_get_node_pointer(tree, node->parent);

    return ds_dbvt_get_node_pointer(tree, parent->children[parent->children[0] != node_index]);
}

void ds_dbvt_nodes_smallest_volume_recursive(struct ds_dbvt_t* tree, uint32_t node_index, uint32_t cur_node_index, uint32_t* smallest_index, float* smallest_area)
{
    struct ds_dbvn_t* node;
    struct ds_dbvn_t* cur_node;
    vec3_t max;
    vec3_t min;
    vec3_t extents;
    float area;
    node = ds_dbvt_get_node_pointer(tree, node_index);
    cur_node = ds_dbvt_get_node_pointer(tree, cur_node_index);
    if(cur_node != node)
    {
        if(ds_dbvt_box_overlap(&node->max, &node->min, &cur_node->max, &cur_node->min))
        {
            if(cur_node->children[0] != cur_node->children[1])
            {
                ds_dbvt_nodes_smallest_volume_recursive(tree, node_index, cur_node->children[0], smallest_index, smallest_area);
                ds_dbvt_nodes_smallest_volume_recursive(tree, node_index, cur_node->children[1], smallest_index, smallest_area);
            }
        }

        ds_dbvt_pair_aabb(tree, node_index, cur_node_index, &max, &min);
        // extents = max - min;
        vec3_t_sub(&extents, &max, &min);
        area = (extents.x * extents.y +
                extents.x * extents.z +
                extents.y * extents.z) * 2.0;

        if(area < *smallest_area)
        {
            *smallest_index = cur_node_index;
            *smallest_area = area;
        }
    }
}

uint32_t ds_dbvt_nodes_smallest_volume(struct ds_dbvt_t* tree, uint32_t node_index)
{
    float smallest_area = FLT_MAX;
    uint32_t smallest_node_index = tree->root;
    ds_dbvt_nodes_smallest_volume_recursive(tree, node_index, tree->root, &smallest_node_index, &smallest_area);
//    if(smallest_node_index == node_index)
//    {
//        smallest_node_index = INVALID_DBVH_NODE_INDEX;
//    }
    return smallest_node_index;
}

uint32_t ds_dbvt_box_overlap(vec3_t *a_max, vec3_t *a_min, vec3_t *b_max, vec3_t *b_min)
{
    return a_max->x >= b_min->x && a_min->x <= b_max->x &&
           a_max->y >= b_min->y && a_min->y <= b_max->y &&
           a_max->z >= b_min->z && a_min->z <= b_max->z;
}

void ds_dbvt_pair_aabb(struct ds_dbvt_t *tree, uint32_t node_a, uint32_t node_b, vec3_t *max, vec3_t *min)
{
    struct ds_dbvn_t *a_ptr;
    struct ds_dbvn_t *b_ptr;

    int i;

    a_ptr = ds_dbvt_get_node_pointer(tree, node_a);
    b_ptr = ds_dbvt_get_node_pointer(tree, node_b);


    if(a_ptr && b_ptr)
    {
        *max = vec3_t_c(-FLT_MAX, -FLT_MAX, -FLT_MAX);
        *min = vec3_t_c(FLT_MAX, FLT_MAX, FLT_MAX);

        for(i = 0; i < 3; i++)
        {
            if(a_ptr->max.comps[i] > max->comps[i]) max->comps[i] = a_ptr->max.comps[i];
            if(a_ptr->min.comps[i] < min->comps[i]) min->comps[i] = a_ptr->min.comps[i];
        }

        for(i = 0; i < 3; i++)
        {
            if(b_ptr->max.comps[i] > max->comps[i]) max->comps[i] = b_ptr->max.comps[i];
            if(b_ptr->min.comps[i] < min->comps[i]) min->comps[i] = b_ptr->min.comps[i];
        }
    }
}

void ds_dbvt_recompute_volumes(struct ds_dbvt_t *tree, int start_node_index)
{
    struct ds_dbvn_t *node;

    node = ds_dbvt_get_node_pointer(tree, start_node_index);

    if(node)
    {
        if(node->children[0] != node->children[1])
        {
            ds_dbvt_pair_aabb(tree, node->children[0], node->children[1], &node->max, &node->min);
        }

        ds_dbvt_recompute_volumes(tree, node->parent);
    }
}

void ds_dbvt_box_contents_recursive(struct ds_list_t *contents, struct ds_dbvt_t *tree, uint32_t node_index, vec3_t *aabb_max, vec3_t *aabb_min)
{
    struct ds_dbvn_t *node;
    node = ds_dbvt_get_node_pointer(tree, node_index);

    if(node && ds_dbvt_box_overlap(aabb_max, aabb_min, &node->max, &node->min))
    {
        if(node->children[0] == node->children[1])
        {
            ds_list_add_element(contents, &node->contents);
        }
        else
        {
            ds_dbvt_box_contents_recursive(contents, tree, node->children[0], aabb_max, aabb_min);
            ds_dbvt_box_contents_recursive(contents, tree, node->children[1], aabb_max, aabb_min);
        }
    }
}

struct ds_list_t *ds_dbvt_box_contents(struct ds_dbvt_t *tree, vec3_t *aabb_max, vec3_t *aabb_min)
{
    static struct ds_list_t contents;
    if(contents.buffers == NULL)
    {
        contents = ds_list_create(sizeof(void *), 128);
    }
    contents.cursor = 0;
    ds_dbvt_box_contents_recursive(&contents, tree, tree->root, aabb_max, aabb_min);
    return &contents;
}

#endif


#endif // DS_DBVT_H
