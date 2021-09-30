#include "ed_brush.h"
#include "dstuff/ds_buffer.h"
#include "r_main.h"
#include "ed_bsp.h"

uint32_t ed_cube_brush_indices[][4] =
{
    /* -Z */
    {0, 1, 2, 3},
    /* +Z */
    {4, 5, 6, 7},
    /* -X */
    {0, 3, 5, 4},
    /* +X */
    {7, 6, 2, 1},
    /* -Y */
    {5, 3, 2, 6},
    /* +Y */
    {0, 4, 7, 1}
};

vec3_t ed_cube_brush_vertices[] =
{
    vec3_t_c(-0.5, 0.5, -0.5),
    vec3_t_c(0.5, 0.5, -0.5),
    vec3_t_c(0.5, -0.5, -0.5),
    vec3_t_c(-0.5, -0.5, -0.5),

    vec3_t_c(-0.5, 0.5, 0.5),
    vec3_t_c(-0.5, -0.5, 0.5),
    vec3_t_c(0.5, -0.5, 0.5),
    vec3_t_c(0.5, 0.5, 0.5),
};

vec3_t ed_cube_brush_normals[] =
{
    vec3_t_c(0.0, 0.0, -1.0),
    vec3_t_c(0.0, 0.0, 1.0),
    vec3_t_c(-1.0, 0.0, 0.0),
    vec3_t_c(1.0, 0.0, 0.0),
    vec3_t_c(0.0, -1.0, 0.0),
    vec3_t_c(0.0, 1.0, 0.0)
};

vec3_t ed_cube_brush_tangents[] =
{
    vec3_t_c(1.0, 0.0, 0.0),
    vec3_t_c(-1.0, 0.0, 0.0),
    vec3_t_c(0.0, 0.0, 1.0),
    vec3_t_c(0.0, 0.0, -1.0),
    vec3_t_c(1.0, 0.0, 0.0),
    vec3_t_c(-1.0, 0.0, 0.0),
};

extern struct ed_world_context_data_t ed_w_ctx_data;

extern struct ds_slist_t ed_polygons;
extern struct ds_slist_t ed_bsp_nodes;

struct ed_brush_t *ed_CreateBrush(vec3_t *position, mat3_t *orientation, vec3_t *size)
{
    uint32_t index;
    struct ed_brush_t *brush;
    vec3_t dims;
    vec3_t_fabs(&dims, size);

    index = ds_slist_add_element(&ed_w_ctx_data.brush.brushes, NULL);
    brush = ds_slist_get_element(&ed_w_ctx_data.brush.brushes, index);
    brush->index = index;

    brush->vertices = ds_slist_create(sizeof(vec3_t), 8);
//    brush->polygons = ds_slist_create(sizeof(struct ed_polygon_t), 16);
//    brush->bsp_nodes = ds_slist_create(sizeof(struct ed_bspn_t), 16);

    for(uint32_t vert_index = 0; vert_index < brush->vertices.size; vert_index++)
    {
        uint32_t index = ds_slist_add_element(&brush->vertices, NULL);
        vec3_t *vertice = ds_slist_get_element(&brush->vertices, vert_index);
        vertice->x = dims.x * ed_cube_brush_vertices[vert_index].x;
        vertice->y = dims.y * ed_cube_brush_vertices[vert_index].y;
        vertice->z = dims.z * ed_cube_brush_vertices[vert_index].z;
    }

    brush->orientation = *orientation;
    brush->position = *position;
    brush->faces = NULL;

    struct ed_face_t *last_face = NULL;
    struct ed_edge_t *last_edge = NULL;

