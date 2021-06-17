#include "ed_bsp.h"
#include <stdlib.h>

extern struct stack_list_t ed_polygons;
extern struct stack_list_t ed_bsp_nodes;

#define ED_BSP_DELTA 0.0001

struct ed_polygon_t *ed_AllocPolygon()
{
    uint32_t index = add_stack_list_element(&ed_polygons, NULL);
    struct ed_polygon_t *polygon = get_stack_list_element(&ed_polygons, index);

    polygon->index = index;
    polygon->next = NULL;
    polygon->prev = NULL;

    if(!polygon->vertices.elem_size)
    {
        polygon->vertices = ds_create_buffer(sizeof(struct r_vert_t), 0);
    }

    return polygon;
}

void ed_FreePolygon(struct ed_polygon_t *polygon)
{
    if(polygon->index != 0xffffffff)
    {
        remove_stack_list_element(&ed_polygons, polygon->index);
        polygon->index = 0xffffffff;
    }
}

struct ed_bspn_t *ed_AllocNode()
{
    uint32_t index = add_stack_list_element(&ed_bsp_nodes, NULL);
    struct ed_bspn_t *node = get_stack_list_element(&ed_bsp_nodes, index);

    node->index = index;
    node->front = NULL;
    node->back = NULL;
    node->splitter = NULL;

    return node;
}

void ed_FreeNode(struct ed_bspn_t *node)
{
    if(node->index != 0xffffffff)
    {
        remove_stack_list_element(&ed_bsp_nodes, node->index);
        node->index = 0xffffffff;
    }
}

struct ed_polygon_t *ed_PolygonsFromBrush(struct ed_brush_t *brush)
{
    struct ed_polygon_t *polygons = NULL;
    struct ed_polygon_t *last_polygon = NULL;
    vec3_t *brush_vertices = (struct vec3_t *)brush->vertices.buffer;

    for(uint32_t face_index = 0; face_index < brush->faces.cursor; face_index++)
    {
        struct ed_face_t *face = get_list_element(&brush->faces, face_index);
        struct ed_polygon_t *polygon = ed_AllocPolygon();

        if(polygon->vertices.buffer_size < face->indices.buffer_size)
        {
            ds_resize_buffer(&polygon->vertices, face->indices.buffer_size);
        }

        uint32_t *indices = (uint32_t *)face->indices.buffer;
        struct r_vert_t *vertices = (struct r_vert_t *)polygon->vertices.buffer;
        polygon->normal = face->normal;

        for(uint32_t vert_index = 0; vert_index < face->indices.buffer_size; vert_index++)
        {
            struct r_vert_t *vertex = vertices + vert_index;
            vertex->pos = brush_vertices[indices[vert_index]];
            vertex->normal = vec4_t_c(face->normal.x, face->normal.y, face->normal.z, 1.0);
            vertex->tangent = face->tangent;
            vertex->tex_coords = vec2_t_c(0.0, 0.0);
        }

        if(!polygons)
        {
            polygons = polygon;
        }
        else
        {
            last_polygon->next = polygon;
            polygon->prev = last_polygon;
        }

        last_polygon = polygon;
    }

    return polygons;
}

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

struct ed_bspn_t *ed_BspFromPolygons(struct ed_polygon_t *polygons)
{
    struct ed_polygon_t *splitter = NULL;
    struct ed_polygon_t *last_splitter = NULL;
    struct ed_polygon_t *side_lists[2] = {NULL, NULL};
    struct ed_bspn_t *node = NULL;

    if(polygons)
    {
        splitter = ed_BestSplitter(polygons);
        struct r_vert_t *splitter_vert = (struct r_vert_t *)splitter->vertices.buffer;
        ed_UnlinkPolygon(splitter, &polygons);

        node = ed_AllocNode();
        node->splitter = splitter;
        last_splitter = splitter;

        while(polygons)
        {
            struct ed_polygon_t *polygon = polygons;
            ed_UnlinkPolygon(polygon, &polygons);

            uint32_t side = ed_PolygonOnSplitter(polygon, &splitter_vert->pos, &splitter_vert->normal);

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
            node->front = ed_BspFromPolygons(side_lists[ED_SPLITTER_SIDE_FRONT]);
        }

        if(side_lists[ED_SPLITTER_SIDE_BACK])
        {
            node->back = ed_BspFromPolygons(side_lists[ED_SPLITTER_SIDE_BACK]);
        }
    }

    return node;
}

