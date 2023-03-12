#include "light.h"
#include "../engine/r_main.h"
#include "../engine/r_draw_i.h"

extern struct ed_obj_funcs_t ed_obj_funcs[];
extern mat4_t r_view_projection_matrix;

static void *ed_CreateLightObject(vec3_t *position, mat3_t *orientation, vec3_t *scale, void *args)
{
    struct ed_light_args_t *light_args = (struct ed_light_args_t *)args;

    switch(light_args->type)
    {
        case R_LIGHT_TYPE_POINT:
            return r_CreatePointLight(position, &light_args->color, light_args->range, light_args->energy);
        break;

        case R_LIGHT_TYPE_SPOT:
            return r_CreateSpotLight(position, &light_args->color, &mat3_t_c_id(), light_args->range, light_args->energy, light_args->outer_angle, 0.1);
        break;
    }
//    return r_CreateLight(light_args->type, position, &light_args->color, light_args->radius, light_args->energy);
}

static void ed_DestroyLightObject(struct ed_obj_t *object)
{
    r_DestroyLight((struct r_light_t *)object->base_obj);
}

static void ed_UpdateLightObject(struct ed_obj_t *object, struct ed_obj_event_t *event)
{
    struct r_light_t *light = (struct r_light_t *)object->base_obj;

    mat3_t orientation;
    mat3_t_identity(&orientation);

    if(event != NULL)
    {
        switch(event->type)
        {
            case ED_OBJ_EVENT_TYPE_OPERATOR:
            {
                switch(event->operator.type)
                {
                    case ED_OPERATOR_TRANSFORM:
                    {
                        switch(event->operator.transform.type)
                        {
                            case ED_TRANSFORM_OPERATOR_MODE_TRANSLATE:
                            {
                                vec3_t_add(&light->position, &light->position, &event->operator.transform.translation.translation);
                            }
                            break;

                            case ED_TRANSFORM_OPERATOR_MODE_ROTATE:
                            {
                                if(light->type == R_LIGHT_TYPE_SPOT)
                                {
                                    struct r_spot_light_t *spot_light = (struct r_spot_light_t *)light;
                                    mat3_t_mul(&spot_light->orientation, &spot_light->orientation, &event->operator.transform.rotation.rotation);
                                    orientation = spot_light->orientation;
                                }
                            }
                            break;
                        }
                    }
                    break;
                }
            }
            break;

            case ED_OBJ_EVENT_TYPE_PICK:

            break;
        }

//        switch(event->operator->type)
//        {
//            case ED_OPERATOR_TRANSFORM:
//            {
//                switch(event->transform_event.type)
//                {
//                    case ED_TRANSFORM_OPERATOR_MODE_TRANSLATE:
//                    {
//                        vec3_t_add(&light->position, &light->position, &event->transform_event.translation.translation);
//                    }
//                    break;
//
//                    case ED_TRANSFORM_OPERATOR_MODE_ROTATE:
//                    {
//                        if(light->type == R_LIGHT_TYPE_SPOT)
//                        {
//                            struct r_spot_light_t *spot_light = (struct r_spot_light_t *)light;
//                            mat3_t_mul(&spot_light->orientation, &spot_light->orientation, &event->transform_event.rotation.rotation);
//                            orientation = spot_light->orientation;
//                        }
//
//                    }
//                    break;
//                }
//            }
//            break;
//        }
    }

//    switch(light->type)
//    {
//        case R_LIGHT_TYPE_POINT:
//        {
//            struct r_point_light_t *point_light = (struct r_point_light_t *)light;
//            mat4_t_identity(&object->transform);
//            object->transform.rows[3] = vec4_t_c(point_light->position.x, point_light->position.y, point_light->position.z, 1.0);
//        }
//        break;
//
//        case R_LIGHT_TYPE_SPOT:
//        {
//            struct r_spot_light_t *spot_light = (struct r_spot_light_t *)light;
//            mat4_t_comp(&object->transform, &spot_light->orientation, &spot_light->position);
//        }
//        break;
//    }

    mat4_t_comp(&object->transform, &orientation, &light->position);
}

static struct r_i_draw_list_t *ed_PickLightObject(struct ed_obj_t *object, struct r_i_cmd_buffer_t *command_buffer, void *args)
{
    struct r_light_t *light = (struct r_light_t *)object->base_obj;
    struct r_i_draw_list_t *draw_list = NULL;

    struct r_i_depth_t depth_state = {
        .enable = GL_TRUE,
        .func = GL_LESS
    };

    struct r_i_raster_t raster_state = {
        .point_size = 14.0,
    };

    struct r_vert_t vertices[] = {
        [0] = {.color = vec4_t_c(light->color.x, light->color.y, light->color.z, 1.0)},
//        [1] = {.color = vec4_t_c(1.0, 1.0, 1.0, 1.0)}
    };

    r_i_SetDepth(command_buffer, NULL, &depth_state);

    mat4_t model_view_projection_matrix;
    mat4_t_mul(&model_view_projection_matrix, &object->transform, &r_view_projection_matrix);

