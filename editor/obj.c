#include <stdio.h>
#include "../engine/r_defs.h"
#include "../engine/r_main.h"
#include "../engine/r_draw_i.h"
#include "obj.h"
#include "brush.h"
#include "light.h"

struct ed_obj_funcs_t ed_obj_funcs[ED_OBJ_TYPE_LAST] = {
    [ED_OBJ_TYPE_BRUSH] = {
        .create                 = ed_CreateBrushObject,
        .destroy                = ed_DestroyBrushObject,
        .update_handle_obj      = ed_UpdateBrushHandleObject,
        .update_base_obj        = ed_UpdateBrushBaseObject,
        .render_pick            = ed_RenderPickBrushObject,
        .render_draw            = ed_RenderDrawBrushObject,
        .draw_transform         = ed_WorldSpaceDrawTransform
    },
    [ED_OBJ_TYPE_FACE] = {
        .create                 = ed_CreateFaceObject,
        .destroy                = ed_DestroyFaceObject,
        .update_handle_obj      = ed_UpdateFaceHandleObject,
        .update_base_obj        = ed_UpdateFaceBaseObject,
        .render_draw            = ed_RenderDrawFaceObject,
        .draw_transform         = ed_FaceObjectDrawTransform
    }
//    [ED_OBJ_TYPE_LIGHT] = {
//        .create                 = ed_CreateLightObject,
//        .destroy                = ed_DestroyLightObject,
//        .update_handle_obj      = ed_UpdateLightHandleObject,
//        .render_pick            = ed_RenderPickLightObject,
//        .render_outline         = ed_RenderOutlineLightObject
//    }
};

struct r_i_draw_state_t ed_obj_render_pick_states[ED_OBJ_TYPE_LAST] = {
    [ED_OBJ_TYPE_BRUSH] = {
        .rasterizer = &(struct r_i_raster_t){
            .polygon_mode = GL_FILL,
        }
    }
};

//struct r_i_draw_state_t ed_obj_render_draw_states[ED_OBJ_TYPE_LAST] = {
//    [ED_OBJ_TYPE_BRUSH] = {
//        .rasterizer = &(struct r_i_raster_t){
//            .polygon_mode = GL_LINE,
//        }
//    }
//};

extern uint32_t             r_width;
extern uint32_t             r_height;
extern mat4_t               r_view_projection_matrix;
struct r_framebuffer_t *    ed_pick_framebuffer;
struct r_i_cmd_buffer_t     ed_pick_cmd_buffer;
struct r_shader_t *         ed_picking_shader;
struct r_shader_t *         ed_outline_shader;

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
            [ED_PICK_SHADER_UNIFORM_OBJ_DATA0] = {
                .name = "ed_obj_data0",
                .type = R_UNIFORM_TYPE_INT,
            },
            [ED_PICK_SHADER_UNIFORM_OBJ_DATA1] = {
                .name = "ed_obj_data1",
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
}

void ed_ObjShutdown()
{
    r_DestroyFramebuffer(ed_pick_framebuffer);
}

struct ed_obj_context_t ed_CreateObjContext()
{
    struct ed_obj_context_t context = {};

    for(uint32_t type = ED_OBJ_TYPE_BRUSH; type < ED_OBJ_TYPE_LAST; type++)
    {
        context.objects[type] = ds_slist_create(sizeof(struct ed_obj_t), 512);
    }

    context.selections = ds_list_create(sizeof(struct ed_obj_result_t ), 512);

    return context;
}

void ed_DestroyObjContext(struct ed_obj_context_t *context)
{
    for(uint32_t type = ED_OBJ_TYPE_BRUSH; type < ED_OBJ_TYPE_LAST; type++)
    {
        ds_slist_destroy(&context->objects[type]);
    }

    ds_list_destroy(&context->selections);
}

struct ed_obj_h ed_CreateObj(struct ed_obj_context_t *context, uint32_t type, vec3_t *position, mat3_t *orientation, vec3_t *scale, void *args)
{
    struct ed_obj_h handle = ED_INVALID_OBJ;

    if(context && type < ED_OBJ_TYPE_LAST)
    {
        uint32_t index = ds_slist_add_element(&context->objects[type], NULL);
        struct ed_obj_t *object = ds_slist_get_element(&context->objects[type], index);

        object->index = index;
        object->type = type;
        object->selection_index = ED_INVALID_OBJ_SELECTION_INDEX;
        object->base_obj = ed_obj_funcs[type].create(position, orientation, scale, args);
        object->context = context;

        ed_obj_funcs[type].update_handle_obj(object);

        handle.index = index;
        handle.type = type;
    }