    for(uint32_t face_index = 0; face_index < 6; face_index++)
    {
        struct ed_face_t *face = ed_AllocFace();

        face->material = r_GetDefaultMaterial();
        face->polygons = ed_AllocFacePolygon();
        face->tex_coords_scale = vec2_t_c(1.0, 1.0);
        face->tex_coords_rot = 0.0;
        face->brush = brush;

        struct ed_face_polygon_t *face_polygon = face->polygons;
        face_polygon->normal = ed_cube_brush_normals[face_index];
        face_polygon->tangent = ed_cube_brush_tangents[face_index];
        face_polygon->face = face;

        for(uint32_t vert_index = 0; vert_index < 4; vert_index++)
        {
//            struct ed_edge_t *edge = ed_AllocEdge();

            uint32_t vert0 = ed_cube_brush_indices[face_index][vert_index];
            uint32_t vert1 = ed_cube_brush_indices[face_index][(vert_index + 1) % 4];

            struct ed_edge_t *edge = last_edge;

            while(edge)
            {
                if(edge->verts[0] == vert1 && edge->verts[1] == vert0)
                {
                    break;
                }

                edge = edge->polygons[0].prev;
            }

            if(!edge)
            {
                edge = ed_AllocEdge();
                edge->brush = brush;
                edge->polygons[0].prev = last_edge;
                last_edge = edge;
            }

            uint32_t polygon_index = edge->polygons[0].polygon != NULL;
            edge->polygons[polygon_index].polygon = face_polygon;
            edge->verts[polygon_index] = vert0;
            edge->verts[!polygon_index] = vert1;

//            edge->brush = brush;
//            edge->polygon = face_polygon;
//            edge->vert0 = ed_cube_brush_indices[face_index][vert_index];
//            edge->vert1 = ed_cube_brush_indices[face_index][(vert_index + 1) % 4];

            /* edges in a face are stored in a doubly linked list. All edges of the brush are linked
            by the prev pointer, and only edges that belong to a specific face are linked by the prev
            pointer. This is to allow us to simplify linking edges to its siblings down below.
            Everything after this will iterate over edges using only the next pointer, so it's fine to
            just forget the prev pointer exists and not clear it as well. */

            if(!face_polygon->edges)
            {
                face_polygon->edges = edge;
            }
            else
            {
                polygon_index = face_polygon->last_edge->polygons[1].polygon == face_polygon;
                face_polygon->last_edge->polygons[polygon_index].next = edge;
            }

            face_polygon->last_edge = edge;
        }

        if(!brush->faces)
        {
            brush->faces = face;
        }
        else
        {
            last_face->next = face;
            face->prev = last_face;
        }

        last_face = face;
    }

//    struct ed_edge_t *edge = last_edge;
//    struct ed_edge_t *prev_edge_list = last_edge;

//    while(edge->polygons[0].prev)
//    {
//        if(prev_edge_list == edge)
//        {
//            /* since edges are allocated as faces are created, the edges of a face will be adjacent to one another.
//            To avoid testing edges belonging to the same face we skip them until reaching an edge that doesn't belong
//            to that face. */
//
//            while(prev_edge_list && prev_edge_list->polygon == edge->polygon)
//            {
//                prev_edge_list = prev_edge_list->polygons[0].prev;
//            }
//        }
//
//        struct ed_edge_t *prev_edge = prev_edge_list;
//
//        while(prev_edge)
//        {
//            if(edge->vert0 == prev_edge->vert1 && edge->vert1 == prev_edge->vert0)
//            {
//                /* Two edges that represent a shared edge will have the same vert indexes,
//                but always in reverse order. */
//                edge->sibling = prev_edge;
//                prev_edge->sibling = edge;
//                break;
//            }
//
//            prev_edge = prev_edge->prev;
//        }
//
//        edge = edge->prev;
//    }

    brush->model = NULL;
    ed_UpdateBrush(brush);

    return brush;
}

void ed_DestroyBrush(struct ed_brush_t *brush)
{
    if(brush)
    {
        ed_w_ctx_data.brush.brush_vert_count -= brush->model->verts.buffer_size;
        ed_w_ctx_data.brush.brush_index_count -= brush->model->indices.buffer_size;

        for(uint32_t batch_index = 0; batch_index < brush->model->batches.buffer_size; batch_index++)
        {
            struct r_batch_t *batch = (struct r_batch_t *)brush->model->batches.buffer + batch_index;
            struct ed_brush_batch_t *global_batch = ed_GetGlobalBrushBatch(batch->material);
            global_batch->batch.count -= batch->count;
        }

//        for(uint32_t face_index = 0; face_index < brush->faces.cursor; face_index++)
//        {
//            struct ed_face_t *face = ds_list_get_element(&brush->faces, face_index);
//            ds_buffer_destroy(&face->indices);
//
//            if(face->polygon)
//            {
//                ds_buffer_destroy(&face->polygon->vertices);
//            }
//
//            if(face->clipped_polygons)
//            {
//                while(face->clipped_polygons)
//                {
//                    ds_buffer_destroy(&face->clipped_polygons->vertices);
//                    face->clipped_polygons = face->clipped_polygons->next;
//                }
//            }
//        }

        struct ed_face_t *face = brush->faces;

        while(face)
        {
            struct ed_face_t *next_face = face->next;
            struct ed_face_polygon_t *polygon = face->polygons;

            while(polygon)
            {
                struct ed_edge_t *edge = polygon->edges;

                while(edge)
                {
                    uint32_t polygon_index = edge->polygons[1].polygon == polygon;
                    struct ed_edge_t *next_edge = edge->polygons[polygon_index].next;
                    ed_FreeEdge(edge);
                    edge = next_edge;
                }

                struct ed_face_polygon_t *next_polygon = polygon->next;
                ed_FreeFacePolygon(polygon);
                polygon = next_polygon;
            }

            struct ed_bsp_polygon_t *clipped_polygon = face->clipped_polygons;

            while(clipped_polygon)
            {
                struct ed_bsp_polygon_t *next_polygon = clipped_polygon->next;
                ed_FreeBspPolygon(next_polygon, 0);
                clipped_polygon = next_polygon;
            }

            ed_FreeFace(face);
            face = next_face;
        }

        ds_slist_destroy(&brush->vertices);
        r_DestroyModel(brush->model);

        ds_slist_remove_element(&ed_w_ctx_data.brush.brushes, brush->index);
        brush->index = 0xffffffff;
    }
}

