#include <float.h>
#include "ed_brush.h"
#include "ed_pick_defs.h"
#include "ed_level_defs.h"
#include "dstuff/ds_buffer.h"
#include "../engine/r_main.h"
#include "../engine/g_main.h"
#include "../engine/phys.h"
#include "../engine/ent.h"
#include "ed_bsp.h"

//extern struct p_tmesh_shape_t *l_world_shape;
//extern struct p_collider_t *l_world_collider;

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

extern struct ed_level_state_t ed_level_state;

//extern struct ds_slist_t ed_polygons;
//extern struct ds_slist_t ed_bsp_nodes;

struct ed_brush_t *ed_AllocBrush()
{
    uint32_t index;
    struct ed_brush_t *brush;

    index = ds_slist_add_element(&ed_level_state.brush.brushes, NULL);
    brush = ds_slist_get_element(&ed_level_state.brush.brushes, index);
    brush->index = index;
    brush->modified_index = 0xffffffff;
    brush->pickable = NULL;
    brush->next = NULL;
    brush->prev = NULL;
    brush->last = NULL;
    brush->main_brush = NULL;
    brush->entity = NULL;
    brush->faces = NULL;
    brush->last_face = NULL;
    brush->face_count = 0;
    brush->edge_count = 0;
    brush->polygon_count = 0;
    brush->model = NULL;

    brush->vertices = ds_slist_create(sizeof(struct ed_vert_t), 8);
    brush->vert_transforms = ds_list_create(sizeof(struct ed_vert_transform_t), 32);

    return brush;
}

struct ed_brush_t *ed_CreateBrush(vec3_t *position, mat3_t *orientation, vec3_t *size)
{
    struct ed_brush_t *brush;
    vec3_t dims;
    vec3_t_fabs(&dims, size);

    brush = ed_AllocBrush();
//    brush->vertices = ds_slist_create(sizeof(struct ed_vert_t), 8);
//    brush->vert_transforms = ds_list_create(sizeof(struct ed_vert_transform_t), 32);
    brush->main_brush = brush;
    brush->update_flags |= ED_BRUSH_UPDATE_FLAG_FACE_POLYGONS;

    for(uint32_t vert_index = 0; vert_index < brush->vertices.size; vert_index++)
    {
//        uint32_t index = ed_AllocVertex(brush);
//        vec3_t *vertice = ed_GetVertex(brush, index);

        struct ed_vert_t *vertice = ed_AllocVert(brush);
        vertice->vert.x = dims.x * ed_cube_brush_vertices[vert_index].x;
        vertice->vert.y = dims.y * ed_cube_brush_vertices[vert_index].y;
        vertice->vert.z = dims.z * ed_cube_brush_vertices[vert_index].z;
    }

    brush->orientation = *orientation;
    brush->position = *position;

    struct ed_edge_t *brush_edges = NULL;

    for(uint32_t face_index = 0; face_index < 6; face_index++)
    {
        struct ed_face_t *face = ed_AllocFace(brush);

        face->material = ed_GetGlobalBrushBatch(r_GetDefaultMaterial());
        face->tex_coords_scale = vec2_t_c(1.0, 1.0);
        face->tex_coords_rot = 0.0;

        struct ed_face_polygon_t *face_polygon = ed_AllocFacePolygon(brush, face);
        face->flags = ED_FACE_FLAG_GEOMETRY_MODIFIED;

        for(uint32_t vert_index = 0; vert_index < 4; vert_index++)
        {
            struct ed_vert_t *vert0 = ed_GetVert(brush, ed_cube_brush_indices[face_index][vert_index]);
            struct ed_vert_t *vert1 = ed_GetVert(brush, ed_cube_brush_indices[face_index][(vert_index + 1) % 4]);

            struct ed_edge_t *edge = brush_edges;

            while(edge)
            {
                if(edge->verts[0].vert == vert1 && edge->verts[1].vert == vert0)
                {
                    break;
                }

                edge = edge->init_next;
            }

            if(!edge)
            {
                edge = ed_AllocEdge(brush);
                edge->brush = brush;
                edge->init_next = brush_edges;
                brush_edges = edge;

                edge->verts[0].vert = vert0;
                edge->verts[1].vert = vert1;
                ed_LinkVertEdge(vert0, edge);
                ed_LinkVertEdge(vert1, edge);
            }

            uint32_t polygon_side = edge->polygons[0].polygon != NULL;
            edge->polygons[polygon_side].polygon = face_polygon;
            ed_LinkFacePolygonEdge(face_polygon, edge);
        }
    }

    ed_UpdateBrush(brush);

    return brush;
}

struct ed_brush_t *ed_CopyBrush(struct ed_brush_t *src_brush)
{
    struct ed_brush_t *dst_brush = ed_AllocBrush();
//    dst_brush->vertices = ds_slist_create(sizeof(struct ed_vert_t), 8);
//    dst_brush->vert_transforms = ds_list_create(sizeof(struct ed_vert_transform_t), 32);
    dst_brush->main_brush = dst_brush;
    dst_brush->update_flags |= ED_BRUSH_UPDATE_FLAG_FACE_POLYGONS;

    for(uint32_t vert_index = 0; vert_index < src_brush->vertices.cursor; vert_index++)
    {
        struct ed_vert_t *src_vert = ed_GetVert(src_brush, vert_index);

        if(src_vert)
        {
            struct ed_vert_t *dst_vert = ed_AllocVert(dst_brush);
            dst_vert->vert = src_vert->vert;
            /* we'll repair src_vert->index later. This is just so we can
            map a src_vert to a dst_vert */
            src_vert->index = dst_vert->index;
        }
    }

    dst_brush->orientation = src_brush->orientation;
    dst_brush->position = src_brush->position;

    struct ed_edge_t *dst_edges = NULL;
    struct ed_face_t *src_face = src_brush->faces;

