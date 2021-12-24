#include <stdio.h>

#include "ed_pick.h"
#include "ed_brush.h"
#include "ed_level.h"
#include "../engine/g_main.h"
#include "../engine/r_main.h"
#include "../engine/ent.h"
#include "../engine/g_game.h"
#include "../engine/g_enemy.h"

extern struct ed_level_state_t ed_level_state;
extern struct r_shader_t *ed_picking_shader;
extern mat4_t r_view_projection_matrix;
extern mat4_t r_camera_matrix;
extern mat4_t r_projection_matrix;
extern int32_t r_width;
extern int32_t r_height;
extern uint32_t r_vertex_buffer;
extern uint32_t r_index_buffer;
uint32_t ed_picking_framebuffer;
uint32_t ed_picking_depth_texture;
uint32_t ed_picking_object_texture;
//uint32_t ed_show_renderer_info_window;
extern struct r_model_t *ed_light_pickable_model;

void ed_PickingInit()
{
    glGenFramebuffers(1, &ed_picking_framebuffer);
    glGenTextures(1, &ed_picking_object_texture);
    glBindTexture(GL_TEXTURE_2D, ed_picking_object_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32UI, r_width, r_height, 0, GL_RG_INTEGER, GL_INT, NULL);

    glGenTextures(1, &ed_picking_depth_texture);
    glBindTexture(GL_TEXTURE_2D, ed_picking_depth_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, r_width, r_height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, ed_picking_framebuffer);
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ed_picking_object_texture, 0);
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, ed_picking_depth_texture, 0);
}

