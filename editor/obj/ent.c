#include "ent.h"
#include "../../engine/ent.h"
#include "../../engine/r_draw_i.h"

extern struct ed_obj_funcs_t ed_obj_funcs[];
extern mat4_t r_view_projection_matrix;


static void *ed_CreateEntityObject(vec3_t *position, mat3_t *orientation, vec3_t *scale, void *args)
{
    struct ed_ent_args_t *entity_args = (struct ed_ent_args_t *)args;
    return e_SpawnEntity(entity_args->def, position, scale, orientation);
}

static void ed_DestroyEntityObject(struct ed_obj_t *object)
{
    e_DestroyEntity((struct e_entity_t *)object->base_obj);
}

static void ed_UpdateEntityObject(struct ed_obj_t *object, struct ed_obj_event_t *event)
{
    struct e_entity_t *entity = object->base_obj;

    if(event != NULL)
    {
        switch(event->type)
        {
            case ED_OBJ_EVENT_TYPE_OPERATOR:
            {
                switch(event->operator.type)
                {
                    case ED_OPERATOR_TRANSFORM:
                        switch(event->operator.transform.type)
                        {
                            case ED_TRANSFORM_OPERATOR_TRANSFORM_TYPE_TRANSLATE:
                            {
                                e_TranslateEntity(entity, &event->operator.transform.translation.translation);
                            }
                            break;

                            case ED_TRANSFORM_OPERATOR_TRANSFORM_TYPE_ROTATE:
                            {
                                e_RotateEntity(entity, &event->operator.transform.rotation.rotation);
                            }
                            break;

                            case ED_TRANSFORM_OPERATOR_TRANSFORM_TYPE_SCALE:
                            {
                                e_ScaleEntity(entity, &event->operator.transform.scale.scale);
                            }
                            break;
                        }
                    break;
                }
            }
            break;
        }
//        switch(event->operator->type)
//        {
//            case ED_OPERATOR_TRANSFORM:
//                switch(event->transform_event.type)
//                {
//                    case ED_TRANSFORM_OPERATOR_MODE_TRANSLATE:
//                    {
//                        e_TranslateEntity(entity, &event->transform_event.translation.translation);
//                    }
//                    break;
//
//                    case ED_TRANSFORM_OPERATOR_MODE_ROTATE:
//                    {
//                        e_RotateEntity(entity, &event->transform_event.rotation.rotation);
//                    }
//                    break;
//                }
//            break;
//        }

        e_UpdateEntityNode(entity->node, &mat4_t_c_id());
    }

    object->transform = entity->transform->transform;
}

static void ed_PickEntityObjectRecursive(struct e_entity_t *entity, struct r_i_cmd_buffer_t *command_buffer, struct r_i_draw_list_t *draw_list)
{
    mat4_t model_view_projection_matrix;
    mat4_t_mul(&model_view_projection_matrix, &entity->transform->transform, &r_view_projection_matrix);
    struct r_i_uniform_t uniform = {
        .uniform = R_UNIFORM_MODEL_VIEW_PROJECTION_MATRIX,
        .count = 1,
        .value = &model_view_projection_matrix
    };

    struct r_i_draw_range_t *range = draw_list->ranges + draw_list->range_count;
    draw_list->range_count++;

    range->start = entity->model->model->model_start;
    range->count = entity->model->model->model_count;

    r_i_SetUniforms(command_buffer, range, &uniform, 1);

    if(entity->node && entity->node->children)
    {
        struct e_node_t *node = entity->node->children;

        while(node)
        {
            ed_PickEntityObjectRecursive(node->entity, command_buffer, draw_list);
            node = node->next;
        }
    }
}

static struct r_i_draw_list_t *ed_PickEntityObject(struct ed_obj_t *object, struct r_i_cmd_buffer_t *command_buffer, void *args)
{
    struct e_entity_t *entity = object->base_obj;
    struct r_i_draw_list_t *draw_list = r_i_AllocDrawList(command_buffer, entity->def->node_count);

    draw_list->indexed = 1;
    draw_list->mode = GL_TRIANGLES;
    draw_list->range_count = 0;

    ed_PickEntityObjectRecursive(entity, command_buffer, draw_list);

    return draw_list;
}

static struct r_i_draw_list_t *ed_DrawEntityObject(struct ed_obj_t *object, struct r_i_cmd_buffer_t *command_buffer)
{
    return NULL;
}

static struct r_i_draw_list_t *ed_DrawEntityObjectSelected(struct ed_obj_t *object, struct r_i_cmd_buffer_t *command_buffer)
{
    struct e_entity_t *entity = object->base_obj;
    struct r_i_draw_list_t *draw_list = NULL;
    struct r_i_raster_t rasterizer;
    struct r_i_stencil_t stencil;
    struct r_i_depth_t depth;
    struct r_i_draw_mask_t draw_mask;
    vec4_t outline_color = vec4_t_c(0.3, 0.3, 1.0, 1.0);

    struct r_i_uniform_t uniforms[2] = {
        {
            .uniform = ED_OUTLINE_SHADER_UNIFORM_COLOR,
            .count = 1,
            .value = &outline_color
        },
    };

    draw_list = r_i_AllocDrawList(command_buffer, entity->def->node_count * 3);

