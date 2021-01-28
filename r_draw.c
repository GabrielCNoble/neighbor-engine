#include "r_draw.h"
#include "SDL2/SDL.h"
#include "GL/glew.h"
#include "dstuff/ds_stack_list.h"
#include "dstuff/ds_file.h"
#include "dstuff/ds_mem.h"
#include "dstuff/ds_obj.h"
#include <stdlib.h>
#include <stdio.h>


extern struct list_t r_sorted_batches;
struct r_imm_batch_t *r_cur_imm_batch;
extern struct list_t r_immediate_batches;
extern struct stack_list_t r_shaders;
extern struct stack_list_t r_textures;
extern struct stack_list_t r_materials;
extern struct stack_list_t r_models;

extern uint32_t r_vertex_buffer;
extern struct ds_heap_t r_vertex_heap;
extern uint32_t r_index_buffer;
extern struct ds_heap_t r_index_heap;
extern uint32_t r_immediate_cursor;
extern uint32_t r_immediate_buffer;
    
extern uint32_t r_vao;
extern struct r_shader_t *r_lit_shader;
extern struct r_shader_t *r_immediate_shader;
extern struct r_shader_t *r_current_shader;

extern struct r_model_t *test_model;
extern struct r_texture_t *r_default_texture;
extern struct r_material_t *r_default_material;

extern uint32_t r_light_buffer_cursor;
extern uint32_t r_light_index_buffer_cursor;
extern struct r_l_data_t *r_light_buffer;
extern uint32_t r_light_uniform_buffer;
extern uint16_t *r_light_index_buffer;
extern uint32_t r_light_index_uniform_buffer;
extern struct r_cluster_t *r_clusters;
extern uint32_t r_cluster_texture;

mat4_t r_projection_matrix;
mat4_t r_view_matrix;
mat4_t r_inv_view_matrix;
mat4_t r_view_projection_matrix;

extern SDL_Window *r_window;
extern SDL_GLContext *r_context;
extern int32_t r_width; 
extern int32_t r_height; 

void r_BeginFrame()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glBindVertexArray(r_vao);
    
    if(r_light_buffer_cursor)
    {
        glBindBuffer(GL_UNIFORM_BUFFER, r_light_uniform_buffer);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, r_light_buffer_cursor * sizeof(struct r_l_data_t), r_light_buffer);
        glBindBuffer(GL_UNIFORM_BUFFER, r_light_index_uniform_buffer);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, R_MAX_CLUSTER_LIGHTS * R_CLUSTER_COUNT * sizeof(uint32_t), r_light_index_buffer);
    }
    glActiveTexture(GL_TEXTURE0 + R_CLUSTERS_TEX_UNIT);
    glBindTexture(GL_TEXTURE_3D, r_cluster_texture);
    glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, R_CLUSTER_ROW_WIDTH, R_CLUSTER_ROWS, R_CLUSTER_SLICES, GL_RG_INTEGER, GL_UNSIGNED_INT, r_clusters);
    glBindBufferBase(GL_UNIFORM_BUFFER, R_LIGHTS_UNIFORM_BUFFER_BINDING, r_light_uniform_buffer);
    glBindBufferBase(GL_UNIFORM_BUFFER, R_LIGHT_INDICES_UNIFORM_BUFFER_BINDING, r_light_index_uniform_buffer);
}

void r_EndFrame()
{
    r_sorted_batches.cursor = 0;
    r_immediate_batches.cursor = 0;
    r_immediate_cursor = 0;
    r_light_buffer_cursor = 0;
    r_light_index_buffer_cursor = 0;
    r_cur_imm_batch = NULL;
    SDL_GL_SwapWindow(r_window);
}


void r_SetViewPos(vec3_t *pos)
{
    r_view_matrix.rows[3].x = pos->x;
    r_view_matrix.rows[3].y = pos->y;
    r_view_matrix.rows[3].z = pos->z;
    r_UpdateViewProjectionMatrix();
}

void r_TranslateView(vec3_t *disp)
{
    r_view_matrix.rows[3].x += disp->x;
    r_view_matrix.rows[3].y += disp->y;
    r_view_matrix.rows[3].z += disp->z;
    r_UpdateViewProjectionMatrix();
}