    while(src_face)
    {
        struct ed_face_t *dst_face = ed_AllocFace(dst_brush);

        dst_face->material = src_face->material;
        dst_face->tex_coords_scale = src_face->tex_coords_scale;
        dst_face->tex_coords_rot = src_face->tex_coords_rot;

        struct ed_face_polygon_t *src_polygon = src_face->polygons;

        while(src_polygon)
        {
            struct ed_edge_t *src_edge = src_polygon->edges;
            struct ed_face_polygon_t *dst_polygon = ed_AllocFacePolygon(dst_brush, dst_face);

            dst_polygon->normal = src_polygon->normal;
            dst_polygon->tangent = src_polygon->tangent;
            dst_polygon->center = src_polygon->center;

            while(src_edge)
            {
                uint32_t polygon_side = src_edge->polygons[1].polygon == src_polygon;
                struct ed_vert_t *src_vert0 = src_edge->verts[polygon_side].vert;
                struct ed_vert_t *src_vert1 = src_edge->verts[!polygon_side].vert;
                struct ed_edge_t *next_src_edge = src_edge->polygons[polygon_side].next;

                struct ed_vert_t *dst_vert0 = ed_GetVert(dst_brush, src_vert0->index);
                struct ed_vert_t *dst_vert1 = ed_GetVert(dst_brush, src_vert1->index);

                struct ed_edge_t *dst_edge = dst_edges;

                while(dst_edge)
                {
                    if(dst_edge->verts[1].vert == dst_vert0 && dst_edge->verts[0].vert == dst_vert1)
                    {
                        break;
                    }

                    dst_edge = dst_edge->init_next;
                }

                if(!dst_edge)
                {
                    dst_edge = ed_AllocEdge(dst_brush);
                    dst_edge->brush = dst_brush;
                    dst_edge->init_next = dst_edges;
                    dst_edges = dst_edge;

                    dst_edge->verts[0].vert = dst_vert0;
                    dst_edge->verts[1].vert = dst_vert1;

                    ed_LinkVertEdge(dst_vert0, dst_edge);
                    ed_LinkVertEdge(dst_vert1, dst_edge);
                }

                /* the first polygon to be linked to an edge will have polygon_index == 0 */
                polygon_side = dst_edge->polygons[0].polygon != NULL;
                dst_edge->polygons[polygon_side].polygon = dst_polygon;
                ed_LinkFacePolygonEdge(dst_polygon, dst_edge);
                src_edge = next_src_edge;
            }

            src_polygon = src_polygon->next;
        }

        src_face = src_face->next;
    }

    for(uint32_t vert_index = 0; vert_index < src_brush->vertices.cursor; vert_index++)
    {
        struct ed_vert_t *vert = ed_GetVert(src_brush, vert_index);

        if(vert)
        {
            vert->index = vert_index;
        }
    }

    ed_UpdateBrush(dst_brush);

    return dst_brush;
}

void ed_DestroyBrush(struct ed_brush_t *brush)
{
    if(brush)
    {
//        ed_level_state.brush.brush_model_vert_count -= brush->model->verts.buffer_size;
//        ed_level_state.brush.brush_model_index_count -= brush->model->indices.buffer_size;

//        for(uint32_t batch_index = 0; batch_index < brush->model->batches.buffer_size; batch_index++)
//        {
//            struct r_batch_t *batch = (struct r_batch_t *)brush->model->batches.buffer + batch_index;
//            struct ed_brush_batch_t *global_batch = ed_GetGlobalBrushBatch(batch->material);
//            global_batch->batch.count -= batch->count;
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
                    ed_FreeEdge(brush, edge);
                    edge = next_edge;
                }

                struct ed_face_polygon_t *next_polygon = polygon->next;
                ed_FreeFacePolygon(brush, polygon);
                polygon = next_polygon;
            }

            struct ed_bsp_polygon_t *clipped_polygon = face->clipped_polygons;

            while(clipped_polygon)
            {
                struct ed_bsp_polygon_t *next_polygon = clipped_polygon->next;
                ed_FreeBspPolygon(next_polygon, 0);
                clipped_polygon = next_polygon;
            }

            ed_FreeFace(brush, face);
            face = next_face;
        }

        ed_level_state.brush.brush_vert_count -= brush->vertices.used;
        ds_slist_destroy(&brush->vertices);
        ds_list_destroy(&brush->vert_transforms);
        r_DestroyModel(brush->model);
        e_DestroyEntity(brush->entity);

        ds_slist_remove_element(&ed_level_state.brush.brushes, brush->index);
        brush->index = 0xffffffff;
    }
}

struct ed_brush_batch_t *ed_GetGlobalBrushBatch(struct r_material_t *material)
{
    struct ed_brush_batch_t *batch = NULL;

    for(uint32_t batch_index = 0; batch_index < ed_level_state.brush.brush_batches.cursor; batch_index++)
    {
        batch = ds_list_get_element(&ed_level_state.brush.brush_batches, batch_index);

        if(batch->batch.material == material)
        {
            break;
        }
    }

    if(!batch)
    {
        uint32_t index = ds_list_add_element(&ed_level_state.brush.brush_batches, NULL);
        batch = ds_list_get_element(&ed_level_state.brush.brush_batches, index);
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
        brush = ds_slist_get_element(&ed_level_state.brush.brushes, index);

        if(brush && brush->index == 0xffffffff)
        {
            brush = NULL;
        }
    }

    return brush;
}

struct ed_face_t *ed_AllocFace(struct ed_brush_t *brush)
{
    uint32_t index = ds_slist_add_element(&ed_level_state.brush.brush_faces, NULL);
    struct ed_face_t *face = ds_slist_get_element(&ed_level_state.brush.brush_faces, index);

    face->index = index;
    face->polygons = NULL;
    face->last_polygon = NULL;
    face->clipped_polygons = NULL;
    face->next = NULL;
    face->prev = NULL;
    face->material = NULL;
    face->brush = brush;
    face->pickable = NULL;

    if(!brush->faces)
    {
        brush->faces = face;
    }
    else
    {
        brush->last_face->next = face;
        face->prev = brush->last_face;
    }

    brush->last_face = face;
    brush->face_count++;

    return face;
}

struct ed_face_t *ed_GetFace(uint32_t index)
{
    struct ed_face_t *face;

    face = ds_slist_get_element(&ed_level_state.brush.brush_faces, index);

    if(face && face->index == 0xffffffff)
    {
        face = NULL;
    }

    return face;
}

void ed_FreeFace(struct ed_brush_t *brush, struct ed_face_t *face)
{
    if(face && face->index != 0xffffffff)
    {
        ds_slist_remove_element(&ed_level_state.brush.brush_faces, face->index);
        face->index = 0xffffffff;
        brush->face_count--;
    }
}

