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
struct r_i_state_t *r_i_current_state = NULL;

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
    glDisable(GL_SCISSOR_TEST);

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
    r_i_current_state = NULL;
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
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glDisable(GL_BLEND);
    glDisable(GL_SCISSOR_TEST);
    glScissor(0, 0, r_width, r_height);
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
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, r_immediate_index_buffer);
    r_BindShader(r_immediate_shader);

    struct r_i_state_t *immediate_state = NULL;
    struct r_i_transform_t *immediate_view_projection_matrix = NULL;
    struct r_i_transform_t *immediate_model_matrix = NULL;
    struct r_i_verts_t *immediate_verts = NULL;
    struct r_i_indices_t *immediate_indices = NULL;

    for(uint32_t cmd_index = 0; cmd_index < r_immediate_cmds.cursor; cmd_index++)
    {
        struct r_i_cmd_t *cmd = get_list_element(&r_immediate_cmds, cmd_index);

        switch(cmd->type)
        {
            case R_I_CMD_SET_MATRIX:
            {
                struct r_i_transform_t *transform = cmd->data;

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
                immediate_state = cmd->data;
            break;

            case R_I_CMD_SET_BUFFERS:
            {
                struct r_i_geometry_t *geometry = cmd->data;
                immediate_verts = geometry->verts;
                immediate_indices = geometry->indices;
            }
            break;

            case R_I_CMD_DRAW:
            {
                if(immediate_state)
                {
                    if(immediate_state->shader)
                    {
                        r_BindShader(immediate_state->shader->shader);
                    }

                    if(immediate_state->textures)
                    {
                        for(uint32_t texture_index = 0; texture_index < immediate_state->texture_count; texture_index++)
                        {
                            struct r_i_texture_t *texture = immediate_state->textures + texture_index;
                            glActiveTexture(texture->tex_unit);

                            if(texture->texture)
                            {
                                glBindTexture(GL_TEXTURE_2D, texture->texture->handle);
                            }
                            else
                            {
                                glBindTexture(GL_TEXTURE_2D, 0);
                            }

                            r_SetUniform1i(R_UNIFORM_TEX0 + texture_index, texture->tex_unit - GL_TEXTURE0);
                        }
                    }

                    if(immediate_state->blending)
                    {
                        if(immediate_state->blending->enable)
                        {
                            glEnable(GL_BLEND);
                            glBlendFunc(immediate_state->blending->src_factor, immediate_state->blending->dst_factor);
                        }
                        else
                        {
                            glDisable(GL_BLEND);
                        }
                    }

                    if(immediate_state->stencil)
                    {
                        if(immediate_state->stencil->enable)
                        {
                            glEnable(GL_STENCIL_TEST);
                            glStencilOp(immediate_state->stencil->stencil_fail, immediate_state->stencil->depth_fail, immediate_state->stencil->depth_pass);
                            glStencilFunc(immediate_state->stencil->operation, immediate_state->stencil->ref, immediate_state->stencil->mask);
                        }
                        else
                        {
                            glDisable(GL_STENCIL_TEST);
                        }
                    }

                    if(immediate_state->depth)
                    {
                        if(immediate_state->depth->enable)
                        {
                            glEnable(GL_DEPTH_TEST);
                            glDepthFunc(immediate_state->depth->func);
                        }
                        else
                        {
                            glDisable(GL_DEPTH_TEST);
                        }
                    }

                    if(immediate_state->cull_face)
                    {
                        if(immediate_state->cull_face->enable)
                        {
                            glEnable(GL_CULL_FACE);
                            glCullFace(immediate_state->cull_face->cull_face);
                        }
                        else
                        {
                            glDisable(GL_CULL_FACE);
                        }
                    }

                    if(immediate_state->scissor)
                    {
                        if(immediate_state->scissor->enable)
                        {
                            glEnable(GL_SCISSOR_TEST);
                            glScissor(immediate_state->scissor->x, immediate_state->scissor->y, immediate_state->scissor->width, immediate_state->scissor->height);
                        }
                        else
                        {
                            glDisable(GL_SCISSOR_TEST);
                        }
                    }
                }

                if(immediate_model_matrix || immediate_view_projection_matrix || (immediate_state && immediate_state->shader))
                {
                    mat4_t_mul(&model_view_projection_matrix, &model_matrix, &view_projection_matrix);
                    r_SetUniformMatrix4(R_UNIFORM_MODEL_VIEW_PROJECTION_MATRIX, &model_view_projection_matrix);
                }

                immediate_model_matrix = NULL;
                immediate_view_projection_matrix = NULL;
                immediate_state = NULL;

                uint32_t mode = 0;
                struct r_i_draw_list_t *draw_list = cmd->data;
//                struct r_i_verts_t *verts = draw_list->verts;
//                struct r_i_indices_t *indices = draw_list->indices;

//                if(verts)
//                {
                switch(cmd->sub_type)
                {
                    case R_I_DRAW_CMD_LINE_LIST:
//                        glLineWidth(verts->size);
                        mode = GL_LINES;
                    break;

                    case R_I_DRAW_CMD_POINT_LIST:
//                        glPointSize(verts->size);
                        mode = GL_POINTS;
                    break;

                    case R_I_DRAW_CMD_TRIANGLE_LIST:
                        mode = GL_TRIANGLES;
                    break;
                }

                if(immediate_verts)
                {
                    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(struct r_vert_t) * immediate_verts->count, immediate_verts->verts);
                    immediate_verts = NULL;
                }

                if(immediate_indices)
                {
                    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(uint32_t) * immediate_indices->count, immediate_indices->indices);
                    immediate_indices = NULL;
                }

                if(draw_list->indexed)
                {
                    for(uint32_t cmd_index = 0; cmd_index < draw_list->command_count; cmd_index++)
                    {
                        struct r_i_draw_cmd_t *draw_cmd = draw_list->commands + cmd_index;
                        glDrawElements(mode, draw_cmd->count, GL_UNSIGNED_INT, (void *)(draw_cmd->start * sizeof(uint32_t)));
                    }
                }
                else
                {
                    for(uint32_t cmd_index = 0; cmd_index < draw_list->command_count; cmd_index++)
                    {
                        struct r_i_draw_cmd_t *draw_cmd = draw_list->commands + cmd_index;
                        glDrawArrays(mode, draw_cmd->start, draw_cmd->count);
                    }
                }
//                }
            }
            break;
        }

        if(cmd->data)
        {
            struct r_i_data_t *data = (struct r_i_data_t *)((char *)cmd->data - R_IMMEDIATE_DATA_SLOT_SIZE);
            if(data->flags & R_IMMEDIATE_DATA_FLAG_BIG)
            {
                mem_Free(data);
            }
        }
    }
}