struct ed_brush_batch_t *ed_GetGlobalBrushBatch(struct r_material_t *material)
{
    struct ed_brush_batch_t *batch = NULL;

    for(uint32_t batch_index = 0; batch_index < ed_w_ctx_data.brush.brush_batches.cursor; batch_index++)
    {
        batch = ds_list_get_element(&ed_w_ctx_data.brush.brush_batches, batch_index);

        if(batch->batch.material == material)
        {
            break;
        }
    }

    if(!batch)
    {
        uint32_t index = ds_list_add_element(&ed_w_ctx_data.brush.brush_batches, NULL);
        batch = ds_list_get_element(&ed_w_ctx_data.brush.brush_batches, index);
        batch->index = index;
        batch->batch.material = material;
        batch->batch.count = 0;
        batch->batch.start = 0;
    }

    return batch;
}

struct ed_brush_t *ed_GetBrush(uint32_t index)
{
    struct ed_brush_t *brush = NULL;

    if(index != 0xffffffff)
    {
        brush = ds_slist_get_element(&ed_w_ctx_data.brush.brushes, index);

        if(brush && brush->index == 0xffffffff)
        {
            brush = NULL;
        }
    }

    return brush;
}

struct ed_face_t *ed_AllocFace()
{
    uint32_t index = ds_slist_add_element(&ed_w_ctx_data.brush.brush_faces, NULL);
    struct ed_face_t *face = ds_slist_get_element(&ed_w_ctx_data.brush.brush_faces, index);

    face->index = index;
    face->polygons = NULL;
    face->clipped_polygons = NULL;
    face->next = NULL;
    face->prev = NULL;
    face->material = NULL;
    face->brush = NULL;

    return face;
}

struct ed_face_t *ed_GetFace(uint32_t index)
{
    struct ed_face_t *face;

    face = ds_slist_get_element(&ed_w_ctx_data.brush.brush_faces, index);

    if(face && face->index == 0xffffffff)
    {
        face = NULL;
    }

    return face;
}

void ed_FreeFace(struct ed_face_t *face)
{
    if(face && face->index != 0xffffffff)
    {
        ds_slist_remove_element(&ed_w_ctx_data.brush.brush_faces, face->index);
        face->index = 0xffffffff;
    }
}

struct ed_face_polygon_t *ed_AllocFacePolygon()
{
    uint32_t index = ds_slist_add_element(&ed_w_ctx_data.brush.brush_face_polygons, NULL);
    struct ed_face_polygon_t *polygon = ds_slist_get_element(&ed_w_ctx_data.brush.brush_face_polygons, index);

    polygon->index = index;
    polygon->face = NULL;
    polygon->brush = NULL;
    polygon->next = NULL;
    polygon->prev = NULL;
    polygon->edges = NULL;
    polygon->last_edge = NULL;

    return polygon;
}

void ed_FreeFacePolygon(struct ed_face_polygon_t *polygon)
{
    if(polygon && polygon->index != 0xffffffff)
    {
        ds_slist_remove_element(&ed_w_ctx_data.brush.brush_face_polygons, polygon->index);
        polygon->index = 0xffffffff;
    }
}

