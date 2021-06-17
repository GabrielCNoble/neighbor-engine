#include "r_draw.h"
#include "SDL2/SDL.h"
#include "GL/glew.h"
#include "dstuff/ds_stack_list.h"
#include "dstuff/ds_file.h"
#include "dstuff/ds_mem.h"
#include "dstuff/ds_obj.h"
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <stdalign.h>


//extern struct list_t r_sorted_batches;
//struct r_imm_batch_t *r_cur_imm_batch;
//extern struct list_t r_immediate_batches;

extern struct list_t r_world_cmds;
extern struct list_t r_entity_cmds;
extern struct list_t r_immediate_cmds;
extern struct list_t r_immediate_data;

extern struct stack_list_t r_shaders;
extern struct stack_list_t r_textures;
extern struct stack_list_t r_materials;
extern struct stack_list_t r_models;

extern uint32_t r_vertex_buffer;
extern struct ds_heap_t r_vertex_heap;
extern uint32_t r_index_buffer;
extern struct ds_heap_t r_index_heap;
extern uint32_t r_immediate_cursor;
extern uint32_t r_immediate_vertex_buffer;
extern uint32_t r_immediate_index_buffer;

extern uint32_t r_vao;
extern struct r_shader_t *r_z_prepass_shader;
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
extern float r_z_near;
extern float r_z_far;
extern float r_denom;
extern int32_t r_width;
extern int32_t r_height;
extern uint32_t r_cluster_row_width;
extern uint32_t r_cluster_rows;

uint32_t r_use_z_prepass = 1;

