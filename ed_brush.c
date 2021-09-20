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

    index = ds_slist_add_element(&ed_w_ctx_data.brushes, NULL);
    brush = ds_slist_get_element(&ed_w_ctx_data.brushes, index);
    brush->index = index;

    brush->vertices = ds_buffer_create(sizeof(vec3_t), 8);
    brush->polygons = ds_slist_create(sizeof(struct ed_polygon_t), 16);
    brush->bsp_nodes = ds_slist_create(sizeof(struct ed_bspn_t), 16);
    vec3_t *vertices = (vec3_t *)brush->vertices.buffer;

    for(uint32_t vert_index = 0; vert_index < brush->vertices.buffer_size; vert_index++)
    {
        vertices[vert_index].x = dims.x * ed_cube_brush_vertices[vert_index].x;
        vertices[vert_index].y = dims.y * ed_cube_brush_vertices[vert_index].y;
        vertices[vert_index].z = dims.z * ed_cube_brush_vertices[vert_index].z;
    }

    brush->faces = ds_list_create(sizeof(struct ed_face_t), 6);

    brush->orientation = *orientation;
    brush->position = *position;

    for(uint32_t face_index = 0; face_index < brush->faces.size; face_index++)
    {
        ds_list_add_element(&brush->faces, NULL);
        struct ed_face_t *face = ds_list_get_element(&brush->faces, face_index);

        face->material = r_GetDefaultMaterial();
        face->normal = ed_cube_brush_normals[face_index];
        face->tangent = ed_cube_brush_tangents[face_index];
        face->indices = ds_buffer_create(sizeof(uint32_t), 4);
        ds_buffer_fill(&face->indices, 0, ed_cube_brush_indices[face_index], 4);
    }

    brush->model = NULL;

    ed_UpdateBrush(brush);

    return brush;
}

void ed_DestroyBrush(struct ed_brush_t *brush)
{
    if(brush)
    {
        ed_w_ctx_data.global_brush_vert_count -= brush->model->verts.buffer_size;
        ed_w_ctx_data.global_brush_index_count -= brush->model->indices.buffer_size;

        for(uint32_t batch_index = 0; batch_index < brush->model->batches.buffer_size; batch_index++)
        {
            struct r_batch_t *batch = (struct r_batch_t *)brush->model->batches.buffer + batch_index;
            struct ed_brush_batch_t *global_batch = ed_GetGlobalBrushBatch(batch->material);
            global_batch->batch.count -= batch->count;
        }

        for(uint32_t face_index = 0; face_index < brush->faces.cursor; face_index++)
        {
            struct ed_face_t *face = ds_list_get_element(&brush->faces, face_index);
            ds_buffer_destroy(&face->indices);

            if(face->polygon)
            {
                ds_buffer_destroy(&face->polygon->vertices);
            }

            if(face->clipped_polygons)
            {
                while(face->clipped_polygons)
                {
                    ds_buffer_destroy(&face->clipped_polygons->vertices);
                    face->clipped_polygons = face->clipped_polygons->next;
                }
            }
        }

        ds_list_destroy(&brush->faces);
        ds_buffer_destroy(&brush->vertices);
        ds_slist_destroy(&brush->polygons);
        ds_slist_destroy(&brush->bsp_nodes);
        r_DestroyModel(brush->model);

        ds_slist_remove_element(&ed_w_ctx_data.brushes, brush->index);
        brush->index = 0xffffffff;
    }
}

struct ed_brush_batch_t *ed_GetGlobalBrushBatch(struct r_material_t *material)
{
    struct ed_brush_batch_t *batch = NULL;

    for(uint32_t batch_index = 0; batch_index < ed_w_ctx_data.global_brush_batches.cursor; batch_index++)
    {
        batch = ds_list_get_element(&ed_w_ctx_data.global_brush_batches, batch_index);

        if(batch->batch.material == material)
        {
            break;
        }
    }

    if(!batch)
    {
        uint32_t index = ds_list_add_element(&ed_w_ctx_data.global_brush_batches, NULL);
        batch = ds_list_get_element(&ed_w_ctx_data.global_brush_batches, index);
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
        brush = ds_slist_get_element(&ed_w_ctx_data.brushes, index);

        if(brush && brush->index == 0xffffffff)
        {
            brush = NULL;
        }
    }

    return brush;
}

struct ed_polygon_t *ed_AllocPolygon(struct ed_brush_t *brush)
{
    struct ed_polygon_t *polygon = NULL;
    uint32_t index;

    if(brush)
    {
        index = ds_slist_add_element(&brush->polygons, NULL);
        polygon = ds_slist_get_element(&brush->polygons, index);
    }
    else
    {
        index = ds_slist_add_element(&ed_polygons, NULL);
        polygon = ds_slist_get_element(&ed_polygons, index);
    }

