#include "editor.h"
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
uint32_t ed_grid_vert_count;
struct r_vert_t *ed_grid;

uint32_t ed_cube_brush_indices [] = 
{
    /* -Z */
    0, 1, 2, 2, 3, 0,
    
    /* +Z */
    4, 5, 6, 6, 7, 4,
    
    /* -X */
    8, 9, 10, 10, 11, 8,
    
    /* +X */
    12, 13, 14, 14, 15, 12,
    
    /* -Y */
    16, 17, 18, 18, 19, 16,
    
    /* +Y */
    19, 20, 21, 21, 22, 19
};

struct r_vert_t ed_cube_brush_verts[] = 
{
    /* -Z */
    {.pos = vec3_t_c(-0.5, 0.5,-0.5), .normal = vec3_t_c(0, 0,-1), .tangent = vec3_t_c(1, 0, 0), .tex_coords = vec2_t_c(0, 1)},
    {.pos = vec3_t_c(-0.5,-0.5,-0.5), .normal = vec3_t_c(0, 0,-1), .tangent = vec3_t_c(1, 0, 0), .tex_coords = vec2_t_c(0, 0)},
    {.pos = vec3_t_c( 0.5,-0.5,-0.5), .normal = vec3_t_c(0, 0,-1), .tangent = vec3_t_c(1, 0, 0), .tex_coords = vec2_t_c(1, 0)},
    {.pos = vec3_t_c( 0.5, 0.5,-0.5), .normal = vec3_t_c(0, 0,-1), .tangent = vec3_t_c(1, 0, 0), .tex_coords = vec2_t_c(1, 1)},
    
    /* +Z */
    {.pos = vec3_t_c( 0.5, 0.5, 0.5), .normal = vec3_t_c(0, 0, 1), .tangent = vec3_t_c(-1, 0, 0), .tex_coords = vec2_t_c(0, 1)},
    {.pos = vec3_t_c( 0.5,-0.5, 0.5), .normal = vec3_t_c(0, 0, 1), .tangent = vec3_t_c(-1, 0, 0), .tex_coords = vec2_t_c(0, 0)},
    {.pos = vec3_t_c(-0.5,-0.5, 0.5), .normal = vec3_t_c(0, 0, 1), .tangent = vec3_t_c(-1, 0, 0), .tex_coords = vec2_t_c(1, 0)},
    {.pos = vec3_t_c(-0.5, 0.5, 0.5), .normal = vec3_t_c(0, 0, 1), .tangent = vec3_t_c(-1, 0, 0), .tex_coords = vec2_t_c(1, 1)},
    
    /* -X */
    {.pos = vec3_t_c(-0.5, 0.5,-0.5), .normal = vec3_t_c(-1, 0, 0), .tangent = vec3_t_c(0, 0,-1), .tex_coords = vec2_t_c(0, 1)},
    {.pos = vec3_t_c(-0.5,-0.5,-0.5), .normal = vec3_t_c(-1, 0, 0), .tangent = vec3_t_c(0, 0,-1), .tex_coords = vec2_t_c(0, 0)},
    {.pos = vec3_t_c(-0.5,-0.5, 0.5), .normal = vec3_t_c(-1, 0, 0), .tangent = vec3_t_c(0, 0,-1), .tex_coords = vec2_t_c(1, 0)},
    {.pos = vec3_t_c(-0.5, 0.5, 0.5), .normal = vec3_t_c(-1, 0, 0), .tangent = vec3_t_c(0, 0,-1), .tex_coords = vec2_t_c(1, 1)},
    