struct ed_face_polygon_t *ed_AllocFacePolygon(struct ed_brush_t *brush, struct ed_face_t *face)
{
    uint32_t index = ds_slist_add_element(&ed_level_state.brush.brush_face_polygons, NULL);
    struct ed_face_polygon_t *polygon = ds_slist_get_element(&ed_level_state.brush.brush_face_polygons, index);

    polygon->index = index;
    polygon->face = face;
    polygon->brush = brush;
    polygon->next = NULL;
    polygon->prev = NULL;
    polygon->edges = NULL;
    polygon->last_edge = NULL;
    polygon->edge_count = 0;

    if(!face->polygons)
    {
        face->polygons = polygon;
    }
    else
    {
        face->last_polygon->next = polygon;
        polygon->prev = face->last_polygon;
    }

    face->last_polygon = polygon;
    brush->polygon_count++;

    return polygon;
}

void ed_FreeFacePolygon(struct ed_brush_t *brush, struct ed_face_polygon_t *polygon)
{
    if(polygon && polygon->index != 0xffffffff)
    {
        ds_slist_remove_element(&ed_level_state.brush.brush_face_polygons, polygon->index);
        polygon->index = 0xffffffff;
        brush->polygon_count--;
    }
}

void ed_LinkFacePolygonEdge(struct ed_face_polygon_t *polygon, struct ed_edge_t *edge)
{
    uint32_t polygon_side = edge->polygons[1].polygon == polygon;
    edge->polygons[polygon_side].polygon = polygon;

    if(!polygon->edges)
    {
        polygon->edges = edge;
    }
    else
    {
        edge->polygons[polygon_side].prev = polygon->last_edge;
        polygon_side = polygon->last_edge->polygons[1].polygon == polygon;
        polygon->last_edge->polygons[polygon_side].next = edge;
    }
    polygon->edge_count++;
    polygon->last_edge = edge;
}

void ed_UnlinkFacePolygonEdge(struct ed_face_polygon_t *polygon, struct ed_edge_t *edge)
{
    uint32_t polygon_side = edge->polygons[1].polygon == polygon;

    if(polygon->edges == edge)
    {
        polygon->edges = edge->polygons[polygon_side].next;
    }
    else
    {
        uint32_t prev_polygon_side = edge->polygons[polygon_side].prev->polygons[1].polygon == polygon;
        edge->polygons[polygon_side].prev->polygons[prev_polygon_side].next = edge->polygons[polygon_side].next;
    }

    if(polygon->last_edge == edge)
    {
        polygon->last_edge = edge->polygons[polygon_side].prev;
    }
    else
    {
        uint32_t next_polygon_side = edge->polygons[polygon_side].next->polygons[1].polygon == polygon;
        edge->polygons[polygon_side].next->polygons[next_polygon_side].prev = edge->polygons[polygon_side].prev;
    }

    polygon->edge_count--;
}

void ed_RemoveFacePolygon(struct ed_brush_t *brush, struct ed_face_polygon_t *polygon)
{
    if(polygon && polygon->index != 0xffffffff)
    {
        if(!polygon->prev)
        {

        }

        ed_FreeFacePolygon(brush, polygon);
    }
}

struct ed_edge_t *ed_AllocEdge(struct ed_brush_t *brush)
{
    uint32_t index = ds_slist_add_element(&ed_level_state.brush.brush_edges, NULL);
    struct ed_edge_t *edge = ds_slist_get_element(&ed_level_state.brush.brush_edges, index);

    edge->index = index;
    edge->brush = NULL;
    edge->init_next = NULL;
    edge->verts[0].vert = NULL;
    edge->verts[1].vert = NULL;
    edge->polygons[0].next = NULL;
    edge->polygons[0].prev = NULL;
    edge->polygons[0].polygon = NULL;
    edge->polygons[1].next = NULL;
    edge->polygons[1].prev = NULL;
    edge->polygons[1].polygon = NULL;
    edge->s_index = 0xffffffff;
    edge->model_start_flag = 0;

    brush->edge_count++;

    return edge;
}

struct ed_edge_t *ed_GetEdge(uint32_t index)
{
    struct ed_edge_t *edge;

    edge = ds_slist_get_element(&ed_level_state.brush.brush_edges, index);

    if(edge && edge->index == 0xffffffff)
    {
        edge = NULL;
    }

    return edge;
}

void ed_FreeEdge(struct ed_brush_t *brush, struct ed_edge_t *edge)
{
    if(edge && edge->index != 0xffffffff)
    {
        ds_slist_remove_element(&ed_level_state.brush.brush_edges, edge->index);
        edge->index = 0xffffffff;
        brush->edge_count--;
    }
}

struct ed_vert_t *ed_AllocVert(struct ed_brush_t *brush)
{
    struct ed_vert_t *vert = NULL;

    if(brush)
    {
        uint32_t index = ds_slist_add_element(&brush->vertices, NULL);
        vert = ds_slist_get_element(&brush->vertices, index);
        vert->index = index;
        vert->edges = NULL;
        ed_level_state.brush.brush_vert_count++;
    }

    return vert;
}

struct ed_vert_t *ed_GetVert(struct ed_brush_t *brush, uint32_t index)
{
    struct ed_vert_t *vert = NULL;

    if(brush)
    {
        vert = ds_slist_get_element(&brush->vertices, index);

        if(vert && vert->index == 0xffffffff)
        {
            vert = NULL;
        }
    }

    return vert;
}

void ed_LinkVertEdge(struct ed_vert_t *vert, struct ed_edge_t *edge)
{
    uint32_t vert_side = edge->verts[1].vert == vert;

    edge->verts[vert_side].next = vert->edges;
    edge->verts[vert_side].edge = edge;
    if(vert->edges)
    {
        vert->edges->prev = &edge->verts[vert_side];
    }
    vert->edges = &edge->verts[vert_side];
}

