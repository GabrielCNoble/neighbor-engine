#include <float.h>
#include "ed_brush.h"
#include "dstuff/ds_buffer.h"
#include "r_main.h"
#include "game.h"
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

//extern struct ds_slist_t ed_polygons;
//extern struct ds_slist_t ed_bsp_nodes;

struct ed_brush_t *ed_AllocBrush()
{
    uint32_t index;
    struct ed_brush_t *brush;

    index = ds_slist_add_element(&ed_w_ctx_data.brush.brushes, NULL);
    brush = ds_slist_get_element(&ed_w_ctx_data.brush.brushes, index);
    brush->index = index;
    brush->modified_index = 0xffffffff;
    brush->pickable = NULL;
    brush->next = NULL;
    brush->prev = NULL;
    brush->last = NULL;
    brush->main_brush = NULL;
    brush->entity = NULL;

    return brush;
}

struct ed_brush_t *ed_CreateBrush(vec3_t *position, mat3_t *orientation, vec3_t *size)
{
    struct ed_brush_t *brush;
    vec3_t dims;
    vec3_t_fabs(&dims, size);

    brush = ed_AllocBrush();
    brush->vertices = ds_slist_create(sizeof(vec3_t), 8);
    brush->vert_transforms = ds_list_create(sizeof(struct ed_vert_transform_t), 32);
    brush->main_brush = brush;
    brush->flags |= ED_BRUSH_FLAG_GEOMETRY_MODIFIED;

    for(uint32_t vert_index = 0; vert_index < brush->vertices.size; vert_index++)
    {
        uint32_t index = ed_AllocVertex(brush);
        vec3_t *vertice = ed_GetVertex(brush, index);
        vertice->x = dims.x * ed_cube_brush_vertices[vert_index].x;
        vertice->y = dims.y * ed_cube_brush_vertices[vert_index].y;
        vertice->z = dims.z * ed_cube_brush_vertices[vert_index].z;
    }

    brush->orientation = *orientation;
    brush->position = *position;
    brush->faces = NULL;

    struct ed_face_t *last_face = NULL;
    struct ed_edge_t *brush_edges = NULL;

