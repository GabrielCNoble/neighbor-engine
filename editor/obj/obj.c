#include <stdio.h>
#include "../engine/r_defs.h"
#include "../engine/r_main.h"
#include "../engine/r_draw_i.h"
#include "../engine/input.h"
#include "../engine/gui.h"
#include "obj.h"
#include "brush.h"
#include "light.h"

struct ed_obj_funcs_t           ed_obj_funcs[ED_OBJ_TYPE_LAST] = {};
struct ed_operator_funcs_t      ed_operator_funcs[ED_OPERATOR_LAST] = {};
//struct r_model_t *              ed_translation_widget_model;
//struct r_model_t *              ed_rotation_widget_model;
extern mat4_t                   r_view_projection_matrix;
extern mat4_t                   r_camera_matrix;
extern float                    r_fov;
extern float                    r_z_near;
extern float                    r_z_far;

struct r_model_t *              ed_transform_operator_models[ED_TRANSFORM_OPERATOR_MODE_LAST];
float                           ed_transform_operator_zoom[ED_TRANSFORM_OPERATOR_MODE_LAST] = {
    [ED_TRANSFORM_OPERATOR_MODE_TRANSLATE] = 35.0,
    [ED_TRANSFORM_OPERATOR_MODE_ROTATE] = 1.0
};

char *ed_obj_names[ED_OBJ_TYPE_LAST] = {
    [ED_OBJ_TYPE_BRUSH] = "Brush",
    [ED_OBJ_TYPE_LIGHT] = "Light",
    [ED_OBJ_TYPE_ENTITY] = "Entity"
};

struct r_i_draw_state_t ed_obj_render_pick_states[ED_OBJ_TYPE_LAST] = {
    [ED_OBJ_TYPE_BRUSH] = {
        .rasterizer = &(struct r_i_raster_t){
            .polygon_mode = GL_FILL,
        }
    },

    [ED_OBJ_TYPE_ENTITY] = {
        .rasterizer = &(struct r_i_raster_t){
            .polygon_mode = GL_FILL,
        }
    },
};

uint32_t ed_obj_all_types[] = {
    ED_OBJ_TYPE_BRUSH,
    ED_OBJ_TYPE_ENTITY,
    ED_OBJ_TYPE_LIGHT,
};


extern uint32_t             r_width;
extern uint32_t             r_height;
extern mat4_t               r_view_projection_matrix;
struct r_framebuffer_t *    ed_pick_framebuffer;
struct r_i_cmd_buffer_t     ed_pick_cmd_buffer;
struct r_shader_t *         ed_picking_shader;
struct r_shader_t *         ed_outline_shader;

void ed_TransformOperatorUpdate(struct ed_obj_context_t *context, struct ed_operator_t *operator)
{
    operator->objects.cursor = 0;

    if(context->selections.cursor)
    {
        uint32_t index = ds_list_add_element(&operator->objects, NULL);
        struct ed_obj_t *operator_object = ds_list_get_element(&operator->objects, index);

        operator_object->context = context;
        operator_object->index = index;
        operator_object->type = ED_OPERATOR_TRANSFORM;
        operator_object->base_obj = operator;
        mat4_t_identity(&operator_object->transform);

        vec3_t translation = vec3_t_c(0, 0, 0);

        for(uint32_t index = 0; index < context->selections.cursor; index++)
        {
            struct ed_obj_result_t *object = ds_list_get_element(&context->selections, index);
            vec3_t_add(&translation, &translation, &object->object->transform.rows[3].xyz);
        }

        vec3_t_div(&translation, &translation, (float)context->selections.cursor);

        operator_object->transform.rows[3].xyz = translation;
    }
}

void ed_TransformOperatorTransforms(struct ed_obj_t *object, mat4_t *transforms)
{
    struct ed_operator_t *operator = object->base_obj;
    struct ed_transform_operator_data_t *data = operator->data;
    mat4_t local_transform;

    mat4_t_identity(&local_transform);
    mat4_t_rotate_y(&local_transform, 0.5);
    mat4_t_mul(&local_transform, &local_transform, &object->transform);
    ed_CameraSpaceDrawTransform(&local_transform, &local_transform, ed_transform_operator_zoom[data->mode]);
    vec3_t_add(&local_transform.rows[3].xyz, &local_transform.rows[3].xyz, &object->transform.rows[3].xyz);
    mat4_t_mul(&transforms[0], &local_transform, &r_view_projection_matrix);

    mat4_t_identity(&local_transform);
    mat4_t_rotate_x(&local_transform, -0.5);
    mat4_t_mul(&local_transform, &local_transform, &object->transform);
    ed_CameraSpaceDrawTransform(&local_transform, &local_transform, ed_transform_operator_zoom[data->mode]);
    vec3_t_add(&local_transform.rows[3].xyz, &local_transform.rows[3].xyz, &object->transform.rows[3].xyz);
    mat4_t_mul(&transforms[1], &local_transform, &r_view_projection_matrix);

    ed_CameraSpaceDrawTransform(&object->transform, &local_transform, ed_transform_operator_zoom[data->mode]);
    vec3_t_add(&local_transform.rows[3].xyz, &local_transform.rows[3].xyz, &object->transform.rows[3].xyz);
    mat4_t_mul(&transforms[2], &local_transform, &r_view_projection_matrix);
}

void ed_DrawTransformOperatorAxis(struct ed_obj_t *object, mat4_t *transforms, struct r_i_cmd_buffer_t *command_buffer, struct r_i_draw_list_t *draw_list, uint32_t first_range)
{
    struct ed_operator_t *operator = object->base_obj;
    struct r_i_draw_range_t *ranges = draw_list->ranges + first_range;

    for(uint32_t axis = 0; axis < 3; axis++)
    {
        struct r_i_uniform_t uniform = {
            .uniform = ED_PICK_SHADER_UNIFORM_MODEL_VIEW_PROJECTION_MATRIX,
            .value = &transforms[axis],
            .count = 1,
        };

        ranges[axis].start = 0;
        ranges[axis].count = draw_list->mesh->indices.count;
        r_i_SetUniforms(command_buffer, &ranges[axis], &uniform, 1);
    }
}