void ed_FreeVert(struct ed_brush_t *brush, uint32_t index)
{
    if(brush)
    {
        struct ed_vert_t *vertex = ds_slist_get_element(&brush->vertices, index);

        if(vertex && vertex->index != 0xffffffff)
        {
            ds_slist_remove_element(&brush->vertices, index);
            vertex->index = 0xffffffff;
            ed_level_state.brush.brush_vert_count--;
        }
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
            transform->rotation = vec3_t_c(0.0, 0.0, 0.0);
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

void ed_SetFaceMaterial(struct ed_brush_t *brush, uint32_t face_index, struct ed_brush_batch_t *material)
{
    if(brush)
    {
        struct ed_face_t *face = ed_GetFace(face_index);
        face->brush->update_flags |= ED_BRUSH_UPDATE_FLAG_FACE_POLYGONS;
        face->material = material;
    }
}

void ed_TranslateBrushFace(struct ed_brush_t *brush, uint32_t face_index, vec3_t *translation)
{
    struct ed_face_t *face = ed_GetFace(face_index);

    if(face)
    {
        face->brush->update_flags |= ED_BRUSH_UPDATE_FLAG_TRANSFORM_VERTS;
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

                struct ed_vert_transform_t *transform = ed_FindVertTransform(brush, edge->verts[polygon_index].vert->index);
                transform->translation = local_translation;
                edge->polygons[!polygon_index].polygon->face->flags |= ED_FACE_FLAG_GEOMETRY_MODIFIED;
                edge = edge->polygons[polygon_index].next;
            }

            polygon = polygon->next;
        }
    }
}

void ed_TranslateBrushEdge(struct ed_brush_t *brush, uint32_t edge_index, vec3_t *translation)
{
    struct ed_edge_t *edge = ed_GetEdge(edge_index);

    if(edge)
    {
        mat3_t brush_orientation;
        vec3_t local_translation;

        mat3_t_transpose(&brush_orientation, &brush->orientation);
        mat3_t_vec3_t_mul(&local_translation, translation, &brush_orientation);

        struct ed_vert_transform_t *vert_transform;
        vert_transform = ed_FindVertTransform(brush, edge->verts[0].vert->index);
        vert_transform->translation = local_translation;
        vert_transform = ed_FindVertTransform(brush, edge->verts[1].vert->index);
        vert_transform->translation = local_translation;
    }
}

void ed_RotateBrushFace(struct ed_brush_t *brush, uint32_t face_index, mat3_t *rotation)
{
    struct ed_face_t *face = ed_GetFace(face_index);

    if(face)
    {
        face->brush->update_flags |= ED_BRUSH_UPDATE_FLAG_TRANSFORM_VERTS;
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
                vec3_t vert = edge->verts[polygon_index].vert->vert;
                vec3_t_sub(&vert, &vert, &polygon->center);
                vec3_t translation;

                mat3_t_vec3_t_mul(&translation, &vert, &local_rotation);
                vec3_t_sub(&translation, &translation, &vert);

                struct ed_face_polygon_t *neighbor = edge->polygons[!polygon_index].polygon;
//                vec3_t_mul(&translation, &polygon->normal, -vec3_t_dot(&polygon->normal, &translation));

                struct ed_vert_transform_t *transform = ed_FindVertTransform(brush, edge->verts[polygon_index].vert->index);
                transform->rotation = translation;
                edge->polygons[!polygon_index].polygon->face->flags |= ED_FACE_FLAG_GEOMETRY_MODIFIED;
                edge = edge->polygons[polygon_index].next;
            }

            polygon = polygon->next;
        }
    }
}

void ed_UpdateBrushEntity(struct ed_brush_t *brush)
{
    if(!brush->entity)
    {
        struct e_ent_def_t ent_def = {};
        ent_def.model = brush->model;
        ent_def.scale = vec3_t_c(1.0, 1.0, 1.0);
        ent_def.collider.type = P_COLLIDER_TYPE_LAST;
        brush->entity = e_SpawnEntity(&ent_def, &brush->position, &vec3_t_c(1.0, 1.0, 1.0), &brush->orientation);
    }
    else
    {
        struct e_node_t *transform = brush->entity->node;
        transform->position = brush->position;
        transform->orientation = brush->orientation;
    }
}