    if(!polygon->vertices.elem_size)
    {
        polygon->vertices = ds_buffer_create(sizeof(struct r_vert_t), 0);
    }

    polygon->index = index;
    polygon->prev = NULL;
    polygon->next = NULL;
    polygon->brush = brush;

    return polygon;
}

struct ed_polygon_t *ed_CopyPolygon(struct ed_polygon_t *src)
{
    struct ed_polygon_t *copy = NULL;

    if(src)
    {
        copy = ed_AllocPolygon(src->brush);

        copy->normal = src->normal;
        copy->material = src->material;

        if(copy->vertices.buffer_size < src->vertices.buffer_size)
        {
            ds_buffer_resize(&copy->vertices, src->vertices.buffer_size);
        }

        memcpy(copy->vertices.buffer, src->vertices.buffer, src->vertices.elem_size * src->vertices.buffer_size);
    }

    return copy;
}

void ed_FreePolygon(struct ed_polygon_t *polygon)
{
    if(polygon)
    {
        if(polygon->brush)
        {
            ds_slist_remove_element(&polygon->brush->polygons, polygon->index);
        }
        else
        {
            ds_slist_remove_element(&ed_polygons, polygon->index);
        }

        polygon->index = 0xffffffff;
    }
}

struct ed_bspn_t *ed_AllocBspNode(struct ed_brush_t *brush)
{
    struct ed_bspn_t *node = NULL;
    uint32_t index;

    if(brush)
    {
        index = ds_slist_add_element(&brush->bsp_nodes, NULL);
        node = ds_slist_get_element(&brush->bsp_nodes, index);
    }
    else
    {
        index = ds_slist_add_element(&ed_bsp_nodes, NULL);
        node = ds_slist_get_element(&ed_bsp_nodes, index);
    }

    node->front = NULL;
    node->back = NULL;
    node->splitter = NULL;
    node->brush = brush;
    node->index = index;

    return node;
}

void ed_FreeBspNode(struct ed_bspn_t *node)
{
    if(node)
    {
        if(node->brush)
        {
            ds_slist_remove_element(&node->brush->bsp_nodes, node->index);
        }
        else
        {
            ds_slist_remove_element(&ed_bsp_nodes, node->index);
        }

        node->index = 0xffffffff;
    }
}