    /* +X */
    {.pos = vec3_t_c( 0.5, 0.5, 0.5), .normal = vec3_t_c(1, 0, 0), .tangent = vec3_t_c(0, 0, 1), .tex_coords = vec2_t_c(0, 1)},
    {.pos = vec3_t_c( 0.5,-0.5, 0.5), .normal = vec3_t_c(1, 0, 0), .tangent = vec3_t_c(0, 0, 1), .tex_coords = vec2_t_c(0, 0)},
    {.pos = vec3_t_c( 0.5,-0.5,-0.5), .normal = vec3_t_c(1, 0, 0), .tangent = vec3_t_c(0, 0, 1), .tex_coords = vec2_t_c(1, 0)},
    {.pos = vec3_t_c( 0.5, 0.5,-0.5), .normal = vec3_t_c(1, 0, 0), .tangent = vec3_t_c(0, 0, 1), .tex_coords = vec2_t_c(1, 1)},
    
    /* -Y */
    {.pos = vec3_t_c(-0.5,-0.5, 0.5), .normal = vec3_t_c(0,-1, 0), .tangent = vec3_t_c(-1, 0, 0), .tex_coords = vec2_t_c(0, 1)},
    {.pos = vec3_t_c(-0.5,-0.5,-0.5), .normal = vec3_t_c(0,-1, 0), .tangent = vec3_t_c(-1, 0, 0), .tex_coords = vec2_t_c(0, 0)},
    {.pos = vec3_t_c( 0.5,-0.5,-0.5), .normal = vec3_t_c(0,-1, 0), .tangent = vec3_t_c(-1, 0, 0), .tex_coords = vec2_t_c(1, 0)},
    {.pos = vec3_t_c( 0.5,-0.5, 0.5), .normal = vec3_t_c(0,-1, 0), .tangent = vec3_t_c(-1, 0, 0), .tex_coords = vec2_t_c(1, 1)},
    
    /* +Y */
    {.pos = vec3_t_c(-0.5, 0.5,-0.5), .normal = vec3_t_c(0, 1, 0), .tangent = vec3_t_c(1, 0, 0), .tex_coords = vec2_t_c(0, 1)},
    {.pos = vec3_t_c(-0.5, 0.5, 0.5), .normal = vec3_t_c(0, 1, 0), .tangent = vec3_t_c(1, 0, 0), .tex_coords = vec2_t_c(0, 0)},
    {.pos = vec3_t_c( 0.5, 0.5, 0.5), .normal = vec3_t_c(0, 1, 0), .tangent = vec3_t_c(1, 0, 0), .tex_coords = vec2_t_c(1, 0)},
    {.pos = vec3_t_c( 0.5, 0.5,-0.5), .normal = vec3_t_c(0, 1, 0), .tangent = vec3_t_c(1, 0, 0), .tex_coords = vec2_t_c(1, 1)},
};

float ed_camera_pitch;
float ed_camera_yaw;
vec3_t ed_camera_pos;

extern uint32_t g_game_state;
extern mat4_t r_view_matrix;
extern float r_z_near;
extern float r_fov;
extern uint32_t r_width;
extern uint32_t r_height;


#define ED_GRID_DIVS 301

struct ed_state_t ed_world_context_states[] = 
{
    [ED_WORLD_CONTEXT_STATE_IDLE] = ed_WorldContextIdleState,
    [ED_WORLD_CONTEXT_STATE_LEFT_CLICK] = ed_WorldContextLeftClickState,
    [ED_WORLD_CONTEXT_STATE_CREATING_BRUSH] = ed_WorldContextStateCreateingBrush,
};

struct ed_world_context_data_t ed_world_context_data;

