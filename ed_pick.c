#include <stdio.h>

#include "ed_pick.h"
#include "ed_lev_editor.h"
#include "ed_brush.h"
#include "game.h"
#include "r_main.h"

extern struct ed_world_context_data_t ed_w_ctx_data;
extern struct r_shader_t *ed_picking_shader;
extern mat4_t r_view_projection_matrix;
extern mat4_t r_camera_matrix;
extern mat4_t r_projection_matrix;
extern int32_t r_width;
extern int32_t r_height;
extern uint32_t r_vertex_buffer;
extern uint32_t r_index_buffer;
extern uint32_t ed_picking_framebuffer;
extern struct r_model_t *ed_light_pickable_model;

void ed_BeginPicking()
{
    glBindBuffer(GL_ARRAY_BUFFER, r_vertex_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, r_index_buffer);
    r_BindShader(ed_picking_shader);

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, ed_picking_framebuffer);
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_BLEND);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);

}

uint32_t ed_EndPicking(int32_t mouse_x, int32_t mouse_y, struct ed_pickable_t *result)
{
    mouse_y = r_height - (mouse_y + 1);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, ed_picking_framebuffer);
    int32_t pickable_index[2] = {};
    glReadPixels(mouse_x, mouse_y, 1, 1, GL_RG_INTEGER, GL_UNSIGNED_INT, pickable_index);

    if(pickable_index[0])
    {
        result->index = pickable_index[0] - 1;
        result->type = pickable_index[1] - 1;
        return 1;
    }

    return 0;
}

void ed_PickableModelViewProjectionMatrix(struct ed_pickable_t *pickable, mat4_t *parent_transform, mat4_t *model_view_projection_matrix)
{
    mat4_t transform;

    if(pickable->draw_transf_flags & ED_PICKABLE_DRAW_TRANSF_FLAG_FIXED_CAM_DIST)
    {
        transform = pickable->draw_transform;

        *model_view_projection_matrix = r_camera_matrix;
        vec3_t_sub(&model_view_projection_matrix->rows[3].xyz, &model_view_projection_matrix->rows[3].xyz, &parent_transform->rows[3].xyz);
        vec3_t_normalize(&model_view_projection_matrix->rows[3].xyz, &model_view_projection_matrix->rows[3].xyz);
        vec3_t_mul(&model_view_projection_matrix->rows[3].xyz, &model_view_projection_matrix->rows[3].xyz, pickable->camera_distance);
    }
    else
    {
        if(parent_transform)
        {
            mat4_t_mul(&transform, &pickable->draw_transform, parent_transform);
        }
        else
        {
            transform = pickable->draw_transform;
        }

        *model_view_projection_matrix = r_camera_matrix;
    }

    mat4_t_invvm(model_view_projection_matrix, model_view_projection_matrix);
    mat4_t_mul(model_view_projection_matrix, &transform, model_view_projection_matrix);
    mat4_t_mul(model_view_projection_matrix, model_view_projection_matrix, &r_projection_matrix);
}

//void ed_DrawPickable(struct ed_pickable_t *pickable, mat4_t *parent_transform)
//{
//    mat4_t model_matrix;
//
//    if(parent_transform)
//    {
//        mat4_t_mul(&model_matrix, &pickable->transform, parent_transform);
//    }
//    else
//    {
//        model_matrix = pickable->transform;
//    }
//
//    mat4_t_mul(&model_matrix, &pickable->draw_offset, &model_matrix);
//    r_SetDefaultUniformMat4(R_UNIFORM_MODEL_MATRIX, &model_matrix);
//    r_SetNamedUniformI(r_GetNamedUniform(ed_picking_shader, "ed_index"), pickable->index + 1);
//    r_SetNamedUniformI(r_GetNamedUniform(ed_picking_shader, "ed_type"), pickable->type + 1);
//    struct ed_pickable_range_t *range = pickable->ranges;
//
//    while(range)
//    {
//        glDrawElements(pickable->mode, range->count, GL_UNSIGNED_INT, (void *)(sizeof(uint32_t) * range->start));
//        range = range->next;
//    }
//}

struct ed_pickable_t *ed_SelectPickable(int32_t mouse_x, int32_t mouse_y, struct ds_slist_t *pickables, mat4_t *parent_transform, uint32_t ignore_types)
{
    struct ed_pickable_t *selection = NULL;
    mat4_t view_projection_matrix;
    mat4_t model_matrix;

    ed_BeginPicking();
    struct r_named_uniform_t *ed_index = r_GetNamedUniform(ed_picking_shader, "ed_index");
    struct r_named_uniform_t *ed_type = r_GetNamedUniform(ed_picking_shader, "ed_type");