void *r_i_AllocImmediateData(uint32_t size)
{
    struct r_i_data_t *data;

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
        required_slots--;
        r_immediate_data.cursor += required_slots;
        data->flags = 0;
    }
    else
    {
        data = mem_Calloc(1, required_size);
        data->flags = R_IMMEDIATE_DATA_FLAG_BIG;
    }

    /* this is guaranteed to give an properly aligned address for any data type */
    data->data = (char *)data + R_IMMEDIATE_DATA_SLOT_SIZE;
    memset(data->data, 0, size);
    return data->data;
}

void *r_i_AllocImmediateExternData(uint32_t size)
{
    struct r_i_data_t *data;
    data = mem_Calloc(1, sizeof(struct r_i_data_t) + size);
    data->flags = R_IMMEDIATE_DATA_FLAG_EXTERN;
    data->data = (char *)data + R_IMMEDIATE_DATA_SLOT_SIZE;
    return data->data;
}

void r_i_FreeImmediateExternData(void *data)
{
    struct r_i_data_t *immediate_data = (struct r_i_data_t *)((char *)data - R_IMMEDIATE_DATA_SLOT_SIZE);
    mem_Free(immediate_data);
}

void r_i_ImmediateCmd(uint16_t type, uint16_t sub_type, void *data)
{
    uint32_t cmd_index = add_list_element(&r_immediate_cmds, NULL);
    struct r_i_cmd_t *cmd = get_list_element(&r_immediate_cmds, cmd_index);

    cmd->data = data;
    cmd->type = type;
    cmd->sub_type = sub_type;
}

struct r_i_state_t *r_i_GetCurrentState()
{
    if(!r_i_current_state)
    {
        r_i_current_state = r_i_AllocImmediateData(sizeof(struct r_i_state_t));
    }

    return r_i_current_state;
}

void r_i_SetCurrentState()
{
    if(r_i_current_state)
    {
        r_i_ImmediateCmd(R_I_CMD_SET_STATE, 0, r_i_current_state);
        r_i_current_state = NULL;
    }
}

void r_i_SetShader(struct r_shader_t *shader)
{
    struct r_i_state_t *state = r_i_GetCurrentState();

    if(!state->shader)
    {
        state->shader = r_i_AllocImmediateData(sizeof(struct r_i_shader_t));
    }

    if(!shader)
    {
        state->shader->shader = r_immediate_shader;
    }
    else
    {
        state->shader->shader = shader;
    }
}

void r_i_SetBlending(uint16_t enable, uint16_t src_factor, uint16_t dst_factor)
{
    struct r_i_state_t *state = r_i_GetCurrentState();

    if(!state->blending)
    {
        state->blending = r_i_AllocImmediateData(sizeof(struct r_i_blending_t));
    }

    state->blending->enable = enable;
    state->blending->src_factor = src_factor;
    state->blending->dst_factor = dst_factor;
}

void r_i_SetDepth(uint16_t enable, uint16_t func)
{
    struct r_i_state_t *state = r_i_GetCurrentState(sizeof(struct r_i_state_t));

    if(!state->depth)
    {
        state->depth = r_i_AllocImmediateData(sizeof(struct r_i_depth_t));
    }

    state->depth->enable = enable;
    state->depth->func = func;
}

void r_i_SetCullFace(uint16_t enable, uint16_t cull_face)
{
    struct r_i_state_t *state = r_i_GetCurrentState(sizeof(struct r_i_state_t));

    if(!state->cull_face)
    {
        state->cull_face = r_i_AllocImmediateData(sizeof(struct r_i_cull_face_t));
    }

    state->cull_face->enable = enable;
    state->cull_face->cull_face = cull_face;
}