struct r_i_draw_list_t *ed_PickTransformOperatorObject(struct ed_obj_t *object, struct r_i_cmd_buffer_t *command_buffer, struct ed_obj_event_t *event)
{
    struct ed_operator_t *operator = object->base_obj;
    struct ed_transform_operator_data_t *data = operator->data;
    struct r_i_draw_list_t *draw_list = NULL;

    if(operator->objects.cursor)
    {
        draw_list = r_i_AllocDrawList(command_buffer, 3);
        draw_list->mesh = r_i_AllocMeshForModel(command_buffer, ed_transform_operator_models[data->mode]);

        mat4_t model_view_projection_matrix[3];
        ed_TransformOperatorTransforms(object, model_view_projection_matrix);

        for(uint32_t index = 0; index < 3; index++)
        {
            struct r_i_uniform_t uniform = {
                .uniform = ED_PICK_SHADER_UNIFORM_OBJ_EXTRA0,
                .value = &(uint32_t){index},
                .count = 1
            };

            r_i_SetUniforms(command_buffer, &draw_list->ranges[index], &uniform, 1);
        }

        ed_DrawTransformOperatorAxis(object, model_view_projection_matrix, command_buffer, draw_list, 0);

        draw_list->mode = GL_TRIANGLES;
        draw_list->indexed = 1;
    }

    return draw_list;
}

struct r_i_draw_list_t *ed_DrawTransformOperatorObject(struct ed_obj_t *object, struct r_i_cmd_buffer_t *command_buffer)
{
    struct ed_operator_t *operator = object->base_obj;
    struct ed_transform_operator_data_t *data = operator->data;
    struct r_i_draw_list_t *draw_list = NULL;

    if(operator->objects.cursor)
    {
        draw_list = r_i_AllocDrawList(command_buffer, 6);
        mat4_t model_view_projection_matrix[3];
        draw_list->mesh = r_i_AllocMeshForModel(command_buffer, ed_transform_operator_models[data->mode]);

        ed_TransformOperatorTransforms(object, model_view_projection_matrix);

        struct r_i_depth_t depth_state = {
            .enable = R_I_ENABLE,
            .func = GL_ALWAYS,
        };

        struct r_i_draw_mask_t draw_mask_state = {
            .red = GL_FALSE,
            .green = GL_FALSE,
            .blue = GL_FALSE,
            .alpha = GL_FALSE,
            .depth = GL_TRUE,
            .stencil = 0xff,
        };

        r_i_SetDepth(command_buffer, &draw_list->ranges[0], &depth_state);
        r_i_SetDrawMask(command_buffer, &draw_list->ranges[0], &draw_mask_state);
        ed_DrawTransformOperatorAxis(object, model_view_projection_matrix, command_buffer, draw_list, 0);

        depth_state = (struct r_i_depth_t){
            .enable = R_I_ENABLE,
            .func = GL_LEQUAL,
        };

        draw_mask_state = (struct r_i_draw_mask_t) {
            .red = GL_TRUE,
            .green = GL_TRUE,
            .blue = GL_TRUE,
            .alpha = GL_TRUE,
            .depth = GL_TRUE,
            .stencil = 0xff,
        };

        r_i_SetShader(command_buffer, ed_outline_shader);

        vec4_t axis_colors[] = {
            vec4_t_c(1.0, 0.0, 0.0, 1.0),
            vec4_t_c(0.0, 1.0, 0.0, 1.0),
            vec4_t_c(0.0, 0.0, 1.0, 1.0),
        };

        for(uint32_t index = 0; index < 3; index++)
        {
            struct r_i_uniform_t uniform = {
                .uniform = ED_OUTLINE_SHADER_UNIFORM_COLOR,
                .value = &axis_colors[index],
                .count = 1
            };

            r_i_SetUniforms(command_buffer, &draw_list->ranges[index + 3], &uniform, 1);
        }

        r_i_SetDepth(command_buffer, &draw_list->ranges[3], &depth_state);
        r_i_SetDrawMask(command_buffer, &draw_list->ranges[3], &draw_mask_state);
        ed_DrawTransformOperatorAxis(object, model_view_projection_matrix, command_buffer, draw_list, 3);

        draw_list->mode = GL_TRIANGLES;
        draw_list->indexed = 1;
    }

    return draw_list;
}

