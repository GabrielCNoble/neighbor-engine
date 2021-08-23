#include "ed_w_ctx.h"
#include "ed_pick.h"
#include "ed_brush.h"
#include "ed_main.h"
#include "r_main.h"
#include "input.h"

extern struct ed_context_t ed_contexts[];
struct ed_world_context_data_t ed_world_context_data;
extern mat4_t r_camera_matrix;
extern struct r_shader_t *ed_center_grid_shader;
extern struct r_shader_t *ed_picking_shader;
struct r_shader_t *ed_outline_shader;
extern struct r_i_verts_t *ed_grid;
extern struct ds_slist_t r_lights;
extern uint32_t r_width;
extern uint32_t r_height;
extern float r_fov;
extern float r_z_near;

struct ed_state_t ed_world_context_states[] =
{
    [ED_WORLD_CONTEXT_STATE_IDLE] = ed_WorldContextIdleState,
    [ED_WORLD_CONTEXT_STATE_LEFT_CLICK] = ed_WorldContextLeftClickState,
    [ED_WORLD_CONTEXT_STATE_BRUSH_BOX] = ed_WorldContextStateBrushBox,
    [ED_WORLD_CONTEXT_STATE_CREATE_BRUSH] = ed_WorldContextCreateBrush,
    [ED_WORLD_CONTEXT_STATE_PROCESS_SELECTION] = ed_WorldContextProcessSelection,
};

void ed_WorldContextInit()
{
    ed_contexts[ED_CONTEXT_WORLD].update = ed_WorldContextUpdate;
    ed_contexts[ED_CONTEXT_WORLD].states = ed_world_context_states;
    ed_contexts[ED_CONTEXT_WORLD].current_state = ED_WORLD_CONTEXT_STATE_IDLE;
    ed_contexts[ED_CONTEXT_WORLD].context_data = &ed_world_context_data;
    ed_world_context_data.selections = ds_list_create(sizeof(uint32_t), 512);
    ed_world_context_data.pickables[0] = ds_slist_create(sizeof(struct ed_pickable_t), 512);
    ed_world_context_data.pickables[1] = ds_slist_create(sizeof(struct ed_pickable_t), 512);
    ed_world_context_data.active_pickable_list = 0;
    ed_world_context_data.brushes = ds_slist_create(sizeof(struct ed_brush_t), 512);
    ed_world_context_data.global_brush_batches = ds_list_create(sizeof(struct ed_brush_batch_t), 512);
}

void ed_WorldContextShutdown()
{

}

void ed_WorldContextFlyCamera()
{
    float dx;
    float dy;

    in_GetMouseDelta(&dx, &dy);

    ed_world_context_data.camera_pitch += dy;
    ed_world_context_data.camera_yaw -= dx;

    if(ed_world_context_data.camera_pitch > 0.5)
    {
        ed_world_context_data.camera_pitch = 0.5;
    }
    else if(ed_world_context_data.camera_pitch < -0.5)
    {
        ed_world_context_data.camera_pitch = -0.5;
    }

    vec4_t translation = {};

    if(in_GetKeyState(SDL_SCANCODE_W) & IN_KEY_STATE_PRESSED)
    {
        translation.z -= 0.05;
    }
    if(in_GetKeyState(SDL_SCANCODE_S) & IN_KEY_STATE_PRESSED)
    {
        translation.z += 0.05;
    }

    if(in_GetKeyState(SDL_SCANCODE_A) & IN_KEY_STATE_PRESSED)
    {
        translation.x -= 0.05;
    }
    if(in_GetKeyState(SDL_SCANCODE_D) & IN_KEY_STATE_PRESSED)
    {
        translation.x += 0.05;
    }

    mat4_t_vec4_t_mul_fast(&translation, &r_camera_matrix, &translation);
    vec3_t_add(&ed_world_context_data.camera_pos, &ed_world_context_data.camera_pos, &vec3_t_c(translation.x, translation.y, translation.z));
}