void r_SetViewPitch(float pitch)
{
    mat4_t pitch_matrix;
    mat4_t_identity(&pitch_matrix);
    mat4_t_pitch(&pitch_matrix, pitch);
    mat4_t_mul(&r_view_matrix, &r_view_matrix, &pitch_matrix);
    r_UpdateViewProjectionMatrix();
}

void r_SetViewYaw(float yaw)
{
    mat4_t yaw_matrix;
    mat4_t_identity(&yaw_matrix);
    mat4_t_yaw(&yaw_matrix, yaw);
    mat4_t_mul(&r_view_matrix, &r_view_matrix, &yaw_matrix);
    r_UpdateViewProjectionMatrix();
}

void r_UpdateViewProjectionMatrix()
{
    mat4_t_invvm(&r_inv_view_matrix, &r_view_matrix);
    mat4_t_mul(&r_view_projection_matrix, &r_inv_view_matrix, &r_projection_matrix);
}

void r_DrawModel(mat4_t *transform, struct r_model_t *model)
{
    mat4_t model_view_matrix;    
    mat4_t_mul(&model_view_matrix, transform, &r_inv_view_matrix);
    for(uint32_t batch_index = 0; batch_index < model->batch_count; batch_index++)
    {
        uint32_t index = add_list_element(&r_sorted_batches, NULL);
        struct r_draw_batch_t *draw_batch = get_list_element(&r_sorted_batches, index);
        draw_batch->batch = model->batches[batch_index];
        draw_batch->model_view_matrix = model_view_matrix;
    }
}

void r_DrawRange(mat4_t *transform, struct r_material_t *material, uint32_t start, uint32_t count)
{
    mat4_t model_view_matrix;    
    mat4_t_mul(&model_view_matrix, transform, &r_inv_view_matrix);
    
    uint32_t index = add_list_element(&r_sorted_batches, NULL);
    struct r_draw_batch_t *draw_batch = get_list_element(&r_sorted_batches, index);
    draw_batch->batch.material = material;
    draw_batch->batch.start = start;
    draw_batch->batch.count = count;
    draw_batch->model_view_matrix = model_view_matrix;
}

int32_t r_CompareBatches(void *a, void *b)
{
    struct r_draw_batch_t *batch_a = (struct r_draw_batch_t *)a;
    struct r_draw_batch_t *batch_b = (struct r_draw_batch_t *)b;
    
    return (int32_t)batch_a->batch.material - (int32_t)batch_b->batch.material;
}

void r_DrawSortedBatches()
{   
    qsort_list(&r_sorted_batches, r_CompareBatches);
    
    struct r_material_t *current_material;
    mat4_t model_view_projection_matrix;
         
    glBindBuffer(GL_ARRAY_BUFFER, r_vertex_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, r_index_buffer);
    r_BindShader(r_lit_shader);
    r_SetUniform1i(R_UNIFORM_CLUSTERS, R_CLUSTERS_TEX_UNIT);
    
    for(uint32_t batch_index = 0; batch_index < r_sorted_batches.cursor; batch_index++)
    {
        struct r_draw_batch_t *draw_batch = get_list_element(&r_sorted_batches, batch_index);
        
        if(draw_batch->batch.material != current_material)
        {
            current_material = draw_batch->batch.material;
            r_BindMaterial(current_material);
        }
        
        mat4_t_mul(&model_view_projection_matrix, &draw_batch->model_view_matrix, &r_projection_matrix);
        r_SetUniformMatrix4(R_UNIFORM_MVP, &model_view_projection_matrix);
        r_SetUniformMatrix4(R_UNIFORM_MV, &draw_batch->model_view_matrix);
        glDrawElements(GL_TRIANGLES, draw_batch->batch.count, GL_UNSIGNED_INT, (void *)(draw_batch->batch.start * sizeof(uint32_t)));
    }
}