void ed_UpdateBrush(struct ed_brush_t *brush)
{
    struct r_model_geometry_t geometry = {};
    struct ed_polygon_t *polygons = NULL;
    uint32_t rebuild_bsp = 1;

    vec3_t axes[] =
    {
        vec3_t_c(1.0, 0.0, 0.0),
        vec3_t_c(0.0, 1.0, 0.0),
        vec3_t_c(0.0, 0.0, 1.0),
    };

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
            struct ed_vert_t *brush_vert = ed_GetVert(brush, transform->index);
            vec3_t_add(&brush_vert->vert, &brush_vert->vert, &transform->translation);
            vec3_t_add(&brush_vert->vert, &brush_vert->vert, &transform->rotation);
        }

        /* find out how much the new brush center moved away from the old one */
        for(uint32_t vert_index = 0; vert_index < brush->vertices.cursor; vert_index++)
        {
            struct ed_vert_t *vert = ed_GetVert(brush, vert_index);
            vec3_t_add(&vert_translation, &vert_translation, &vert->vert);
        }

        vec3_t_div(&vert_translation, &vert_translation, (float)brush->vertices.cursor);
        /* translate verts so the new center becomes the current center */
        for(uint32_t vert_index = 0; vert_index < brush->vertices.cursor; vert_index++)
        {
            struct ed_vert_t *vert = ed_GetVert(brush, vert_index);
            vec3_t_sub(&vert->vert, &vert->vert, &vert_translation);
        }

        /* translate the brush accordingly, so it visibly stays at the same place */
        mat3_t_vec3_t_mul(&vert_translation, &vert_translation, &brush->orientation);
        vec3_t_add(&brush->position, &brush->position, &vert_translation);

        /* we touched the vertices, so we'll need to do a bunch of updating now... */
        brush->flags |= ED_BRUSH_FLAG_GEOMETRY_MODIFIED;
        brush->update_flags |= ED_BRUSH_UPDATE_FLAG_FACE_POLYGONS;
    }

    brush->vert_transforms.cursor = 0;
    struct ed_face_t *face = NULL;

    if(brush->update_flags & ED_BRUSH_UPDATE_FLAG_FACE_POLYGONS)
    {
        face = brush->faces;

        while(face)
        {
            face->center = vec3_t_c(0, 0, 0);
            struct ed_face_polygon_t *face_polygon = face->polygons;
            struct ed_face_polygon_t *best_uv_origin = NULL;
            float best_sqrd_origin_dist = 0.0;
            uint32_t point_count = 0;
            /* recompute center/normal/tangent for each polygon of this face */
            while(face_polygon)
            {
                struct ed_edge_t *first_edge = face_polygon->edges;
                uint32_t polygon_side = first_edge->polygons[1].polygon == face_polygon;
                struct ed_edge_t *second_edge = first_edge->polygons[polygon_side].next;

                vec3_t edge0_vec;
                vec3_t edge1_vec;

                struct ed_vert_t *vert0 = first_edge->verts[polygon_side].vert;
                struct ed_vert_t *vert1 = first_edge->verts[!polygon_side].vert;
                polygon_side = second_edge->polygons[1].polygon == face_polygon;
                struct ed_vert_t *vert2 = second_edge->verts[!polygon_side].vert;

                vec3_t_sub(&edge0_vec, &vert1->vert, &vert0->vert);
                vec3_t_sub(&edge1_vec, &vert2->vert, &vert1->vert);

                vec3_t_cross(&face_polygon->normal, &edge1_vec, &edge0_vec);
                vec3_t_normalize(&face_polygon->normal, &face_polygon->normal);

                face_polygon->tangent = edge0_vec;
                vec3_t_normalize(&face_polygon->tangent, &face_polygon->tangent);

                face_polygon->center = vec3_t_c(0.0, 0.0, 0.0);

                while(first_edge)
                {
                    polygon_side = first_edge->polygons[1].polygon == face_polygon;
                    struct ed_vert_t *vert = first_edge->verts[polygon_side].vert;
                    vec3_t_add(&face_polygon->center, &face_polygon->center, &vert->vert);
                    first_edge = first_edge->polygons[polygon_side].next;
                }

                vec3_t_div(&face_polygon->center, &face_polygon->center, (float)face_polygon->edge_count);

                vec3_t_add(&face->center, &face->center, &face_polygon->center);
                point_count++;


                face_polygon = face_polygon->next;
            }

            vec3_t_div(&face->center, &face->center, (float)point_count);

            face = face->next;
        }

        brush->update_flags |= ED_BRUSH_UPDATE_FLAG_CLIPPED_POLYGONS;
    }

    if(brush->update_flags & ED_BRUSH_UPDATE_FLAG_CLIPPED_POLYGONS)
    {
        face = brush->faces;

        while(face)
        {
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

            brush->clipped_vert_count += face->clipped_vert_count;
            brush->clipped_index_count += face->clipped_index_count;
            brush->clipped_polygon_count += face->clipped_polygon_count;

            face = face->next;
        }

        brush->update_flags |= ED_BRUSH_UPDATE_FLAG_UV_COORDS;
    }

    if(brush->update_flags & ED_BRUSH_UPDATE_FLAG_UV_COORDS)
    {
        struct ed_face_t *face = brush->faces;

        mat3_t inverse_orientation;
        mat3_t_transpose(&inverse_orientation, &brush->orientation);

        while(face)
        {
            struct ed_bsp_polygon_t *clipped_polygon = face->clipped_polygons;

            while(clipped_polygon)
            {
                vec3_t plane_origin;
                mat3_t plane_orientation;
                float max_axis_proj = -FLT_MAX;
                uint32_t j_axis_index = 0;

                plane_orientation.rows[1] = clipped_polygon->normal;
                mat3_t_vec3_t_mul(&plane_orientation.rows[1], &plane_orientation.rows[1], &brush->orientation);

                for(uint32_t comp_index = 0; comp_index < 3; comp_index++)
                {
                    float axis_proj = fabsf(plane_orientation.rows[1].comps[comp_index]);

                    if(axis_proj > max_axis_proj)
                    {
                        max_axis_proj = axis_proj;
                        j_axis_index = comp_index;
                    }
                }

                uint32_t k_axis_index = (j_axis_index + 1) % 3;
                vec3_t_cross(&plane_orientation.rows[0], &axes[k_axis_index], &plane_orientation.rows[1]);
                vec3_t_normalize(&plane_orientation.rows[0], &plane_orientation.rows[0]);

                vec3_t_cross(&plane_orientation.rows[2], &plane_orientation.rows[1], &plane_orientation.rows[0]);
                vec3_t_normalize(&plane_orientation.rows[2], &plane_orientation.rows[2]);
                vec3_t_mul(&plane_origin, &plane_orientation.rows[1], vec3_t_dot(&plane_orientation.rows[1], &clipped_polygon->face_polygon->center));
                vec3_t_sub(&plane_origin, &plane_origin, &brush->position);
                mat3_t_mul(&plane_orientation, &plane_orientation, &inverse_orientation);
                mat3_t_vec3_t_mul(&plane_origin, &plane_origin, &inverse_orientation);

                for(uint32_t vert_index = 0; vert_index < clipped_polygon->vertices.cursor; vert_index++)
                {
                    struct r_vert_t *vert = ds_list_get_element(&clipped_polygon->vertices, vert_index);
                    vec3_t vert_vec;

                    vec3_t_sub(&vert_vec, &vert->pos, &plane_origin);
                    vert->tex_coords.x = vec3_t_dot(&vert_vec, &plane_orientation.rows[0]);
                    vert->tex_coords.y = vec3_t_dot(&vert_vec, &plane_orientation.rows[2]);
                }

                clipped_polygon = clipped_polygon->next;
            }

            face = face->next;
        }

        brush->update_flags |= ED_BRUSH_UPDATE_FLAG_DRAW_GEOMETRY;
    }

    if(brush->update_flags & ED_BRUSH_UPDATE_FLAG_UV_COORDS)
    {
        struct ds_buffer_t *batch_buffer;
        struct ds_buffer_t *vertex_buffer;
        struct ds_buffer_t *index_buffer;
        struct ds_buffer_t *polygon_buffer;

        if(ed_level_state.brush.polygon_buffer.buffer_size < brush->clipped_polygon_count)
        {
            ds_buffer_resize(&ed_level_state.brush.polygon_buffer, brush->clipped_polygon_count);
        }

        if(ed_level_state.brush.vertex_buffer.buffer_size < brush->clipped_vert_count)
        {
            ds_buffer_resize(&ed_level_state.brush.vertex_buffer, brush->clipped_vert_count);
        }

        if(ed_level_state.brush.index_buffer.buffer_size < brush->clipped_index_count)
        {
            ds_buffer_resize(&ed_level_state.brush.index_buffer, brush->clipped_index_count);
        }

        polygon_buffer = &ed_level_state.brush.polygon_buffer;
        vertex_buffer = &ed_level_state.brush.vertex_buffer;
        index_buffer = &ed_level_state.brush.index_buffer;
        batch_buffer = &ed_level_state.brush.batch_buffer;

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

                if(batch->material == polygon->face_polygon->face->material->batch.material)
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

            uint32_t vert_count = polygon->vertices.cursor;
            uint32_t triangle_count = (vert_count - 2);
            uint32_t root_vertex = vertex_count;
            struct ed_face_polygon_t *face_polygon = polygon->face_polygon;
            struct ed_edge_t *edge = face_polygon->edges;
            uint32_t edge_start = polygon->model_start;
            uint32_t vert_offset = 2;

            while(edge)
            {
                uint32_t polygon_side = edge->polygons[1].polygon == face_polygon;

                if(!polygon_side)
                {
                    edge->model_start = edge_start;
                }

                edge_start++;
                vert_offset--;
                if(!vert_offset)
                {
                    edge_start++;
                    vert_offset = 2;
                }

                edge = edge->polygons[polygon_side].next;
            }

            /* to allow edge pickables to work properly, we need to triangulate
            the faces in such a way that the first vertex of the last external edge
            isn't the last in the list. Here we'll build a triangle fan. */

            /* triangulate first triangle, since this is a special case
            where the root vertex will come first */
            uint32_t vert_index = root_vertex + 1;
            batch_indices[polygon_batch->count] = root_vertex;
            polygon_batch->count++;
            batch_indices[polygon_batch->count] = vert_index;
            vert_index++;
            polygon_batch->count++;
            batch_indices[polygon_batch->count] = vert_index;
            polygon_batch->count++;

            vert_count -= 3;

            while(vert_count)
            {
                /* fill indices in a way that makes the root vertex be
                the last every time */
                batch_indices[polygon_batch->count] = vert_index;
                polygon_batch->count++;
                vert_index++;

                batch_indices[polygon_batch->count] = vert_index;
                polygon_batch->count++;

                batch_indices[polygon_batch->count] = root_vertex;
                polygon_batch->count++;

                vert_count--;
            }

            polygon->model_count = polygon_batch->count - polygon->model_count;
            index_count += polygon->model_count;

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
            brush->model = r_CreateModel(&geometry, NULL, "brush_model");
        }
        else
        {
//            ed_level_state.brush.brush_model_vert_count -= brush->model->verts.buffer_size;
//            ed_level_state.brush.brush_model_index_count -= brush->model->indices.buffer_size;
//
//            for(uint32_t batch_index = 0; batch_index < brush->model->batches.buffer_size; batch_index++)
//            {
//                struct r_batch_t *batch = (struct r_batch_t *)brush->model->batches.buffer + batch_index;
//                struct ed_brush_batch_t *global_batch = ed_GetGlobalBrushBatch(batch->material);
//                global_batch->batch.count -= batch->count;
//            }

            r_UpdateModelGeometry(brush->model, &geometry);
        }