struct ed_edge_t *ed_AllocEdge()
{
    uint32_t index = ds_slist_add_element(&ed_w_ctx_data.brush.brush_edges, NULL);
    struct ed_edge_t *edge = ds_slist_get_element(&ed_w_ctx_data.brush.brush_edges, index);

    edge->index = index;
    edge->brush = NULL;
    edge->verts[0] = 0xffffffff;
    edge->verts[1] = 0xffffffff;
    edge->polygons[0].next = NULL;
    edge->polygons[0].prev = NULL;
    edge->polygons[0].polygon = NULL;
    edge->polygons[1].next = NULL;
    edge->polygons[1].prev = NULL;
    edge->polygons[1].polygon = NULL;

    return edge;
}

struct ed_edge_t *ed_GetEdge(uint32_t index)
{
    struct ed_edge_t *edge;

    edge = ds_slist_get_element(&ed_w_ctx_data.brush.brush_edges, index);

    if(edge && edge->index == 0xffffffff)
    {
        edge = NULL;
    }

    return edge;
}

void ed_FreeEdge(struct ed_edge_t *edge)
{
    if(edge && edge->index != 0xffffffff)
    {
        ds_slist_remove_element(&ed_w_ctx_data.brush.brush_edges, edge->index);
        edge->index = 0xffffffff;
    }
}

int compare_polygons(const void *a, const void *b)
{
    struct ed_bsp_polygon_t *polygon_a = *(struct ed_bsp_polygon_t **)a;
    struct ed_bsp_polygon_t *polygon_b = *(struct ed_bsp_polygon_t **)b;

    if(polygon_a->face_polygon->face->material > polygon_b->face_polygon->face->material)
    {
        return 1;
    }
    else if (polygon_a->face_polygon->face->material == polygon_b->face_polygon->face->material)
    {
        return 0;
    }

    return -1;
}

