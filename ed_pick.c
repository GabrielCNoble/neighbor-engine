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
//extern uint32_t ed_picking_shader_type_uniform;
//extern uint32_t ed_picking_shader_index_uniform;

void ed_BeginPicking()
{
    glBindBuffer(GL_ARRAY_BUFFER, r_vertex_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, r_index_buffer);

    r_BindShader(ed_picking_shader);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, ed_picking_framebuffer);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_BLEND);
    glDepthFunc(GL_LESS);

}

uint32_t ed_EndPicking(int32_t mouse_x, int32_t mouse_y, struct ed_pick_result_t *result)
{
    mouse_y = r_height - (mouse_y + 1);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, ed_picking_framebuffer);
    int32_t pickable_index[2];
    glReadPixels(mouse_x, mouse_y, 1, 1, GL_RG_INTEGER, GL_UNSIGNED_INT, pickable_index);

    if(pickable_index[0])
    {
        result->index = pickable_index[0] - 1;
        result->type = pickable_index[1] - 1;
        return 1;
    }

    return 0;
}

void ed_DrawPickable(struct ed_pickable_t *pickable, mat4_t *parent_transform)
{
    mat4_t model_view_projection_matrix;

    if(parent_transform)
    {
        mat4_t_mul(&model_view_projection_matrix, &pickable->transform, parent_transform);
    }
    else
    {
        mat4_t_identity(&model_view_projection_matrix);
    }

    mat4_t_mul(&model_view_projection_matrix, &model_view_projection_matrix, &r_view_projection_matrix);
    r_SetDefaultUniformMat4(R_UNIFORM_MODEL_VIEW_PROJECTION_MATRIX, &model_view_projection_matrix);
    r_SetNamedUniformI(r_GetNamedUniform(ed_picking_shader, "ed_index"), pickable->index + 1);
    r_SetNamedUniformI(r_GetNamedUniform(ed_picking_shader, "ed_type"), pickable->type + 1);
    struct ed_pickable_range_t *range = pickable->ranges;

    while(range)
    {
        glDrawElements(pickable->mode, range->count, GL_UNSIGNED_INT, (void *)(sizeof(uint32_t) * range->start));
        range = range->next;
    }
}

struct ed_pickable_t *ed_SelectPickable(int32_t mouse_x, int32_t mouse_y, struct ds_slist_t *pickables, mat4_t *parent_transform)
{
    mat4_t model_view_projection_matrix;
    struct ed_pickable_t *selection = NULL;

    ed_BeginPicking();

    for(uint32_t pickable_index = 0; pickable_index < pickables->cursor; pickable_index++)
    {
        struct ed_pickable_t *pickable = ds_slist_get_element(pickables, pickable_index);

        if(pickable && pickable->index != 0xffffffff)
        {
            ed_DrawPickable(pickable, parent_transform);
        }
    }

    struct ed_pick_result_t result;

    if(ed_EndPicking(mouse_x, mouse_y, &result))
    {
        selection = ds_slist_get_element(pickables, result.index);
    }

    return selection;
}