void ed_Init()
{
    ed_brushes = create_stack_list(sizeof(struct ed_brush_t), 512);
    ed_grid_vert_count = 4 * ED_GRID_DIVS + 4;
    
    ed_grid = mem_Calloc(ed_grid_vert_count, sizeof(struct r_vert_t));
    struct r_vert_t *vert = ed_grid;
    
    for(int32_t div_index = 0; div_index < ED_GRID_DIVS / 2; div_index++)
    {
        float grid_x = ED_GRID_DIVS / 2 - div_index;
        
        vert->pos = vec3_t_c(-grid_x, 0.0, -ED_GRID_DIVS / 2);
        vert->color = vec3_t_c(1.0, 1.0, 1.0);
        vert++;
        
        vert->pos = vec3_t_c(-grid_x, 0.0, ED_GRID_DIVS / 2);
        vert->color = vec3_t_c(1.0, 1.0, 1.0);
        vert++;
        
        vert->pos = vec3_t_c(grid_x, 0.0, ED_GRID_DIVS / 2);
        vert->color = vec3_t_c(1.0, 1.0, 1.0);
        vert++;
        
        vert->pos = vec3_t_c(grid_x, 0.0, -ED_GRID_DIVS / 2);
        vert->color = vec3_t_c(1.0, 1.0, 1.0);
        vert++;
    }
    
    vert->pos = vec3_t_c(0.0, 0.0, -ED_GRID_DIVS / 2);
    vert->color = vec3_t_c(1.0, 1.0, 1.0);
    vert++;
    vert->pos = vec3_t_c(0.0, 0.0, ED_GRID_DIVS / 2);
    vert->color = vec3_t_c(1.0, 1.0, 1.0);
    vert++;
    
    struct r_vert_t *first_half = ed_grid;
    for(int32_t div_index = 0; div_index < ED_GRID_DIVS * 2; div_index++)
    {
        vert->pos = vec3_t_c(first_half->pos.z, first_half->pos.y, first_half->pos.x);
        vert->color = first_half->color;
        vert++;
        first_half++;
    }
    
    vert->pos = vec3_t_c(-ED_GRID_DIVS / 2, 0.0, 0.0);
    vert->color = vec3_t_c(1.0, 1.0, 1.0);
    vert++;
    vert->pos = vec3_t_c(ED_GRID_DIVS / 2, 0.0, 0.0);
    vert->color = vec3_t_c(1.0, 1.0, 1.0);
    vert++;
    
    ed_contexts[ED_CONTEXT_WORLD].update = ed_WorldContextUpdate;
    ed_contexts[ED_CONTEXT_WORLD].states = ed_world_context_states;
    ed_contexts[ED_CONTEXT_WORLD].current_state = ED_WORLD_CONTEXT_STATE_IDLE;
    ed_contexts[ED_CONTEXT_WORLD].context_data = &ed_world_context_data;
    ed_world_context_data.selections = create_list(sizeof(struct ed_selection_t), 512);
    ed_active_context = ed_contexts + ED_CONTEXT_WORLD;
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

void ed_DrawGrid()
{
    r_i_SetTransform(NULL);
    r_i_SetPolygonMode(GL_LINE);
    r_i_SetPrimitiveType(GL_LINE_STRIP);
    r_i_SetSize(1.0);
    
    r_i_DrawImmediate(ed_grid, ED_GRID_DIVS * 2);
    
    r_i_SetTransform(NULL);
    r_i_DrawImmediate(ed_grid + ED_GRID_DIVS * 2, ED_GRID_DIVS * 2);
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
}

void ed_WorldContextIdleState(struct ed_context_t *context, uint32_t just_changed)
{
    uint32_t state = in_GetMouseButtonState(SDL_BUTTON_MIDDLE);
    
    if(state & IN_KEY_STATE_PRESSED)
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
    
    in_GetMouseDelta(&dx, &dy);
        
    if(left_button_state & IN_KEY_STATE_PRESSED)
    {
        if(dx || dy)
        {
            ed_SetContextState(context, ED_WORLD_CONTEXT_STATE_CREATING_BRUSH);
        }
    }
}

void ed_WorldContextStateCreateingBrush(struct ed_context_t *context, uint32_t just_changed)
{
    struct ed_world_context_data_t *context_data = (struct ed_world_context_data_t *)context->context_data;
    
    if(in_GetMouseButtonState(SDL_BUTTON_LEFT) & IN_KEY_STATE_PRESSED)
    {
        vec3_t mouse_pos;
        vec3_t camera_pos;
        vec4_t mouse_vec = {};
        
        float aspect = (float)r_width / (float)r_height;
        float top = tan(r_fov) * r_z_near;
        float right = top * aspect;
        
        in_GetMousePos(&mouse_vec.x, &mouse_vec.y);
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
        
        r_i_SetTransform(NULL);
        r_i_SetPrimitiveType(GL_LINES);
        
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
            
            r_i_DrawLine(&start, &vec3_t_c(start.x, start.y, end.z), &vec3_t_c(0.0, 1.0, 0.0), 2.0);
            r_i_DrawLine(&vec3_t_c(start.x, start.y, end.z), &end, &vec3_t_c(0.0, 1.0, 0.0), 2.0);
            r_i_DrawLine(&end, &vec3_t_c(end.x, start.y, start.z), &vec3_t_c(0.0, 1.0, 0.0), 2.0);
            r_i_DrawLine(&vec3_t_c(end.x, start.y, start.z), &start, &vec3_t_c(0.0, 1.0, 0.0), 2.0);
        }
    }
    else
    {
        ed_SetContextState(context, ED_WORLD_CONTEXT_STATE_IDLE);
    }
}