    struct r_i_uniform_t uniforms[] = {
        {
            .uniform = R_UNIFORM_MODEL_VIEW_PROJECTION_MATRIX,
            .count = 1,
            .value = &model_view_projection_matrix
        }
    };

    draw_list = r_i_AllocDrawList(command_buffer, 1);
    r_i_SetUniforms(command_buffer, draw_list->ranges, uniforms, 1);

    draw_list->mesh = r_i_AllocMesh(command_buffer, sizeof(struct r_vert_t), 1, 0);
    memcpy(draw_list->mesh->verts.verts, vertices, sizeof(vertices));

    draw_list->ranges[0].count = 1;
    draw_list->ranges[0].start = 0;
    draw_list->mode = GL_POINTS;
    r_i_SetRasterizer(command_buffer, &draw_list->ranges[0], &raster_state);

//    raster_state.point_size = 14.0;

//    draw_list->ranges[1].count = 1;
//    draw_list->ranges[1].start = 1;
//    draw_list->mode = GL_POINTS;
//    r_i_SetRasterizer(command_buffer, &draw_list->ranges[1], &raster_state);
////
//    raster_state.point_size = 14.0;
//    r_i_SetRasterizer(command_buffer, &draw_list->ranges[1], &raster_state);

    return draw_list;
}

static struct r_i_draw_list_t *ed_DrawLightObject(struct ed_obj_t *object, struct r_i_cmd_buffer_t *command_buffer)
{
    struct r_light_t *light = (struct r_light_t *)object->base_obj;
    struct r_i_draw_list_t *draw_list = NULL;

    struct r_i_depth_t depth_state = {
        .enable = GL_TRUE,
        .func = GL_LESS
    };

    struct r_i_raster_t raster_state = {
        .point_size = 8.0,
    };

    struct r_vert_t vertices[] = {
        [0] = {.color = vec4_t_c(light->color.x, light->color.y, light->color.z, 1.0)},
        [1] = {.color = vec4_t_c(1.0, 1.0, 1.0, 1.0)}
    };

    r_i_SetDepth(command_buffer, NULL, &depth_state);

    mat4_t model_view_projection_matrix;
    mat4_t_mul(&model_view_projection_matrix, &object->transform, &r_view_projection_matrix);

    struct r_i_uniform_t uniforms[] = {
        {
            .uniform = R_UNIFORM_MODEL_VIEW_PROJECTION_MATRIX,
            .count = 1,
            .value = &model_view_projection_matrix
        }
    };

    draw_list = r_i_AllocDrawList(command_buffer, 2);
    r_i_SetUniforms(command_buffer, draw_list->ranges, uniforms, 1);

    draw_list->mesh = r_i_AllocMesh(command_buffer, sizeof(struct r_vert_t), 2, 0);
    memcpy(draw_list->mesh->verts.verts, vertices, sizeof(vertices));

    draw_list->ranges[0].count = 1;
    draw_list->ranges[0].start = 0;
    draw_list->mode = GL_POINTS;
    r_i_SetRasterizer(command_buffer, &draw_list->ranges[0], &raster_state);

    raster_state.point_size = 14.0;

    draw_list->ranges[1].count = 1;
    draw_list->ranges[1].start = 1;
    draw_list->mode = GL_POINTS;
    r_i_SetRasterizer(command_buffer, &draw_list->ranges[1], &raster_state);
//
    raster_state.point_size = 14.0;
    r_i_SetRasterizer(command_buffer, &draw_list->ranges[1], &raster_state);

    return draw_list;
}

static struct r_i_draw_list_t *ed_DrawLightObjectSelected(struct ed_obj_t *object, struct r_i_cmd_buffer_t *command_buffer)
{
    return NULL;
}

void ed_InitLightObjectFuncs()
{
    ed_obj_funcs[ED_OBJ_TYPE_LIGHT] = (struct ed_obj_funcs_t){
        .create = ed_CreateLightObject,
        .destroy = ed_DestroyLightObject,
        .update = ed_UpdateLightObject,
        .pick = ed_PickLightObject,
        .draw = ed_DrawLightObject,
        .draw_selected = ed_DrawLightObjectSelected
    };
}

//void *ed_CreateLightObject(vec3_t *position, mat3_t *orientation, vec3_t *scale, void *args)
//{
//    struct ed_light_args_t *light_args = (struct ed_light_args_t *)args;
//    return r_CreateLight(light_args->type, position, &light_args->color, light_args->radius, light_args->energy);
//}

//void ed_DestroyLightObject(void *base_obj)
//{
//
//}

//struct r_i_draw_list_t *ed_RenderPickLightObject(struct ed_obj_t *object, struct r_i_cmd_buffer_t *cmd_buffer)
//{
//    return NULL;
//}
//
//struct r_i_draw_list_t *ed_RenderOutlineLightObject(struct ed_obj_result_t *object, struct r_i_cmd_buffer_t *cmd_buffer)
//{
//    return NULL;
//}
//
//void ed_UpdateLightHandleObject(struct ed_obj_t *object)
//{
//
//}
//
//void ed_UpdateLightBaseObject(struct ed_obj_result_t *object)
//{
//
//}