void r_i_SetStencil(uint16_t enable, uint16_t sfail, uint16_t dfail, uint16_t dpass, uint16_t op, uint8_t mask, uint8_t ref)
{
    struct r_i_state_t *state = r_i_GetCurrentState(sizeof(struct r_i_state_t));

    if(state->stencil)
    {
        state->stencil = r_i_AllocImmediateData(sizeof(struct r_i_stencil_t));
    }

    state->stencil->enable = enable;
    state->stencil->stencil_fail = sfail;
    state->stencil->depth_fail = dfail;
    state->stencil->depth_pass = dpass;
    state->stencil->operation = op;
    state->stencil->mask = mask;
    state->stencil->ref = ref;
}

void r_i_SetScissor(uint16_t enable, uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
    struct r_i_state_t *state = r_i_GetCurrentState(sizeof(struct r_i_state_t));

    if(!state->scissor)
    {
        state->scissor = r_i_AllocImmediateData(sizeof(struct r_i_scissor_t));
    }

    state->scissor->x = x;
    state->scissor->y = y;
    state->scissor->width = width;
    state->scissor->height = height;
    state->scissor->enable = enable;
}

void r_i_SetTextures(uint32_t texture_count, struct r_i_texture_t *textures)
{
    struct r_i_state_t *state = r_i_GetCurrentState(sizeof(struct r_i_state_t));
    state->textures = textures;
    state->texture_count = texture_count;
}

void r_i_SetTexture(struct r_texture_t *texture, uint32_t tex_unit)
{
    struct r_i_texture_t *immediate_texture = r_i_AllocImmediateData(sizeof(struct r_i_texture_t));
    immediate_texture->texture = texture;
    immediate_texture->tex_unit = tex_unit;
    r_i_SetTextures(1, immediate_texture);
}

void r_i_SetBuffers(struct r_i_verts_t *verts, struct r_i_indices_t *indices)
{
    struct r_i_geometry_t *geometry = r_i_AllocImmediateData(sizeof(struct r_i_geometry_t));
    geometry->indices = indices;
    geometry->verts = verts;
    r_i_ImmediateCmd(R_I_CMD_SET_BUFFERS, 0, geometry);
}

void r_i_SetMatrix(mat4_t *matrix, uint16_t sub_type)
{
    struct r_i_transform_t *transform = r_i_AllocImmediateData(sizeof(struct r_i_transform_t));

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

void r_i_DrawImmediate(uint16_t sub_type, struct r_i_draw_list_t *list)
{
    r_i_SetCurrentState();
    r_i_ImmediateCmd(R_I_CMD_DRAW, sub_type, list);
}

void r_i_DrawVerts(uint16_t sub_type, struct r_i_verts_t *verts)
{
    struct r_i_draw_list_t *draw_list = r_i_AllocImmediateData(sizeof(struct r_i_draw_list_t));

    draw_list->commands = r_i_AllocImmediateData(sizeof(struct r_i_draw_cmd_t));
    draw_list->commands[0].start = 0;
    draw_list->commands[0].count = verts->count;
    draw_list->command_count = 1;
    draw_list->indexed = 0;

    r_i_SetBuffers(verts, NULL);
    r_i_DrawImmediate(sub_type, draw_list);
}

void r_i_DrawVertsIndexed(uint16_t sub_type, struct r_i_verts_t *verts, struct r_i_indices_t *indices)
{
    struct r_i_draw_list_t *draw_list = r_i_AllocImmediateData(sizeof(struct r_i_draw_list_t));
    draw_list->commands = r_i_AllocImmediateData(sizeof(struct r_i_draw_cmd_t));
    draw_list->commands[0].start = 0;
    draw_list->commands[0].count = indices->count;
    draw_list->command_count = 1;
    draw_list->indexed = 1;
    r_i_SetBuffers(verts, indices);
    r_i_DrawImmediate(sub_type, draw_list);
}

void r_i_DrawPoint(vec3_t *position, vec4_t *color, float size)
{
    struct r_i_verts_t *verts = r_i_AllocImmediateData(sizeof(struct r_i_verts_t) + sizeof(struct r_vert_t));

    verts->count = 1;
    verts->size = size;

    verts->verts[0].pos = *position;
    verts->verts[0].normal = *color;

    r_i_DrawVerts(R_I_DRAW_CMD_POINT_LIST, verts);
}

void r_i_DrawLine(vec3_t *start, vec3_t *end, vec4_t *color, float width)
{
    struct r_i_verts_t *verts = r_i_AllocImmediateData(sizeof(struct r_i_verts_t) + sizeof(struct r_vert_t) * 2);

    verts->count = 2;
    verts->size = width;

    verts->verts[0].pos = *start;
    verts->verts[0].color = *color;
    verts->verts[1].pos = *end;
    verts->verts[1].color = *color;

    r_i_DrawVerts(R_I_DRAW_CMD_LINE_LIST, verts);
}





