#include <stdio.h>

#include "ed_pick.h"
#include "ed_brush.h"
#include "game.h"
#include "r_main.h"

extern struct ed_world_context_data_t ed_world_context_data;
extern struct r_shader_t *ed_picking_shader;
extern mat4_t r_view_projection_matrix;
extern int32_t r_width;
extern int32_t r_height;
extern uint32_t r_vertex_buffer;
extern uint32_t r_index_buffer;
extern uint32_t ed_picking_framebuffer;
extern uint32_t ed_picking_shader_type_uniform;
extern uint32_t ed_picking_shader_index_uniform;

struct ed_pickable_t *ed_CreatePickable()
{
    struct ed_pickable_t *pickable;
    uint32_t index;

    index = ds_slist_add_element(&ed_world_context_data.pickables[ed_world_context_data.active_pickable_list], NULL);
    pickable = ds_slist_get_element(&ed_world_context_data.pickables[ed_world_context_data.active_pickable_list], index);
    pickable->index = index;

    return pickable;
}

void ed_DestroyPickable(struct ed_pickable_t *pickable)
{
    if(pickable)
    {
        switch(pickable->type)
        {
            case ED_PICKABLE_TYPE_BRUSH:
                ed_DestroyBrush(ed_GetBrush(pickable->pick_index));
            break;

            case ED_PICKABLE_TYPE_LIGHT:
                r_DestroyLight(r_GetLight(pickable->pick_index));
            break;

            case ED_PICKABLE_TYPE_ENTITY:
                g_DestroyEntity(g_GetEntity(pickable->pick_index));
            break;
        }
    }
}

struct ed_pickable_t *ed_GetPickable(uint32_t index)
{
    struct ed_pickable_t *pickable = NULL;

    pickable = ds_slist_get_element(&ed_world_context_data.pickables[ed_world_context_data.active_pickable_list], index);

    if(pickable && pickable->index == 0xffffffff)
    {
        pickable = NULL;
    }

    return pickable;
}

struct ed_pickable_t *ed_CreateBrushPickable(vec3_t *position, mat3_t *orientation, vec3_t *size)
{
    struct ed_pickable_t *pickable = NULL;

    struct ed_brush_t *brush = ed_CreateBrush(position, orientation, size);
    struct r_batch_t *first_batch = (struct r_batch_t *)brush->model->batches.buffer;

    pickable = ed_CreatePickable();
    pickable->type = ED_PICKABLE_TYPE_BRUSH;
    pickable->mode = GL_TRIANGLES;
    pickable->pick_index = brush->index;
    pickable->start = first_batch->start;
    pickable->count = brush->model->indices.buffer_size;

    return pickable;
}

struct ed_pickable_t *ed_CreateLightObject(vec3_t *pos, vec3_t *color, float radius, float energy)
{
    return NULL;
}

struct ed_pickable_t *ed_CreateEntityObject(mat4_t *transform, struct r_model_t *model)
{
    return NULL;
}

void ed_UpdatePickables()
{
    for(uint32_t pickable_index = 0; pickable_index < ed_world_context_data.pickables[ed_world_context_data.active_pickable_list].cursor; pickable_index++)
    {
        struct ed_pickable_t *pickable = ed_GetPickable(pickable_index);
        if(pickable)
        {
            mat4_t_identity(&pickable->transform);

            switch(pickable->type)
            {
                case ED_PICKABLE_TYPE_BRUSH:
                {
                    struct ed_brush_t *brush = ed_GetBrush(pickable->pick_index);
                    mat4_t_comp(&pickable->transform, &brush->orientation, &brush->position);
                }
                break;

                case ED_PICKABLE_TYPE_LIGHT:

                break;

                case ED_PICKABLE_TYPE_ENTITY:

                break;
            }
        }
    }
}

