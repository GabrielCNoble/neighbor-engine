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

//extern struct ds_slist_t ed_brushes;
//extern struct ds_list_t ed_global_brush_batches;
//extern uint32_t ed_global_brush_index_count;
//extern uint32_t ed_global_brush_vert_count;

extern struct ed_world_context_data_t ed_world_context_data;

struct ed_brush_t *ed_CreateBrush(vec3_t *position, mat3_t *orientation, vec3_t *size)
{
    uint32_t index;
    struct ed_brush_t *brush;
    vec3_t dims;
    vec3_t_fabs(&dims, size);

    index = ds_slist_add_element(&ed_world_context_data.brushes, NULL);
    brush = ds_slist_get_element(&ed_world_context_data.brushes, index);
    brush->index = index;

    brush->vertices = ds_buffer_create(sizeof(vec3_t), 8);
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
        ed_world_context_data.global_brush_vert_count -= brush->model->verts.buffer_size;
        ed_world_context_data.global_brush_index_count -= brush->model->indices.buffer_size;

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
        }

        ds_list_destroy(&brush->faces);
        ds_buffer_destroy(&brush->vertices);
        r_DestroyModel(brush->model);

        ds_slist_remove_element(&ed_world_context_data.brushes, brush->index);
        brush->index = 0xffffffff;
    }
}

struct ed_brush_batch_t *ed_GetGlobalBrushBatch(struct r_material_t *material)
{
    struct ed_brush_batch_t *batch = NULL;

    for(uint32_t batch_index = 0; batch_index < ed_world_context_data.global_brush_batches.cursor; batch_index++)
    {
        batch = ds_list_get_element(&ed_world_context_data.global_brush_batches, batch_index);

        if(batch->batch.material == material)
        {
            break;
        }
    }

    if(!batch)
    {
        uint32_t index = ds_list_add_element(&ed_world_context_data.global_brush_batches, NULL);
        batch = ds_list_get_element(&ed_world_context_data.global_brush_batches, index);
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
        brush = ds_slist_get_element(&ed_world_context_data.brushes, index);

        if(brush && brush->index == 0xffffffff)
        {
            brush = NULL;
        }
    }

    return brush;
}

void ed_UpdateBrush(struct ed_brush_t *brush)
{
    struct ed_polygon_t *polygons;
    struct r_model_geometry_t geometry = {};

    polygons = ed_PolygonsFromBrush(brush);
    brush->bsp = ed_BspFromPolygons(polygons);
    ed_GeometryFromBsp(&geometry, brush->bsp);

    if(!brush->model)
    {
        brush->model = r_CreateModel(&geometry, NULL);
    }
    else
    {
        ed_world_context_data.global_brush_vert_count -= brush->model->verts.buffer_size;
        ed_world_context_data.global_brush_index_count -= brush->model->indices.buffer_size;

        for(uint32_t batch_index = 0; batch_index < brush->model->batches.buffer_size; batch_index++)
        {
            struct r_batch_t *batch = (struct r_batch_t *)brush->model->batches.buffer + batch_index;
            struct ed_brush_batch_t *global_batch = ed_GetGlobalBrushBatch(batch->material);
            global_batch->batch.count -= batch->count;
        }

        r_UpdateModelGeometry(brush->model, &geometry);
    }

    ed_world_context_data.global_brush_vert_count += brush->model->verts.buffer_size;
    ed_world_context_data.global_brush_index_count += brush->model->indices.buffer_size;

    for(uint32_t batch_index = 0; batch_index < brush->model->batches.buffer_size; batch_index++)
    {
        struct r_batch_t *batch = (struct r_batch_t *)brush->model->batches.buffer + batch_index;
        struct ed_brush_batch_t *global_batch = ed_GetGlobalBrushBatch(batch->material);
        global_batch->batch.count += batch->count;
    }
}

void ed_BuildWorldGeometry()
{
    struct ds_buffer_t vertices = ds_buffer_create(sizeof(struct r_vert_t), ed_world_context_data.global_brush_vert_count);
    struct ds_buffer_t indices = ds_buffer_create(sizeof(uint32_t), ed_world_context_data.global_brush_index_count);
    struct ds_buffer_t batches = ds_buffer_create(sizeof(struct r_batch_t), ed_world_context_data.global_brush_batches.cursor);

    for(uint32_t global_batch_index = 0; global_batch_index < batches.buffer_size; global_batch_index++)
    {
        struct r_batch_t *batch = (struct r_batch_t *)batches.buffer + global_batch_index;
        struct ed_brush_batch_t *brush_batch = ds_list_get_element(&ed_world_context_data.global_brush_batches, global_batch_index);

        *batch = brush_batch->batch;

        if(global_batch_index)
        {
            struct r_batch_t *prev_batch = (struct r_batch_t *)batches.buffer + (global_batch_index - 1);
            batch->start = prev_batch->start + prev_batch->count;
        }

        batch->count = 0;

        for(uint32_t brush_index = 0; brush_index < ed_world_context_data.brushes.cursor; brush_index++)
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