    return handle;
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

struct ed_obj_result_t ed_PickObject(struct ed_obj_context_t *context, int32_t mouse_x, int32_t mouse_y, uint32_t ignore_mask)
{
    struct ed_obj_result_t result = {};

    struct r_i_framebuffer_t framebuffer = { .framebuffer = ed_pick_framebuffer };
    struct r_i_depth_t depth = { .enable = GL_TRUE, .func = GL_LESS };
    struct r_i_stencil_t stencil = { .enable = GL_FALSE };
    struct r_i_blending_t blending = { .enable = GL_FALSE };
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

    for(uint32_t obj_type = ED_OBJ_TYPE_BRUSH; obj_type < ED_OBJ_TYPE_LAST; obj_type++)
    {
        if(ignore_mask & (1 << obj_type))
        {
            continue;
        }

        struct ed_obj_funcs_t *funcs = ed_obj_funcs + obj_type;
        r_i_SetRasterizer(&ed_pick_cmd_buffer, NULL, ed_obj_render_pick_states[obj_type].rasterizer);

        for(uint32_t obj_index = 0; obj_index < context->objects[obj_type].cursor; obj_index++)
        {
            struct ed_obj_t *object = ed_GetObject(context, (struct ed_obj_h){.type = obj_type, .index = obj_index});
            uint32_t pick_type = obj_type + 1;
            uint32_t pick_index = obj_index + 1;
            if(object)
            {
                mat4_t model_view_projection_matrix;
                funcs->draw_transform(object, &model_view_projection_matrix);
                struct r_i_draw_list_t *draw_list = funcs->render_pick(object, &ed_pick_cmd_buffer);

                if(draw_list != NULL)
                {
                    struct r_i_uniform_t uniforms[] = {
                        [0] = {
                            .uniform = ED_PICK_SHADER_UNIFORM_MODEL_VIEW_PROJECTION_MATRIX,
                            .value = &model_view_projection_matrix,
                            .count = 1,
                        },
                        [1] = {
                            .uniform = ED_PICK_SHADER_UNIFORM_OBJ_TYPE,
                            .value = &pick_type,
                            .count = 1,
                        },
                        [2] = {
                            .uniform = ED_PICK_SHADER_UNIFORM_OBJ_INDEX,
                            .value = &pick_index,
                            .count = 1,
                        },
                    };
                    r_i_SetUniforms(&ed_pick_cmd_buffer, NULL, uniforms, 3);
                    r_i_DrawList(&ed_pick_cmd_buffer, draw_list);
                }
            }
        }

        break;
    }

    r_RunImmCmdBuffer(&ed_pick_cmd_buffer);
    mouse_y = r_height - (mouse_y + 1);
    int32_t pick[4] = {};
    r_SampleFramebuffer(ed_pick_framebuffer, mouse_x, mouse_y, 1, 1, sizeof(pick), pick);

    if(pick[0])
    {
        result.object = ed_GetObject(context, (struct ed_obj_h){.type = pick[1] - 1, .index = pick[0] - 1});
        result.data0 = pick[2];
        result.data1 = pick[3];
    }

//    printf("%x %d %d\n", result.object, result.data0, result.data1);

    return result;
}

void ed_DrawSelections(struct ed_obj_context_t *context, struct r_i_cmd_buffer_t *cmd_buffer)
{
    r_i_SetShader(cmd_buffer, ed_outline_shader);

    for(uint32_t selection_index = 0; selection_index < context->selections.cursor; selection_index++)
    {
        struct ed_obj_result_t *object = (struct ed_obj_result_t *)ds_list_get_element(&context->selections, selection_index);
        mat4_t model_view_projection_matrix;
        ed_obj_funcs[object->object->type].draw_transform(object->object, &model_view_projection_matrix);
        struct r_i_uniform_t uniforms = {
            .uniform = R_UNIFORM_MODEL_VIEW_PROJECTION_MATRIX,
            .count = 1,
            .value = &model_view_projection_matrix
        };
        r_i_SetUniforms(cmd_buffer, NULL, &uniforms, 1);
        struct r_i_draw_list_t *draw_list = ed_obj_funcs[object->object->type].render_draw(object, cmd_buffer);

        if(draw_list != NULL)
        {
            r_i_DrawList(cmd_buffer, draw_list);
        }

    }
}

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
}

/*
============================================================================
============================================================================
============================================================================
*/

void ed_WorldSpaceDrawTransform(struct ed_obj_t *object, mat4_t *model_view_projection_matrix)
{
    mat4_t_mul(model_view_projection_matrix, &object->transform, &r_view_projection_matrix);
}

void ed_CameraSpaceDrawTransform(struct ed_obj_t *object, mat4_t *model_view_projection_matrix)
{

}