    /* fill stencil buffer with 0xff where the model is */
    rasterizer = (struct r_i_raster_t) {
        .polygon_mode = GL_FILL,
        .cull_face = GL_BACK,
        .cull_enable = R_I_ENABLE,
        .line_width = 2.0
    };
    stencil = (struct r_i_stencil_t) {
        .enable = R_I_ENABLE,
        .func = GL_ALWAYS,
        .stencil_fail = GL_KEEP,
        .depth_fail = GL_KEEP,
        .depth_pass = GL_REPLACE,
        .mask = 0xff,
        .ref = 0xff
    };

    draw_mask = (struct r_i_draw_mask_t) {
        .red = GL_FALSE,
        .green = GL_FALSE,
        .blue = GL_FALSE,
        .alpha = GL_FALSE,
        .depth = GL_FALSE,
        .stencil = 0xff
    };

    depth = (struct r_i_depth_t) {
        .func = GL_LESS,
        .enable = R_I_DISABLE
    };

    draw_list->range_count = 0;

    r_i_SetUniforms(command_buffer, &draw_list->ranges[draw_list->range_count], uniforms, 1);
    r_i_SetRasterizer(command_buffer, &draw_list->ranges[draw_list->range_count], &rasterizer);
    r_i_SetStencil(command_buffer, &draw_list->ranges[draw_list->range_count], &stencil);
    r_i_SetDrawMask(command_buffer, &draw_list->ranges[draw_list->range_count], &draw_mask);
    r_i_SetDepth(command_buffer, &draw_list->ranges[draw_list->range_count], &depth);

    ed_PickEntityObjectRecursive(entity, command_buffer, draw_list);

    /* draw wireframe only where the stencil buffer isn't 0xff */
    rasterizer = (struct r_i_raster_t) {
        .polygon_mode = GL_LINE,
        .cull_face = GL_BACK,
        .cull_enable = R_I_ENABLE,

    };
    stencil = (struct r_i_stencil_t) {
        .enable = R_I_ENABLE,
        .func = GL_NOTEQUAL,
        .stencil_fail = GL_KEEP,
        .depth_fail = GL_KEEP,
        .depth_pass = GL_KEEP,
        .mask = 0xff,
        .ref = 0xff
    };

    draw_mask = (struct r_i_draw_mask_t) {
        .red = GL_TRUE,
        .green = GL_TRUE,
        .blue = GL_TRUE,
        .alpha = GL_TRUE,
        .depth = GL_FALSE,
        .stencil = 0xff
    };

    depth = (struct r_i_depth_t) {
        .func = GL_LESS,
        .enable = R_I_ENABLE,
    };

    r_i_SetRasterizer(command_buffer, &draw_list->ranges[draw_list->range_count], &rasterizer);
    r_i_SetStencil(command_buffer, &draw_list->ranges[draw_list->range_count], &stencil);
    r_i_SetDrawMask(command_buffer, &draw_list->ranges[draw_list->range_count], &draw_mask);
    r_i_SetDepth(command_buffer, &draw_list->ranges[draw_list->range_count], &depth);

    ed_PickEntityObjectRecursive(entity, command_buffer, draw_list);

    /* clear brush stencil pixels back to 0 */
    rasterizer = (struct r_i_raster_t) {
        .polygon_mode = GL_FILL,
        .cull_face = GL_BACK,
        .cull_enable = R_I_ENABLE
    };
    stencil = (struct r_i_stencil_t) {
        .enable = R_I_ENABLE,
        .func = GL_ALWAYS,
        .stencil_fail = GL_REPLACE,
        .depth_fail = GL_REPLACE,
        .depth_pass = GL_REPLACE,
        .mask = 0xff,
        .ref = 0x00
    };

    draw_mask = (struct r_i_draw_mask_t) {
        .red = GL_FALSE,
        .green = GL_FALSE,
        .blue = GL_FALSE,
        .alpha = GL_FALSE,
        .depth = GL_FALSE,
        .stencil = 0xff
    };

    depth = (struct r_i_depth_t) {
        .func = GL_LEQUAL,
        .enable = R_I_DISABLE
    };

    r_i_SetRasterizer(command_buffer, &draw_list->ranges[draw_list->range_count], &rasterizer);
    r_i_SetStencil(command_buffer, &draw_list->ranges[draw_list->range_count], &stencil);
    r_i_SetDrawMask(command_buffer, &draw_list->ranges[draw_list->range_count], &draw_mask);
    r_i_SetDepth(command_buffer, &draw_list->ranges[draw_list->range_count], &depth);

    ed_PickEntityObjectRecursive(entity, command_buffer, draw_list);

    draw_list->mode = GL_TRIANGLES;
    draw_list->indexed = 1;

    return draw_list;
}


void ed_InitEntityObjectFuncs()
{
    ed_obj_funcs[ED_OBJ_TYPE_ENTITY] = (struct ed_obj_funcs_t) {
        .create = ed_CreateEntityObject,
        .destroy = ed_DestroyEntityObject,
        .update = ed_UpdateEntityObject,
        .pick = ed_PickEntityObject,
        .draw = ed_DrawEntityObject,
        .draw_selected = ed_DrawEntityObjectSelected
    };
}