void ed_ObjInit()
{
    struct r_framebuffer_desc_t framebuffer_desc = {
        .color_attachments[0] = &(struct r_texture_desc_t) {
            .format = R_FORMAT_RGBA32UI,
            .min_filter = GL_NEAREST,
            .mag_filter = GL_NEAREST,
        },
        .depth_attachment = &(struct r_texture_desc_t) {
            .format = R_FORMAT_DEPTH32F,
            .min_filter = GL_LINEAR,
            .mag_filter = GL_LINEAR
        },
        .width = r_width,
        .height = r_height
    };
    ed_pick_framebuffer = r_CreateFramebuffer(&framebuffer_desc);

    ed_pick_cmd_buffer = r_AllocImmediateCmdBuffer(sizeof(struct r_i_cmd_t));
    struct r_shader_desc_t shader_desc;

    shader_desc = (struct r_shader_desc_t){
        .vertex_code = "shaders/ed_pick.vert",
        .fragment_code = "shaders/ed_pick.frag",
        .vertex_layout = &R_DEFAULT_VERTEX_LAYOUT,
        .uniforms = (struct r_uniform_t[]) {
            [ED_PICK_SHADER_UNIFORM_MODEL_VIEW_PROJECTION_MATRIX] = R_DEFAULT_UNIFORMS[R_UNIFORM_MODEL_VIEW_PROJECTION_MATRIX],
            [ED_PICK_SHADER_UNIFORM_OBJ_TYPE] = {
                .name = "ed_obj_type",
                .type = R_UNIFORM_TYPE_INT,
            },
            [ED_PICK_SHADER_UNIFORM_OBJ_INDEX] = {
                .name = "ed_obj_index",
                .type = R_UNIFORM_TYPE_INT
            },
            [ED_PICK_SHADER_UNIFORM_OBJ_EXTRA0] = {
                .name = "ed_obj_extra0",
                .type = R_UNIFORM_TYPE_INT,
            },
            [ED_PICK_SHADER_UNIFORM_OBJ_EXTRA1] = {
                .name = "ed_obj_extra1",
                .type = R_UNIFORM_TYPE_INT,
            },
        },
        .uniform_count = 5
    };
    ed_picking_shader = r_LoadShader(&shader_desc);

    shader_desc = (struct r_shader_desc_t){
        .vertex_code = "shaders/ed_outline.vert",
        .fragment_code = "shaders/ed_outline.frag",
        .vertex_layout = &R_DEFAULT_VERTEX_LAYOUT,
        .uniforms = (struct r_uniform_t[]) {
            [ED_OUTLINE_SHADER_UNIFORM_MODEL_VIEW_PROJECTION_MATRIX] = R_DEFAULT_UNIFORMS[R_UNIFORM_MODEL_VIEW_PROJECTION_MATRIX],
            [ED_OUTLINE_SHADER_UNIFORM_COLOR] = {
                .name = "ed_color",
                .type = R_UNIFORM_TYPE_VEC4,
            },
        },
        .uniform_count = 2
    };
    ed_outline_shader = r_LoadShader(&shader_desc);

    ed_transform_operator_models[ED_TRANSFORM_OPERATOR_MODE_TRANSLATE] = r_LoadModel("models/twidget.mof");
    ed_transform_operator_models[ED_TRANSFORM_OPERATOR_MODE_ROTATE] = r_LoadModel("models/rwidget.mof");

    ed_operator_funcs[ED_OPERATOR_TRANSFORM] = (struct ed_operator_funcs_t) {
        .update = ed_TransformOperatorUpdate,
        .obj_funcs = (struct ed_obj_funcs_t){
            .pick = ed_PickTransformOperatorObject,
            .draw = ed_DrawTransformOperatorObject
        }
    };

//    ed_operator_funcs[ED_OPERATOR_ROTATE] = (struct ed_operator_funcs_t) {
//        .update = ed_TranslateOperatorUpdate,
//        .obj_funcs = (struct ed_obj_funcs_t){
//            .pick = ed_PickTranslateOperatorObject,
//            .draw = ed_DrawTranslateOperatorObject
//        }
//    };


}

void ed_ObjShutdown()
{
    r_DestroyFramebuffer(ed_pick_framebuffer);
}

struct ed_obj_context_t ed_CreateObjContext(void **operator_data)
{
    struct ed_obj_context_t context = {};

    for(uint32_t type = ED_OBJ_TYPE_BRUSH; type < ED_OBJ_TYPE_LAST; type++)
    {
        context.objects[type] = ds_slist_create(sizeof(struct ed_obj_t), 512);
    }

    for(uint32_t type = ED_OPERATOR_TRANSFORM; type < ED_OPERATOR_LAST; type++)
    {
        context.operators[type].objects = ds_list_create(sizeof(struct ed_obj_t), 16);
        context.operators[type].visible = 0;
        context.operators[type].data = operator_data[type];
    }

//    context.operators[ED_OPERATOR_TRANSLATE].model = ed_translation_widget_model;
//    context.operators[ED_OPERATOR_TRANSLATE].update_callback = NULL;
//    context.operators[ED_OPERATOR_ROTATE].model = ed_rotation_widget_model;
//    context.operators[ED_OPERATOR_ROTATE].update_callback = NULL;

//    ed_operator_funcs[ED_OPERATOR_TRANSLATE].update(&context, &context.operators[ED_OPERATOR_TRANSLATE]);

    context.selections = ds_list_create(sizeof(struct ed_obj_result_t ), 512);
//    context.cmd_buffer = r_AllocImmediateCmdBuffer(4096);

    return context;
}

void ed_DestroyObjContext(struct ed_obj_context_t *context)
{
    for(uint32_t type = ED_OBJ_TYPE_BRUSH; type < ED_OBJ_TYPE_LAST; type++)
    {
        ds_slist_destroy(&context->objects[type]);
    }

    for(uint32_t type = ED_OPERATOR_TRANSFORM; type < ED_OPERATOR_LAST; type++)
    {
        ds_list_destroy(&context->operators[type].objects);
    }

    ds_list_destroy(&context->selections);
}

struct ed_obj_t *ed_CreateObj(struct ed_obj_context_t *context, uint32_t type, vec3_t *position, mat3_t *orientation, vec3_t *scale, void *args)
{
    struct ed_obj_t *object = NULL;

    if(context && type < ED_OBJ_TYPE_LAST)
    {
        uint32_t index = ds_slist_add_element(&context->objects[type], NULL);
        object = ds_slist_get_element(&context->objects[type], index);

        object->index = index;
        object->type = type;
        object->selection_index = ED_INVALID_OBJ_SELECTION_INDEX;
        object->base_obj = ed_obj_funcs[type].create(position, orientation, scale, args);
        object->context = context;

        ed_obj_funcs[type].update(object, NULL);
    }

    return object;
}

void ed_DestroyObj(struct ed_obj_context_t *context, struct ed_obj_t *object)
{
    if(context && object)
    {
        ed_obj_funcs[object->type].destroy(object);
        ds_slist_remove_element(&context->objects[object->type], object->index);
        object->index = 0xffffffff;
    }
}

struct ed_obj_t *ed_GetObject(struct ed_obj_context_t *context, struct ed_obj_h handle)
{
    struct ed_obj_t *obj = NULL;

    if(context && handle.type < ED_OBJ_TYPE_LAST)
    {
        obj = ds_slist_get_element(&context->objects[handle.type], handle.index);

        if(obj && obj->index == 0xffffffff)
        {
            obj = NULL;
        }
    }

    return obj;
}

void ed_BeginPick(struct ed_obj_context_t *context)
{
    struct r_i_framebuffer_t framebuffer = { .framebuffer = ed_pick_framebuffer };
    struct r_i_depth_t depth = { .enable = R_I_ENABLE, .func = GL_LESS };
    struct r_i_stencil_t stencil = { .enable = R_I_DISABLE };
    struct r_i_blending_t blending = { .enable = R_I_DISABLE };
    struct r_i_clear_t clear = {
        .r = 0.0, .g = 0.0, .b = 0.0, .a = 0.0, .depth = 1.0,
        .bitmask = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT
    };

    r_i_SetFramebuffer(&ed_pick_cmd_buffer, NULL, &framebuffer);
    r_i_SetDepth(&ed_pick_cmd_buffer, NULL, &depth);
    r_i_SetStencil(&ed_pick_cmd_buffer, NULL, &stencil);
    r_i_SetBlending(&ed_pick_cmd_buffer, NULL, &blending);
    r_i_Clear(&ed_pick_cmd_buffer, NULL, &clear);
    r_i_SetShader(&ed_pick_cmd_buffer, ed_picking_shader);
}

