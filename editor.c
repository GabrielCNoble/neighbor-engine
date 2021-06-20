#include "editor.h"
#include "ed_bsp.h"
#include "dstuff/ds_stack_list.h"
#include "dstuff/ds_list.h"
#include "dstuff/ds_mem.h"
#include "dstuff/ds_vector.h"
#include "dstuff/ds_matrix.h"
#include "game.h"
#include "input.h"
#include "r_draw.h"

struct stack_list_t ed_brushes;
struct ed_context_t *ed_active_context;
struct ed_context_t ed_contexts[ED_CONTEXT_LAST];
//uint32_t ed_grid_vert_count;
//struct r_vert_t *ed_grid;
struct r_i_verts_t *ed_grid;
struct stack_list_t ed_polygons;
struct stack_list_t ed_bsp_nodes;

struct r_shader_t *ed_center_grid_shader;
struct r_shader_t *ed_picking_shader;
uint32_t ed_picking_shader_type_uniform;
uint32_t ed_picking_shader_index_uniform;

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

float ed_camera_pitch;
float ed_camera_yaw;
vec3_t ed_camera_pos;
uint32_t ed_picking_framebuffer;
uint32_t ed_picking_depth_texture;
uint32_t ed_picking_object_texture;

extern uint32_t g_game_state;
extern mat4_t r_view_matrix;
extern float r_z_near;
extern float r_fov;
extern uint32_t r_width;
extern uint32_t r_height;
extern mat4_t r_view_projection_matrix;
extern uint32_t r_vertex_buffer;
extern uint32_t r_index_buffer;
extern struct stack_list_t r_lights;


#define ED_GRID_DIVS 301
#define ED_GRID_QUAD_SIZE 500.0

struct ed_state_t ed_world_context_states[] =
{
    [ED_WORLD_CONTEXT_STATE_IDLE] = ed_WorldContextIdleState,
    [ED_WORLD_CONTEXT_STATE_LEFT_CLICK] = ed_WorldContextLeftClickState,
    [ED_WORLD_CONTEXT_STATE_BRUSH_BOX] = ed_WorldContextStateBrushBox,
    [ED_WORLD_CONTEXT_STATE_CREATE_BRUSH] = ed_WorldContextCreateBrush
};

struct ed_world_context_data_t ed_world_context_data;

void ed_Init()
{
    ed_brushes = create_stack_list(sizeof(struct ed_brush_t), 512);
    ed_grid = r_i_AllocImmediateExternData(sizeof(struct r_i_verts_t) + sizeof(struct r_vert_t) * 6);

    ed_grid->count = 6;
    ed_grid->verts[0].pos = vec3_t_c(-ED_GRID_QUAD_SIZE, 0.0, -ED_GRID_QUAD_SIZE);
    ed_grid->verts[0].tex_coords = vec2_t_c(0.0, 0.0);
    ed_grid->verts[0].color = vec4_t_c(1.0, 0.0, 0.0, 1.0);

    ed_grid->verts[1].pos = vec3_t_c(-ED_GRID_QUAD_SIZE, 0.0, ED_GRID_QUAD_SIZE);
    ed_grid->verts[1].tex_coords = vec2_t_c(1.0, 0.0);
    ed_grid->verts[1].color = vec4_t_c(0.0, 1.0, 0.0, 1.0);

    ed_grid->verts[2].pos = vec3_t_c(ED_GRID_QUAD_SIZE, 0.0, ED_GRID_QUAD_SIZE);
    ed_grid->verts[2].tex_coords = vec2_t_c(1.0, 1.0);
    ed_grid->verts[2].color = vec4_t_c(0.0, 0.0, 1.0, 1.0);

    ed_grid->verts[3].pos = vec3_t_c(ED_GRID_QUAD_SIZE, 0.0, ED_GRID_QUAD_SIZE);
    ed_grid->verts[3].tex_coords = vec2_t_c(1.0, 1.0);
    ed_grid->verts[3].color = vec4_t_c(0.0, 0.0, 1.0, 1.0);

    ed_grid->verts[4].pos = vec3_t_c(ED_GRID_QUAD_SIZE, 0.0, -ED_GRID_QUAD_SIZE);
    ed_grid->verts[4].tex_coords = vec2_t_c(0.0, 1.0);
    ed_grid->verts[4].color = vec4_t_c(0.0, 0.0, 1.0, 1.0);

    ed_grid->verts[5].pos = vec3_t_c(-ED_GRID_QUAD_SIZE, 0.0, -ED_GRID_QUAD_SIZE);
    ed_grid->verts[5].tex_coords = vec2_t_c(0.0, 0.0);
    ed_grid->verts[5].color = vec4_t_c(1.0, 0.0, 0.0, 1.0);

    ed_center_grid_shader = r_LoadShader("shaders/ed_grid.vert", "shaders/ed_grid.frag");
    ed_picking_shader = r_LoadShader("shaders/ed_pick.vert", "shaders/ed_pick.frag");
    ed_picking_shader_type_uniform = r_GetUniformIndex(ed_picking_shader, "ed_type");
    ed_picking_shader_index_uniform = r_GetUniformIndex(ed_picking_shader, "ed_index");

    SDL_DisplayMode desktop_display_mode;

    SDL_GetDesktopDisplayMode(0, &desktop_display_mode);

    glGenFramebuffers(1, &ed_picking_framebuffer);
    glGenTextures(1, &ed_picking_object_texture);
    glBindTexture(GL_TEXTURE_2D, ed_picking_object_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, r_width, r_height, 0, GL_RG, GL_FLOAT, NULL);

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

    ed_contexts[ED_CONTEXT_WORLD].update = ed_WorldContextUpdate;
    ed_contexts[ED_CONTEXT_WORLD].states = ed_world_context_states;
    ed_contexts[ED_CONTEXT_WORLD].current_state = ED_WORLD_CONTEXT_STATE_IDLE;
    ed_contexts[ED_CONTEXT_WORLD].context_data = &ed_world_context_data;
    ed_world_context_data.selections = create_list(sizeof(struct ed_selection_t), 512);
    ed_active_context = ed_contexts + ED_CONTEXT_WORLD;

    ed_polygons = create_stack_list(sizeof(struct ed_polygon_t ), 1024);
    ed_bsp_nodes = create_stack_list(sizeof(struct ed_bspn_t), 1024);
}