void ed_WorldContextDrawGrid()
{
    r_i_SetModelMatrix(NULL);
    r_i_SetViewProjectionMatrix(NULL);
    r_i_SetBlending(GL_TRUE, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    r_i_SetStencil(GL_FALSE, GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE);
    r_i_SetRasterizer(GL_FALSE, GL_BACK, GL_FILL);
    r_i_SetShader(ed_center_grid_shader);
    r_i_DrawVerts(R_I_DRAW_CMD_TRIANGLE_LIST, ed_grid, 1.0);
    r_i_SetShader(NULL);
    r_i_SetBlending(GL_TRUE, GL_ONE, GL_ZERO);
    r_i_DrawLine(&vec3_t_c(-10000.0, 0.0, 0.0), &vec3_t_c(10000.0, 0.0, 0.0), &vec4_t_c(1.0, 0.0, 0.0, 1.0), 3.0);
    r_i_DrawLine(&vec3_t_c(0.0, 0.0, -10000.0), &vec3_t_c(0.0, 0.0, 10000.0), &vec4_t_c(0.0, 0.0, 1.0, 1.0), 3.0);
}

void ed_WorldContextDrawBrushes()
{
    for(uint32_t brush_index = 0; brush_index < ed_world_context_data.brushes.cursor; brush_index++)
    {
        struct ed_brush_t *brush = ed_GetBrush(brush_index);

        if(brush)
        {
            mat4_t transform;
            mat4_t_identity(&transform);
            mat4_t_comp(&transform, &brush->orientation, &brush->position);
            r_DrawEntity(&transform, brush->model);
        }
    }
}

void ed_WorldContextDrawLights()
{
    r_i_SetModelMatrix(NULL);
    r_i_SetViewProjectionMatrix(NULL);


    for(uint32_t light_index = 0; light_index < r_lights.cursor; light_index++)
    {
        struct r_light_t *light = r_GetLight(light_index);

        if(light)
        {
            vec3_t position = vec3_t_c(light->data.pos_rad.x, light->data.pos_rad.y, light->data.pos_rad.z);
            vec4_t color = vec4_t_c(light->data.color_res.x, light->data.color_res.y, light->data.color_res.z, 1.0);
            r_i_DrawPoint(&position, &color, 8.0);
        }
    }
}

void ed_WorldContextDrawSelections()
{
    if(ed_world_context_data.selections.cursor)
    {
        r_i_SetViewProjectionMatrix(NULL);
        r_i_SetShader(ed_outline_shader);
        r_i_SetBuffers(NULL, NULL);
        r_i_SetRasterizer(GL_TRUE, GL_FRONT, GL_LINE);

        uint32_t selection_count = ed_world_context_data.selections.cursor - 1;
        uint32_t selection_index = 0;
        uint8_t stencil_value = 1;

        for(uint32_t index = 0; index < 2; index++)
        {
            if(!index && selection_count)
            {
                r_i_SetUniform(r_GetNamedUniform(ed_outline_shader, "ed_color"), 1, &vec4_t_c(1.0, 0.2, 0.0, 1.0));
            }
            else
            {
                r_i_SetUniform(r_GetNamedUniform(ed_outline_shader, "ed_color"), 1, &vec4_t_c(1.0, 0.4, 0.0, 1.0));
            }

            for(; selection_index < selection_count; selection_index++)
            {
                uint32_t pickable_index = *(uint32_t *)ds_list_get_element(&ed_world_context_data.selections, selection_index);
                struct ed_pickable_t *pickable = ed_GetPickable(pickable_index);

                r_i_SetModelMatrix(&pickable->transform);

                struct r_i_draw_list_t *draw_list = r_i_AllocDrawList(1);
                draw_list->commands[0].start = pickable->start;
                draw_list->commands[0].count = pickable->count;
                draw_list->size = 4.0;
                draw_list->indexed = 1;

                r_i_SetDrawMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE, 0xff);
                r_i_SetDepth(GL_TRUE, GL_ALWAYS);
                r_i_SetStencil(GL_TRUE, GL_KEEP, GL_KEEP, GL_REPLACE, GL_ALWAYS, 0xff, 0xff);
                r_i_SetRasterizer(GL_TRUE, GL_FRONT, GL_FILL);
                r_i_DrawImmediate(R_I_DRAW_CMD_TRIANGLE_LIST, draw_list);

                draw_list = r_i_AllocDrawList(1);
                draw_list->commands[0].start = pickable->start;
                draw_list->commands[0].count = pickable->count;
                draw_list->size = 4.0;
                draw_list->indexed = 1;

                r_i_SetDrawMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE, 0xff);
                r_i_SetDepth(GL_TRUE, GL_ALWAYS);
                r_i_SetStencil(GL_TRUE, GL_KEEP, GL_KEEP, GL_KEEP, GL_EQUAL, 0xff, 0x00);
                r_i_SetRasterizer(GL_TRUE, GL_FRONT, GL_LINE);
                r_i_DrawImmediate(R_I_DRAW_CMD_TRIANGLE_LIST, draw_list);


//                draw_list = r_i_AllocDrawList(1);
//                draw_list->commands[0].start = pickable->start;
//                draw_list->commands[0].count = pickable->count;
//                draw_list->size = 4.0;
//                draw_list->indexed = 1;
//
//                r_i_SetDrawMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE, 0xff);
//                r_i_SetDepth(GL_TRUE, GL_LESS);
//                r_i_SetStencil(GL_TRUE, GL_KEEP, GL_KEEP, GL_REPLACE, GL_EQUAL, 0xff, 0x00);
//                r_i_SetRasterizer(GL_TRUE, GL_FRONT, GL_LINE);
//                r_i_DrawImmediate(R_I_DRAW_CMD_TRIANGLE_LIST, draw_list);

            }

            selection_count++;
        }

        r_i_SetRasterizer(GL_TRUE, GL_BACK, GL_FILL);
        r_i_SetDepth(GL_TRUE, GL_LESS);
        r_i_SetStencil(GL_FALSE, GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE);
    }
}