struct ed_obj_result_t ed_EndPick(struct ed_obj_context_t *context, int32_t mouse_x, int32_t mouse_y)
{
    struct ed_obj_result_t result = {.index = 0xffffffff, .type = 0xffffffff};

    r_RunImmCmdBuffer(&ed_pick_cmd_buffer);
    mouse_y = r_height - (mouse_y + 1);
    int32_t pick[4] = {};
    r_SampleFramebuffer(ed_pick_framebuffer, mouse_x, mouse_y, 1, 1, sizeof(pick), pick);

    if(pick[0])
    {
//        result.object = ed_GetObject(context, (struct ed_obj_h){.type = pick[1] - 1, .index = pick[0] - 1});
        result.index = pick[0] - 1;
        result.type = pick[1] - 1;
        result.extra0 = pick[2];
        result.extra1 = pick[3];
    }

    return result;
}

void ed_PickObjectWithFuncs(struct ed_obj_t *object, struct ed_obj_funcs_t *funcs)
{
    struct r_i_draw_list_t *draw_list = funcs->pick(object, &ed_pick_cmd_buffer, NULL);
    uint32_t pick_type = object->type + 1;
    uint32_t pick_index = object->index + 1;

    if(draw_list != NULL)
    {
        struct r_i_uniform_t uniforms[] = {
            [0] = {
                .uniform = ED_PICK_SHADER_UNIFORM_OBJ_TYPE,
                .value = &pick_type,
                .count = 1,
            },
            [1] = {
                .uniform = ED_PICK_SHADER_UNIFORM_OBJ_INDEX,
                .value = &pick_index,
                .count = 1,
            },
        };
        r_i_SetUniforms(&ed_pick_cmd_buffer, NULL, uniforms, 2);
        r_i_DrawList(&ed_pick_cmd_buffer, draw_list);
    }
}

void ed_PickObjectFromListWithFuncs(struct ds_slist_t *objects, struct ed_obj_funcs_t *funcs)
{
    for(uint32_t obj_index = 0; obj_index < objects->cursor; obj_index++)
    {
        struct ed_obj_t *object = ds_slist_get_element(objects, obj_index);

        if(object != NULL && object->index != ED_INVALID_OBJ_INDEX)
        {
            ed_PickObjectWithFuncs(object, funcs);
        }
    }
}

struct ed_obj_result_t ed_PickObjectWithFilter(struct ed_obj_context_t *context, int32_t mouse_x, int32_t mouse_y, uint32_t *types, uint32_t type_count)
{
    ed_BeginPick(context);

    for(uint32_t type_index = 0; type_index < type_count; type_index++)
    {
        struct ed_obj_funcs_t *funcs = ed_obj_funcs + types[type_index];
        struct ds_slist_t *objects = context->objects + types[type_index];
        if(ed_obj_render_pick_states[types[type_index]].rasterizer)
        {
            r_i_SetRasterizer(&ed_pick_cmd_buffer, NULL, ed_obj_render_pick_states[types[type_index]].rasterizer);
        }
        ed_PickObjectFromListWithFuncs(objects, funcs);
    }

    return ed_EndPick(context, mouse_x, mouse_y);
}

struct ed_obj_result_t ed_PickObject(struct ed_obj_context_t *context, int32_t mouse_x, int32_t mouse_y)
{
    struct ed_obj_result_t result = ed_PickObjectWithFilter(context, mouse_x, mouse_y, ed_obj_all_types, 3);
    result.object = ed_GetObject(context, (struct ed_obj_h){.type = result.type, .index = result.index});
    return result;
}

struct ed_obj_result_t ed_PickOperator(struct ed_obj_context_t *context, int32_t mouse_x, int32_t mouse_y)
{
    ed_BeginPick(context);

    for(uint32_t operator_type = ED_OPERATOR_TRANSFORM; operator_type < ED_OPERATOR_LAST; operator_type++)
    {
        struct ed_operator_t *operator = context->operators + operator_type;
        struct ed_operator_funcs_t *funcs = ed_operator_funcs + operator_type;
        for(uint32_t instance_index = 0; instance_index < operator->objects.cursor; instance_index++)
        {
            struct ed_obj_t *operator_object = ds_list_get_element(&operator->objects, instance_index);
            ed_PickObjectWithFuncs(operator_object, &funcs->obj_funcs);
        }
    }

    return ed_EndPick(context, mouse_x, mouse_y);
}

void ed_UpdateOperator(struct ed_obj_context_t *context, struct ed_operator_t *operator)
{
    struct ed_operator_funcs_t *funcs = ed_operator_funcs + operator->type;
    operator->objects.cursor = 0;
    funcs->update(context, operator);
}

void ed_UpdateOperators(struct ed_obj_context_t *context)
{
    for(uint32_t type = ED_OPERATOR_TRANSFORM; type < ED_OPERATOR_LAST; type++)
    {
        struct ed_operator_t *operator = context->operators + type;
        struct ed_operator_funcs_t *funcs = ed_operator_funcs + type;
        operator->objects.cursor = 0;
        funcs->update(context, operator);
    }
}

void ed_ApplyOperatorOnSelection(struct ed_obj_context_t *context, struct ed_operator_t *operator, struct ed_operator_event_t *event, uint32_t selection_index)
{
    struct ed_obj_result_t *result = ds_list_get_element(&context->selections, selection_index);
    struct ed_obj_funcs_t *funcs = ed_obj_funcs + result->object->type;
    funcs->update(result->object, event);
}

void ed_ApplyOperator(struct ed_obj_context_t *context, struct ed_operator_t *operator, struct ed_operator_event_t *event)
{
    event->operator = operator;

    for(uint32_t index = 0; index < context->selections.cursor; index++)
    {
        ed_ApplyOperatorOnSelection(context, operator, event, index);
    }

    ed_UpdateOperators(context);
}