void ed_Shutdown()
{

}

void ed_UpdateEditor()
{
    if(in_GetKeyState(SDL_SCANCODE_P) & IN_KEY_STATE_JUST_PRESSED)
    {
        g_SetGameState(G_GAME_STATE_PLAYING);
    }
    else
    {
        for(uint32_t context_index = 0; context_index < ED_CONTEXT_LAST; context_index++)
        {
            struct ed_context_t *context = ed_contexts + context_index;
            context->update();

            if(context == ed_active_context)
            {
                uint32_t just_changed = context->current_state != context->next_state;
                context->current_state = context->next_state;
                context->states[context->current_state].update(context, just_changed);
            }
        }
    }
}

void ed_FlyCamera()
{
    float dx;
    float dy;

    in_GetMouseDelta(&dx, &dy);

    ed_camera_pitch += dy;
    ed_camera_yaw -= dx;

    if(ed_camera_pitch > 0.5)
    {
        ed_camera_pitch = 0.5;
    }
    else if(ed_camera_pitch < -0.5)
    {
        ed_camera_pitch = -0.5;
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

    mat4_t_vec4_t_mul_fast(&translation, &r_view_matrix, &translation);
    vec3_t_add(&ed_camera_pos, &ed_camera_pos, &vec3_t_c(translation.x, translation.y, translation.z));
}


void ed_SetContextState(struct ed_context_t *context, uint32_t state)
{
    context->next_state = state;
}

void ed_WorldContextUpdate()
{
    r_SetViewPos(&ed_camera_pos);
    r_SetViewPitchYaw(ed_camera_pitch, ed_camera_yaw);
    ed_DrawGrid();
    ed_DrawBrushes();
    ed_DrawLights();

    int32_t mouse_x;
    int32_t mouse_y;
    struct ed_selection_t selection = {};

    in_GetMousePos(&mouse_x, &mouse_y);
    if(ed_PickObject(mouse_x, mouse_y, &selection))
    {
        struct ed_brush_t *brush = ed_GetBrush(selection.selection.index);
        mat4_t model_matrix;
        mat4_t_comp(&model_matrix, &brush->orientation, &brush->position);
        r_i_SetModelMatrix(&model_matrix);
        r_i_SetViewProjectionMatrix(NULL);
        uint32_t verts_size = sizeof(struct r_i_verts_t) + sizeof(struct r_vert_t) * brush->model->verts.buffer_size;
        uint32_t indices_size = sizeof(struct r_i_indices_t) + sizeof(uint32_t) * brush->model->indices.buffer_size;

        struct r_i_verts_t *verts = r_i_AllocImmediateData(verts_size);
        struct r_i_indices_t *indices = r_i_AllocImmediateData(indices_size);

        verts->count = brush->model->verts.buffer_size;
        indices->count = brush->model->indices.buffer_size;

        for(uint32_t vert_index = 0; vert_index < verts->count; vert_index++)
        {
            struct r_vert_t *out_vert = verts->verts + vert_index;
            struct r_vert_t *in_vert = (struct r_vert_t *)brush->model->verts.buffer + vert_index;
            *out_vert = *in_vert;
            out_vert->normal = vec4_t_c(1.0, 0.0, 0.0, 0.2);
        }

        memcpy(indices->indices, brush->model->indices.buffer, indices->count * sizeof(uint32_t));
        r_i_SetBlending(GL_TRUE, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        r_i_SetDepth(GL_TRUE, GL_LEQUAL);
        r_i_DrawVertsIndexed(R_I_DRAW_CMD_TRIANGLE_LIST, verts, indices);
    }
}

void ed_WorldContextIdleState(struct ed_context_t *context, uint32_t just_changed)
{
    if(in_GetMouseButtonState(SDL_BUTTON_RIGHT) & IN_KEY_STATE_PRESSED)
    {
        ed_FlyCamera();
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
    uint32_t left_button_state = in_GetMouseButtonState(SDL_BUTTON_LEFT);
    float dx = 0;
    float dy = 0;
//    int32_t mouse_x;
//    int32_t mouse_y;

    in_GetMouseDelta(&dx, &dy);
//    in_GetMousePos(&mouse_x, &mouse_y);
//    printf("%d %d\n", mouse_x, mouse_y);

    if(left_button_state & IN_KEY_STATE_PRESSED)
    {
//        if(just_changed)
//        {
//            ed_PickObject(mouse_x, mouse_y);
//        }

        if(dx || dy)
        {
            ed_SetContextState(context, ED_WORLD_CONTEXT_STATE_BRUSH_BOX);
        }
    }
    else
    {
        ed_SetContextState(context, ED_WORLD_CONTEXT_STATE_IDLE);
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
            mat4_t_vec4_t_mul_fast(&mouse_vec, &r_view_matrix, &mouse_vec);

            camera_pos.x = r_view_matrix.rows[3].x;
            camera_pos.y = r_view_matrix.rows[3].y;
            camera_pos.z = r_view_matrix.rows[3].z;

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

    ed_CreateBrush(&position, &orientation, &size);

    ed_SetContextState(context, ED_WORLD_CONTEXT_STATE_IDLE);
}

uint32_t ed_PickObject(int32_t mouse_x, int32_t mouse_y, struct ed_selection_t *selection)
{
    mouse_y = r_height - (mouse_y + 1);
    mat4_t model_view_projection_matrix;
    glBindBuffer(GL_ARRAY_BUFFER, r_vertex_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, r_index_buffer);

    r_BindShader(ed_picking_shader);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, ed_picking_framebuffer);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUniform1i(ed_picking_shader_type_uniform, ED_SELECTION_TYPE_BRUSH + 1);

    for(uint32_t brush_index = 0; brush_index < ed_brushes.cursor; brush_index++)
    {
        struct ed_brush_t *brush = ed_GetBrush(brush_index);

        if(brush)
        {
            mat4_t model_matrix;
            mat4_t_comp(&model_matrix, &brush->orientation, &brush->position);
            mat4_t_mul(&model_view_projection_matrix, &model_matrix, &r_view_projection_matrix);
            r_SetUniformMatrix4(R_UNIFORM_MODEL_VIEW_PROJECTION_MATRIX, &model_view_projection_matrix);
            glUniform1i(ed_picking_shader_index_uniform, brush_index);
            struct r_model_t *model = brush->model;
            for(uint32_t batch_index = 0; batch_index < model->batches.buffer_size; batch_index++)
            {
                struct r_batch_t *batch = (struct r_batch_t *)model->batches.buffer + batch_index;
                glDrawElements(GL_TRIANGLES, batch->count, GL_UNSIGNED_INT, (void *)(batch->start * sizeof(uint32_t)));
            }
        }
    }

    glBindFramebuffer(GL_READ_FRAMEBUFFER, ed_picking_framebuffer);
    int32_t pick_values[2];
    glReadPixels(mouse_x, mouse_y, 1, 1, GL_RG, GL_FLOAT, pick_values);

    if(pick_values[0])
    {
        selection->type = pick_values[0] - 1;
        selection->selection.index = pick_values[1];
        return 1;
    }

    return 0;
}

void ed_DrawGrid()
{
    r_i_SetModelMatrix(NULL);
    r_i_SetViewProjectionMatrix(NULL);
    r_i_SetBlending(GL_TRUE, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    r_i_SetCullFace(GL_FALSE, 0);
    r_i_SetShader(ed_center_grid_shader);
    r_i_DrawVerts(R_I_DRAW_CMD_TRIANGLE_LIST, ed_grid);
    r_i_SetShader(NULL);
    r_i_SetBlending(GL_TRUE, GL_ONE, GL_ZERO);
    r_i_DrawLine(&vec3_t_c(-10000.0, 0.0, 0.0), &vec3_t_c(10000.0, 0.0, 0.0), &vec4_t_c(1.0, 0.0, 0.0, 1.0), 3.0);
    r_i_DrawLine(&vec3_t_c(0.0, 0.0, -10000.0), &vec3_t_c(0.0, 0.0, 10000.0), &vec4_t_c(0.0, 0.0, 1.0, 1.0), 3.0);
}

void ed_DrawBrushes()
{
    for(uint32_t brush_index = 0; brush_index < ed_brushes.cursor; brush_index++)
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

void ed_DrawLights()
{
    r_i_SetModelMatrix(NULL);
    r_i_SetViewProjectionMatrix(NULL);


    for(uint32_t light_index = 0; light_index < r_lights.cursor; light_index++)
    {
        struct r_light_t *light = r_GetLight(light_index);

        if(light)
        {
            vec3_t position = vec3_t_c(light->data.pos_rad.x, light->data.pos_rad.y, light->data.pos_rad.z);
            vec4_t color = vec4_t_c(light->data.color_type.x, light->data.color_type.y, light->data.color_type.z, 1.0);
            r_i_DrawPoint(&position, &color, 8.0);
        }
    }
}

struct ed_brush_t *ed_CreateBrush(vec3_t *position, mat3_t *orientation, vec3_t *size)
{
    uint32_t index;
    struct ed_brush_t *brush;
    vec3_t dims;
    vec3_t_fabs(&dims, size);

    index = add_stack_list_element(&ed_brushes, NULL);
    brush = get_stack_list_element(&ed_brushes, index);
    brush->index = index;

    brush->vertices = ds_create_buffer(sizeof(vec3_t), 8);
    vec3_t *vertices = (vec3_t *)brush->vertices.buffer;

    for(uint32_t vert_index = 0; vert_index < brush->vertices.buffer_size; vert_index++)
    {
        vertices[vert_index].x = dims.x * ed_cube_brush_vertices[vert_index].x;
        vertices[vert_index].y = dims.y * ed_cube_brush_vertices[vert_index].y;
        vertices[vert_index].z = dims.z * ed_cube_brush_vertices[vert_index].z;
    }

    brush->faces = create_list(sizeof(struct ed_face_t), 6);

    brush->orientation = *orientation;
    brush->position = *position;

    for(uint32_t face_index = 0; face_index < brush->faces.size; face_index++)
    {
        add_list_element(&brush->faces, NULL);
        struct ed_face_t *face = get_list_element(&brush->faces, face_index);

        face->material = r_GetDefaultMaterial();
        face->normal = ed_cube_brush_normals[face_index];
        face->tangent = ed_cube_brush_tangents[face_index];
        face->indices = ds_create_buffer(sizeof(uint32_t), 4);
        ds_fill_buffer(&face->indices, 0, ed_cube_brush_indices[face_index], 4);
    }

    brush->model = NULL;

    ed_UpdateBrush(brush);

    return brush;
}

struct ed_brush_t *ed_GetBrush(uint32_t index)
{
    struct ed_brush_t *brush = NULL;

    if(index != 0xffffffff)
    {
        brush = get_stack_list_element(&ed_brushes, index);

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
        brush->model = r_CreateModel2(&geometry, NULL);
    }
    else
    {
        r_UpdateModelGeometry(brush->model, &geometry);
    }
}

