void ed_WorldContextUpdate()
{
    r_SetViewPos(&ed_world_context_data.camera_pos);
    r_SetViewPitchYaw(ed_world_context_data.camera_pitch, ed_world_context_data.camera_yaw);
    ed_UpdatePickables();
    ed_WorldContextDrawSelections();
    ed_WorldContextDrawGrid();
    ed_WorldContextDrawBrushes();
    ed_WorldContextDrawLights();
}

void ed_WorldContextIdleState(struct ed_context_t *context, uint32_t just_changed)
{
    if(in_GetMouseButtonState(SDL_BUTTON_MIDDLE) & IN_KEY_STATE_PRESSED)
    {
        ed_WorldContextFlyCamera();
    }
    else
    {
        if(in_GetMouseButtonState(SDL_BUTTON_LEFT) & IN_KEY_STATE_JUST_PRESSED)
        {
            ed_SetContextState(context, ED_WORLD_CONTEXT_STATE_LEFT_CLICK);
        }
    }
}

void ed_WorldContextLeftClickState(struct ed_context_t *context, uint32_t just_changed)
{
    struct ed_world_context_data_t *context_data = (struct ed_world_context_data_t *)context->context_data;
    uint32_t left_button_state = in_GetMouseButtonState(SDL_BUTTON_LEFT);
    float dx = 0;
    float dy = 0;
    int32_t mouse_x;
    int32_t mouse_y;

    in_GetMouseDelta(&dx, &dy);
    in_GetMousePos(&mouse_x, &mouse_y);

    if(left_button_state & IN_KEY_STATE_JUST_PRESSED)
    {
        context_data->last_selected = ed_SelectPickable(mouse_x, mouse_y);
    }

    if(left_button_state & IN_KEY_STATE_PRESSED)
    {
        if(dx || dy)
        {
            ed_SetContextState(context, ED_WORLD_CONTEXT_STATE_BRUSH_BOX);
        }
    }
    else
    {
        if(context_data->last_selected)
        {
            ed_SetContextState(context, ED_WORLD_CONTEXT_STATE_PROCESS_SELECTION);
        }
        else
        {
            ed_SetContextState(context, ED_WORLD_CONTEXT_STATE_IDLE);
        }
    }
}