void r_DrawImmediateBatches()
{
    glBindBuffer(GL_ARRAY_BUFFER, r_immediate_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    r_BindShader(r_immediate_shader);
    
    for(uint32_t batch_index = 0; batch_index < r_immediate_batches.cursor; batch_index++)
    {
        struct r_imm_batch_t *batch = get_list_element(&r_immediate_batches, batch_index);
        
        if(!batch->count)
        {
            continue;
        }
        
        r_SetUniformMatrix4(R_UNIFORM_MVP, &batch->transform);
        glPolygonMode(GL_FRONT_AND_BACK, batch->polygon_mode);
        glLineWidth(batch->size);
        glPointSize(batch->size);
        glDrawArrays(batch->primitive_type, batch->start, batch->count);
    }
}

struct r_imm_batch_t *r_i_BeginBatch()
{
    if(r_cur_imm_batch && !r_cur_imm_batch->count)
    {
        return r_cur_imm_batch;
    }
    
    uint32_t index = add_list_element(&r_immediate_batches, NULL);
    struct r_imm_batch_t *batch = get_list_element(&r_immediate_batches, index);
    
    if(r_cur_imm_batch)
    {
        *batch = *r_cur_imm_batch;
    }
    else
    {
        batch->polygon_mode = GL_FILL;
        batch->size = 1.0;
        batch->primitive_type = GL_TRIANGLES;
    }
    
    r_cur_imm_batch = batch;
    batch->start = r_immediate_cursor;
    batch->count = 0;
    
    return r_cur_imm_batch;
}

struct r_imm_batch_t *r_i_GetCurrentBatch()
{
    if(!r_cur_imm_batch)
    {
        r_i_BeginBatch();
    }
    
    return r_cur_imm_batch;
}

void r_i_SetTransform(mat4_t *transform)
{
    struct r_imm_batch_t *batch = r_i_BeginBatch();
    
    if(!transform)
    {
        batch->transform = r_view_projection_matrix;
    }
    else
    {
        batch->transform = *transform;
    }
}

void r_i_SetPolygonMode(uint16_t polygon_mode)
{
    struct r_imm_batch_t *batch = r_i_GetCurrentBatch();
    
    if(batch->polygon_mode != polygon_mode)
    {
        batch = r_i_BeginBatch();
    }
    
    batch->polygon_mode = polygon_mode;
}

void r_i_SetSize(float size)
{
    struct r_imm_batch_t *batch = r_i_GetCurrentBatch();
    
    if(batch->size != size)
    {
        batch = r_i_BeginBatch();
    }
    
    batch->size = size;
}

void r_i_SetPrimitiveType(uint32_t primitive_type)
{
    struct r_imm_batch_t *batch = r_i_GetCurrentBatch();
    
    if(batch->primitive_type != primitive_type)
    {
        batch = r_i_BeginBatch();
    }
    
    batch->primitive_type = primitive_type;
}

void r_i_DrawImmediate(struct r_vert_t *verts, uint32_t count)
{
    uint32_t index = add_list_element(&r_immediate_batches, NULL);
    struct r_imm_batch_t *batch = r_i_GetCurrentBatch();
    
    uint32_t offset = batch->start + batch->count;
    glBindBuffer(GL_ARRAY_BUFFER, r_immediate_buffer);
    glBufferSubData(GL_ARRAY_BUFFER, offset * sizeof(struct r_vert_t), sizeof(struct r_vert_t) * count, verts);
    batch->count += count;
    r_immediate_cursor += count;
}

void r_i_DrawPoint(vec3_t *position, vec3_t *color, float radius)
{
    struct r_vert_t vert;
    
    vert.pos = *position;
    vert.color = *color;
    
    r_i_SetPrimitiveType(GL_POINTS);
    r_i_SetSize(radius);
    r_i_DrawImmediate(&vert, 1);
}

void r_i_DrawLine(vec3_t *from, vec3_t *to, vec3_t *color, float width)
{
    struct r_vert_t verts[2] = 
    {
        [0] = {.pos = *from, .color = *color},
        [1] = {.pos = *to, .color = *color}
    };
    
    r_i_SetPrimitiveType(GL_LINES);
    r_i_SetSize(width);
    r_i_DrawImmediate(verts, 2);
}

//void r_DrawLine(vec3_t *from, vec3_t *to, vec3_t *color)
//{
//    struct r_vert_t verts[2];
//    
//    verts[0].pos = *from;
//    verts[0].color = *color;
//    
//    verts[1].pos = *to;
//    verts[1].color = *color;
//    
//    r_DrawImmediate(verts, 2, GL_LINES, 1.0);
//}
//
//void r_DrawLines(struct r_vert_t *verts, uint32_t count)
//{
//    r_DrawImmediate(verts, count, GL_LINES, 1.0);
//}