void ed_UpdateBrush(struct ed_brush_t *brush)
{
    struct r_model_geometry_t geometry = {};
    struct ed_polygon_t *polygons = NULL;
    uint32_t rebuild_bsp = 1;

    brush->clipped_vert_count = 0;
    brush->clipped_index_count = 0;
    brush->clipped_polygon_count = 0;

    struct ed_face_t *face = brush->faces;

    while(face)
    {
        face->clipped_polygons = ed_BspPolygonFromBrushFace(face);
        struct ed_bsp_polygon_t *bsp_polygon = face->clipped_polygons;

        face->clipped_vert_count = 0;
        face->clipped_index_count = 0;
        face->clipped_polygon_count = 0;

        while(bsp_polygon)
        {
            face->clipped_vert_count += bsp_polygon->vertices.cursor;
            face->clipped_index_count += (bsp_polygon->vertices.cursor - 2) * 3;
            face->clipped_polygon_count++;
            bsp_polygon = bsp_polygon->next;
        }

        brush->clipped_vert_count += face->clipped_vert_count;
        brush->clipped_index_count += face->clipped_index_count;
        brush->clipped_polygon_count += face->clipped_polygon_count;

        face = face->next;
    }

//    if(!brush->bsp || rebuild_bsp)
//    {
//        brush->bsp = ed_BrushBspFromPolygons(polygons);
//    }

    struct ds_buffer_t batch_buffer;
    struct ds_buffer_t vertex_buffer;
    struct ds_buffer_t index_buffer;
    struct ds_buffer_t polygon_buffer;

    polygon_buffer = ds_buffer_create(sizeof(struct ed_bsp_polygon_t *), brush->clipped_polygon_count);
    vertex_buffer = ds_buffer_create(sizeof(struct r_vert_t), brush->clipped_vert_count);
    index_buffer = ds_buffer_create(sizeof(uint32_t), brush->clipped_index_count);
    batch_buffer = ds_buffer_create(sizeof(struct r_batch_t), 0);

    uint32_t polygon_index = 0;

    face = brush->faces;

    while(face)
    {
        struct ed_bsp_polygon_t *clipped_polygons = face->clipped_polygons;

        while(clipped_polygons)
        {
            ((struct ed_bsp_polygon_t **)polygon_buffer.buffer)[polygon_index] = clipped_polygons;
            polygon_index++;
            clipped_polygons = clipped_polygons->next;
        }

        face = face->next;
    }

    qsort(polygon_buffer.buffer, polygon_buffer.buffer_size, polygon_buffer.elem_size, compare_polygons);

    uint32_t *indices = (uint32_t *)index_buffer.buffer;
    struct r_vert_t *vertices = (struct r_vert_t *)vertex_buffer.buffer;
    struct r_batch_t *batches = (struct r_batch_t *)batch_buffer.buffer;
    uint32_t first_vertex = 0;

    for(uint32_t polygon_index = 0; polygon_index < polygon_buffer.buffer_size; polygon_index++)
    {
        struct ed_bsp_polygon_t *polygon = ((struct ed_bsp_polygon_t **)polygon_buffer.buffer)[polygon_index];
        struct r_batch_t *polygon_batch = NULL;
        uint32_t polygon_batch_index;

        for(polygon_batch_index = 0; polygon_batch_index < batch_buffer.buffer_size; polygon_batch_index++)
        {
            struct r_batch_t *batch = batches + polygon_batch_index;

            if(batch->material == polygon->face_polygon->face->material)
            {
                polygon_batch = batch;
                break;
            }
        }

        if(!polygon_batch)
        {
            ds_buffer_resize(&batch_buffer, batch_buffer.buffer_size + 1);
            batches = (struct r_batch_t *)batch_buffer.buffer;
            polygon_batch = batches + polygon_batch_index;
            polygon_batch->count = 0;
            polygon_batch->start = 0;
            polygon_batch->material = polygon->face_polygon->face->material;

            if(polygon_batch_index)
            {
                struct r_batch_t *prev_batch = batch_buffer.buffer + polygon_batch_index - 1;
                polygon_batch->start = prev_batch->start + prev_batch->count;
            }
        }

        for(uint32_t vert_index = 0; vert_index < polygon->vertices.cursor; vert_index++)
        {
            vertices[first_vertex + vert_index] = *(struct r_vert_t *)ds_list_get_element(&polygon->vertices, vert_index);
        }

        uint32_t *batch_indices = indices + polygon_batch->start;

        polygon->model_start = polygon_batch->start + polygon_batch->count;
        polygon->model_count = polygon_batch->count;
        for(uint32_t vert_index = 1; vert_index < polygon->vertices.cursor - 1;)
        {
            batch_indices[polygon_batch->count] = first_vertex;
            polygon_batch->count++;

            batch_indices[polygon_batch->count] = first_vertex + vert_index;
            vert_index++;
            polygon_batch->count++;

            batch_indices[polygon_batch->count] = first_vertex + vert_index;
            polygon_batch->count++;
        }

        polygon->model_count = polygon_batch->count - polygon->model_count;


//        struct ed_face_polygon_t *face_polygon = polygon->face_polygon;
//        struct ed_edge_t *edge = face_polygon->edges;

//        uint32_t vert_index = polygon->model_start;
//
//        while(edge)
//        {
//            edge->model_start = vert_index;
//            vert_index++;
//            edge = edge->next;
//        }

        first_vertex += polygon->vertices.cursor;
    }

    geometry.batches = batch_buffer.buffer;
    geometry.batch_count = batch_buffer.buffer_size;
    geometry.verts = vertex_buffer.buffer;
    geometry.vert_count = vertex_buffer.buffer_size;
    geometry.indices = index_buffer.buffer;
    geometry.index_count = index_buffer.buffer_size;

    if(!brush->model)
    {
        brush->model = r_CreateModel(&geometry, NULL);
    }
    else
    {
        ed_w_ctx_data.brush.brush_vert_count -= brush->model->verts.buffer_size;
        ed_w_ctx_data.brush.brush_index_count -= brush->model->indices.buffer_size;

        for(uint32_t batch_index = 0; batch_index < brush->model->batches.buffer_size; batch_index++)
        {
            struct r_batch_t *batch = (struct r_batch_t *)brush->model->batches.buffer + batch_index;
            struct ed_brush_batch_t *global_batch = ed_GetGlobalBrushBatch(batch->material);
            global_batch->batch.count -= batch->count;
        }

        r_UpdateModelGeometry(brush->model, &geometry);
    }

    ed_w_ctx_data.brush.brush_vert_count += brush->model->verts.buffer_size;
    ed_w_ctx_data.brush.brush_index_count += brush->model->indices.buffer_size;

    for(uint32_t batch_index = 0; batch_index < brush->model->batches.buffer_size; batch_index++)
    {
        struct r_batch_t *batch = (struct r_batch_t *)brush->model->batches.buffer + batch_index;
        struct ed_brush_batch_t *global_batch = ed_GetGlobalBrushBatch(batch->material);
        global_batch->batch.count += batch->count;
    }

    ds_buffer_destroy(&polygon_buffer);
    ds_buffer_destroy(&vertex_buffer);
    ds_buffer_destroy(&index_buffer);
    ds_buffer_destroy(&batch_buffer);
}