void ed_DrawBrushes()
{
    for(uint32_t brush_index = 0; brush_index < ed_brushes.cursor; brush_index++)
    {
        struct ed_brush_t *brush = get_stack_list_element(&ed_brushes, brush_index);
        
        if(brush->index == 0xffffffff)
        {
            continue;
        }
        
        
    }
}

struct ed_brush_t *ed_CreateBrush(vec3_t *position, mat3_t *orientation, vec3_t *size)
{
    uint32_t index;
    struct ed_brush_t *brush;
    
    index = add_stack_list_element(&ed_brushes, NULL);
    brush = get_stack_list_element(&ed_brushes, index);
    brush->index = index;
    
//    brush->indices = ds_create_buffer(sizeof(uint32_t), 36);
//    memcpy(brush->indices.buffer, ed_cube_brush_indices, sizeof(uint32_t) * brush->indices.buffer_size);
//    brush->vertices = ds_create_buffer(sizeof(struct r_vert_t), 24);
//    memcpy(brush->vertices.buffer, ed_cube_brush_verts, sizeof(struct r_vert_t) * brush->vertices.buffer_size);
    
//    brush->vert_chunk = DS_INVALID_CHUNK_HANDLE;
//    brush->index_chunk = DS_INVALID_CHUNK_HANDLE;
//    brush->batches = create_list(sizeof(struct r_batch_t), 6);
//    struct r_batch_t *batch = get_list_element(&brush->batches, 0);
//    batch->material = r_GetDefaultMaterial();
//    batch->start = 0;
//    batch->count = 
    
    ed_UpdateBrush(brush);
    
    return brush;
}

void ed_UpdateBrush(struct ed_brush_t *brush)
{
//    struct ds_chunk_t *vert_chunk;
//    struct ds_chunk_t *index_chunk;
//    
//    vert_chunk = r_GetVerticesChunk(brush->vert_chunk);
//    index_chunk = r_GetIndicesChunk(brush->index_chunk);
//    
//    if(!vert_chunk)
//    {
//        brush->vert_chunk = r_AllocateVertices(brush->vertice_count);
//        brush->index_chunk = r_AllocateIndices(brush->indice_count);
//    }
//    else
//    {
//        if(vert_chunk->size / sizeof(struct r_vert_t) < brush->vertice_count)
//        {
//            r_FreeVertices(brush->vert_chunk);
//            brush->vert_chunk = r_AllocateVertices(brush->vertice_count);
//        }
//        
//        if(index_chunk->size / sizeof(uint32_t) < brush->indice_count)
//        {
//            r_FreeIndices(brush->index_chunk);
//            brush->index_chunk = r_AllocateIndices(brush->indice_count);
//        }
//    }
//    
//    
//    r_FillVertices(brush->vert_chunk, brush->vertices, brush->vertice_count);
//    r_FillIndices(brush->index_chunk, brush->indices, brush->indice_count, 0);
}

