    for(uint32_t pickable_index = 0; pickable_index < pickables->cursor; pickable_index++)
    {
        struct ed_pickable_t *pickable = ed_GetPickableOnList(pickable_index, pickables);

        if(pickable && (!(pickable->type & ignore_types)))
        {
            mat4_t model_view_projection_matrix;
            ed_PickableModelViewProjectionMatrix(pickable, parent_transform, &model_view_projection_matrix);
            r_SetDefaultUniformMat4(R_UNIFORM_MODEL_VIEW_PROJECTION_MATRIX, &model_view_projection_matrix);
            r_SetNamedUniformI(ed_index, pickable->index + 1);
            r_SetNamedUniformI(ed_type, pickable->type + 1);
            struct ed_pickable_range_t *range = pickable->ranges;

            if(pickable->mode == GL_POINTS)
            {
                glPointSize(8.0);
            }
            else
            {
                glPointSize(1.0);
            }

            while(range)
            {
                glDrawElements(pickable->mode, range->count, GL_UNSIGNED_INT, (void *)(sizeof(uint32_t) * range->start));
                range = range->next;
            }
        }
    }

    struct ed_pickable_t result;

    if(ed_EndPicking(mouse_x, mouse_y, &result))
    {
        selection = ds_slist_get_element(pickables, result.index);
    }

    return selection;
}

struct ed_pickable_t *ed_SelectWidget(int32_t mouse_x, int32_t mouse_y, struct ed_widget_t *widget, mat4_t *widget_transform)
{
    return ed_SelectPickable(mouse_x, mouse_y, &widget->pickables, widget_transform, 0);
}

struct ed_widget_t *ed_CreateWidget()
{
    uint32_t index;
    struct ed_widget_t *widget;

    index = ds_slist_add_element(&ed_w_ctx_data.widgets, NULL);
    widget = ds_slist_get_element(&ed_w_ctx_data.widgets, index);

    widget->index = index;
//    widget->mvp_mat_fn = ed_WidgetDefaultComputeModelViewProjectionMatrix;
    widget->setup_ds_fn = ed_WidgetDefaultSetupPickableDrawState;

    if(!widget->pickables.buffers)
    {
        widget->pickables = ds_slist_create(sizeof(struct ed_pickable_t), 4);
    }

    return widget;
}

void ed_DestroyWidget(struct ed_widget_t *widget)
{

}

void ed_DrawWidget(struct ed_widget_t *widget, mat4_t *widget_transform)
{
    mat4_t view_projection_matrix;
//    widget->mvp_mat_fn(&view_projection_matrix, widget_transform);

//    r_i_SetViewProjectionMatrix(&view_projection_matrix);
    r_i_SetDepth(GL_TRUE, GL_ALWAYS);
    r_i_SetStencil(GL_TRUE, GL_KEEP, GL_KEEP, GL_REPLACE, GL_ALWAYS, 0xff, 0x01);
    r_i_SetModelMatrix(NULL);

    for(uint32_t pickable_index = 0; pickable_index < widget->pickables.cursor; pickable_index++)
    {
        struct ed_pickable_t *pickable = ed_GetPickableOnList(pickable_index, &widget->pickables);

        if(pickable)
        {
//            mat4_t transform = pickable->draw_transform;
//            mat4_t_mul(&transform, &transform, widget_transform);

            mat4_t model_view_projection_matrix;
            ed_PickableModelViewProjectionMatrix(pickable, widget_transform, &model_view_projection_matrix);

//            r_i_SetModelMatrix(&pickable->transform);
            r_i_SetViewProjectionMatrix(&model_view_projection_matrix);
            struct r_i_draw_list_t *draw_list = r_i_AllocDrawList(1);
            draw_list->commands[0].start = pickable->ranges->start;
            draw_list->commands[0].count = pickable->ranges->count;
            draw_list->indexed = 1;
            r_i_DrawImmediate(R_I_DRAW_CMD_TRIANGLE_LIST, draw_list);
        }
    }

    r_i_SetDepth(GL_TRUE, GL_LEQUAL);
    r_i_SetStencil(GL_TRUE, GL_KEEP, GL_KEEP, GL_DECR, GL_EQUAL, 0xff, 0x01);

    for(uint32_t pickable_index = 0; pickable_index < widget->pickables.cursor; pickable_index++)
    {
        struct ed_pickable_t *pickable = ed_GetPickableOnList(pickable_index, &widget->pickables);
        widget->setup_ds_fn(pickable_index, pickable);

        if(pickable)
        {
//            r_i_SetModelMatrix(&pickable->transform);
            mat4_t model_view_projection_matrix;
            ed_PickableModelViewProjectionMatrix(pickable, widget_transform, &model_view_projection_matrix);
            r_i_SetViewProjectionMatrix(&model_view_projection_matrix);
            struct r_i_draw_list_t *draw_list = r_i_AllocDrawList(1);
            draw_list->commands[0].start = pickable->ranges->start;
            draw_list->commands[0].count = pickable->ranges->count;
            draw_list->indexed = 1;
            r_i_DrawImmediate(R_I_DRAW_CMD_TRIANGLE_LIST, draw_list);
        }
    }

    r_i_SetDepth(GL_TRUE, GL_LESS);
    r_i_SetStencil(GL_FALSE, GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE);
}

