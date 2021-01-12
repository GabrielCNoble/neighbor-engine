#include "r_draw.h"
#include "SDL2/SDL.h"
#include "GL/glew.h"
#include "dstuff/ds_stack_list.h"
#include "dstuff/ds_file.h"
#include "dstuff/ds_mem.h"
#include "dstuff/ds_obj.h"
#include <stdlib.h>
#include <stdio.h>


//struct stack_list_t d_sprites[D_MAX_LAYERS];
extern struct list_t r_draw_batches;
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
//extern struct ds_heap_t r_immediate_heap;
    
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
        glBufferSubData(GL_UNIFORM_BUFFER, 0, r_light_index_buffer_cursor * sizeof(uint16_t), r_light_index_buffer);
    }
    
    glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, R_CLUSTER_ROW_WIDTH, R_CLUSTER_ROWS, R_CLUSTER_SLICES, GL_RG_INTEGER, GL_UNSIGNED_SHORT, r_clusters);
    glBindBufferBase(GL_UNIFORM_BUFFER, R_LIGHTS_UNIFORM_BUFFER_BINDING, r_light_uniform_buffer);
    glBindBufferBase(GL_UNIFORM_BUFFER, R_LIGHT_INDICES_UNIFORM_BUFFER_BINDING, r_light_index_uniform_buffer);
}

void r_EndFrame()
{
    r_draw_batches.cursor = 0;
    r_immediate_batches.cursor = 0;
    r_immediate_cursor = 0;
    r_light_buffer_cursor = 0;
    r_light_index_buffer_cursor = 0;
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
        uint32_t index = add_list_element(&r_draw_batches, NULL);
        struct r_draw_batch_t *draw_batch = get_list_element(&r_draw_batches, index);
        draw_batch->batch = model->batches[batch_index];
        draw_batch->model_view_matrix = model_view_matrix;
    }
}

void r_DrawRange(mat4_t *transform, struct r_material_t *material, uint32_t start, uint32_t count)
{
    mat4_t model_view_matrix;    
    mat4_t_mul(&model_view_matrix, transform, &r_inv_view_matrix);
    
    uint32_t index = add_list_element(&r_draw_batches, NULL);
    struct r_draw_batch_t *draw_batch = get_list_element(&r_draw_batches, index);
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

void r_DrawBatches()
{   
    qsort_list(&r_draw_batches, r_CompareBatches);
    
    struct r_material_t *current_material;
    mat4_t model_view_projection_matrix;
         
    glBindBuffer(GL_ARRAY_BUFFER, r_vertex_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, r_index_buffer);
    r_BindShader(r_lit_shader);
    
//    r_SetUniformMatrix4(R_UNIFORM_IVM, &r_inv_view_matrix);
    r_SetUniform1i(R_UNIFORM_CLUSTERS, r_cluster_texture);
    
    for(uint32_t batch_index = 0; batch_index < r_draw_batches.cursor; batch_index++)
    {
        struct r_draw_batch_t *draw_batch = get_list_element(&r_draw_batches, batch_index);
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
    
    r_SetUniformMatrix4(R_UNIFORM_MVP, &r_view_projection_matrix);
    
    for(uint32_t batch_index = 0; batch_index < r_immediate_batches.cursor; batch_index++)
    {
        struct r_immediate_batch_t *batch = get_list_element(&r_immediate_batches, batch_index);
        switch(batch->mode)
        {
            case GL_POINTS:
                glPointSize(batch->size);
            break;
            
            case GL_LINES:
                glLineWidth(batch->size);
            break;
        }
        glDrawArrays(batch->mode, batch->start, batch->count);
    }
}

void r_DrawImmediate(struct r_vert_t *verts, uint32_t count, uint32_t mode, float size)
{
    uint32_t index = add_list_element(&r_immediate_batches, NULL);
    struct r_immediate_batch_t *batch = get_list_element(&r_immediate_batches, index);
    batch->start = r_immediate_cursor;
    batch->count = count;
    batch->mode = mode;
    batch->size = size;
    
    glBindBuffer(GL_ARRAY_BUFFER, r_immediate_buffer);
    glBufferSubData(GL_ARRAY_BUFFER, r_immediate_cursor * sizeof(struct r_vert_t), sizeof(struct r_vert_t) * count, verts);
    r_immediate_cursor += count;
}

void r_DrawPoint(vec3_t *position, vec3_t *color, float radius)
{
    struct r_vert_t vert;
    
    vert.pos = *position;
    vert.color = *color;
    
    r_DrawImmediate(&vert, 1, GL_POINTS, radius);
}

void r_DrawLine(vec3_t *from, vec3_t *to, vec3_t *color)
{
    struct r_vert_t verts[2];
    
    verts[0].pos = *from;
    verts[0].color = *color;
    
    verts[1].pos = *to;
    verts[1].color = *color;
    
    r_DrawImmediate(verts, 2, GL_LINES, 1.0);
}

void r_DrawLines(struct r_vert_t *verts, uint32_t count)
{
    r_DrawImmediate(verts, count, GL_LINES, 1.0);
}