void ed_UpdateObjectContext(struct ed_obj_context_t *context, struct r_i_cmd_buffer_t *command_buffer)
{
    r_i_SetShader(command_buffer, NULL);

    for(uint32_t type = ED_OBJ_TYPE_BRUSH; type < ED_OBJ_TYPE_LAST; type++)
    {
        struct ed_obj_funcs_t *funcs = ed_obj_funcs + type;
        if(funcs->draw != NULL)
        {
            for(uint32_t index = 0; index < context->objects[type].cursor; index++)
            {
                struct ed_obj_t *object = ed_GetObject(context, (struct ed_obj_h){.index = index, .type = type});
                if(object != NULL)
                {
                    struct r_i_draw_list_t *draw_list = funcs->draw(object, command_buffer);

                    if(draw_list != NULL)
                    {
                        r_i_DrawList(command_buffer, draw_list);
                    }
                }
            }
        }
    }

    /* draw selections */
    r_i_SetShader(command_buffer, ed_outline_shader);

    for(uint32_t selection_index = 0; selection_index < context->selections.cursor; selection_index++)
    {
        struct ed_obj_result_t *object = (struct ed_obj_result_t *)ds_list_get_element(&context->selections, selection_index);
        struct ed_obj_funcs_t *funcs = ed_obj_funcs + object->object->type;
        struct r_i_draw_list_t *draw_list = funcs->draw_selected(object->object, command_buffer);

        if(draw_list != NULL)
        {
            r_i_DrawList(command_buffer, draw_list);
        }
    }

    struct r_i_draw_mask_t draw_mask_state = {
        .red = GL_TRUE,
        .green = GL_TRUE,
        .blue = GL_TRUE,
        .alpha = GL_TRUE,
        .depth = GL_TRUE
    };

    struct r_i_depth_t depth_state = {
        .enable = R_I_ENABLE,
        .func = GL_LESS,
    };

    struct r_i_blending_t blend_state = {
        .enable = R_I_DISABLE
    };

    r_i_SetDrawMask(command_buffer, NULL, &draw_mask_state);
    r_i_SetDepth(command_buffer, NULL, &depth_state);
    r_i_SetBlending(command_buffer, NULL, &blend_state);

    /* draw operators */

    for(uint32_t type = ED_OPERATOR_TRANSFORM; type < ED_OPERATOR_LAST; type++)
    {
        struct ed_operator_funcs_t *func = ed_operator_funcs + type;
        struct ed_operator_t *operator = context->operators + type;
        if(operator->objects.cursor && operator->visible)
        {
            for(uint32_t object_index = 0; object_index < operator->objects.cursor; object_index++)
            {
                struct ed_obj_t *operator_object = ds_list_get_element(&operator->objects, object_index);
                struct r_i_draw_list_t *draw_list = func->obj_funcs.draw(operator_object, command_buffer);

                if(draw_list != NULL)
                {
                    r_i_DrawList(command_buffer, draw_list);
                }
            }
        }
    }
}

//void ed_DrawSelections(struct ed_obj_context_t *context, struct r_i_cmd_buffer_t *cmd_buffer)
//{
//    r_i_SetShader(cmd_buffer, ed_outline_shader);
//
//    for(uint32_t selection_index = 0; selection_index < context->selections.cursor; selection_index++)
//    {
//        struct ed_obj_result_t *object = (struct ed_obj_result_t *)ds_list_get_element(&context->selections, selection_index);
//        struct ed_obj_funcs_t *funcs = ed_obj_funcs + object->object->type;
//        struct r_i_draw_list_t *draw_list = funcs->draw_selected(object->object, cmd_buffer);
//
//        if(draw_list != NULL)
//        {
//            r_i_DrawList(cmd_buffer, draw_list);
//        }
//    }
//}

void ed_AddObjToSelections(struct ed_obj_context_t *context, uint32_t multiple, struct ed_obj_result_t *obj)
{
    if(context != NULL && obj != NULL && obj->object != NULL && obj->object->context == context)
    {
        if(obj->object->selection_index != ED_INVALID_OBJ_SELECTION_INDEX)
        {
            uint32_t last_index = context->selections.cursor - 1;
            uint32_t selection_index = obj->object->selection_index;

            /* This selection already exists in the list. In this case, it can either be the
            main selection (last in the list), in which case it'll be dropped from the list, or
            it can be some other selection, in which case it'll be re-added at the back of the
            list, becoming the main selection. Either way, we need to remove it here. */
            ed_DropObjFromSelections(context, obj);

            if(selection_index >= last_index)
            {
                /* pickable is the last in the list */
                if(multiple || !context->selections.cursor)
                {
                    /* the behavior is, if this is the last pickable, there are more pickables
                    in the list and the multiple selection key is down, this pickable gets dropped.
                    If, instead, this is the last pickable, there are more pickables in the list
                    but the key is not being held, then this pickable becomes the only pickable in
                    the list. If, then, it gets selected again, without the multiple selection key
                    down, it gets dropped. */
                    return;
                }
            }
        }

        if(!multiple)
        {
            ed_ClearSelections(context);
        }

        /* This is either a new selection, or an already existing selection becoming the main selection. */
        obj->object->selection_index = ds_list_add_element(&context->selections, obj);

        ed_UpdateOperators(context);
    }
}

void ed_DropObjFromSelections(struct ed_obj_context_t *context, struct ed_obj_result_t *obj)
{
    if(context != NULL && obj != NULL && obj->object != NULL && obj->object->context == context)
    {
        if(obj->object->selection_index != ED_INVALID_OBJ_SELECTION_INDEX)
        {
            uint32_t last_index = context->selections.cursor - 1;
            ds_list_remove_element(&context->selections, obj->object->selection_index);

            if(context->selections.cursor && obj->object->selection_index != last_index)
            {
                struct ed_obj_result_t *moved_obj = (struct ed_obj_result_t *)ds_list_get_element(&context->selections, obj->object->selection_index);
                moved_obj->object->selection_index = obj->object->selection_index;
            }

            obj->object->selection_index = ED_INVALID_OBJ_SELECTION_INDEX;

            ed_UpdateOperators(context);
        }
    }
}