//void ed_WidgetDefaultComputeModelViewProjectionMatrix(mat4_t *view_projection_matrix, mat4_t *widget_transform)
//{
//    mat4_t_identity(view_projection_matrix);
//    *view_projection_matrix = r_camera_matrix;
//
//    vec3_t_sub(&view_projection_matrix->rows[3].xyz, &view_projection_matrix->rows[3].xyz, &widget_transform->rows[3].xyz);
//    vec3_t_normalize(&view_projection_matrix->rows[3].xyz, &view_projection_matrix->rows[3].xyz);
//    vec3_t_mul(&view_projection_matrix->rows[3].xyz, &view_projection_matrix->rows[3].xyz, 50.0);
//
//    mat4_t_invvm(view_projection_matrix, view_projection_matrix);
//    mat4_t_mul(view_projection_matrix, view_projection_matrix, &r_projection_matrix);
//}

void ed_WidgetDefaultSetupPickableDrawState(uint32_t pickable_index, struct ed_pickable_t *pickable)
{

}

struct ed_pickable_range_t *ed_AllocPickableRange()
{
    struct ed_pickable_range_t *range = NULL;
    uint32_t index = ds_slist_add_element(&ed_w_ctx_data.pickable_ranges, NULL);
    range = ds_slist_get_element(&ed_w_ctx_data.pickable_ranges, index);
    range->index = index;
    return range;
}

void ed_FreePickableRange(struct ed_pickable_range_t *range)
{
    if(range && range->index != 0xffffffff)
    {
        ds_slist_remove_element(&ed_w_ctx_data.pickable_ranges, range->index);
        range->index = 0xffffffff;
    }
}

struct ds_slist_t *ed_PickableListFromType(uint32_t type)
{
//    switch(type)
//    {
//        case ED_PICKABLE_TYPE_BRUSH:
//        case ED_PICKABLE_TYPE_LIGHT:
//        case ED_PICKABLE_TYPE_ENTITY:
//            return &ed_w_ctx_data.pickables.lists[ED_LEV_EDITOR_EDIT_MODE_OBJECT].pickables;
//
//        case ED_PICKABLE_TYPE_FACE:
//        case ED_PICKABLE_TYPE_EDGE:
//            return &ed_w_ctx_data.pickables.lists[ED_LEV_EDITOR_EDIT_MODE_BRUSH].pickables;
//
//    }
//
//    return NULL;
}

struct ed_pickable_t *ed_CreatePickableOnList(uint32_t type, struct ds_slist_t *pickables)
{
    struct ed_pickable_t *pickable;
    uint32_t index;

    index = ds_slist_add_element(pickables, NULL);
    pickable = ds_slist_get_element(pickables, index);
    pickable->index = index;
    pickable->list = pickables;
    pickable->type = type;
    pickable->transform_flags = 0;
    pickable->draw_transf_flags = 0;
    pickable->draw_render_flags = 0;
    pickable->camera_distance = 0.0;
    pickable->selection_index = 0xffffffff;
    pickable->modified_index = 0xffffffff;
    pickable->update_index = 0xffffffff;

//    mat4_t_identity(&pickable->draw_offset);

    return pickable;
}

struct ed_pickable_t *ed_CreatePickable(uint32_t type)
{
//    struct ds_slist_t *list = ed_PickableListFromType(type);
    struct ed_pickable_t *pickable = ed_CreatePickableOnList(type, &ed_w_ctx_data.pickables.pickables);
    return pickable;
}