int compare_polygons(const void *a, const void *b)
{
    struct ed_polygon_t *polygon_a = *(struct ed_polygon_t **)a;
    struct ed_polygon_t *polygon_b = *(struct ed_polygon_t **)b;

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

void ed_UpdateBrush(struct ed_brush_t *brush)
{
    struct r_model_geometry_t geometry = {};
    struct ed_polygon_t *polygons = NULL;
    uint32_t rebuild_bsp = 1;
    vec3_t *brush_verts = brush->vertices.buffer;

    brush->clipped_vert_count = 0;
    brush->clipped_index_count = 0;
    brush->clipped_polygon_count = 0;

    for(uint32_t face_index = 0; face_index < brush->faces.cursor; face_index++)
    {
        struct ed_face_t *face = ds_list_get_element(&brush->faces, face_index);

        if(!face->polygon)
        {
            face->polygon = ed_AllocPolygon(brush);
        }

        if(face->polygon->vertices.buffer_size < face->indices.buffer_size)
        {
            ds_buffer_resize(&face->polygon->vertices, face->indices.buffer_size);
        }

        uint32_t *face_indices = face->indices.buffer;
        struct r_vert_t *polygon_vertices = face->polygon->vertices.buffer;

        for(uint32_t vert_index = 0; vert_index < face->indices.buffer_size; vert_index++)
        {
            struct r_vert_t *vert = polygon_vertices + vert_index;
            vert->pos = brush_verts[face_indices[vert_index]];
            vert->normal = vec4_t_c(face->normal.x, face->normal.y, face->normal.z, 0.0);
            vert->tangent = face->tangent;
            vert->tex_coords = vec2_t_c(0.0, 0.0);
        }

        face->polygon->material = face->material;

        face->clipped_polygons = ed_CopyPolygon(face->polygon);
        face->clipped_vert_count = face->polygon->vertices.buffer_size;
        face->clipped_index_count = (face->clipped_vert_count - 2) * 3;
        face->clipped_polygon_count = 1;

        face->polygon->next = polygons;
        if(polygons)
        {
            polygons->prev = face->polygon;
        }
        polygons = face->polygon;

        brush->clipped_vert_count += face->clipped_vert_count;
        brush->clipped_index_count += face->clipped_index_count;
        brush->clipped_polygon_count++;
    }

    if(!brush->bsp || rebuild_bsp)
    {
        brush->bsp = ed_BrushBspFromPolygons(polygons);
    }

    struct ds_buffer_t batch_buffer;
    struct ds_buffer_t vertex_buffer;
    struct ds_buffer_t index_buffer;
    struct ds_buffer_t polygon_buffer;

    polygon_buffer = ds_buffer_create(sizeof(struct ed_polygon_t *), brush->clipped_polygon_count);
    vertex_buffer = ds_buffer_create(sizeof(struct r_vert_t), brush->clipped_vert_count);
    index_buffer = ds_buffer_create(sizeof(uint32_t), brush->clipped_index_count);
    batch_buffer = ds_buffer_create(sizeof(struct r_batch_t), 0);

    uint32_t polygon_index = 0;

    for(uint32_t face_index = 0; face_index < brush->faces.cursor; face_index++)
    {
        struct ed_face_t *face = ds_list_get_element(&brush->faces, face_index);
        struct ed_polygon_t *clipped_polygons = face->clipped_polygons;

        while(clipped_polygons)
        {
            ((struct ed_polygon_t **)polygon_buffer.buffer)[polygon_index] = clipped_polygons;
            polygon_index++;
            clipped_polygons = clipped_polygons->next;
        }
    }

    qsort(polygon_buffer.buffer, polygon_buffer.buffer_size, polygon_buffer.elem_size, compare_polygons);

    uint32_t *indices = (uint32_t *)index_buffer.buffer;
    struct r_vert_t *vertices = (struct r_vert_t *)vertex_buffer.buffer;
    struct r_batch_t *batches = (struct r_batch_t *)batch_buffer.buffer;
    uint32_t first_vertex = 0;

    for(uint32_t polygon_index = 0; polygon_index < polygon_buffer.buffer_size; polygon_index++)
    {
        struct ed_polygon_t *polygon = ((struct ed_polygon_t **)polygon_buffer.buffer)[polygon_index];
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
            ds_buffer_resize(&batch_buffer, batch_buffer.buffer_size + 1);
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

        polygon->mesh_start = polygon_batch->start + polygon_batch->count;
        polygon->mesh_count = polygon_batch->count;
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

        polygon->mesh_count = polygon_batch->count - polygon->mesh_count;

        first_vertex += polygon->vertices.buffer_size;
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
        ed_w_ctx_data.global_brush_vert_count -= brush->model->verts.buffer_size;
        ed_w_ctx_data.global_brush_index_count -= brush->model->indices.buffer_size;

        for(uint32_t batch_index = 0; batch_index < brush->model->batches.buffer_size; batch_index++)
        {
            struct r_batch_t *batch = (struct r_batch_t *)brush->model->batches.buffer + batch_index;
            struct ed_brush_batch_t *global_batch = ed_GetGlobalBrushBatch(batch->material);
            global_batch->batch.count -= batch->count;
        }

        r_UpdateModelGeometry(brush->model, &geometry);
    }

    ed_w_ctx_data.global_brush_vert_count += brush->model->verts.buffer_size;
    ed_w_ctx_data.global_brush_index_count += brush->model->indices.buffer_size;

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

void ed_BuildWorldGeometry()
{
    struct ds_buffer_t vertices = ds_buffer_create(sizeof(struct r_vert_t), ed_w_ctx_data.global_brush_vert_count);
    struct ds_buffer_t indices = ds_buffer_create(sizeof(uint32_t), ed_w_ctx_data.global_brush_index_count);
    struct ds_buffer_t batches = ds_buffer_create(sizeof(struct r_batch_t), ed_w_ctx_data.global_brush_batches.cursor);

    for(uint32_t global_batch_index = 0; global_batch_index < batches.buffer_size; global_batch_index++)
    {
        struct r_batch_t *batch = (struct r_batch_t *)batches.buffer + global_batch_index;
        struct ed_brush_batch_t *brush_batch = ds_list_get_element(&ed_w_ctx_data.global_brush_batches, global_batch_index);

        *batch = brush_batch->batch;

        if(global_batch_index)
        {
            struct r_batch_t *prev_batch = (struct r_batch_t *)batches.buffer + (global_batch_index - 1);
            batch->start = prev_batch->start + prev_batch->count;
        }

        batch->count = 0;

        for(uint32_t brush_index = 0; brush_index < ed_w_ctx_data.brushes.cursor; brush_index++)
        {
            struct ed_brush_t *brush = ed_GetBrush(brush_index);

            if(brush)
            {
                for(uint32_t model_batch_index = 0; model_batch_index < brush->model->batches.buffer_size; model_batch_index++)
                {
                    struct r_batch_t *model_batch = (struct r_batch_t *)brush->model->batches.buffer + model_batch_index;

                    if(model_batch->material == batch->material)
                    {

                    }
                }
            }
        }
    }
}