uint32_t ed_PolygonsFromBsp(struct ds_buffer_t *polygons, struct ed_bspn_t *bsp)
{
    struct ed_polygon_t *polygon = bsp->splitter;
    uint32_t vert_count = 0;

    while(polygon)
    {
        ds_resize_buffer(polygons, polygons->buffer_size + 1);
        ds_fill_buffer(polygons, polygons->buffer_size - 1, polygon, 1);
        vert_count += polygon->vertices.buffer_size;
        polygon = polygon->next;
    }

    if(bsp->front)
    {
        vert_count += ed_PolygonsFromBsp(polygons, bsp->front);
    }

    if(bsp->back)
    {
        vert_count += ed_PolygonsFromBsp(polygons, bsp->back);
    }

    return vert_count;
}

int compare_polygons(const void *a, const void *b)
{
    struct ed_polygon_t *polygon_a = (struct ed_polygon_t *)a;
    struct ed_polygon_t *polygon_b = (struct ed_polygon_t *)b;

    if(polygon_a->material > polygon_b->material)
    {
        return 1;
    }
    else if (polygon_a->material == polygon_b->material)
    {
        return 0;
    }

    return -1;
}

void ed_GeometryFromBsp(struct r_model_geometry_t *geometry, struct ed_bspn_t *bsp)
{
    struct ds_buffer_t batch_buffer;
    struct ds_buffer_t vertex_buffer;
    struct ds_buffer_t index_buffer;
    struct ds_buffer_t polygon_buffer;
    uint32_t vert_count;
    uint32_t first_vertex = 0;

    polygon_buffer = ds_create_buffer(sizeof(struct ed_polygon_t), 0);
    vert_count = ed_PolygonsFromBsp(&polygon_buffer, bsp);
    qsort(polygon_buffer.buffer, polygon_buffer.buffer_size, polygon_buffer.elem_size, compare_polygons);

    vertex_buffer = ds_create_buffer(sizeof(struct r_vert_t), vert_count);
    index_buffer = ds_create_buffer(sizeof(uint32_t), (vert_count - 2) * 3);
    batch_buffer = ds_create_buffer(sizeof(struct r_batch_t), 0);

    uint32_t *indices = (uint32_t *)index_buffer.buffer;
    struct r_vert_t *vertices = (struct r_vert_t *)vertex_buffer.buffer;
    struct ed_polygon_t *polygons = (struct ed_polygon_t *)polygon_buffer.buffer;
    struct r_batch_t *batches = (struct r_batch_t *)batch_buffer.buffer;

    for(uint32_t polygon_index = 0; polygon_index < polygon_buffer.buffer_size; polygon_index++)
    {
        struct ed_polygon_t *polygon = polygons + polygon_index;
        struct r_batch_t *polygon_batch = NULL;
        uint32_t polygon_batch_index;

        for(polygon_batch_index = 0; polygon_batch_index < batch_buffer.buffer_size; polygon_batch_index++)
        {
            struct r_batch_t *batch = batches + polygon_batch_index;

            if(batch->material == polygon->material)
            {
                polygon_batch = batch;
                break;
            }
        }

        if(!polygon_batch)
        {
            ds_resize_buffer(&batch_buffer, batch_buffer.buffer_size + 1);
            batches = (struct r_batch_t *)batch_buffer.buffer;
            polygon_batch = batches + polygon_batch_index;
            polygon_batch->count = 0;
            polygon_batch->start = 0;
            polygon_batch->material = polygon->material;

            if(polygon_batch_index)
            {
                struct r_batch_t *prev_batch = batch_buffer.buffer + polygon_batch_index - 1;
                polygon_batch->start = prev_batch->start + prev_batch->count;
            }
        }

        memcpy(vertices + first_vertex, polygon->vertices.buffer, polygon->vertices.buffer_size * sizeof(struct r_vert_t));
        uint32_t *batch_indices = indices + polygon_batch->start;

        for(uint32_t vert_index = 1; vert_index < polygon->vertices.buffer_size - 1;)
        {
            batch_indices[polygon_batch->count] = first_vertex;
            polygon_batch->count++;

            batch_indices[polygon_batch->count] = first_vertex + vert_index;
            vert_index++;
            polygon_batch->count++;

            batch_indices[polygon_batch->count] = first_vertex + vert_index;
            polygon_batch->count++;
        }

        first_vertex += polygon->vertices.buffer_size;
    }

    geometry->batches = batch_buffer.buffer;
    geometry->batch_count = batch_buffer.buffer_size;
    geometry->verts = vertex_buffer.buffer;
    geometry->vert_count = vertex_buffer.buffer_size;
    geometry->indices = index_buffer.buffer;
    geometry->index_count = index_buffer.buffer_size;

    ds_destroy_buffer(&polygon_buffer);
}