void ed_TranslateBrushFace(struct ed_brush_t *brush, uint32_t face_index, vec3_t *translation)
{
    struct ed_face_t *face = ed_GetFace(face_index);

    if(face)
    {
        struct ed_face_polygon_t *polygon = face->polygons;
        struct ed_brush_t *brush = face->brush;

        while(polygon)
        {
            struct ed_edge_t *edge = polygon->edges;

            while(edge)
            {
                uint32_t polygon_index = edge->polygons[1].polygon == polygon;
                vec3_t *brush_vertex = ds_slist_get_element(&face->brush->vertices, edge->verts[polygon_index]);
                vec3_t_add(brush_vertex, brush_vertex, translation);
                edge = edge->polygons[polygon_index].next;
            }

            struct ed_edge_t *first_edge = polygon->edges;
            uint32_t polygon_index = first_edge->polygons[1].polygon == polygon;
            struct ed_edge_t *second_edge = first_edge->polygons[polygon_index].next;

            vec3_t edge0_vec;
            vec3_t edge1_vec;

            vec3_t *vert0 = ds_slist_get_element(&brush->vertices, first_edge->verts[polygon_index]);
            vec3_t *vert1 = ds_slist_get_element(&brush->vertices, first_edge->verts[!polygon_index]);
            polygon_index = second_edge->polygons[1].polygon == polygon;
            vec3_t *vert2 = ds_slist_get_element(&brush->vertices, second_edge->verts[!polygon_index]);

            vec3_t_sub(&edge0_vec, vert1, vert0);
            vec3_t_sub(&edge1_vec, vert2, vert1);

            vec3_t_cross(&polygon->normal, &edge1_vec, &edge0_vec);
            vec3_t_normalize(&polygon->normal, &polygon->normal);

            polygon->tangent = edge0_vec;
            vec3_t_normalize(&polygon->tangent, &polygon->tangent);

            polygon = polygon->next;
        }

        polygon = face->polygons;

        while(polygon)
        {
            struct ed_edge_t *edge = polygon->edges;

            while(edge)
            {
                uint32_t polygon_index = edge->polygons[0].polygon == polygon;
                struct ed_face_polygon_t *neighbor_polygon = edge->polygons[polygon_index].polygon;

                struct ed_edge_t *first_edge = neighbor_polygon->edges;
                polygon_index = first_edge->polygons[1].polygon == neighbor_polygon;
                struct ed_edge_t *second_edge = first_edge->polygons[polygon_index].next;

                vec3_t edge0_vec;
                vec3_t edge1_vec;

                vec3_t *vert0 = ds_slist_get_element(&brush->vertices, first_edge->verts[polygon_index]);
                vec3_t *vert1 = ds_slist_get_element(&brush->vertices, first_edge->verts[!polygon_index]);
                polygon_index = second_edge->polygons[1].polygon == neighbor_polygon;
                vec3_t *vert2 = ds_slist_get_element(&brush->vertices, second_edge->verts[!polygon_index]);

                vec3_t_sub(&edge0_vec, vert1, vert0);
                vec3_t_sub(&edge1_vec, vert2, vert1);

                vec3_t_cross(&neighbor_polygon->normal, &edge1_vec, &edge0_vec);
                vec3_t_normalize(&neighbor_polygon->normal, &neighbor_polygon->normal);

                neighbor_polygon->tangent = edge0_vec;
                vec3_t_normalize(&neighbor_polygon->tangent, &neighbor_polygon->tangent);


                polygon_index = edge->polygons[1].polygon == polygon;
                edge = edge->polygons[polygon_index].next;
            }

            polygon = polygon->next;
        }

        ed_UpdateBrush(brush);
    }
}

