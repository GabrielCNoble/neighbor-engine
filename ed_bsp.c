#include <stdlib.h>

#include "ed_bsp.h"
#include "ed_brush.h"

#define ED_BSP_DELTA 0.0001

void ed_UnlinkPolygon(struct ed_polygon_t *polygon, struct ed_polygon_t **first_polygon)
{
    if(polygon->prev)
    {
        polygon->prev->next = polygon->next;
    }
    else
    {
        *first_polygon = polygon->next;

        if(polygon->next)
        {
            (*first_polygon)->prev = NULL;
        }
    }

    if(polygon->next)
    {
        polygon->next->prev = polygon->prev;
    }

    polygon->next = NULL;
    polygon->prev = NULL;
}

uint32_t ed_PolygonOnSplitter(struct ed_polygon_t *polygon, vec3_t *point, vec3_t *normal)
{
    struct r_vert_t *vertices = (struct r_vert_t *)polygon->vertices.buffer;
    float prev_dist = 0.0;
    int32_t sides = 0;

    for(uint32_t vert_index = 0; vert_index < polygon->vertices.buffer_size; vert_index++)
    {
        struct r_vert_t *vertex = vertices + vert_index;
        vec3_t polygon_splitter_vec;
        vec3_t_sub(&polygon_splitter_vec, &vertex->pos, point);
        float cur_dist = vec3_t_dot(&polygon_splitter_vec, normal);

        if(cur_dist * prev_dist < -(ED_BSP_DELTA * ED_BSP_DELTA))
        {
            return ED_SPLITTER_SIDE_STRADDLE;
        }

        if(cur_dist > ED_BSP_DELTA)
        {
            sides++;
        }
        else if(cur_dist < -ED_BSP_DELTA)
        {
            sides--;
        }

        prev_dist = cur_dist;
    }

    if(sides > 0)
    {
        return ED_SPLITTER_SIDE_FRONT;
    }
    else if(sides < 0)
    {
        return ED_SPLITTER_SIDE_BACK;
    }

    if(vec3_t_dot(normal, &vertices->normal) > 0.0)
    {
        return ED_SPLITTER_SIDE_ON_FRONT;
    }

    return ED_SPLITTER_SIDE_ON_BACK;
}

struct ed_polygon_t *ed_BestSplitter(struct ed_polygon_t *polygons)
{
    struct ed_polygon_t *splitter = polygons;
    struct ed_polygon_t *best_splitter = NULL;
    uint32_t best_split_count = 0xffffffff;

    while(splitter)
    {
        struct r_vert_t *splitter_vertex = (struct r_vert_t *)splitter->vertices.buffer;
        struct ed_polygon_t *polygon = splitter->next;
        uint32_t split_count = 0;

        while(polygon)
        {
            if(ed_PolygonOnSplitter(polygon, &splitter_vertex->pos, &splitter_vertex->normal) == ED_SPLITTER_SIDE_STRADDLE)
            {
                split_count++;
            }

            polygon = polygon->next;
        }

        if(split_count < best_split_count)
        {
            best_split_count = split_count;
            best_splitter = splitter;
        }

        splitter = splitter->next;
    }

    return best_splitter;
}

struct ed_bspn_t *ed_BrushBspFromPolygons(struct ed_polygon_t *polygons)
{
    struct ed_polygon_t *splitter = NULL;
    struct ed_polygon_t *last_splitter = NULL;
    struct ed_polygon_t *side_lists[2] = {NULL, NULL};
    struct ed_bspn_t *node = NULL;

    if(polygons)
    {
        splitter = ed_BestSplitter(polygons);
        struct r_vert_t *splitter_vert = splitter->vertices.buffer;
        ed_UnlinkPolygon(splitter, &polygons);

        node = ed_AllocBspNode(splitter->brush);
        node->splitter = splitter;
        last_splitter = splitter;

        while(polygons)
        {
            struct ed_polygon_t *polygon = polygons;
            ed_UnlinkPolygon(polygon, &polygons);

            uint32_t side = ed_PolygonOnSplitter(polygon, &splitter_vert->pos, &splitter_vert->normal.xyz);

            switch(side)
            {
                case ED_SPLITTER_SIDE_FRONT:
                case ED_SPLITTER_SIDE_BACK:
                {
                    struct ed_polygon_t *side_list = side_lists[side];
                    if(side_list)
                    {
                        side_list->prev = polygon;
                    }
                    polygon->next = side_list;
                    side_list = polygon;
                    side_lists[side] = side_list;
                }
                break;

                case ED_SPLITTER_SIDE_ON_FRONT:
                case ED_SPLITTER_SIDE_ON_BACK:
                    last_splitter->next = polygon;
                    polygon->prev = last_splitter;
                    last_splitter = polygon;
                break;
            }
        }

        if(side_lists[ED_SPLITTER_SIDE_FRONT])
        {
            node->front = ed_BrushBspFromPolygons(side_lists[ED_SPLITTER_SIDE_FRONT]);
        }

        if(side_lists[ED_SPLITTER_SIDE_BACK])
        {
            node->back = ed_BrushBspFromPolygons(side_lists[ED_SPLITTER_SIDE_BACK]);
        }
    }

    return node;
}