void ed_ClearSelections(struct ed_obj_context_t *context)
{
    for(uint32_t index = 0; index < context->selections.cursor; index++)
    {
        struct ed_obj_result_t *obj = (struct ed_obj_result_t *)ds_list_get_element(&context->selections, index);
        obj->object->selection_index = ED_INVALID_OBJ_SELECTION_INDEX;
    }

    context->selections.cursor = 0;

    ed_UpdateOperators(context);

//    struct ed_operator_t *translate_operator = context->operators + ED_OPERATOR_TRANSLATE;
//    struct ed_operator_funcs_t *funcs = ed_operator_funcs + ED_OPERATOR_TRANSLATE;
//
//    funcs->update(context, translate_operator);
}

/*
============================================================================
============================================================================
============================================================================
*/

uint32_t ed_LeftClickPickState(struct ed_tool_context_t *context, struct ed_tool_t *tool, uint32_t just_changed)
{
    if(in_GetMouseButtonState(SDL_BUTTON_LEFT) & IN_KEY_STATE_JUST_PRESSED)
    {
        struct ed_obj_context_t *obj_context = (struct ed_obj_context_t *)tool->data;
        int32_t mouse_x;
        int32_t mouse_y;

        in_GetMousePos(&mouse_x, &mouse_y);

        obj_context->last_picked = ed_PickOperator(obj_context, mouse_x, mouse_y);

        if(obj_context->last_picked.index != 0xffffffff && obj_context->last_picked.type != 0xffffffff)
        {
            ed_NextToolState(context, ed_ApplyOperatorState);
            return 0;
        }

        obj_context->last_picked = ed_PickObject(obj_context, mouse_x, mouse_y);

        if(obj_context->last_picked.index != 0xffffffff && obj_context->last_picked.type != 0xffffffff)
        {
            uint32_t shift_down = in_GetKeyState(SDL_SCANCODE_LSHIFT) & IN_KEY_STATE_PRESSED;
            ed_AddObjToSelections(obj_context, shift_down, &obj_context->last_picked);
        }

        ed_NextToolState(context, NULL);
    }

    return 0;
}

uint32_t ed_ApplyOperatorState(struct ed_tool_context_t *context, struct ed_tool_t *tool, uint32_t just_changed)
{
//    struct ed_level_state_t *context_data = &ed_level_state;
    struct ed_obj_context_t *obj_context = tool->data;
    uint32_t mouse_state = in_GetMouseButtonState(SDL_BUTTON_LEFT);

    if(mouse_state & IN_KEY_STATE_PRESSED)
    {
//        struct ed_widget_t *manipulator = context_data->manipulator.widgets[context_data->manipulator.transform_type];

        struct ed_operator_event_t event;
        struct ed_operator_t *transform_operator = &obj_context->operators[ED_OPERATOR_TRANSFORM];
        struct ed_transform_operator_data_t *data = transform_operator->data;
        struct ed_obj_t *operator_obj = ds_list_get_element(&transform_operator->objects, 0);
        uint32_t axis_index = obj_context->last_picked.extra0;
        mat4_t *operator_transform = &operator_obj->transform;
        vec3_t axis_vec = operator_transform->rows[obj_context->last_picked.extra0].xyz;
        vec3_t intersection;
        vec4_t axis_color[] =
        {
            vec4_t_c(1.0, 0.0, 0.0, 1.0),
            vec4_t_c(0.0, 1.0, 0.0, 1.0),
            vec4_t_c(0.0, 0.0, 1.0, 1.0),
        };
        int32_t mouse_x;
        int32_t mouse_y;

        in_GetMousePos(&mouse_x, &mouse_y);

//        r_i_SetShader(NULL);
//        r_i_SetViewProjectionMatrix(NULL);
//        r_i_SetModelMatrix(NULL);

        switch(data->mode)
        {
            case ED_TRANSFORM_OPERATOR_MODE_TRANSLATE:
            {
                event.transform_event.type = ED_TRANSFORM_OPERATOR_MODE_TRANSLATE;

                vec3_t manipulator_cam_vec;
                vec3_t_sub(&manipulator_cam_vec, &r_camera_matrix.rows[3].xyz, &operator_transform ->rows[3].xyz);
                float proj = vec3_t_dot(&manipulator_cam_vec, &axis_vec);

                vec3_t plane_normal;
                vec3_t_fmadd(&plane_normal, &manipulator_cam_vec, &axis_vec, -proj);
                vec3_t_normalize(&plane_normal, &plane_normal);

                ed_CameraRay(mouse_x, mouse_y, &operator_transform->rows[3].xyz, &plane_normal, &intersection);

                vec3_t cur_offset;
                vec3_t_sub(&cur_offset, &intersection, &operator_transform->rows[3].xyz);
                proj = vec3_t_dot(&cur_offset, &axis_vec);
                vec3_t_mul(&cur_offset, &axis_vec, proj);

                if(just_changed)
                {
                    data->start_pos = operator_transform->rows[3].xyz;
                    data->prev_offset = cur_offset;
                }

                vec3_t_sub(&cur_offset, &cur_offset, &data->prev_offset);

                if(data->linear_snap)
                {
                    for(uint32_t index = 0; index < 3; index++)
                    {
                        float f = floorf(cur_offset.comps[index] / data->linear_snap);
                        cur_offset.comps[index] = data->linear_snap * f;
                    }
                }

                event.transform_event.translation.translation = cur_offset;

                int32_t window_x;
                int32_t window_y;
                vec3_t pos = operator_transform->rows[3].xyz;
                vec3_t disp;
                vec3_t_sub(&disp, &pos, &data->start_pos);


                ed_PointPixelCoords(&window_x, &window_y, &operator_transform->rows[3].xyz);
                igSetNextWindowPos((ImVec2){window_x, window_y}, 0, (ImVec2){0.0, 0.0});
                igSetNextWindowBgAlpha(0.25);
                if(igBegin("displacement", NULL, ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoTitleBar |
                                             ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDecoration))
                {
                    igText("disp: [%f m, %f m, %f m]", disp.x, disp.y, disp.z);
                    igText("pos: [%f, %f, %f]", pos.x, pos.y, pos.z);
                }
                igEnd();

            }
            break;

            case ED_TRANSFORM_OPERATOR_MODE_ROTATE:
            {
                event.transform_event.type = ED_TRANSFORM_OPERATOR_MODE_ROTATE;

                ed_CameraRay(mouse_x, mouse_y, &operator_transform->rows[3].xyz, &axis_vec, &intersection);

                vec3_t manipulator_mouse_vec;
                vec3_t_sub(&manipulator_mouse_vec, &intersection, &operator_transform->rows[3].xyz);
                vec3_t_normalize(&manipulator_mouse_vec, &manipulator_mouse_vec);

                if(just_changed)
                {
                    data->prev_offset = manipulator_mouse_vec;
                }

                vec3_t angle_vec;
                vec3_t_cross(&angle_vec, &manipulator_mouse_vec, &data->prev_offset);
                float angle = asin(vec3_t_dot(&angle_vec, &axis_vec)) / 3.14159265;
                mat3_t_identity(&event.transform_event.rotation.rotation);

                if(data->angular_snap)
                {
                    if(angle > 0.0)
                    {
                        angle = floorf(angle / data->angular_snap);
                    }
                    else
                    {
                        angle = ceilf(angle / data->angular_snap);
                    }

                    angle *= data->angular_snap;
                }

                if(angle)
                {
                    data->prev_offset = manipulator_mouse_vec;
                }



                switch(axis_index)
                {
                    case 0:
                        mat3_t_rotate_x(&event.transform_event.rotation.rotation, angle);
                    break;

                    case 1:
                        mat3_t_rotate_y(&event.transform_event.rotation.rotation, angle);
                    break;

                    case 2:
                        mat3_t_rotate_z(&event.transform_event.rotation.rotation, angle);
                    break;
                }

            }
            break;
        }

        ed_ApplyOperator(obj_context, transform_operator, &event);
    }
    else
    {
        ed_NextToolState(context, NULL);
        obj_context->last_picked.type = 0xffffffff;
        obj_context->last_picked.index = 0xffffffff;
    }
}