void ed_BuildWorldGeometry()
{
//    struct ds_buffer_t vertices = ds_buffer_create(sizeof(struct r_vert_t), ed_w_ctx_data.brush.brush_vert_count);
//    struct ds_buffer_t indices = ds_buffer_create(sizeof(uint32_t), ed_w_ctx_data.brush.brush_index_count);
//    struct ds_buffer_t batches = ds_buffer_create(sizeof(struct r_batch_t), ed_w_ctx_data.brush.brush_batches.cursor);
//
//    for(uint32_t global_batch_index = 0; global_batch_index < batches.buffer_size; global_batch_index++)
//    {
//        struct r_batch_t *batch = (struct r_batch_t *)batches.buffer + global_batch_index;
//        struct ed_brush_batch_t *brush_batch = ds_list_get_element(&ed_w_ctx_data.brush.brush_batches, global_batch_index);
//
//        *batch = brush_batch->batch;
//
//        if(global_batch_index)
//        {
//            struct r_batch_t *prev_batch = (struct r_batch_t *)batches.buffer + (global_batch_index - 1);
//            batch->start = prev_batch->start + prev_batch->count;
//        }
//
//        batch->count = 0;
//
//        for(uint32_t brush_index = 0; brush_index < ed_w_ctx_data.brush.brushes.cursor; brush_index++)
//        {
//            struct ed_brush_t *brush = ed_GetBrush(brush_index);
//
//            if(brush)
//            {
//                for(uint32_t model_batch_index = 0; model_batch_index < brush->model->batches.buffer_size; model_batch_index++)
//                {
//                    struct r_batch_t *model_batch = (struct r_batch_t *)brush->model->batches.buffer + model_batch_index;
//
//                    if(model_batch->material == batch->material)
//                    {
//
//                    }
//                }
//            }
//        }
//    }
}

/*
=============================================================
=============================================================
=============================================================
*/

struct ed_bsp_node_t *ed_AllocBspNode()
{
    struct ed_bsp_node_t *node = NULL;
    uint32_t index;

    index = ds_slist_add_element(&ed_w_ctx_data.brush.bsp_nodes, NULL);
    node = ds_slist_get_element(&ed_w_ctx_data.brush.bsp_nodes, index);

    node->front = NULL;
    node->back = NULL;
    node->splitter = NULL;
    node->index = index;

    return node;
}

void ed_FreeBspNode(struct ed_bsp_node_t *node)
{
    if(node)
    {
        ds_slist_remove_element(&ed_w_ctx_data.brush.bsp_nodes, node->index);
        node->index = 0xffffffff;
    }
}

struct ed_bsp_polygon_t *ed_BspPolygonFromBrushFace(struct ed_face_t *face)
{
    struct ed_face_polygon_t *face_polygon = face->polygons;
    struct ed_bsp_polygon_t *bsp_polygons = NULL;
    struct ed_bsp_polygon_t *last_bsp_polygon = NULL;
    struct ed_bsp_polygon_t dummy_polygon;
    struct ed_bsp_polygon_t *bsp_polygon = &dummy_polygon;

    dummy_polygon.next = face->clipped_polygons;

    while(face_polygon)
    {
        if(!bsp_polygon->next)
        {
            bsp_polygon = ed_AllocBspPolygon(face_polygon->edge_count);
        }
        else
        {
            bsp_polygon = bsp_polygon->next;
        }

        bsp_polygon->vertices.cursor = 0;
        bsp_polygon->face_polygon = face_polygon;
        bsp_polygon->normal = face_polygon->normal;
        bsp_polygon->point = vec3_t_c(0.0, 0.0, 0.0);

        struct ds_list_t *polygon_verts = &bsp_polygon->vertices;
        struct ds_slist_t *brush_verts = &face->brush->vertices;
        struct ed_edge_t *edge = face_polygon->edges;

        while(edge)
        {
            uint32_t polygon_index = edge->polygons[1].polygon == face_polygon;
            vec3_t *brush_vert = ds_slist_get_element(brush_verts, edge->verts[polygon_index]);
            struct r_vert_t *polygon_vert = ds_list_get_element(polygon_verts, ds_list_add_element(polygon_verts, NULL));

            vec3_t_add(&bsp_polygon->point, &bsp_polygon->point, brush_vert);

            polygon_vert->pos = *brush_vert;
            polygon_vert->normal.xyz = face_polygon->normal;
            polygon_vert->tangent = face_polygon->tangent;
            polygon_vert->tex_coords = vec2_t_c(0.0, 0.0);

            edge = edge->polygons[polygon_index].next;
        }

        vec3_t_div(&bsp_polygon->point, &bsp_polygon->point, bsp_polygon->vertices.cursor);

        if(!bsp_polygons)
        {
            bsp_polygons = bsp_polygon;
        }
        else
        {
            last_bsp_polygon->next = bsp_polygon;
            bsp_polygon->prev = last_bsp_polygon;
        }

        last_bsp_polygon = bsp_polygon;
        face_polygon = face_polygon->next;
    }