//        ed_level_state.brush.brush_model_vert_count += brush->model->verts.buffer_size;
//        ed_level_state.brush.brush_model_index_count += brush->model->indices.buffer_size;
//
//        for(uint32_t batch_index = 0; batch_index < brush->model->batches.buffer_size; batch_index++)
//        {
//            struct r_batch_t *batch = (struct r_batch_t *)brush->model->batches.buffer + batch_index;
//            struct ed_brush_batch_t *global_batch = ed_GetGlobalBrushBatch(batch->material);
//            global_batch->batch.count += batch->count;
//        }
    }

    brush->update_flags = 0;

    ed_UpdateBrushEntity(brush);

    brush->flags = 0;
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

    index = ds_slist_add_element(&ed_level_state.brush.bsp_nodes, NULL);
    node = ds_slist_get_element(&ed_level_state.brush.bsp_nodes, index);

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
        ds_slist_remove_element(&ed_level_state.brush.bsp_nodes, node->index);
        node->index = 0xffffffff;
    }
}

void ed_FreeBspTree(struct ed_bsp_node_t *bsp)
{
    if(bsp->front)
    {
        ed_FreeBspTree(bsp->front);
    }

    if(bsp->back)
    {
        ed_FreeBspTree(bsp->back);
    }

    ed_FreeBspPolygon(bsp->splitter, 1);
    ed_FreeBspNode(bsp);
}

struct ed_bsp_polygon_t *ed_BspPolygonsFromBrush(struct ed_brush_t *brush)
{
    struct ed_face_t *face = brush->faces;
    struct ed_bsp_polygon_t *polygons = NULL;

    while(face)
    {
        struct ed_bsp_polygon_t *face_polygons = ed_CopyBspPolygons(face->clipped_polygons);
        struct ed_bsp_polygon_t *last_polygon = NULL;
        struct ed_bsp_polygon_t *next_last_polygon = face_polygons;

        do
        {
            for(uint32_t vert_index = 0; vert_index < next_last_polygon->vertices.cursor; vert_index++)
            {
                struct r_vert_t *vert = ds_list_get_element(&next_last_polygon->vertices, vert_index);

                mat3_t_vec3_t_mul(&vert->pos, &vert->pos, &brush->orientation);
                vec3_t_add(&vert->pos, &vert->pos, &brush->position);

                mat3_t_vec3_t_mul(&vert->normal.xyz, &vert->normal.xyz, &brush->orientation);
                mat3_t_vec3_t_mul(&vert->tangent, &vert->tangent, &brush->orientation);
            }

            mat3_t_vec3_t_mul(&next_last_polygon->point, &next_last_polygon->point, &brush->orientation);
            vec3_t_add(&next_last_polygon->point, &next_last_polygon->point, &brush->position);
            mat3_t_vec3_t_mul(&next_last_polygon->normal, &next_last_polygon->normal, &brush->orientation);

            last_polygon = next_last_polygon;
            next_last_polygon = next_last_polygon->next;
        }
        while(next_last_polygon);

        last_polygon->next = polygons;

        if(polygons)
        {
            polygons->prev = last_polygon;
        }
        polygons = face_polygons;

        face = face->next;
    }

    return polygons;
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

        uint32_t polygon_side = edge->polygons[1].polygon == face_polygon;
        struct ed_vert_t *first_vert = edge->verts[polygon_side].vert;