struct ed_pickable_t *ed_SelectPickable(int32_t mouse_x, int32_t mouse_y)
{
    mouse_y = r_height - (mouse_y + 1);
    mat4_t model_view_projection_matrix;
    struct ed_pickable_t *selection = NULL;
    glBindBuffer(GL_ARRAY_BUFFER, r_vertex_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, r_index_buffer);

    r_BindShader(ed_picking_shader);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, ed_picking_framebuffer);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    for(uint32_t pickable_index = 0; pickable_index < ed_world_context_data.pickables[ed_world_context_data.active_pickable_list].cursor; pickable_index++)
    {
        struct ed_pickable_t *pickable = ed_GetPickable(pickable_index);

        if(pickable)
        {
            mat4_t_mul(&model_view_projection_matrix, &pickable->transform, &r_view_projection_matrix);
            r_SetDefaultUniformMat4(R_UNIFORM_MODEL_VIEW_PROJECTION_MATRIX, &model_view_projection_matrix);
            glUniform1i(ed_picking_shader_index_uniform, pickable->index + 1);
            glUniform1i(ed_picking_shader_type_uniform, pickable->index + 1);
            glDrawElements(pickable->mode, pickable->count, GL_UNSIGNED_INT, (void *)(sizeof(uint32_t) * pickable->start));
        }
    }

    glBindFramebuffer(GL_READ_FRAMEBUFFER, ed_picking_framebuffer);
    int32_t pickable_index[2];
    while(glGetError() != GL_NO_ERROR);
    glReadPixels(mouse_x, mouse_y, 1, 1, GL_RED_INTEGER, GL_UNSIGNED_INT, pickable_index);

    if(pickable_index[0])
    {
        selection = ed_GetPickable(pickable_index[0] - 1);
    }

    return selection;
}

void ed_ClearSelections()
{
    ed_world_context_data.selections.cursor = 0;
}

void ed_AddSelection(struct ed_pickable_t *selection, uint32_t multiple_key_down)
{
    uint32_t selection_index = 0xffffffff;

    for(uint32_t index = 0; index < ed_world_context_data.selections.cursor; index++)
    {
        uint32_t pickable_index = *(uint32_t *)ds_list_get_element(&ed_world_context_data.selections, index);

        if(pickable_index == selection->index)
        {
            selection_index = index;
            break;
        }
    }

    if(selection_index != 0xffffffff)
    {
        uint32_t old_cursor = ed_world_context_data.selections.cursor;
        /* This selection already exists in the list. In this case, it can either be the
        main selection (last in the list), in which case it'll be dropped from the list, or
        it can be some other selection, in which case it'll be re-added at the back of the
        list, becoming the main selection. Either way, we need to remove it here. */
        ds_list_remove_element(&ed_world_context_data.selections, selection_index);


        if(selection_index >= old_cursor - 1)
        {
            printf("dropped selection\n");
            /* This is the main selection, in which case we don't need to do anything else, since
            it's been dropped from the list already. */
            return;
        }
    }

    if(!multiple_key_down)
    {
        ed_ClearSelections();
    }

    printf("added selection\n");

    /* This is either a new selection, or an already existing selection becoming the main selection. */
    ds_list_add_element(&ed_world_context_data.selections, &selection->index);
}

uint32_t ed_FindSelectionIndex(struct ed_pickable_t *selection)
{
    for(uint32_t selection_index = 0; selection_index < ed_world_context_data.selections.cursor; selection_index++)
    {
        uint32_t pickable_index = *(uint32_t *)ds_list_get_element(&ed_world_context_data.selections, selection_index);
        struct ed_pickable_t *pickable = ed_GetPickable(pickable_index);

        if(pickable->type == selection->type && pickable->pick_index == selection->pick_index)
        {
            return selection_index;
        }
    }

    return 0xffffffff;
}

void ed_TranslateSelected(vec3_t *translation)
{
    for(uint32_t pickable_index = 0; pickable_index < ed_world_context_data.selections.cursor; pickable_index++)
    {

    }
}

void ed_RotateSelected(mat3_t *rotation)
{

}