    struct ed_bsp_polygon_t *unused_polygon = last_bsp_polygon->next;

    while(unused_polygon)
    {
        struct ed_bsp_polygon_t *next_polygon = unused_polygon->next;
        ed_FreeBspPolygon(unused_polygon, 0);
        unused_polygon = next_polygon;
    }

    return bsp_polygons;
}

struct ed_bsp_polygon_t *ed_AllocBspPolygon(uint32_t vert_count)
{
    uint32_t index = ds_slist_add_element(&ed_w_ctx_data.brush.bsp_polygons, NULL);
    struct ed_bsp_polygon_t *polygon = ds_slist_get_element(&ed_w_ctx_data.brush.bsp_polygons, index);

    polygon->index = index;
    polygon->face_polygon = NULL;
    polygon->next = NULL;
    polygon->prev = NULL;

    if(!polygon->vertices.buffers)
    {
        polygon->vertices = ds_list_create(sizeof(struct r_vert_t), 8);
    }

    polygon->vertices.cursor = 0;

    return polygon;
}

void ed_FreeBspPolygon(struct ed_bsp_polygon_t *polygon, uint32_t free_verts)
{
    if(polygon && polygon->index != 0xffffffff)
    {
        if(free_verts)
        {
            ds_list_destroy(&polygon->vertices);
            polygon->vertices.buffers = NULL;
        }

        ds_slist_remove_element(&ed_w_ctx_data.brush.bsp_polygons, polygon->index);
        polygon->index = 0xffffffff;
    }
}

#define ED_BSP_DELTA 0.0001

void ed_UnlinkPolygon(struct ed_bsp_polygon_t *polygon, struct ed_bsp_polygon_t **first_polygon)
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

uint32_t ed_PolygonOnSplitter(struct ed_bsp_polygon_t *polygon, vec3_t *point, vec3_t *normal)
{
//    struct r_vert_t *vertices = (struct r_vert_t *)polygon->vertices.buffer;
    float prev_dist = 0.0;
    int32_t sides = 0;

    for(uint32_t vert_index = 0; vert_index < polygon->vertices.buffer_size; vert_index++)
    {
        struct r_vert_t *vertex = ds_list_get_element(&polygon->vertices, vert_index);
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

    if(vec3_t_dot(normal, &polygon->normal) > 0.0)
    {
        return ED_SPLITTER_SIDE_ON_FRONT;
    }

    return ED_SPLITTER_SIDE_ON_BACK;
}

struct ed_bsp_polygon_t *ed_BestSplitter(struct ed_bsp_polygon_t *polygons)
{
    struct ed_bsp_polygon_t *splitter = polygons;
    struct ed_bsp_polygon_t *best_splitter = NULL;
    uint32_t best_split_count = 0xffffffff;

    while(splitter)
    {
        struct r_vert_t *splitter_vertex = ds_list_get_element(&splitter->vertices, 0);
        struct ed_bsp_polygon_t *polygon = splitter->next;
        uint32_t split_count = 0;

        while(polygon)
        {
            if(ed_PolygonOnSplitter(polygon, &splitter_vertex->pos, &splitter->normal) == ED_SPLITTER_SIDE_STRADDLE)
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

struct ed_bsp_node_t *ed_BrushBspFromPolygons(struct ed_bsp_polygon_t *polygons)
{
    struct ed_bsp_polygon_t *splitter = NULL;
    struct ed_bsp_polygon_t *last_splitter = NULL;
    struct ed_bsp_polygon_t *side_lists[2] = {NULL, NULL};
    struct ed_bsp_node_t *node = NULL;

    if(polygons)
    {
        splitter = ed_BestSplitter(polygons);
        struct r_vert_t *splitter_vert = ds_list_get_element(&splitter->vertices, 0);
        ed_UnlinkPolygon(splitter, &polygons);

        node = ed_AllocBspNode();
        node->splitter = splitter;
        last_splitter = splitter;

        while(polygons)
        {
            struct ed_bsp_polygon_t *polygon = polygons;
            ed_UnlinkPolygon(polygon, &polygons);

            uint32_t side = ed_PolygonOnSplitter(polygon, &splitter_vert->pos, &splitter->normal);

            switch(side)
            {
                case ED_SPLITTER_SIDE_FRONT:
                case ED_SPLITTER_SIDE_BACK:
                {
                    struct ed_bsp_polygon_t *side_list = side_lists[side];
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