void ed_DestroyPickable(struct ed_pickable_t *pickable)
{
    if(pickable && pickable->index != 0xffffffff)
    {
        switch(pickable->type)
        {
            case ED_PICKABLE_TYPE_BRUSH:
            {
                struct ds_list_t *selections = &ed_w_ctx_data.pickables.selections;

                struct ed_brush_t *brush = ed_GetBrush(pickable->primary_index);
                struct ed_face_t *face = brush->faces;

                while(face)
                {
                    struct ed_pickable_t *face_pickable = face->pickable;

                    if(face_pickable->selection_index)
                    {
                        ed_w_DropSelection(face_pickable, selections);
                    }

                    ed_DestroyPickable(face_pickable);

                    face = face->next;
                }

                ed_DestroyBrush(brush);
            }
            break;

            case ED_PICKABLE_TYPE_LIGHT:
                r_DestroyLight(r_GetLight(pickable->primary_index));
            break;

            case ED_PICKABLE_TYPE_ENTITY:
                g_DestroyEntity(g_GetEntity(pickable->primary_index));
            break;
        }

        while(pickable->ranges)
        {
            struct ed_pickable_range_t *next = pickable->ranges->next;
            ed_FreePickableRange(pickable->ranges);
            pickable->ranges = next;
        }

        ds_slist_remove_element(&ed_w_ctx_data.pickables.pickables, pickable->index);
        pickable->index = 0xffffffff;
    }
}

struct ed_pickable_t *ed_GetPickableOnList(uint32_t index, struct ds_slist_t *pickables)
{
    struct ed_pickable_t *pickable = ds_slist_get_element(pickables, index);

    if(pickable && pickable->index == 0xffffffff)
    {
        pickable = NULL;
    }

    return pickable;
}

struct ed_pickable_t *ed_GetPickable(uint32_t index, uint32_t type)
{
    return ed_GetPickableOnList(index, &ed_w_ctx_data.pickables.pickables);
}

struct ed_pickable_t *ed_CreateBrushPickable(vec3_t *position, mat3_t *orientation, vec3_t *size)
{
    struct ed_pickable_t *pickable = NULL;

    struct ed_brush_t *brush = ed_CreateBrush(position, orientation, size);
    struct r_batch_t *first_batch = (struct r_batch_t *)brush->model->batches.buffer;

    pickable = ed_CreatePickable(ED_PICKABLE_TYPE_BRUSH);
    pickable->mode = GL_TRIANGLES;
    pickable->primary_index = brush->index;
    pickable->range_count = 1;
    pickable->ranges = ed_AllocPickableRange();
    mat4_t_comp(&pickable->transform, &brush->orientation, &brush->position);
    pickable->draw_transform = pickable->transform;
    brush->pickable = pickable;
    ed_w_MarkPickableModified(pickable);

    struct ed_face_t *face = brush->faces;

    while(face)
    {
        struct ed_pickable_t *face_pickable = ed_CreatePickable(ED_PICKABLE_TYPE_FACE);
        face_pickable->primary_index = brush->index;
        face_pickable->secondary_index = face->index;
        face_pickable->mode = GL_TRIANGLES;
        face_pickable->range_count = 0;
        mat4_t_comp(&face_pickable->transform, &brush->orientation, &brush->position);
        face_pickable->draw_transform = face_pickable->transform;
        face->pickable = face_pickable;
        ed_w_MarkPickableModified(face_pickable);

        struct ed_face_polygon_t *polygon = face->polygons;

//        while(polygon)
//        {
//            struct ed_edge_t *edge = polygon->edges;
//
//            while(edge)
//            {
//                struct ed_pickable_t *edge_pickable = ed_CreatePickable(ED_PICKABLE_TYPE_EDGE);
//                edge_pickable->primary_index = brush->index;
//                edge_pickable->secondary_index = edge->index;
//                edge_pickable->mode = GL_LINES;
//                edge_pickable->range_count = 0;
//                edge = edge->next;
//            }
//
//            polygon = polygon->next;
//        }

        face = face->next;
    }

    return pickable;
}

struct ed_pickable_t *ed_CreateLightPickable(vec3_t *pos, vec3_t *color, float radius, float energy)
{
    struct r_light_t *light = r_CreateLight(R_LIGHT_TYPE_POINT, pos, color, radius, energy);
    struct ed_pickable_t *pickable = ed_CreatePickable(ED_PICKABLE_TYPE_LIGHT);
    pickable->primary_index = light->index;

    pickable->mode = GL_POINTS;
    pickable->range_count = 1;
    pickable->ranges = ed_AllocPickableRange();
    pickable->ranges->start = ((struct r_batch_t *)ed_light_pickable_model->batches.buffer)[0].start;
    pickable->ranges->count = 1;

    mat3_t rot;
    mat3_t_identity(&rot);
    mat4_t_comp(&pickable->transform, &rot, pos);
    pickable->draw_transform = pickable->transform;

    return pickable;
}

struct ed_pickable_t *ed_CreateEntityPickable(mat4_t *transform, struct r_model_t *model)
{
    return NULL;
}