void ed_WorldContextStateBrushBox(struct ed_context_t *context, uint32_t just_changed)
{
    struct ed_world_context_data_t *context_data = (struct ed_world_context_data_t *)context->context_data;

    if(in_GetMouseButtonState(SDL_BUTTON_LEFT) & IN_KEY_STATE_PRESSED)
    {
        if(in_GetKeyState(SDL_SCANCODE_ESCAPE) & IN_KEY_STATE_PRESSED)
        {
            ed_SetContextState(context, ED_WORLD_CONTEXT_STATE_IDLE);
        }
        else
        {
            vec3_t mouse_pos;
            vec3_t camera_pos;
            vec4_t mouse_vec = {};

            float aspect = (float)r_width / (float)r_height;
            float top = tan(r_fov) * r_z_near;
            float right = top * aspect;

            in_GetNormalizedMousePos(&mouse_vec.x, &mouse_vec.y);
            mouse_vec.x *= right;
            mouse_vec.y *= top;
            mouse_vec.z = -r_z_near;
            vec4_t_normalize(&mouse_vec, &mouse_vec);
            mat4_t_vec4_t_mul_fast(&mouse_vec, &r_camera_matrix, &mouse_vec);

            camera_pos.x = r_camera_matrix.rows[3].x;
            camera_pos.y = r_camera_matrix.rows[3].y;
            camera_pos.z = r_camera_matrix.rows[3].z;

            mouse_pos.x = camera_pos.x + mouse_vec.x;
            mouse_pos.y = camera_pos.y + mouse_vec.y;
            mouse_pos.z = camera_pos.z + mouse_vec.z;

            float dist_a = camera_pos.y;
            float dist_b = mouse_pos.y;
            float denom = (dist_a - dist_b);

            r_i_SetModelMatrix(NULL);
            r_i_SetViewProjectionMatrix(NULL);

            if(denom)
            {
                float frac = dist_a / denom;
                vec3_t intersection = {};
                vec3_t_fmadd(&intersection, &camera_pos, &vec3_t_c(mouse_vec.x, mouse_vec.y, mouse_vec.z), frac);

                if(just_changed)
                {
                    context_data->box_start = intersection;
                }

                context_data->box_end = intersection;
                vec3_t start = context_data->box_start;
                vec3_t end = context_data->box_end;

                r_i_DrawLine(&start, &vec3_t_c(start.x, start.y, end.z), &vec4_t_c(0.0, 1.0, 0.0, 1.0), 2.0);
                r_i_DrawLine(&vec3_t_c(start.x, start.y, end.z), &end, &vec4_t_c(0.0, 1.0, 0.0, 1.0), 2.0);
                r_i_DrawLine(&end, &vec3_t_c(end.x, start.y, start.z), &vec4_t_c(0.0, 1.0, 0.0, 1.0), 2.0);
                r_i_DrawLine(&vec3_t_c(end.x, start.y, start.z), &start, &vec4_t_c(0.0, 1.0, 0.0, 1.0), 2.0);
            }
        }
    }
    else
    {
        if(just_changed)
        {
            ed_SetContextState(context, ED_WORLD_CONTEXT_STATE_IDLE);
        }

        ed_SetContextState(context, ED_WORLD_CONTEXT_STATE_CREATE_BRUSH);
    }
}

void ed_WorldContextCreateBrush(struct ed_context_t *context, uint32_t just_changed)
{
    struct ed_world_context_data_t *context_data = (struct ed_world_context_data_t *)context->context_data;
    vec3_t position;
    vec3_t size;
    mat3_t orientation;

    mat3_t_identity(&orientation);
    vec3_t_sub(&size, &context_data->box_end, &context_data->box_start);
    vec3_t_add(&position, &context_data->box_start, &context_data->box_end);
    vec3_t_mul(&position, &position, 0.5);

    size.y = 1.0;

    ed_CreateBrushPickable(&position, &orientation, &size);
    ed_SetContextState(context, ED_WORLD_CONTEXT_STATE_IDLE);
}

void ed_WorldContextProcessSelection(struct ed_context_t *context, uint32_t just_changed)
{
    struct ed_world_context_data_t *context_data = (struct ed_world_context_data_t *)context->context_data;
    uint32_t mouse_state = in_GetMouseButtonState(SDL_BUTTON_LEFT);
    uint32_t shift_state = in_GetKeyState(SDL_SCANCODE_LSHIFT);

    if(context_data->last_selected->type == ED_PICKABLE_TYPE_MANIPULATOR)
    {

    }
    else
    {
        if(mouse_state & IN_KEY_STATE_JUST_RELEASED)
        {
            int32_t mouse_x;
            int32_t mouse_y;
            in_GetMousePos(&mouse_x, &mouse_y);
            struct ed_pickable_t *selection = ed_SelectPickable(mouse_x, mouse_y);

            if(selection == context_data->last_selected)
            {
                ed_AddSelection(selection, shift_state & IN_KEY_STATE_PRESSED);
            }

            context_data->last_selected = NULL;
            ed_SetContextState(context, ED_WORLD_CONTEXT_STATE_IDLE);
        }
    }
}