    for(uint32_t face_index = 0; face_index < 6; face_index++)
    {
        struct ed_face_t *face = ed_AllocFace();

        face->material = r_GetDefaultMaterial();
        face->polygons = ed_AllocFacePolygon();
        face->tex_coords_scale = vec2_t_c(1.0, 1.0);
        face->tex_coords_rot = 0.0;
        face->brush = brush;

        struct ed_face_polygon_t *face_polygon = face->polygons;
        face->flags = ED_FACE_FLAG_GEOMETRY_MODIFIED;
        face_polygon->face = face;

        for(uint32_t vert_index = 0; vert_index < 4; vert_index++)
        {
            uint32_t vert0 = ed_cube_brush_indices[face_index][vert_index];
            uint32_t vert1 = ed_cube_brush_indices[face_index][(vert_index + 1) % 4];

            struct ed_edge_t *edge = brush_edges;

            while(edge)
            {
                if(edge->verts[0] == vert1 && edge->verts[1] == vert0)
                {
                    break;
                }

                edge = edge->init_next;
            }

            if(!edge)
            {
                edge = ed_AllocEdge();
                edge->brush = brush;
                edge->init_next = brush_edges;
                brush_edges = edge;
            }

            face_polygon->edge_count++;

            uint32_t polygon_index = edge->polygons[0].polygon != NULL;
            edge->polygons[polygon_index].polygon = face_polygon;
            edge->verts[polygon_index] = vert0;
            edge->verts[!polygon_index] = vert1;

            if(!face_polygon->edges)
            {
                face_polygon->edges = edge;
            }
            else
            {
                polygon_index = face_polygon->last_edge->polygons[1].polygon == face_polygon;
                face_polygon->last_edge->polygons[polygon_index].next = edge;
                polygon_index = edge->polygons[1].polygon == face_polygon;
                edge->polygons[polygon_index].prev = face_polygon->last_edge;
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

    brush->model = NULL;
    ed_UpdateBrush(brush);

    return brush;
}

struct ed_brush_t *ed_CopyBrush(struct ed_brush_t *brush)
{
    struct ed_brush_t *copy;
    copy = ed_AllocBrush();
    copy->vertices = ds_slist_create(sizeof(vec3_t), brush->vertices.size);
    copy->vert_transforms = ds_list_create(sizeof(struct ed_vert_transform_t), 32);
    copy->main_brush = copy;
    copy->flags |= ED_BRUSH_FLAG_GEOMETRY_MODIFIED;

    for(uint32_t vert_index = 0; vert_index < brush->vertices.size; vert_index++)
    {
        uint32_t index = ed_AllocVertex(copy);
        vec3_t *copy_vertice = ed_GetVertex(copy, index);
        vec3_t *vertice = ed_GetVertex(copy, vert_index);
        *copy_vertice = *vertice;
    }

    copy->orientation = brush->orientation;
    copy->position = brush->position;
    copy->faces = NULL;

    struct ed_face_t *last_face = NULL;
    struct ed_edge_t *brush_edges = NULL;

    struct ed_face_t *brush_face = brush->faces;

    while(brush_face)
    {
        struct ed_face_t *copy_face = ed_AllocFace();

        copy_face->material = brush_face->material;
        copy_face->polygons = ed_AllocFacePolygon();
        copy_face->tex_coords_scale = brush_face->tex_coords_scale;
        copy_face->tex_coords_rot = brush_face->tex_coords_rot;
        copy_face->brush = copy;

        struct ed_face_polygon_t *face_polygon = copy_face->polygons;
        face->flags = ED_FACE_FLAG_GEOMETRY_MODIFIED;
        face_polygon->face = copy_face;

        for(uint32_t vert_index = 0; vert_index < 4; vert_index++)
        {
            uint32_t vert0 = ed_cube_brush_indices[face_index][vert_index];
            uint32_t vert1 = ed_cube_brush_indices[face_index][(vert_index + 1) % 4];

            struct ed_edge_t *edge = brush_edges;

            while(edge)
            {
                if(edge->verts[0] == vert1 && edge->verts[1] == vert0)
                {
                    break;
                }

                edge = edge->init_next;
            }

            if(!edge)
            {
                edge = ed_AllocEdge();
                edge->brush = brush;
                edge->init_next = brush_edges;
                brush_edges = edge;
            }

            face_polygon->edge_count++;

            uint32_t polygon_index = edge->polygons[0].polygon != NULL;
            edge->polygons[polygon_index].polygon = face_polygon;
            edge->verts[polygon_index] = vert0;
            edge->verts[!polygon_index] = vert1;

            if(!face_polygon->edges)
            {
                face_polygon->edges = edge;
            }
            else
            {
                polygon_index = face_polygon->last_edge->polygons[1].polygon == face_polygon;
                face_polygon->last_edge->polygons[polygon_index].next = edge;
                polygon_index = edge->polygons[1].polygon == face_polygon;
                edge->polygons[polygon_index].prev = face_polygon->last_edge;
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

        brush_face = brush_face->next;
    }

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
        ds_list_destroy(&brush->vert_transforms);
        r_DestroyModel(brush->model);
        g_DestroyEntity(brush->entity);

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
    face->pickable = NULL;

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
    polygon->edge_count = 0;

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
    edge->init_next = NULL;
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

uint32_t ed_AllocVertex(struct ed_brush_t *brush)
{
    if(brush)
    {
        return ds_slist_add_element(&brush->vertices, NULL);
    }

    return 0xffffffff;
}

vec3_t *ed_GetVertex(struct ed_brush_t *brush, uint32_t index)
{
    if(brush)
    {
        return ds_slist_get_element(&brush->vertices, index);
    }

    return NULL;
}

void ed_FreeVertex(struct ed_brush_t *brush, uint32_t index)
{
    if(brush)
    {
        vec3_t *vertex = ds_slist_get_element(&brush->vertices, index);

        if(vertex)
        {
            *vertex = vec3_t_c(0.0, 0.0, 0.0);
        }

        ds_slist_remove_element(&brush->vertices, index);
    }
}

struct ed_vert_transform_t *ed_FindVertTransform(struct ed_brush_t *brush, uint32_t vert_index)
{
    struct ed_vert_transform_t *transform = NULL;

    if(brush)
    {
        for(uint32_t transform_index = 0; transform_index < brush->vert_transforms.cursor; transform_index++)
        {
            struct ed_vert_transform_t *vert_transform = ds_list_get_element(&brush->vert_transforms, transform_index);

            if(vert_transform->index == vert_index)
            {
                transform = vert_transform;
                break;
            }
        }

        if(!transform)
        {
            uint32_t index = ds_list_add_element(&brush->vert_transforms, NULL);
            transform = ds_list_get_element(&brush->vert_transforms, index);
            transform->index = vert_index;
            transform->translation = vec3_t_c(0.0, 0.0, 0.0);
        }
    }

    return transform;
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

void ed_ExtrudeBrushFace(struct ed_brush_t *brush, uint32_t face_index)
{
//    struct ed_face_t *face = ed_GetFace(face_index);
//
//    /* extruding a brush face doesn't work exactly as the name suggests. We won't be
//    extruding a face from this brush, but instead will create a new brush at the
//    extrusion site. This new brush will take ownership of the brush face being
//    extruded, and new faces and edges will be created between the old and the
//    extruded brush. */
//
//    if(face)
//    {
//        struct ed_face_polygon_t *polygon = face->polygons;
//        struct ed_edge_t *new_edges = NULL;
//        struct ed_brush_t *main_brush = brush->main_brush;
//        struct ed_brush_t *extrusion_brush = ed_AllocBrush();
//
//        if(!main_brush->next)
//        {
//            main_brush->next = extrusion_brush;
//        }
//        else
//        {
//            main_brush->last->next = extrusion_brush;
//            extrusion_brush->prev = main_brush->last;
//        }
//
//        main_brush->last = extrusion_brush;
//        extrusion_brush->main_brush = main_brush;
//
//        while(polygon)
//        {
//            /* To extrude a face, we'll go over all the edges of its polygons, and consider
//            only outside edges (edges that don't link polygons that belong to the same face).
//            This outside edge then gets cloned, and a new face (and polygon) gets created
//            between those two edges. */
//
//            struct ed_edge_t *edge = polygon->edges;
//
//            while(edge)
//            {
//                uint32_t polygon_index = edge->polygons[1].polygon == polygon;
//
//                if(edge->polygons[0].polygon->face != edge->polygons[1].polygon->face)
//                {
//                    /* we found an outside edge */
//                    struct ed_face_polygon_t *neighbor_polygon = edge->polygons[!polygon_index].polygon;
//
//                    /* this new edge will replace the edge of the neighbor polygon linked by this outside
//                    edge. */
//                    struct ed_edge_t *new_edge = ed_AllocEdge();
//                    new_edge->polygons[0].polygon = neighbor_polygon;
//                    new_edge->polygons[0].next = edge->polygons[!polygon_index].next;
//                    new_edge->polygons[0].prev = edge->polygons[!polygon_index].prev;
//                    new_edge->verts[0] = edge->verts[!polygon_index];
//                    new_edge->verts[1] = edge->verts[polygon_index];
//
//                    if(new_edge->polygons[0].next)
//                    {
//                        new_edge->polygons[0].next->prev = new_edge;
//                    }
//                    else
//                    {
//                        neighbor_polygon->last_edge = new_edge;
//                    }
//
//                    if(new_edge->polygons[0].prev)
//                    {
//                        new_edge->polygons[0].prev->next = new_edge;
//                    }
//                    else
//                    {
//                        neighbor_polygon->edges = new_edge;
//                    }
//
//                    neighbor_polygon = ed_AllocFacePolygon();
//                    new_edge->polygons[1].polygon = neighbor_polygon;
//                    neighbor_polygon->edges = new_edge;
//
//                    vec3_t *vert = ds_slist_get_element(&brush->vertices, edge->verts[polygon_index]);
//                    edge->verts[polygon_index] = ds_slist_add_element(&brush->vertices, vert);
//
//                    vert = ds_slist_get_element(&brush->vertices, edge->verts[!polygon_index]);
//                    edge->verts[!polygon_index] = ds_slist_add_element(&brush->vertices, vert);
//
//                    neighbor_polygon = ed_AllocFacePolygon();
//                    neighbor_polygon->edges = edge;
//                    neighbor_polygon->last_edge = edge;
//
//                    edge->polygons[!polygon_index].polygon = neighbor_polygon;
//                    edge->polygons[!polygon_index].prev = NULL;
//                    edge->polygons[!polygon_index].next = NULL;
//
//                    uint32_t vert0 = edge->verts[!polygon_index];
//                    uint32_t vert1 = neighbor_polygon->verts[0];
//
//                    struct ed_edge_t *new_edge = new_edges;
//
//                    while(new_edge)
//                    {
//                        if(new_edge->verts[0] == vert1 && new_edge->verst[1] == vert0)
//                        {
//                            break;
//                        }
//                        new_edge = new_edge->init_next;
//                    }
//
//                    if(!new_edge)
//                    {
//                        new_edge = ed_AllocEdge();
//                    }
//                }
//
//                edge = edge->polygons[polygon_index].next;
//            }
//
//            polygon = polygon->next;
//        }
//    }
}

void ed_DeleteBrushFace(struct ed_brush_t *brush, uint32_t face_index)
{

}

void ed_SetFaceMaterial(struct ed_brush_t *brush, uint32_t face_index, struct r_material_t *material)
{
    if(brush)
    {
        struct ed_face_t *face = ed_GetFace(face_index);
        face->flags |= ED_FACE_FLAG_MATERIAL_MODIFIED;
        face->material = material;
    }
}

void ed_TranslateBrushFace(struct ed_brush_t *brush, uint32_t face_index, vec3_t *translation)
{
    struct ed_face_t *face = ed_GetFace(face_index);

    if(face)
    {
        face->flags |= ED_FACE_FLAG_GEOMETRY_MODIFIED;
        struct ed_face_polygon_t *polygon = face->polygons;
        struct ed_brush_t *brush = face->brush;

        mat3_t brush_orientation = brush->orientation;
        mat3_t_transpose(&brush_orientation, &brush_orientation);

        vec3_t local_translation;
        mat3_t_vec3_t_mul(&local_translation, translation, &brush_orientation);

        while(polygon)
        {
            struct ed_edge_t *edge = polygon->edges;

            while(edge)
            {
                uint32_t polygon_index = edge->polygons[1].polygon == polygon;

                struct ed_vert_transform_t *transform = ed_FindVertTransform(brush, edge->verts[polygon_index]);
                transform->translation = local_translation;
//                if(transform->translation.x != local_translation.x || transform->translation.y != local_translation.y ||
//                   transform->translation.z != local_translation.z)
//                {
//                    vec3_t_add(&transform->translation, &transform->translation, &local_translation);
//                }

                edge->polygons[!polygon_index].polygon->face->flags |= ED_FACE_FLAG_GEOMETRY_MODIFIED;
                edge = edge->polygons[polygon_index].next;
            }

            polygon = polygon->next;
        }
    }
}

void ed_RotateBrushFace(struct ed_brush_t *brush, uint32_t face_index, mat3_t *rotation)
{
    struct ed_face_t *face = ed_GetFace(face_index);

    if(face)
    {
        face->flags |= ED_FACE_FLAG_GEOMETRY_MODIFIED;
        struct ed_face_polygon_t *polygon = face->polygons;
        struct ed_brush_t *brush = face->brush;

        mat3_t inverse_brush_orientation;
        mat3_t_transpose(&inverse_brush_orientation, &brush->orientation);

        mat3_t local_rotation;
        mat3_t_mul(&local_rotation, &brush->orientation, rotation);
        mat3_t_mul(&local_rotation, &local_rotation, &inverse_brush_orientation);

        while(polygon)
        {
            struct ed_edge_t *edge = polygon->edges;

            while(edge)
            {
                uint32_t polygon_index = edge->polygons[1].polygon == polygon;
                vec3_t vert = *(vec3_t *)ds_slist_get_element(&brush->vertices, edge->verts[polygon_index]);
                vec3_t_sub(&vert, &vert, &polygon->center);
                vec3_t translation;

                mat3_t_vec3_t_mul(&translation, &vert, &local_rotation);
                vec3_t_sub(&translation, &translation, &vert);

                struct ed_face_polygon_t *neighbor = edge->polygons[!polygon_index].polygon;
//                vec3_t_mul(&translation, &polygon->normal, -vec3_t_dot(&polygon->normal, &translation));

                struct ed_vert_transform_t *transform = ed_FindVertTransform(brush, edge->verts[polygon_index]);
                transform->translation = translation;
//                if(transform->translation.x != translation.x || transform->translation.y != translation.y ||
//                   transform->translation.z != translation.z)
//                {
//                    vec3_t_add(&transform->translation, &transform->translation, &translation);
//                }

                edge->polygons[!polygon_index].polygon->face->flags |= ED_FACE_FLAG_GEOMETRY_MODIFIED;
                edge = edge->polygons[polygon_index].next;
            }

            polygon = polygon->next;
        }
    }
}

void ed_UpdateBrush(struct ed_brush_t *brush)
{
    struct r_model_geometry_t geometry = {};
    struct ed_polygon_t *polygons = NULL;
    uint32_t rebuild_bsp = 1;

    brush->clipped_vert_count = 0;
    brush->clipped_index_count = 0;
    brush->clipped_polygon_count = 0;

    vec3_t vert_translation = vec3_t_c(0.0, 0.0, 0.0);

    if(brush->vert_transforms.cursor)
    {
        for(uint32_t transform_index = 0; transform_index < brush->vert_transforms.cursor; transform_index++)
        {
            /* go over all vertex translations, and apply them accordingly */
            struct ed_vert_transform_t *transform = ds_list_get_element(&brush->vert_transforms, transform_index);
            vec3_t *brush_vert = ed_GetVertex(brush, transform->index);
            vec3_t_add(brush_vert, brush_vert, &transform->translation);
        }

        /* find out how much the new brush center moved away from the old one */
        for(uint32_t vert_index = 0; vert_index < brush->vertices.cursor; vert_index++)
        {
            vec3_t *vert = ed_GetVertex(brush, vert_index);
            vec3_t_add(&vert_translation, &vert_translation, vert);
        }

        vec3_t_div(&vert_translation, &vert_translation, (float)brush->vertices.cursor);
        /* translate verts so the new center becomes the current center */
        for(uint32_t vert_index = 0; vert_index < brush->vertices.cursor; vert_index++)
        {
            vec3_t *vert = ed_GetVertex(brush, vert_index);
            vec3_t_sub(vert, vert, &vert_translation);
        }

        /* translate the brush accordingly, so it visibly stays at the same place */
        mat3_t_vec3_t_mul(&vert_translation, &vert_translation, &brush->orientation);
        vec3_t_add(&brush->position, &brush->position, &vert_translation);

        /* we touched the vertices, so we'll need to do a bunch of updating now... */
        brush->flags |= ED_BRUSH_FLAG_GEOMETRY_MODIFIED;
    }

    brush->vert_transforms.cursor = 0;
    struct ed_face_t *face = brush->faces;

    if(brush->flags & ED_BRUSH_FLAG_GEOMETRY_MODIFIED)
    {
        while(face)
        {
            struct ed_face_polygon_t *face_polygon = face->polygons;
            uint32_t point_count = 0;

            face->center = vec3_t_c(0, 0, 0);
            /* recompute center/normal/tangent/uv coords for each polygon of this face */
            while(face_polygon)
            {
                struct ed_edge_t *first_edge = face_polygon->edges;
                uint32_t polygon_index = first_edge->polygons[1].polygon == face_polygon;
                struct ed_edge_t *second_edge = first_edge->polygons[polygon_index].next;

                vec3_t edge0_vec;
                vec3_t edge1_vec;

                vec3_t *vert0 = ed_GetVertex(brush, first_edge->verts[polygon_index]);
                vec3_t *vert1 = ed_GetVertex(brush, first_edge->verts[!polygon_index]);
                polygon_index = second_edge->polygons[1].polygon == face_polygon;
                vec3_t *vert2 = ed_GetVertex(brush, second_edge->verts[!polygon_index]);

                vec3_t_sub(&edge0_vec, vert1, vert0);
                vec3_t_sub(&edge1_vec, vert2, vert1);

                vec3_t_cross(&face_polygon->normal, &edge1_vec, &edge0_vec);
                vec3_t_normalize(&face_polygon->normal, &face_polygon->normal);

                face_polygon->tangent = edge0_vec;
                vec3_t_normalize(&face_polygon->tangent, &face_polygon->tangent);

                face_polygon->center = vec3_t_c(0.0, 0.0, 0.0);

                while(first_edge)
                {
                    polygon_index = first_edge->polygons[1].polygon == face_polygon;
                    vec3_t_add(&face_polygon->center, &face_polygon->center, ed_GetVertex(brush, first_edge->verts[polygon_index]));
                    first_edge = first_edge->polygons[polygon_index].next;
                }

                vec3_t_div(&face_polygon->center, &face_polygon->center, (float)face_polygon->edge_count);

                vec3_t_add(&face->center, &face->center, &face_polygon->center);
                point_count++;

                face_polygon = face_polygon->next;
            }

            vec3_t_div(&face->center, &face->center, (float)point_count);

            /* regen bsp polygons/reallocate pickable ranges for this face */
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

//            struct ed_pickable_t *face_pickable = face->pickable;
//
//            if(face_pickable->range_count > face->clipped_polygon_count)
//            {
//                while(face_pickable->range_count > face->clipped_polygon_count)
//                {
//                    struct ed_pickable_range_t *next_range = face_pickable->ranges->next;
//                    next_range->prev = NULL;
//
//                    ed_FreePickableRange(face_pickable->ranges);
//                    face_pickable->range_count--;
//                    face_pickable->ranges = next_range;
//                }
//            }
//            else if(face_pickable->range_count < face->clipped_polygon_count)
//            {
//                while(face_pickable->range_count < face->clipped_polygon_count)
//                {
//                    struct ed_pickable_range_t *new_range = ed_AllocPickableRange();
//                    new_range->next = face_pickable->ranges;
//                    if(face_pickable->ranges)
//                    {
//                        face_pickable->ranges->prev = new_range;
//                    }
//                    face_pickable->ranges = new_range;
//                    face_pickable->range_count++;
//                }
//            }

            brush->clipped_vert_count += face->clipped_vert_count;
            brush->clipped_index_count += face->clipped_index_count;
            brush->clipped_polygon_count += face->clipped_polygon_count;

            face = face->next;
        }
    }

//    face = brush->faces;
//
//    while(face)
//    {
//        uint32_t update_material = (brush->flags & ED_BRUSH_FLAG_GEOMETRY_MODIFIED) | (face->flags & ED_FACE_FLAG_MATERIAL_MODIFIED);
//
//        if(update_material)
//        {
//            face->clipped_polygons = ed_BspPolygonFromBrushFace(face);
//            struct ed_bsp_polygon_t *bsp_polygon = face->clipped_polygons;
//
//            face->clipped_vert_count = 0;
//            face->clipped_index_count = 0;
//            face->clipped_polygon_count = 0;
//
//            while(bsp_polygon)
//            {
//                face->clipped_vert_count += bsp_polygon->vertices.cursor;
//                face->clipped_index_count += (bsp_polygon->vertices.cursor - 2) * 3;
//                face->clipped_polygon_count++;
//                bsp_polygon = bsp_polygon->next;
//            }
//
//            struct ed_pickable_t *face_pickable = face->pickable;
//
//            if(pickable->range_count > face->clipped_polygon_count)
//            {
//                while(pickable->range_count > face->clipped_polygon_count)
//                {
//                    struct ed_pickable_range_t *next_range = pickable->ranges->next;
//                    next_range->prev = NULL;
//
//                    ed_FreePickableRange(pickable->ranges);
//                    pickable->range_count--;
//                    pickable->ranges = next_range;
//                }
//            }
//            else if(pickable->range_count < face->clipped_polygon_count)
//            {
//                while(pickable->range_count < face->clipped_polygon_count)
//                {
//                    struct ed_pickable_range_t *new_range = ed_AllocPickableRange();
//                    new_range->next = pickable->ranges;
//                    if(pickable->ranges)
//                    {
//                        pickable->ranges->prev = new_range;
//                    }
//                    pickable->ranges = new_range;
//                    pickable->range_count++;
//                }
//            }
//        }
//
//        brush->clipped_vert_count += face->clipped_vert_count;
//        brush->clipped_index_count += face->clipped_index_count;
//        brush->clipped_polygon_count += face->clipped_polygon_count;
//
//        face->flags = 0;
//        face = face->next;
//    }

    if(brush->flags & ED_BRUSH_FLAG_GEOMETRY_MODIFIED)
    {
        struct ds_buffer_t *batch_buffer;
        struct ds_buffer_t *vertex_buffer;
        struct ds_buffer_t *index_buffer;
        struct ds_buffer_t *polygon_buffer;

        if(ed_w_ctx_data.brush.polygon_buffer.buffer_size < brush->clipped_polygon_count)
        {
            ds_buffer_resize(&ed_w_ctx_data.brush.polygon_buffer, brush->clipped_polygon_count);
        }

        if(ed_w_ctx_data.brush.vertex_buffer.buffer_size < brush->clipped_vert_count)
        {
            ds_buffer_resize(&ed_w_ctx_data.brush.vertex_buffer, brush->clipped_vert_count);
        }

        if(ed_w_ctx_data.brush.index_buffer.buffer_size < brush->clipped_index_count)
        {
            ds_buffer_resize(&ed_w_ctx_data.brush.index_buffer, brush->clipped_index_count);
        }

        polygon_buffer = &ed_w_ctx_data.brush.polygon_buffer;
        vertex_buffer = &ed_w_ctx_data.brush.vertex_buffer;
        index_buffer = &ed_w_ctx_data.brush.index_buffer;
        batch_buffer = &ed_w_ctx_data.brush.batch_buffer;

        uint32_t polygon_count = 0;
        uint32_t vertex_count = 0;
        uint32_t index_count = 0;
        uint32_t batch_count = 0;

        face = brush->faces;

        struct ed_bsp_polygon_t **bsp_polygons = polygon_buffer->buffer;

        while(face)
        {
            struct ed_bsp_polygon_t *clipped_polygons = face->clipped_polygons;

            while(clipped_polygons)
            {
                bsp_polygons[polygon_count] = clipped_polygons;
                polygon_count++;
                clipped_polygons = clipped_polygons->next;
            }

            face = face->next;
        }

        qsort(bsp_polygons, polygon_count, polygon_buffer->elem_size, compare_polygons);

        uint32_t *indices = (uint32_t *)index_buffer->buffer;
        struct r_vert_t *vertices = (struct r_vert_t *)vertex_buffer->buffer;
        struct r_batch_t *batches = (struct r_batch_t *)batch_buffer->buffer;

        geometry.min = vec3_t_c(FLT_MAX, FLT_MAX, FLT_MAX);
        geometry.max = vec3_t_c(-FLT_MAX, -FLT_MAX, -FLT_MAX);

        for(uint32_t polygon_index = 0; polygon_index < polygon_count; polygon_index++)
        {
            struct ed_bsp_polygon_t *polygon = bsp_polygons[polygon_index];
            struct r_batch_t *polygon_batch = NULL;
            uint32_t polygon_batch_index;

            for(polygon_batch_index = 0; polygon_batch_index < batch_count; polygon_batch_index++)
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
                if(batch_buffer->buffer_size <= batch_count)
                {
                    ds_buffer_resize(batch_buffer, batch_count + 1);
                }

                batch_count++;

                batches = (struct r_batch_t *)batch_buffer->buffer;
                polygon_batch = batches + polygon_batch_index;
                polygon_batch->count = 0;
                polygon_batch->start = 0;
                polygon_batch->material = polygon->face_polygon->face->material;

                if(polygon_batch_index)
                {
                    struct r_batch_t *prev_batch = batch_buffer->buffer + polygon_batch_index - 1;
                    polygon_batch->start = prev_batch->start + prev_batch->count;
                }
            }

            for(uint32_t vert_index = 0; vert_index < polygon->vertices.cursor; vert_index++)
            {
                struct r_vert_t *vert = (struct r_vert_t *)ds_list_get_element(&polygon->vertices, vert_index);
                vertices[vertex_count + vert_index] = *vert;

                if(geometry.min.x > vert->pos.x) geometry.min.x = vert->pos.x;
                if(geometry.min.y > vert->pos.y) geometry.min.y = vert->pos.y;
                if(geometry.min.z > vert->pos.z) geometry.min.z = vert->pos.z;

                if(geometry.max.x < vert->pos.x) geometry.max.x = vert->pos.x;
                if(geometry.max.y < vert->pos.y) geometry.max.y = vert->pos.y;
                if(geometry.max.z < vert->pos.z) geometry.max.z = vert->pos.z;
            }

            uint32_t *batch_indices = indices + polygon_batch->start;

            polygon->model_start = polygon_batch->start + polygon_batch->count;
            polygon->model_count = polygon_batch->count;
            for(uint32_t vert_index = 1; vert_index < polygon->vertices.cursor - 1;)
            {
                batch_indices[polygon_batch->count] = vertex_count;
                polygon_batch->count++;

                batch_indices[polygon_batch->count] = vertex_count + vert_index;
                vert_index++;
                polygon_batch->count++;

                batch_indices[polygon_batch->count] = vertex_count + vert_index;
                polygon_batch->count++;
            }

            polygon->model_count = polygon_batch->count - polygon->model_count;
            index_count += polygon->model_count;


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

            vertex_count += polygon->vertices.cursor;
        }

        geometry.batches = batch_buffer->buffer;
        geometry.batch_count = batch_count;
        geometry.verts = vertex_buffer->buffer;
        geometry.vert_count = vertex_count;
        geometry.indices = index_buffer->buffer;
        geometry.index_count = index_count;

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
    }

    mat4_t transform;
    mat4_t_comp(&transform, &brush->orientation, &brush->position);

    if(!brush->entity)
    {
        brush->entity = g_CreateEntity(&transform, NULL, brush->model);
    }

    brush->entity->local_transform = transform;
    brush->flags = 0;
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