uint32_t ed_CameraRay(int32_t mouse_x, int32_t mouse_y, vec3_t *plane_point, vec3_t *plane_normal, vec3_t *result)
{
    vec3_t mouse_pos;
    vec3_t camera_pos;
    vec4_t mouse_vec = {.z = 0.0, .w = 0.0};
    mouse_vec.x = ((float)mouse_x / (float)r_width) * 2.0 - 1.0;
    mouse_vec.y = 1.0 - ((float)mouse_y / (float)r_height) * 2.0;

    float aspect = (float)r_width / (float)r_height;
    float top = tan(r_fov) * r_z_near;
    float right = top * aspect;

    mouse_vec.x *= right;
    mouse_vec.y *= top;
    mouse_vec.z = -r_z_near;

    vec4_t_normalize(&mouse_vec, &mouse_vec);
    mat4_t_vec4_t_mul_fast(&mouse_vec, &r_camera_matrix, &mouse_vec);

    camera_pos = r_camera_matrix.rows[3].xyz;
    vec3_t_add(&mouse_pos, &camera_pos, &mouse_vec.xyz);

    vec3_t plane_vec;
    vec3_t_sub(&plane_vec, &camera_pos, plane_point);
    float dist_a = vec3_t_dot(&plane_vec, plane_normal);

    vec3_t_sub(&plane_vec, &mouse_pos, plane_point);
    float dist_b = vec3_t_dot(&plane_vec, plane_normal);
    float denom = (dist_a - dist_b);

    if(denom)
    {
        float frac = dist_a / denom;

        if(frac >= 0.0)
        {
            vec3_t_fmadd(result, &camera_pos, &mouse_vec.xyz, frac);
            return 1;
        }
    }

    return 0;
}

void ed_PointPixelCoords(int32_t *x, int32_t *y, vec3_t *point)
{
    vec4_t result;
    result.xyz = *point;
    result.w = 1.0;
    mat4_t_vec4_t_mul_fast(&result, &r_view_projection_matrix, &result);
    *x = r_width * ((result.x / result.w) * 0.5 + 0.5);
    *y = r_height * (1.0 - ((result.y / result.w) * 0.5 + 0.5));
}

void ed_CameraSpaceDrawTransform(mat4_t *in_transform, mat4_t *out_transform, float camera_distance)
{
//    mat4_t_identity(out_transform);
    *out_transform = *in_transform;
    vec3_t_sub(&out_transform->rows[3].xyz, &in_transform->rows[3].xyz, &r_camera_matrix.rows[3].xyz);
    vec3_t_normalize(&out_transform->rows[3].xyz, &out_transform->rows[3].xyz);
    vec3_t_mul(&out_transform->rows[3].xyz, &out_transform->rows[3].xyz, camera_distance);
}

/*
============================================================================
============================================================================
============================================================================
*/

void ed_TranslateSelection(struct ed_obj_context_t *context, vec3_t *translation, uint32_t index)
{
    struct ed_operator_event_t event;
    struct ed_operator_t *transform_operator = &context->operators[ED_OPERATOR_TRANSFORM];
    event.transform_event.translation.translation = *translation;
    event.transform_event.type = ED_TRANSFORM_OPERATOR_MODE_TRANSLATE;
    event.operator = transform_operator;
    ed_ApplyOperatorOnSelection(context, transform_operator, &event, index);
    ed_UpdateOperators(context);
}

void ed_RotateSelection(struct ed_obj_context_t *context, mat3_t *rotation, uint32_t index)
{
    struct ed_operator_event_t event;
    struct ed_operator_t *transform_operator = &context->operators[ED_OPERATOR_TRANSFORM];
    event.transform_event.rotation.rotation = *rotation;
    event.transform_event.type = ED_TRANSFORM_OPERATOR_MODE_ROTATE;
    event.operator = transform_operator;
    ed_ApplyOperatorOnSelection(context, transform_operator, &event, index);
    ed_UpdateOperators(context);
}

/*
============================================================================
============================================================================
============================================================================
*/