        while(edge)
        {
            polygon_side = edge->polygons[1].polygon == face_polygon;
            struct ed_vert_t *brush_vert = edge->verts[polygon_side].vert;
            struct r_vert_t *polygon_vert = ds_list_get_element(polygon_verts, ds_list_add_element(polygon_verts, NULL));

            vec3_t_add(&bsp_polygon->point, &bsp_polygon->point, &brush_vert->vert);
            polygon_vert->pos = brush_vert->vert;
            polygon_vert->normal.xyz = face_polygon->normal;
            polygon_vert->tangent = face_polygon->tangent;

            edge = edge->polygons[polygon_side].next;
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
    uint32_t index = ds_slist_add_element(&ed_level_state.brush.bsp_polygons, NULL);
    struct ed_bsp_polygon_t *polygon = ds_slist_get_element(&ed_level_state.brush.bsp_polygons, index);

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

struct ed_bsp_polygon_t *ed_CopyBspPolygons(struct ed_bsp_polygon_t *src_polygons)
{
    struct ed_bsp_polygon_t *dst_polygons = NULL;
    struct ed_bsp_polygon_t *src_polygon = src_polygons;

    while(src_polygon)
    {
        struct ed_bsp_polygon_t *dst_polygon = ed_AllocBspPolygon(0);
        for(uint32_t vert_index = 0; vert_index < src_polygon->vertices.cursor; vert_index++)
        {
            struct r_vert_t *vert = ds_list_get_element(&src_polygon->vertices, vert_index);
            ds_list_add_element(&dst_polygon->vertices, vert);
        }

        dst_polygon->face_polygon = src_polygon->face_polygon;
        dst_polygon->point = src_polygon->point;
        dst_polygon->normal = src_polygon->normal;

        dst_polygon->next = dst_polygons;
        if(dst_polygons)
        {
            dst_polygons->prev = dst_polygon;
        }
        dst_polygons = dst_polygon;
        src_polygon = src_polygon->next;
    }

    return dst_polygons;
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

        ds_slist_remove_element(&ed_level_state.brush.bsp_polygons, polygon->index);
        polygon->index = 0xffffffff;
    }
}

void ed_FreeBspPolygons(struct ed_bsp_polygon_t *polygons)
{
    while(polygons)
    {
        struct ed_bsp_polygon_t *next_polygon = polygons->next;
        ed_FreeBspPolygon(polygons, 1);
        polygons = next_polygon;
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

    for(uint32_t vert_index = 0; vert_index < polygon->vertices.cursor; vert_index++)
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

uint32_t ed_PointOnSplitter(vec3_t *point, vec3_t *plane_point, vec3_t *plane_normal)
{
    vec3_t point_vec;
    vec3_t_sub(&point_vec, point, plane_point);

    float dist = vec3_t_dot(&point_vec, plane_normal);

    if(dist > ED_BSP_DELTA)
    {
        return ED_SPLITTER_SIDE_FRONT;
    }
    else if(dist < -ED_BSP_DELTA)
    {
        return ED_SPLITTER_SIDE_BACK;
    }

    return ED_SPLITTER_SIDE_ON_FRONT;
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

void ed_SplitPolygon(struct ed_bsp_polygon_t *polygon, vec3_t *point, vec3_t *normal, struct ed_bsp_polygon_t **front, struct ed_bsp_polygon_t **back)
{
    struct ds_list_t *verts = &polygon->vertices;

    struct ed_bsp_polygon_t *side_polygons[2];

    side_polygons[0] = ed_AllocBspPolygon(1);
    side_polygons[1] = ed_AllocBspPolygon(1);

    struct ed_bsp_polygon_t *cur_side_polygon;
    uint32_t cur_side_index;

    for(uint32_t vert_index = 0; vert_index < verts->cursor; vert_index++)
    {
        struct r_vert_t *vert0 = ds_list_get_element(verts, vert_index);
        struct r_vert_t *vert1 = ds_list_get_element(verts, (vert_index + 1) % verts->cursor);

        vec3_t vert_vec;
        vec3_t_sub(&vert_vec, &vert0->pos, point);
        float dist0 = vec3_t_dot(&vert_vec, normal);

        vec3_t_sub(&vert_vec, &vert1->pos, point);
        float dist1 = vec3_t_dot(&vert_vec, normal);

        if(dist0 >= 0.0)
        {
            cur_side_index = 0;
        }
        else
        {
            cur_side_index = 1;
        }

        cur_side_polygon = side_polygons[cur_side_index];

        if(dist0 * dist1 <= 0)
        {
            float denom = dist1 - dist0;
            float time = fabs(dist0) / fabs(denom);

            struct r_vert_t new_vert = {};

            vec3_t_lerp(&new_vert.pos, &vert0->pos, &vert1->pos, time);
            vec4_t_lerp(&new_vert.normal, &vert0->normal, &vert1->normal, time);
            vec3_t_lerp(&new_vert.tangent, &vert0->tangent, &vert1->tangent, time);
            vec2_t_lerp(&new_vert.tex_coords, &vert0->tex_coords, &vert1->tex_coords, time);

            ds_list_add_element(&cur_side_polygon->vertices, vert0);
            ds_list_add_element(&cur_side_polygon->vertices, &new_vert);
            cur_side_polygon = side_polygons[!cur_side_index];
            ds_list_add_element(&cur_side_polygon->vertices, &new_vert);
            ds_list_add_element(&cur_side_polygon->vertices, vert1);
        }
        else
        {
            ds_list_add_element(&cur_side_polygon->vertices, vert0);
        }
    }

    for(uint32_t polygon_index = 0; polygon_index < 2; polygon_index++)
    {
        side_polygons[polygon_index]->face_polygon = polygon->face_polygon;
        side_polygons[polygon_index]->normal = polygon->normal;
        side_polygons[polygon_index]->point = polygon->point;
        side_polygons[polygon_index]->used = polygon->used;
    }


    *front = side_polygons[0];
    *back = side_polygons[1];
}

struct ed_bsp_node_t *ed_SolidBspFromPolygons(struct ed_bsp_polygon_t *polygons)
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
                case ED_SPLITTER_SIDE_ON_FRONT:
                case ED_SPLITTER_SIDE_ON_BACK:
                    side = ED_SPLITTER_SIDE_FRONT;
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

                case ED_SPLITTER_SIDE_STRADDLE:
                {
                    struct ed_bsp_polygon_t *front;
                    struct ed_bsp_polygon_t *back;
                    ed_SplitPolygon(splitter, &splitter->point, &splitter->normal, &front, &back);
                    ed_FreeBspPolygon(polygon, 1);

                    front->next = side_lists[ED_SPLITTER_SIDE_FRONT];
                    if(side_lists[ED_SPLITTER_SIDE_FRONT])
                    {
                        side_lists[ED_SPLITTER_SIDE_FRONT]->prev = front;
                    }
                    side_lists[ED_SPLITTER_SIDE_FRONT] = front;

                    back->next = side_lists[ED_SPLITTER_SIDE_BACK];
                    if(side_lists[ED_SPLITTER_SIDE_BACK])
                    {
                        side_lists[ED_SPLITTER_SIDE_BACK]->prev = back;
                    }
                    side_lists[ED_SPLITTER_SIDE_BACK] = back;
                }
                break;
            }
        }

        if(side_lists[ED_SPLITTER_SIDE_FRONT])
        {
            node->front = ed_SolidBspFromPolygons(side_lists[ED_SPLITTER_SIDE_FRONT]);
        }

        if(side_lists[ED_SPLITTER_SIDE_BACK])
        {
            node->back = ed_SolidBspFromPolygons(side_lists[ED_SPLITTER_SIDE_BACK]);
        }
    }