void ed_PickingShutdown()
{

}

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
        transform = pickable->transform;
        mat4_t parent_rotation = *parent_transform;
        parent_rotation.rows[3].xyz = vec3_t_c(0.0, 0.0, 0.0);
        mat4_t_transpose(&parent_rotation, &parent_rotation);
        mat4_t_mul(model_view_projection_matrix, &r_camera_matrix, &parent_rotation);
        vec3_t_sub(&model_view_projection_matrix->rows[3].xyz, &model_view_projection_matrix->rows[3].xyz, &parent_transform->rows[3].xyz);
        vec3_t_normalize(&model_view_projection_matrix->rows[3].xyz, &model_view_projection_matrix->rows[3].xyz);
        vec3_t_mul(&model_view_projection_matrix->rows[3].xyz, &model_view_projection_matrix->rows[3].xyz, pickable->camera_distance);
    }
    else
    {
        if(parent_transform)
        {
            mat4_t_mul(&transform, &pickable->transform, parent_transform);
        }
        else
        {
            transform = pickable->transform;
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

        if(pickable && !((1 << pickable->type) & ignore_types))
        {
            mat4_t base_model_view_projection_matrix;
            ed_PickableModelViewProjectionMatrix(pickable, parent_transform, &base_model_view_projection_matrix);
//            r_SetDefaultUniformMat4(R_UNIFORM_MODEL_VIEW_PROJECTION_MATRIX, &model_view_projection_matrix);
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

            if(pickable->mode == GL_LINES)
            {
                glLineWidth(8.0);
            }
            else
            {
                glLineWidth(1.0);
            }

            while(range)
            {
                mat4_t model_view_projection_matrix;
                mat4_t_mul(&model_view_projection_matrix, &range->offset, &base_model_view_projection_matrix);
                r_SetDefaultUniformMat4(R_UNIFORM_MODEL_VIEW_PROJECTION_MATRIX, &model_view_projection_matrix);
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

    index = ds_slist_add_element(&ed_level_state.widgets, NULL);
    widget = ds_slist_get_element(&ed_level_state.widgets, index);

    widget->index = index;
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

void ed_WidgetDefaultSetupPickableDrawState(uint32_t pickable_index, struct ed_pickable_t *pickable)
{

}

struct ed_pickable_range_t *ed_AllocPickableRange()
{
    struct ed_pickable_range_t *range = NULL;
    uint32_t index = ds_slist_add_element(&ed_level_state.pickable_ranges, NULL);
    range = ds_slist_get_element(&ed_level_state.pickable_ranges, index);
    range->index = index;
    mat4_t_identity(&range->offset);
    return range;
}

void ed_FreePickableRange(struct ed_pickable_range_t *range)
{
    if(range && range->index != 0xffffffff)
    {
        ds_slist_remove_element(&ed_level_state.pickable_ranges, range->index);
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
//            return &ed_level_state.pickables.lists[ED_LEV_EDITOR_EDIT_MODE_OBJECT].pickables;
//
//        case ED_PICKABLE_TYPE_FACE:
//        case ED_PICKABLE_TYPE_EDGE:
//            return &ed_level_state.pickables.lists[ED_LEV_EDITOR_EDIT_MODE_BRUSH].pickables;
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
    pickable->game_pickable_index = 0xffffffff;
    pickable->ranges = NULL;
    pickable->range_count = 0;

//    mat4_t_identity(&pickable->draw_offset);

    return pickable;
}

struct ed_pickable_t *ed_CreatePickable(uint32_t type)
{
    struct ed_pickable_t *pickable = ed_CreatePickableOnList(type, &ed_level_state.pickables.pickables);

    if(type < ED_PICKABLE_TYPE_LAST_GAME_PICKABLE)
    {
        pickable->game_pickable_index = ds_list_add_element(&ed_level_state.pickables.game_pickables[type], &pickable);
    }

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
                struct ds_list_t *selections = &ed_level_state.pickables.selections;
                struct ed_brush_t *brush = ed_GetBrush(pickable->primary_index);
                struct ed_face_t *face = brush->faces;

                ed_level_state.world_data_stale = 1;

                while(face)
                {
                    struct ed_pickable_t *face_pickable = face->pickable;

                    if(face_pickable->selection_index != 0xffffffff)
                    {
                        ed_w_DropSelection(face_pickable, selections);
                    }

                    struct ed_face_polygon_t *face_polygon = face->polygons;

                    while(face_polygon)
                    {
                        struct ed_edge_t *edge = face_polygon->edges;

                        while(edge)
                        {
                            uint32_t polygon_side = edge->polygons[1].polygon == face_polygon;

                            if(edge->pickable)
                            {
                                ed_DestroyPickable(edge->pickable);
                                edge->pickable = NULL;

                            }
                            edge = edge->polygons[polygon_side].next;
                        }

                        face_polygon = face_polygon->next;
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
                e_DestroyEntity(e_GetEntity(pickable->primary_index));
            break;

            case ED_PICKABLE_TYPE_ENEMY:
                g_RemoveEnemy(g_GetEnemy(pickable->secondary_index, pickable->primary_index));
            break;
        }

        while(pickable->ranges)
        {
            struct ed_pickable_range_t *next = pickable->ranges->next;
            ed_FreePickableRange(pickable->ranges);
            pickable->ranges = next;
        }

        if(pickable->type < ED_PICKABLE_TYPE_LAST_GAME_PICKABLE)
        {
            struct ds_list_t *game_pickables = &ed_level_state.pickables.game_pickables[pickable->type];
            ds_list_remove_element(game_pickables, pickable->game_pickable_index);

            if(pickable->game_pickable_index < game_pickables->cursor)
            {
                struct ed_pickable_t *moved_pickable = *(struct ed_pickable_t **)ds_list_get_element(game_pickables, pickable->game_pickable_index);
                moved_pickable->game_pickable_index = pickable->game_pickable_index;
            }
        }

        ds_slist_remove_element(&ed_level_state.pickables.pickables, pickable->index);
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

struct ed_pickable_t *ed_GetPickable(uint32_t index)
{
    return ed_GetPickableOnList(index, &ed_level_state.pickables.pickables);
}

struct ed_pickable_t *ed_CopyPickable(struct ed_pickable_t *src_pickable)
{
    struct ed_pickable_t *copy = NULL;

    switch(src_pickable->type)
    {
        case ED_PICKABLE_TYPE_BRUSH:
        {
            struct ed_brush_t *src_brush = ed_GetBrush(src_pickable->primary_index);
            copy = ed_CreateBrushPickable(NULL, NULL, NULL, ed_CopyBrush(src_brush));
        }
        break;

        case ED_PICKABLE_TYPE_LIGHT:
        {
            struct r_light_t *src_light = r_GetLight(src_pickable->primary_index);
            copy = ed_CreateLightPickable(NULL, NULL, 0, 0, 0, r_CopyLight(src_light));
        }
        break;

        case ED_PICKABLE_TYPE_ENTITY:
        {
            struct e_entity_t *src_entity = e_GetEntity(src_pickable->primary_index);
            copy = ed_CreateEntityPickable(NULL, NULL, NULL, NULL, e_CopyEntity(src_entity));
        }
        break;

        case ED_PICKABLE_TYPE_ENEMY:
        {
//            struct g_enemy_t *
        }
        break;
    }

    return copy;
}

struct ed_pickable_t *ed_CreateBrushPickable(vec3_t *position, mat3_t *orientation, vec3_t *size, struct ed_brush_t *src_brush)
{
    struct ed_pickable_t *pickable = NULL;
    struct ed_brush_t *brush;

    if(src_brush)
    {
        brush = src_brush;
    }
    else
    {
        brush = ed_CreateBrush(position, orientation, size);
    }

    struct r_batch_t *first_batch = (struct r_batch_t *)brush->model->batches.buffer;

    pickable = ed_CreatePickable(ED_PICKABLE_TYPE_BRUSH);
    pickable->mode = GL_TRIANGLES;
    pickable->primary_index = brush->index;
    pickable->range_count = 1;
    pickable->ranges = ed_AllocPickableRange();
    mat4_t_comp(&pickable->transform, &brush->orientation, &brush->position);
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
        face->pickable = face_pickable;
        ed_w_MarkPickableModified(face_pickable);

        struct ed_face_polygon_t *polygon = face->polygons;

        while(polygon)
        {
            struct ed_edge_t *edge = polygon->edges;

            while(edge)
            {
                if(!edge->pickable)
                {
                    struct ed_pickable_t *edge_pickable = ed_CreatePickable(ED_PICKABLE_TYPE_EDGE);
                    edge_pickable->primary_index = brush->index;
                    edge_pickable->secondary_index = edge->index;
                    edge_pickable->mode = GL_LINES;
                    edge_pickable->ranges = ed_AllocPickableRange();
                    edge_pickable->range_count = 1;
                    edge->pickable = edge_pickable;
                    ed_w_MarkPickableModified(edge_pickable);
                }

                uint32_t polygon_side = edge->polygons[1].polygon == polygon;
                edge = edge->polygons[polygon_side].next;
            }

            polygon = polygon->next;
        }

        face = face->next;
    }

    return pickable;
}

struct ed_pickable_t *ed_CreateLightPickable(vec3_t *pos, vec3_t *color, float radius, float energy, uint32_t type, struct r_light_t *src_light)
{
    struct r_light_t *light;

    if(src_light)
    {
        light = src_light;
    }
    else
    {
        if(type == ED_LEVEL_LIGHT_TYPE_POINT)
        {
            light = (struct r_light_t *)r_CreatePointLight(pos, color, radius, energy);
        }
        else
        {
            light = (struct r_light_t *)r_CreateSpotLight(pos, color, NULL, radius, energy, 20, 0.1);
        }
    }

    struct ed_pickable_t *pickable = ed_CreatePickable(ED_PICKABLE_TYPE_LIGHT);
    pickable->primary_index = R_LIGHT_INDEX(light->type, light->index);
    pickable->mode = GL_POINTS;
    pickable->range_count = 1;
    pickable->ranges = ed_AllocPickableRange();
    pickable->ranges->start = ((struct r_batch_t *)ed_light_pickable_model->batches.buffer)[0].start;
    pickable->ranges->count = 1;

    mat3_t orientation;

    if(light->type == R_LIGHT_TYPE_SPOT)
    {
        struct r_spot_light_t *spot_light = (struct r_spot_light_t *)light;
        orientation = spot_light->orientation;
    }
    else
    {
        mat3_t_identity(&orientation);
    }

    mat4_t_comp(&pickable->transform, &orientation, &light->position);
//    pickable->draw_transform = pickable->transform;

    return pickable;
}

struct ed_pickable_t *ed_CreateEntityPickable(struct e_ent_def_t *ent_def, vec3_t *position, vec3_t *scale, mat3_t *orientation, struct e_entity_t *src_entity)
{
    struct e_entity_t *entity;

    if(src_entity)
    {
        entity = src_entity;
    }
    else
    {
        entity = e_SpawnEntity(ent_def, position, scale, orientation);
    }

    struct ed_pickable_t *pickable = ed_CreatePickable(ED_PICKABLE_TYPE_ENTITY);
    pickable->primary_index = entity->index;
    pickable->mode = GL_TRIANGLES;
    pickable->transform = entity->transform->transform;

    ed_w_MarkPickableModified(pickable);

    return pickable;
}

struct ed_pickable_range_t *ed_UpdateEntityPickableRangesRecursive(struct ed_pickable_t *pickable, struct e_entity_t *entity, mat4_t *parent_transform, struct ed_pickable_range_t **cur_range)
{
    struct e_node_t *node = entity->node;
    struct e_model_t *model = entity->model;

    struct ed_pickable_range_t *range = *cur_range;
    pickable->range_count++;

    if(!range)
    {
        range = ed_AllocPickableRange();
    }

    if(!parent_transform)
    {
        range->offset = mat4_t_c_id();
    }
    else
    {
        mat4_t scale = mat4_t_c_id();

        scale.rows[0].x = node->scale.x;
        scale.rows[1].y = node->scale.y;
        scale.rows[2].z = node->scale.z;

        mat4_t_comp(&range->offset, &node->orientation, &node->position);
        mat4_t_mul(&range->offset, &range->offset, parent_transform);
        mat4_t_mul(&range->offset, &scale, &range->offset);
    }

    range->start = model->model->model_start;
    range->count = model->model->model_count;

    *cur_range = range;
    struct e_node_t *child = node->children;

    if(child)
    {
        struct ed_pickable_range_t *next_range = ed_UpdateEntityPickableRangesRecursive(pickable, child->entity, &range->offset, &(*cur_range)->next);
        next_range->prev = range;
        child = child->next;

        while(child)
        {
            ed_UpdateEntityPickableRangesRecursive(pickable, child->entity, &range->offset, &(*cur_range)->next);
            child = child->next;
        }
    }

    return range;
}

void ed_UpdateEntityPickableRanges(struct ed_pickable_t *pickable, struct e_entity_t *entity)
{
    uint32_t range_count = pickable->range_count;
    ed_UpdateEntityPickableRangesRecursive(pickable, entity, NULL, &pickable->ranges);

    if(range_count > pickable->range_count)
    {
        /* we have more ranges than we need, so free the extra at the
        end of the list */
        struct ed_pickable_range_t *range = pickable->ranges;
        range_count = pickable->range_count;

        while(range_count)
        {
            /* skip all used ranges */
           range = range->next;
           range_count--;
        }

        range->prev->next = NULL;

        while(range)
        {
            struct ed_pickable_range_t *next_range = range->next;
            ed_FreePickableRange(range);
            range = next_range;
        }
    }
}

struct ed_pickable_t *ed_CreateEnemyPickable(uint32_t type, vec3_t *position, mat3_t *orientation, struct g_enemy_t *src_enemy)
{
    struct g_enemy_t *enemy;

    if(src_enemy)
    {
        enemy = src_enemy;
    }
    else
    {
        switch(type)
        {
            case G_ENEMY_TYPE_CAMERA:
            {
                struct g_enemy_def_t def = {};
                def.type = G_ENEMY_TYPE_CAMERA;
                def.camera.min_pitch = -0.5;
                def.camera.max_pitch = 0.5;
                def.camera.min_yaw = -0.5;
                def.camera.max_yaw = 0.5;
                def.camera.idle_pitch = 0.2;
                def.camera.range = 10.0;
                def.camera.cur_pitch = 0.2;
                def.camera.cur_yaw = 0.0;
                enemy = g_SpawnEnemy(&def, position, orientation);
            }
            break;
        }
    }

    struct ed_pickable_t *pickable = ed_CreatePickable(ED_PICKABLE_TYPE_ENEMY);
    pickable->primary_index = enemy->thing.index;
    pickable->secondary_index = enemy->type;
    pickable->transform = enemy->thing.entity->transform->transform;

    ed_w_MarkPickableModified(pickable);

    return pickable;
}