//void ed_DrawModelOutline(mat4_t *transform, struct r_model_t *model, struct r_i_cmd_buffer_t *command_buffer, struct r_i_draw_list_t *draw_list, vec4_t *outline_color)
//{
//////    struct ed_brush_t *brush = object->base_obj;
//////    struct r_i_draw_list_t *draw_list = NULL;
////    struct r_i_raster_t rasterizer;
////    struct r_i_stencil_t stencil;
////    struct r_i_depth_t depth;
////    struct r_i_draw_mask_t draw_mask;
//////    vec4_t outline_color = vec4_t_c(1.0, 0.5, 0.0, 1.0);
////
////    mat4_t model_view_projection_matrix;
////    mat4_t_mul(&model_view_projection_matrix, transform, &r_view_projection_matrix);
////
////    struct r_i_uniform_t uniforms[2] = {
////        {
////            .uniform = ED_OUTLINE_SHADER_UNIFORM_COLOR,
////            .count = 1,
////            .value = outline_color
////        },
////        {
////            .uniform = ED_OUTLINE_SHADER_UNIFORM_MODEL_VIEW_PROJECTION_MATRIX,
////            .count = 1,
////            .value = &model_view_projection_matrix
////        }
////    };
////
//////    draw_list = r_i_AllocDrawList(command_buffer, 3);
////
////
//////    color = (struct r_i_uniform_t){
//////        .uniform = ED_OUTLINE_SHADER_UNIFORM_COLOR,
//////        .count = 1,
//////        .value = &outline_color
//////    };
////
////    /* fill stencil buffer with 0xff where the model is */
////    rasterizer = (struct r_i_raster_t) {
////        .polygon_mode = GL_FILL,
////        .cull_face = GL_BACK,
////        .cull_enable = R_I_ENABLE,
////        .line_width = 2.0
////    };
////    stencil = (struct r_i_stencil_t) {
////        .enable = R_I_ENABLE,
////        .func = GL_ALWAYS,
////        .stencil_fail = GL_KEEP,
////        .depth_fail = GL_KEEP,
////        .depth_pass = GL_REPLACE,
////        .mask = 0xff,
////        .ref = 0xff
////    };
////
////    draw_mask = (struct r_i_draw_mask_t) {
////        .red = GL_FALSE,
////        .green = GL_FALSE,
////        .blue = GL_FALSE,
////        .alpha = GL_FALSE,
////        .depth = GL_FALSE,
////        .stencil = 0xff
////    };
////
////    depth = (struct r_i_depth_t) {
////        .func = GL_LESS,
////        .enable = R_I_DISABLE
////    };
////
//////    draw
////
////    r_i_SetUniforms(command_buffer, &draw_list->ranges[0], uniforms, 2);
////    r_i_SetRasterizer(command_buffer, &draw_list->ranges[0], &rasterizer);
////    r_i_SetStencil(command_buffer, &draw_list->ranges[0], &stencil);
////    r_i_SetDrawMask(command_buffer, &draw_list->ranges[0], &draw_mask);
////    r_i_SetDepth(command_buffer, &draw_list->ranges[0], &depth);
////    draw_list->ranges[0].start = model->model_start;
////    draw_list->ranges[0].count = model->model_count;
////
////
////
////    /* draw wireframe only where the stencil buffer isn't 0xff */
////    rasterizer = (struct r_i_raster_t) {
////        .polygon_mode = GL_LINE,
////        .cull_face = GL_BACK,
////        .cull_enable = R_I_ENABLE,
////
////    };
////    stencil = (struct r_i_stencil_t) {
////        .enable = R_I_ENABLE,
////        .func = GL_NOTEQUAL,
////        .stencil_fail = GL_KEEP,
////        .depth_fail = GL_KEEP,
////        .depth_pass = GL_KEEP,
////        .mask = 0xff,
////        .ref = 0xff
////    };
////
////    draw_mask = (struct r_i_draw_mask_t) {
////        .red = GL_TRUE,
////        .green = GL_TRUE,
////        .blue = GL_TRUE,
////        .alpha = GL_TRUE,
////        .depth = GL_FALSE,
////        .stencil = 0xff
////    };
////
////    depth = (struct r_i_depth_t) {
////        .func = GL_LESS,
////        .enable = R_I_ENABLE,
////    };
////
////    r_i_SetRasterizer(command_buffer, &draw_list->ranges[1], &rasterizer);
////    r_i_SetStencil(command_buffer, &draw_list->ranges[1], &stencil);
////    r_i_SetDrawMask(command_buffer, &draw_list->ranges[1], &draw_mask);
////    r_i_SetDepth(command_buffer, &draw_list->ranges[1], &depth);
////    draw_list->ranges[1].start = brush->model->model_start;
////    draw_list->ranges[1].count = brush->model->model_count;
////
////
////    /* clear brush stencil pixels back to 0 */
////    rasterizer = (struct r_i_raster_t) {
////        .polygon_mode = GL_FILL,
////        .cull_face = GL_BACK,
////        .cull_enable = R_I_ENABLE
////    };
////    stencil = (struct r_i_stencil_t) {
////        .enable = R_I_ENABLE,
////        .func = GL_ALWAYS,
////        .stencil_fail = GL_REPLACE,
////        .depth_fail = GL_REPLACE,
////        .depth_pass = GL_REPLACE,
////        .mask = 0xff,
////        .ref = 0x00
////    };
////
////    draw_mask = (struct r_i_draw_mask_t) {
////        .red = GL_FALSE,
////        .green = GL_FALSE,
////        .blue = GL_FALSE,
////        .alpha = GL_FALSE,
////        .depth = GL_FALSE,
////        .stencil = 0xff
////    };
////
////    depth = (struct r_i_depth_t) {
////        .func = GL_LEQUAL,
////        .enable = R_I_DISABLE
////    };
////
////    r_i_SetRasterizer(command_buffer, &draw_list->ranges[2], &rasterizer);
////    r_i_SetStencil(command_buffer, &draw_list->ranges[2], &stencil);
////    r_i_SetDrawMask(command_buffer, &draw_list->ranges[2], &draw_mask);
////    r_i_SetDepth(command_buffer, &draw_list->ranges[2], &depth);
////    draw_list->ranges[2].start = brush->model->model_start;
////    draw_list->ranges[2].count = brush->model->model_count;
////
////
////    draw_list->mode = GL_TRIANGLES;
////    draw_list->indexed = 1;
////
////    return draw_list;
//}