    return node;
}

struct ed_bsp_polygon_t *ed_ClipPolygonToBsp(struct ed_bsp_polygon_t *polygons, struct ed_bsp_node_t *bsp)
{
    struct ed_bsp_polygon_t *side_lists[2] = {NULL, NULL};
    struct ed_bsp_polygon_t *splitter = bsp->splitter;
    struct ed_bsp_polygon_t *clipped_polygons = NULL;

    while(polygons)
    {
        struct ed_bsp_polygon_t *polygon = polygons;
        ed_UnlinkPolygon(polygon, &polygons);
        uint32_t side = ed_PolygonOnSplitter(polygon, &splitter->point, &splitter->normal);

        switch(side)
        {
            case ED_SPLITTER_SIDE_ON_FRONT:
            case ED_SPLITTER_SIDE_ON_BACK:
                side = ED_SPLITTER_SIDE_FRONT;
            case ED_SPLITTER_SIDE_FRONT:
            case ED_SPLITTER_SIDE_BACK:
                polygon->next = side_lists[side];
                if(side_lists[side])
                {
                    side_lists[side]->prev = polygon;
                }
                side_lists[side] = polygon;
            break;

            case ED_SPLITTER_SIDE_STRADDLE:
            {
                struct ed_bsp_polygon_t *front;
                struct ed_bsp_polygon_t *back;

                ed_SplitPolygon(polygon, &splitter->point, &splitter->normal, &front, &back);
                ed_FreeBspPolygon(polygon, 1);

                front->next = side_lists[ED_SPLITTER_SIDE_FRONT];
                if(side_lists[ED_SPLITTER_SIDE_FRONT])
                {
                    side_lists[ED_SPLITTER_SIDE_FRONT]->prev = front;
                }
                side_lists[ED_SPLITTER_SIDE_FRONT] = front;

                back->next = side_lists[ED_SPLITTER_SIDE_BACK];
                if(side_lists[ED_SPLITTER_SIDE_BACK])
                {
                    side_lists[ED_SPLITTER_SIDE_BACK]->prev = back;
                }
                side_lists[ED_SPLITTER_SIDE_BACK] = back;
            }
            break;
        }
    }

    if(side_lists[ED_SPLITTER_SIDE_FRONT])
    {
        if(bsp->front)
        {
            clipped_polygons = ed_ClipPolygonToBsp(side_lists[ED_SPLITTER_SIDE_FRONT], bsp->front);
        }
        else
        {
            clipped_polygons = side_lists[ED_SPLITTER_SIDE_FRONT];
        }
    }

    struct ed_bsp_polygon_t *back_clipped_polygons = NULL;
    if(side_lists[ED_SPLITTER_SIDE_BACK])
    {
        if(bsp->back)
        {
            back_clipped_polygons = ed_ClipPolygonToBsp(side_lists[ED_SPLITTER_SIDE_BACK], bsp->back);
        }
        else
        {
            back_clipped_polygons = side_lists[ED_SPLITTER_SIDE_BACK];
            while(back_clipped_polygons)
            {
                struct ed_bsp_polygon_t *next_back_polygon = back_clipped_polygons->next;
                ed_FreeBspPolygon(back_clipped_polygons, 1);
                back_clipped_polygons = next_back_polygon;
            }
        }
    }

    if(back_clipped_polygons)
    {
        struct ed_bsp_polygon_t *last_back_polygon = back_clipped_polygons;
        while(last_back_polygon->next)
        {
            last_back_polygon = last_back_polygon->next;
        }
        last_back_polygon->next = clipped_polygons;
        if(clipped_polygons)
        {
            clipped_polygons->prev = last_back_polygon;
        }
        clipped_polygons = back_clipped_polygons;
    }


    return clipped_polygons;
}

struct ed_bsp_polygon_t *ed_ClipPolygonLists(struct ed_bsp_polygon_t *polygons_a, struct ed_bsp_polygon_t *polygons_b)
{
    struct ed_bsp_polygon_t *clipped_polygons = polygons_a;

    if(polygons_a && polygons_b)
    {
        struct ed_bsp_node_t *bsp_a = ed_SolidBspFromPolygons(ed_CopyBspPolygons(polygons_a));
        struct ed_bsp_node_t *bsp_b = ed_SolidBspFromPolygons(ed_CopyBspPolygons(polygons_b));

        struct ed_bsp_polygon_t *clipped_a = ed_ClipPolygonToBsp(polygons_a, bsp_b);
        struct ed_bsp_polygon_t *clipped_b = ed_ClipPolygonToBsp(polygons_b, bsp_a);

        clipped_polygons = clipped_a;

        if(clipped_a)
        {
            struct ed_bsp_polygon_t *last_clipped = clipped_a;
            while(last_clipped->next)
            {
                last_clipped = last_clipped->next;
            }
            last_clipped->next = clipped_b;
        }
        else
        {
            clipped_polygons = clipped_b;
        }

        ed_FreeBspTree(bsp_a);
        ed_FreeBspTree(bsp_b);
    }
    else if(polygons_b)
    {
        clipped_polygons = polygons_b;
    }

    return clipped_polygons;
}