void r_BeginFrame()
{
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glViewport(0, 0, r_width, r_height);

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

    r_denom = log(r_z_far / r_z_near);

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void r_EndFrame()
{
    r_light_buffer_cursor = 0;
    r_light_index_buffer_cursor = 0;

    r_world_cmds.cursor = 0;
    r_entity_cmds.cursor = 0;
    r_immediate_cmds.cursor = 0;
    r_immediate_data.cursor = 0;
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

void r_SetViewPitchYaw(float pitch, float yaw)
{
    mat4_t pitch_matrix;
    mat4_t yaw_matrix;
    mat4_t_identity(&pitch_matrix);
    mat4_t_identity(&yaw_matrix);
    mat4_t_pitch(&pitch_matrix, pitch);
    mat4_t_yaw(&yaw_matrix, yaw);
    mat4_t_mul(&pitch_matrix, &pitch_matrix, &yaw_matrix);
    r_view_matrix.rows[0] = pitch_matrix.rows[0];
    r_view_matrix.rows[1] = pitch_matrix.rows[1];
    r_view_matrix.rows[2] = pitch_matrix.rows[2];
    r_UpdateViewProjectionMatrix();
}

//void r_SetViewYaw(float yaw)
//{
//    mat4_t yaw_matrix;
//    mat4_t_identity(&yaw_matrix);
//    mat4_t_yaw(&yaw_matrix, yaw);
//    mat4_t_mul(&r_view_matrix, &r_view_matrix, &yaw_matrix);
//    r_UpdateViewProjectionMatrix();
//}

void r_UpdateViewProjectionMatrix()
{
    mat4_t_invvm(&r_inv_view_matrix, &r_view_matrix);
    mat4_t_mul(&r_view_projection_matrix, &r_inv_view_matrix, &r_projection_matrix);
}

void r_DrawEntity(mat4_t *transform, struct r_model_t *model)
{
    if(model)
    {
        struct r_batch_t *batches = (struct r_batch_t *)model->batches.buffer;
        mat4_t model_view_matrix;
        mat4_t_mul(&model_view_matrix, transform, &r_inv_view_matrix);
        for(uint32_t batch_index = 0; batch_index < model->batches.buffer_size; batch_index++)
        {
            struct r_batch_t *batch = batches + batch_index;
            uint32_t index = add_list_element(&r_entity_cmds, NULL);
            struct r_entity_cmd_t *cmd = get_list_element(&r_entity_cmds, index);
            cmd->model_view_matrix = model_view_matrix;
            cmd->start = batch->start;
            cmd->count = batch->count;
            cmd->material = batch->material;
        }
    }
}

void r_DrawWorld(struct r_material_t *material, uint32_t start, uint32_t count)
{
    if(material)
    {
        uint32_t index = add_list_element(&r_world_cmds, NULL);
        struct r_world_cmd_t *cmd = get_list_element(&r_world_cmds, index);
        cmd->material = material;
        cmd->start = start;
        cmd->count = count;
    }
}

int32_t r_CompareCmds(void *a, void *b)
{
    struct r_entity_cmd_t *cmd_a = (struct r_entity_cmd_t *)a;
    struct r_entity_cmd_t *cmd_b = (struct r_entity_cmd_t *)b;
    if(cmd_a->material < cmd_b->material)
    {
        return -1;
    }
    else if(cmd_a->material > cmd_b->material)
    {
        return 1;
    }

    return 0;
}

void r_DrawCmds()
{
    struct r_material_t *current_material = NULL;
    mat4_t model_view_projection_matrix;
    mat4_t model_view_matrix;

    qsort_list(&r_entity_cmds, r_CompareCmds);

    glBindBuffer(GL_ARRAY_BUFFER, r_vertex_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, r_index_buffer);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glDepthFunc(GL_LESS);

//    if(r_use_z_prepass)
//    {
//    r_BindShader(r_z_prepass_shader);
//    glDepthFunc(GL_LESS);
//
//    for(uint32_t cmd_index = 0; cmd_index < r_entity_cmds.cursor; cmd_index++)
//    {
//        struct r_entity_cmd_t *cmd = get_list_element(&r_entity_cmds, cmd_index);
//        mat4_t_mul(&model_view_projection_matrix, &cmd->model_view_matrix, &r_projection_matrix);
//        r_SetUniformMatrix4(R_UNIFORM_MVP, &model_view_projection_matrix);
//        glDrawElements(GL_TRIANGLES, cmd->count, GL_UNSIGNED_INT, (void *)(cmd->start * sizeof(uint32_t)));
//    }
//
//    glDepthFunc(GL_EQUAL);
//    }

    r_BindShader(r_lit_shader);
    r_SetUniform1i(R_UNIFORM_CLUSTERS, R_CLUSTERS_TEX_UNIT);

    for(uint32_t cmd_index = 0; cmd_index < r_entity_cmds.cursor; cmd_index++)
    {
        struct r_entity_cmd_t *cmd = get_list_element(&r_entity_cmds, cmd_index);

        if(cmd->material != current_material)
        {
            current_material = cmd->material;
            r_BindMaterial(current_material);
        }

        mat4_t_mul(&model_view_projection_matrix, &cmd->model_view_matrix, &r_projection_matrix);
        r_SetUniformMatrix4(R_UNIFORM_MODEL_VIEW_PROJECTION_MATRIX, &model_view_projection_matrix);
        r_SetUniformMatrix4(R_UNIFORM_MODEL_VIEW_MATRIX, &cmd->model_view_matrix);
        glDrawElements(GL_TRIANGLES, cmd->count, GL_UNSIGNED_INT, (void *)(cmd->start * sizeof(uint32_t)));
    }

    mat4_t view_projection_matrix;
    mat4_t model_matrix;

    mat4_t_identity(&view_projection_matrix);
    mat4_t_identity(&model_matrix);

    glBindBuffer(GL_ARRAY_BUFFER, r_immediate_vertex_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    r_BindShader(r_immediate_shader);

    struct r_immediate_shader_t *immediate_shader = NULL;
    struct r_immediate_blending_t *immediate_blending = NULL;
    struct r_immediate_depth_t *immediate_depth = NULL;
    struct r_immediate_stencil_t *immediate_stencil = NULL;
    struct r_immediate_texture_t *immediate_texture = NULL;
    struct r_immediate_transform_t *immediate_model_matrix = NULL;
    struct r_immediate_transform_t *immediate_view_projection_matrix = NULL;
    uint32_t immediate_texture_count = 0;

    for(uint32_t cmd_index = 0; cmd_index < r_immediate_cmds.cursor; cmd_index++)
    {
        struct r_immediate_cmd_t *cmd = get_list_element(&r_immediate_cmds, cmd_index);

        switch(cmd->type)
        {
            case R_I_CMD_SET_MATRIX:
            {
                struct r_immediate_transform_t *transform = cmd->data;

                switch(cmd->sub_type)
                {
                    case R_I_SET_MATRIX_CMD_MODEL_MATRIX:
                        immediate_model_matrix = transform;
                        if(transform->unset)
                        {
                            mat4_t_identity(&immediate_model_matrix->transform);
                        }
                        model_matrix = immediate_model_matrix->transform;
                    break;

                    case R_I_SET_MATRIX_CMD_VIEW_PROJECTION_MATRIX:
                        immediate_view_projection_matrix = transform;
                        if(transform->unset)
                        {
                            immediate_view_projection_matrix->transform = r_view_projection_matrix;
                        }
                        view_projection_matrix = immediate_view_projection_matrix->transform;
                    break;
                }
            }
            break;

            case R_I_CMD_SET_STATE:
            {
                struct r_immediate_state_t *state = cmd->data;

                if(state->shader)
                {
                    immediate_shader = state->shader;
                }

                if(state->blending)
                {
                    immediate_blending = state->blending;
                }

                if(state->depth)
                {
                    immediate_depth = state->depth;
                }

                if(state->stencil)
                {
                    immediate_stencil = state->stencil;
                }

                if(state->textures)
                {
                    immediate_texture = state->textures;
                    immediate_texture_count = state->texture_count;
                }
            }
            break;

            case R_I_CMD_DRAW:
            {
                if(immediate_shader)
                {
                    r_BindShader(immediate_shader->shader);
                }

                if(immediate_texture)
                {
                    for(uint32_t texture_index = 0; texture_index < immediate_texture_count; texture_index++)
                    {
                        struct r_immediate_texture_t *texture = immediate_texture + texture_index;
                        glActiveTexture(texture->tex_unit);
                        glBindTexture(GL_TEXTURE_2D, texture->texture->handle);
                    }
                }

                if(immediate_blending)
                {
                    if(immediate_blending->enable)
                    {
                        glEnable(GL_BLEND);
                        glBlendFunc(immediate_blending->src_factor, immediate_blending->dst_factor);
                    }
                    else
                    {
                        glDisable(GL_BLEND);
                    }
                }

                if(immediate_stencil)
                {
                    if(immediate_stencil->enable)
                    {
                        glEnable(GL_STENCIL_TEST);
                        glStencilOp(immediate_stencil->stencil_fail, immediate_stencil->depth_fail, immediate_stencil->depth_pass);
                        glStencilFunc(immediate_stencil->operation, immediate_stencil->ref, immediate_stencil->mask);
                    }
                    else
                    {
                        glDisable(GL_STENCIL_TEST);
                    }
                }

                if(immediate_depth)
                {
                    if(immediate_depth->enable)
                    {
                        glEnable(GL_DEPTH_TEST);
                        glDepthFunc(immediate_depth->func);
                    }
                    else
                    {
                        glDisable(GL_DEPTH_TEST);
                    }
                }

                if(immediate_model_matrix || immediate_view_projection_matrix || immediate_shader)
                {
                    mat4_t_mul(&model_view_projection_matrix, &model_matrix, &view_projection_matrix);
                    r_SetUniformMatrix4(R_UNIFORM_MODEL_VIEW_PROJECTION_MATRIX, &model_view_projection_matrix);
                    immediate_model_matrix = NULL;
                    immediate_view_projection_matrix = NULL;
                }

                immediate_texture = NULL;
                immediate_blending = NULL;
                immediate_shader = NULL;
                immediate_stencil = NULL;

                uint32_t mode;
                struct r_immediate_geometry_t *geometry = cmd->data;
                struct r_immediate_verts_t *verts = geometry->verts;
                struct r_immediate_indices_t *indices = geometry->indices;

                if(verts)
                {
                    switch(cmd->sub_type)
                    {
                        case R_I_DRAW_CMD_LINE_LIST:
                            glLineWidth(verts->size);
                            mode = GL_LINES;
                        break;

                        case R_I_DRAW_CMD_POINT_LIST:
                            glPointSize(verts->size);
                            mode = GL_POINTS;
                        break;

                        case R_I_DRAW_CMD_TRIANGLE_LIST:
                            mode = GL_TRIANGLES;
                        break;
                    }

                    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(struct r_vert_t) * verts->count, verts->verts);

                    if(indices)
                    {
                        glDrawElements(mode, indices->count, GL_UNSIGNED_INT, indices->indices);
                    }
                    else
                    {
                        glDrawArrays(mode, 0, verts->count);
                    }
                }
            }
            break;
        }

        if(cmd->data)
        {
            struct r_immediate_data_t *data = (struct r_immediate_data_t *)((char *)cmd->data - R_IMMEDIATE_DATA_SLOT_SIZE);
            if(data->flags & R_IMMEDIATE_DATA_FLAG_BIG)
            {
                mem_Free(data);
            }
        }
    }
}

void *r_i_AllocImmediateData(uint32_t size)
{
    struct r_immediate_data_t *data;

    uint32_t required_size = R_IMMEDIATE_DATA_SLOT_SIZE + size;

    if(required_size <= r_immediate_data.buffer_size)
    {
        uint32_t required_slots = required_size / r_immediate_data.elem_size;
        uint32_t available_slots = r_immediate_data.buffer_size - (r_immediate_data.cursor % r_immediate_data.buffer_size);

        if(required_size % R_IMMEDIATE_DATA_SLOT_SIZE)
        {
            required_slots++;
        }

        if(required_slots > available_slots)
        {
             r_immediate_data.cursor += available_slots;
        }

        uint32_t data_index = add_list_element(&r_immediate_data, NULL);
        data = get_list_element(&r_immediate_data, data_index);
        available_slots--;
        r_immediate_data.cursor += available_slots;
        data->flags = 0;
    }
    else
    {
        data = mem_Calloc(1, required_size);
        data->flags = R_IMMEDIATE_DATA_FLAG_BIG;
    }

    /* this is guaranteed to give an properly aligned address for any data type */
    data->data = (char *)data + R_IMMEDIATE_DATA_SLOT_SIZE;

    return data->data;
}

void r_i_ImmediateCmd(uint16_t type, uint16_t sub_type, void *data)
{
    uint32_t cmd_index = add_list_element(&r_immediate_cmds, NULL);
    struct r_immediate_cmd_t *cmd = get_list_element(&r_immediate_cmds, cmd_index);

    cmd->data = data;
    cmd->type = type;
    cmd->sub_type = sub_type;
}

void r_i_SetState(struct r_immediate_state_t *state)
{
    r_i_ImmediateCmd(R_I_CMD_SET_STATE, 0, state);
}

void r_i_SetShader(struct r_shader_t *shader)
{
    struct r_immediate_state_t *state = r_i_AllocImmediateData(sizeof(struct r_immediate_state_t));
    state->shader = r_i_AllocImmediateData(sizeof(struct r_immediate_state_t));

    if(!shader)
    {
        state->shader->shader = r_immediate_shader;
    }
    else
    {
        state->shader->shader = shader;
    }

    r_i_SetState(state);
}

void r_i_SetBlending(uint16_t enable, uint16_t src_factor, uint16_t dst_factor)
{
    struct r_immediate_state_t *state = r_i_AllocImmediateData(sizeof(struct r_immediate_state_t));
    state->blending = r_i_AllocImmediateData(sizeof(struct r_immediate_blending_t));
    state->blending->enable = enable;
    state->blending->src_factor = src_factor;
    state->blending->dst_factor = dst_factor;
    r_i_SetState(state);
}

void r_i_SetDepth(uint16_t enable, uint16_t func)
{
    struct r_immediate_state_t *state = r_i_AllocImmediateData(sizeof(struct r_immediate_state_t));
    state->depth = r_i_AllocImmediateData(sizeof(struct r_immediate_depth_t));
    state->depth->enable = enable;
    state->depth->func = func;
    r_i_SetState(state);
}

void r_i_SetTextures(uint32_t texture_count, struct r_immediate_texture_t *textures)
{
    struct r_immediate_state_t *state = r_i_AllocImmediateData(sizeof(struct r_immediate_state_t));
    state->textures = textures;
    state->texture_count = texture_count;
    r_i_SetState(state);
}

void r_i_SetTexture(struct r_texture_t *texture, uint32_t tex_unit)
{
    struct r_immediate_texture_t *immediate_texture = r_i_AllocImmediateData(sizeof(struct r_immediate_texture_t));
    immediate_texture->texture = texture;
    immediate_texture->tex_unit = tex_unit;
    r_i_SetTextures(1, immediate_texture);
}

void r_i_SetMatrix(mat4_t *matrix, uint16_t sub_type)
{
    struct r_immediate_transform_t *transform = r_i_AllocImmediateData(sizeof(struct r_immediate_transform_t));

    transform->unset = matrix == NULL;

    if(matrix)
    {
        transform->transform = *matrix;
    }

    r_i_ImmediateCmd(R_I_CMD_SET_MATRIX, sub_type, transform);
}

void r_i_SetModelMatrix(mat4_t *model_matrix)
{
    r_i_SetMatrix(model_matrix, R_I_SET_MATRIX_CMD_MODEL_MATRIX);
}

void r_i_SetViewProjectionMatrix(mat4_t *view_projection_matrix)
{
    r_i_SetMatrix(view_projection_matrix, R_I_SET_MATRIX_CMD_VIEW_PROJECTION_MATRIX);
}

void r_i_DrawImmediate(uint16_t sub_type, struct r_immediate_geometry_t *geometry)
{
    r_i_ImmediateCmd(R_I_CMD_DRAW, sub_type, geometry);
}

void r_i_DrawVerts(uint16_t sub_type, struct r_immediate_verts_t *verts)
{
    struct r_immediate_geometry_t *geometry = r_i_AllocImmediateData(sizeof(struct r_immediate_geometry_t));
    geometry->verts = verts;
    geometry->indices = NULL;
    r_i_DrawImmediate(sub_type, geometry);
}

void r_i_DrawVertsIndexed(uint16_t sub_type, struct r_immediate_verts_t *verts, struct r_immediate_indices_t *indices)
{
    struct r_immediate_geometry_t *geometry = r_i_AllocImmediateData(sizeof(struct r_immediate_geometry_t));
    geometry->verts = verts;
    geometry->indices = indices;
    r_i_DrawImmediate(sub_type, geometry);
}

void r_i_DrawPoint(vec3_t *position, vec4_t *color, float size)
{
    struct r_immediate_verts_t *verts = r_i_AllocImmediateData(sizeof(struct r_immediate_verts_t) + sizeof(struct r_vert_t));

    verts->count = 1;
    verts->size = size;

//    verts->verts[0].pos = *position;
//    verts->verts[0].normal = *color;

    memcpy(&verts->verts[0].pos, position, sizeof(vec4_t));
    memcpy(&verts->verts[0].color, color, sizeof(vec4_t));

    r_i_DrawVerts(R_I_DRAW_CMD_POINT_LIST, verts);
}

void r_i_DrawLine(vec3_t *start, vec3_t *end, vec4_t *color, float width)
{
    struct r_immediate_verts_t *verts = r_i_AllocImmediateData(sizeof(struct r_immediate_verts_t) + sizeof(struct r_vert_t) * 20);

    verts->count = 2;
    verts->size = width;


//    memcpy(&verts->verts[0].pos, start, sizeof(vec4_t));
//    memcpy(&verts->verts[0].color, color, sizeof(vec4_t));
//    memcpy(&verts->verts[1].pos, end, sizeof(vec4_t));
//    memcpy(&verts->verts[1].color, color, sizeof(vec4_t));
    verts->verts[0].pos = *start;
    verts->verts[0].color = *color;
    verts->verts[1].pos = *end;
    verts->verts[1].color = *color;

    r_i_DrawVerts(R_I_DRAW_CMD_LINE_LIST, verts);
}





