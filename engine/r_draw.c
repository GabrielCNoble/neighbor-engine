#include "r_draw.h"
#include "SDL2/SDL.h"
#include "GL/glew.h"
#include "dstuff/ds_slist.h"
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

extern struct ds_list_t r_world_cmds;
extern struct ds_list_t r_entity_cmds;
extern struct ds_list_t r_shadow_cmds;
extern struct ds_list_t r_immediate_cmds;
extern struct ds_list_t r_immediate_data;
struct r_i_state_t *r_i_current_state = NULL;

extern struct ds_slist_t r_shaders;
extern struct ds_slist_t r_textures;
extern struct ds_slist_t r_materials;
extern struct ds_slist_t r_models;

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
extern struct r_shader_t *r_shadow_shader;

extern struct r_model_t *test_model;
extern struct r_texture_t *r_default_texture;
extern struct r_material_t *r_default_material;

extern struct ds_list_t r_visible_lights;

extern struct r_point_data_t *r_point_light_buffer;
extern uint32_t r_point_light_buffer_cursor;
extern uint32_t r_point_light_data_uniform_buffer;

extern struct r_spot_data_t *r_spot_light_buffer;
extern uint32_t r_spot_light_buffer_cursor;
extern uint32_t r_spot_light_data_uniform_buffer;

extern uint16_t *r_light_index_buffer;
extern uint32_t r_light_index_buffer_cursor;
extern uint32_t r_light_index_uniform_buffer;


//extern uint32_t *r_shadow_index_buffer;
//extern uint32_t r_shadow_index_buffer_cursor;
//extern uint32_t r_shadow_index_uniform_buffer;

extern struct r_shadow_map_t *r_shadow_map_buffer;
extern uint32_t r_shadow_map_buffer_cursor;
extern uint32_t r_shadow_map_uniform_buffer;

extern uint32_t r_shadow_atlas_texture;
extern uint32_t r_indirect_texture;
extern uint32_t r_shadow_map_framebuffer;
extern vec2_t r_point_shadow_projection_params;
extern mat4_t r_point_shadow_view_projection_matrices[6];
//extern mat4_t r_point_shadow_projection_matrices[6];
//extern mat4_t r_point_shadow_view_matrices[6];

extern struct r_cluster_t *r_clusters;
extern uint32_t r_cluster_texture;

mat4_t r_projection_matrix;
mat4_t r_camera_matrix;
mat4_t r_view_matrix;
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

extern uint32_t r_main_framebuffer;
extern uint32_t r_z_prepass_framebuffer;

//uint32_t r_use_z_prepass = 1;
//uint32_t r_draw_call_count = 0;
//uint32_t r_prev_draw_call_count = 0;

struct r_renderer_state_t r_renderer_state;

extern uint32_t r_uniform_type_sizes[];

#ifdef __cplusplus
extern "C"
{
#endif

void r_BeginFrame()
{
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, r_main_framebuffer);
    glViewport(0, 0, r_width, r_height);
    glDisable(GL_SCISSOR_TEST);
    glDepthMask(GL_TRUE);
    glClearColor(0.01, 0.01, 0.01, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glBindVertexArray(r_vao);

    if(r_point_light_buffer_cursor)
    {
        glBindBuffer(GL_UNIFORM_BUFFER, r_point_light_data_uniform_buffer);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, r_point_light_buffer_cursor * sizeof(struct r_point_data_t), r_point_light_buffer);
    }

    if(r_spot_light_buffer_cursor)
    {
        glBindBuffer(GL_UNIFORM_BUFFER, r_spot_light_data_uniform_buffer);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, r_spot_light_buffer_cursor * sizeof(struct r_spot_data_t), r_spot_light_buffer);
    }

    glBindBuffer(GL_UNIFORM_BUFFER, r_light_index_uniform_buffer);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, R_MAX_CLUSTER_LIGHTS * R_CLUSTER_COUNT * sizeof(uint32_t), r_light_index_buffer);

    glBindBuffer(GL_UNIFORM_BUFFER, r_shadow_map_uniform_buffer);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, r_shadow_map_buffer_cursor * sizeof(struct r_shadow_map_t), r_shadow_map_buffer);

    glActiveTexture(GL_TEXTURE0 + R_CLUSTERS_TEX_UNIT);
    glBindTexture(GL_TEXTURE_3D, r_cluster_texture);
    glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, R_CLUSTER_ROW_WIDTH, R_CLUSTER_ROWS, R_CLUSTER_SLICES, GL_RGB_INTEGER, GL_UNSIGNED_INT, r_clusters);
    glBindBufferBase(GL_UNIFORM_BUFFER, R_POINT_LIGHT_UNIFORM_BUFFER_BINDING, r_point_light_data_uniform_buffer);
    glBindBufferBase(GL_UNIFORM_BUFFER, R_SPOT_LIGHT_UNIFORM_BUFFER_BINDING, r_spot_light_data_uniform_buffer);
    glBindBufferBase(GL_UNIFORM_BUFFER, R_LIGHT_INDICES_UNIFORM_BUFFER_BINDING, r_light_index_uniform_buffer);
    glBindBufferBase(GL_UNIFORM_BUFFER, R_SHADOW_INDICES_BUFFER_BINDING, r_shadow_map_uniform_buffer);

    glActiveTexture(GL_TEXTURE0 + R_SHADOW_ATLAS_TEX_UNIT);
    glBindTexture(GL_TEXTURE_2D, r_shadow_atlas_texture);

    glActiveTexture(GL_TEXTURE0 + R_INDIRECT_TEX_UNIT);
    glBindTexture(GL_TEXTURE_CUBE_MAP, r_indirect_texture);

    r_denom = log(r_z_far / r_z_near);

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glBindBuffer(GL_ARRAY_BUFFER, r_vertex_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, r_index_buffer);

//    r_renderer_state = (struct r_renderer_state_t){};

    r_renderer_state.draw_call_count = 0;
    r_renderer_state.indice_count = 0;
    r_renderer_state.material_swaps = 0;
    r_renderer_state.shader_swaps = 0;
    r_renderer_state.vert_count = 0;
}

void r_EndFrame()
{
    r_point_light_buffer_cursor = 0;
    r_light_index_buffer_cursor = 0;

    r_world_cmds.cursor = 0;
    r_entity_cmds.cursor = 0;
    r_shadow_cmds.cursor = 0;
    r_immediate_cmds.cursor = 0;
    r_immediate_data.cursor = 0;
//    r_prev_draw_call_count = r_draw_call_count;
//    r_draw_call_count = 0;
    r_i_current_state = NULL;

    glDisable(GL_BLEND);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_SCISSOR_TEST);
    glViewport(0, 0, r_width, r_height);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, r_main_framebuffer);

    glBlitFramebuffer(0, 0, r_width, r_height, 0, 0, r_width, r_height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    SDL_GL_SwapWindow(r_window);
}


void r_SetViewPos(vec3_t *pos)
{
    r_camera_matrix.rows[3].x = pos->x;
    r_camera_matrix.rows[3].y = pos->y;
    r_camera_matrix.rows[3].z = pos->z;
    r_UpdateViewProjectionMatrix();
}

void r_TranslateView(vec3_t *disp)
{
    r_camera_matrix.rows[3].x += disp->x;
    r_camera_matrix.rows[3].y += disp->y;
    r_camera_matrix.rows[3].z += disp->z;
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
    r_camera_matrix.rows[0] = pitch_matrix.rows[0];
    r_camera_matrix.rows[1] = pitch_matrix.rows[1];
    r_camera_matrix.rows[2] = pitch_matrix.rows[2];
    r_UpdateViewProjectionMatrix();
}

void r_UpdateViewProjectionMatrix()
{
    mat4_t_invvm(&r_view_matrix, &r_camera_matrix);
    mat4_t_mul(&r_view_projection_matrix, &r_view_matrix, &r_projection_matrix);
}

void r_DrawElements(uint32_t mode, uint32_t count, uint32_t type, const void *indices)
{
    glDrawElements(mode, count, type, indices);
    r_renderer_state.draw_call_count++;
    r_renderer_state.indice_count += count;
//    r_renderer_stats.vert_count +=
}

void r_DrawEntity(mat4_t *model_matrix, struct r_model_t *model)
{
    if(model)
    {
        struct r_batch_t *batches = (struct r_batch_t *)model->batches.buffer;
        mat4_t model_view_matrix;
        mat4_t_mul(&model_view_matrix, model_matrix, &r_view_matrix);
        for(uint32_t batch_index = 0; batch_index < model->batches.buffer_size; batch_index++)
        {
            struct r_batch_t *batch = batches + batch_index;
            uint32_t index = ds_list_add_element(&r_entity_cmds, NULL);
            struct r_entity_cmd_t *cmd = ds_list_get_element(&r_entity_cmds, index);
            cmd->model_view_matrix = model_view_matrix;
            cmd->start = batch->start;
            cmd->count = batch->count;
            cmd->material = batch->material;
        }
    }
}

void r_DrawShadow(mat4_t *model_view_projection_matrix, uint32_t shadow_map, uint32_t start, uint32_t count)
{
    uint32_t index = ds_list_add_element(&r_shadow_cmds, NULL);
    struct r_shadow_cmd_t *shadow_cmd = ds_list_get_element(&r_shadow_cmds, index);
    shadow_cmd->start = start;
    shadow_cmd->count = count;
    shadow_cmd->shadow_map = shadow_map;
    shadow_cmd->model_view_projection_matrix = *model_view_projection_matrix;
}

void r_DrawWorld(struct r_material_t *material, uint32_t start, uint32_t count)
{
    if(material)
    {
        uint32_t index = ds_list_add_element(&r_world_cmds, NULL);
        struct r_world_cmd_t *cmd = ds_list_get_element(&r_world_cmds, index);
        cmd->material = material;
        cmd->start = start;
        cmd->count = count;
    }
}

int32_t r_CompareWorldCmds(void *a, void *b)
{
    struct r_world_cmd_t *cmd_a = (struct r_world_cmd_t *)a;
    struct r_world_cmd_t *cmd_b = (struct r_world_cmd_t *)b;
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

int32_t r_CompareEntityCmds(void *a, void *b)
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

int32_t r_CompareShadowCmds(void *a, void *b)
{
    struct r_shadow_cmd_t *cmd_a = (struct r_shadow_cmd_t *)a;
    struct r_shadow_cmd_t *cmd_b = (struct r_shadow_cmd_t *)b;
    if(cmd_a->shadow_map < cmd_b->shadow_map)
    {
        return -1;
    }
    else if(cmd_a->shadow_map > cmd_b->shadow_map)
    {
        return 1;
    }

    return 0;
}

void r_DrawCmds()
{
    mat4_t model_view_projection_matrix;
    mat4_t model_view_matrix;

    ds_list_qsort(&r_world_cmds, r_CompareWorldCmds);
    ds_list_qsort(&r_entity_cmds, r_CompareEntityCmds);
    ds_list_qsort(&r_shadow_cmds, r_CompareShadowCmds);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glDisable(GL_BLEND);
    glDepthFunc(GL_LESS);


    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, r_shadow_map_framebuffer);
    glEnable(GL_SCISSOR_TEST);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.0, 1.0);
    r_BindShader(r_shadow_shader);

//    uint32_t cur_shadow_map = 0xffffffff;
    struct r_shadow_map_t *cur_shadow_map = NULL;
    for(uint32_t cmd_index = 0; cmd_index < r_shadow_cmds.cursor; cmd_index++)
    {
        struct r_shadow_cmd_t *cmd = ds_list_get_element(&r_shadow_cmds, cmd_index);
        struct r_shadow_map_t *shadow_map = r_GetShadowMap(cmd->shadow_map);

        if(shadow_map != cur_shadow_map)
        {
            uint32_t x_coord = shadow_map->x_coord;
            uint32_t y_coord = shadow_map->y_coord;
            uint32_t resolution = R_SHADOW_BUCKET_RESOLUTION(cmd->shadow_map);

            glScissor(x_coord, y_coord, resolution, resolution);
            glViewport(x_coord, y_coord, resolution, resolution);
            glClear(GL_DEPTH_BUFFER_BIT);

            cur_shadow_map = shadow_map;
        }

        r_SetDefaultUniformMat4(R_UNIFORM_MODEL_VIEW_PROJECTION_MATRIX, &cmd->model_view_projection_matrix);
        glDrawElements(GL_TRIANGLES, cmd->count, GL_UNSIGNED_INT, (void *)(cmd->start * sizeof(uint32_t)));
        r_renderer_state.draw_call_count++;
    }
    glDisable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(0, 0);

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, r_z_prepass_framebuffer);
    glDisable(GL_SCISSOR_TEST);
    glScissor(0, 0, r_width, r_height);
    glViewport(0, 0, r_width, r_height);

    if(r_renderer_state.use_z_prepass)
    {
        r_BindShader(r_z_prepass_shader);
        glDepthFunc(GL_LESS);

        r_SetDefaultUniformMat4(R_UNIFORM_MODEL_VIEW_PROJECTION_MATRIX, &r_view_projection_matrix);
        for(uint32_t cmd_index = 0; cmd_index < r_world_cmds.cursor; cmd_index++)
        {
            struct r_world_cmd_t *cmd = ds_list_get_element(&r_world_cmds, cmd_index);
            glDrawElements(GL_TRIANGLES, cmd->count, GL_UNSIGNED_INT, (void *)(cmd->start * sizeof(uint32_t)));
            r_renderer_state.draw_call_count++;
        }

        for(uint32_t cmd_index = 0; cmd_index < r_entity_cmds.cursor; cmd_index++)
        {
            struct r_entity_cmd_t *cmd = ds_list_get_element(&r_entity_cmds, cmd_index);
            mat4_t_mul(&model_view_projection_matrix, &cmd->model_view_matrix, &r_projection_matrix);
            r_SetDefaultUniformMat4(R_UNIFORM_MODEL_VIEW_PROJECTION_MATRIX, &model_view_projection_matrix);
            glDrawElements(GL_TRIANGLES, cmd->count, GL_UNSIGNED_INT, (void *)(cmd->start * sizeof(uint32_t)));
            r_renderer_state.draw_call_count++;
        }

        glDepthFunc(GL_EQUAL);
        glDepthMask(GL_FALSE);
    }

    struct r_material_t *current_material = NULL;

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, r_main_framebuffer);
    r_BindShader(r_lit_shader);
    r_SetDefaultUniformI(R_UNIFORM_TEX_CLUSTERS, R_CLUSTERS_TEX_UNIT);
    r_SetDefaultUniformI(R_UNIFORM_TEX_SHADOW_ATLAS, R_SHADOW_ATLAS_TEX_UNIT);
    r_SetDefaultUniformI(R_UNIFORM_TEX_INDIRECT, R_INDIRECT_TEX_UNIT);
    r_SetDefaultUniformMat4(R_UNIFORM_CAMERA_MATRIX, &r_camera_matrix);
    r_SetDefaultUniformVec2(R_UNIFORM_POINT_PROJ_PARAMS, &r_point_shadow_projection_params);

    r_SetDefaultUniformMat4(R_UNIFORM_MODEL_VIEW_PROJECTION_MATRIX, &r_view_projection_matrix);
    r_SetDefaultUniformMat4(R_UNIFORM_MODEL_VIEW_MATRIX, &r_view_matrix);
    for(uint32_t cmd_index = 0; cmd_index < r_world_cmds.cursor; cmd_index++)
    {
        struct r_world_cmd_t *cmd = ds_list_get_element(&r_world_cmds, cmd_index);

        if(cmd->material != current_material)
        {
            current_material = cmd->material;
            r_BindMaterial(current_material);
        }

        glDrawElements(GL_TRIANGLES, cmd->count, GL_UNSIGNED_INT, (void *)(cmd->start * sizeof(uint32_t)));
        r_renderer_state.draw_call_count++;
    }

    current_material = NULL;

    for(uint32_t cmd_index = 0; cmd_index < r_entity_cmds.cursor; cmd_index++)
    {
        struct r_entity_cmd_t *cmd = ds_list_get_element(&r_entity_cmds, cmd_index);

        if(cmd->material != current_material)
        {
            current_material = cmd->material;
            r_BindMaterial(current_material);
        }

        mat4_t_mul(&model_view_projection_matrix, &cmd->model_view_matrix, &r_projection_matrix);
        r_SetDefaultUniformMat4(R_UNIFORM_MODEL_VIEW_PROJECTION_MATRIX, &model_view_projection_matrix);
        r_SetDefaultUniformMat4(R_UNIFORM_MODEL_VIEW_MATRIX, &cmd->model_view_matrix);
        glDrawElements(GL_TRIANGLES, cmd->count, GL_UNSIGNED_INT, (void *)(cmd->start * sizeof(uint32_t)));
        r_renderer_state.draw_call_count++;
    }

    mat4_t view_projection_matrix;
    mat4_t model_matrix;

    mat4_t_identity(&view_projection_matrix);
    mat4_t_identity(&model_matrix);
    glDepthMask(GL_TRUE);
//    glBindBuffer(GL_ARRAY_BUFFER, r_immediate_vertex_buffer);
//    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, r_immediate_index_buffer);
    glDepthFunc(GL_LESS);
    r_BindShader(r_immediate_shader);

    struct r_i_state_t *immediate_state = NULL;
    struct r_i_transform_t *immediate_view_projection_matrix = NULL;
    struct r_i_transform_t *immediate_model_matrix = NULL;
    struct r_i_verts_t *immediate_verts = NULL;
    struct r_i_indices_t *immediate_indices = NULL;
    uint32_t cmd_vert_start_offset = 0;
    uint32_t cmd_index_start_offset = 0;

    for(uint32_t cmd_index = 0; cmd_index < r_immediate_cmds.cursor; cmd_index++)
    {
        struct r_i_cmd_t *cmd = ds_list_get_element(&r_immediate_cmds, cmd_index);

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
                if(immediate_state->shader)
                {
                    r_BindShader(immediate_state->shader->shader);
                }

                immediate_state->shader = NULL;
            break;

            case R_I_CMD_SET_UNIFORM:
            {
                struct r_i_uniform_t *uniform = cmd->data;
                r_SetNamedUniform(uniform->uniform, uniform->value);
            }
            break;

            case R_I_CMD_SET_BUFFERS:
            {
                struct r_i_geometry_t *geometry = cmd->data;
                immediate_verts = geometry->verts;
                immediate_indices = geometry->indices;

                if(immediate_verts)
                {
                    cmd_vert_start_offset = R_IMMEDIATE_VERTEX_BUFFER_OFFSET / sizeof(struct r_vert_t);
                }
                else
                {
                    cmd_vert_start_offset = 0;
                }

                if(immediate_indices)
                {
                    cmd_index_start_offset = R_IMMEDIATE_INDEX_BUFFER_OFFSET / sizeof(uint32_t);
                }
                else
                {
                    cmd_index_start_offset = 0;
                }
            }
            break;

            case R_I_CMD_DRAW:
            {
                if(immediate_state)
                {
//                    if(immediate_state->shader)
//                    {
//                        r_BindShader(immediate_state->shader->shader);
//                    }

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

                            r_SetDefaultUniformI(R_UNIFORM_TEX0 + texture_index, texture->tex_unit - GL_TEXTURE0);
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
                            glStencilFunc(immediate_state->stencil->func, immediate_state->stencil->ref, immediate_state->stencil->mask);
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

                            if(immediate_state->depth->func != GL_DONT_CARE)
                            {
                                glDepthFunc(immediate_state->depth->func);
                            }
                        }
                        else
                        {
                            glDisable(GL_DEPTH_TEST);
                        }
                    }

                    if(immediate_state->rasterizer)
                    {
                        if(immediate_state->rasterizer->cull_enable != GL_DONT_CARE)
                        {
                            if(immediate_state->rasterizer->cull_enable)
                            {
                                glEnable(GL_CULL_FACE);
                                glCullFace(immediate_state->rasterizer->cull_face);
                            }
                            else
                            {
                                glDisable(GL_CULL_FACE);
                            }
                        }

                        if(immediate_state->rasterizer->polygon_mode != GL_DONT_CARE)
                        {
                            glPolygonMode(GL_FRONT_AND_BACK, immediate_state->rasterizer->polygon_mode);
                        }
                    }

                    if(immediate_state->draw_mask)
                    {
                        glColorMask(immediate_state->draw_mask->red, immediate_state->draw_mask->green, immediate_state->draw_mask->blue, immediate_state->draw_mask->alpha);
                        glDepthMask(immediate_state->draw_mask->depth);
                        glStencilMask(immediate_state->draw_mask->stencil);
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
                    r_SetDefaultUniformMat4(R_UNIFORM_MODEL_VIEW_PROJECTION_MATRIX, &model_view_projection_matrix);
                }

                immediate_model_matrix = NULL;
                immediate_view_projection_matrix = NULL;
                immediate_state = NULL;

                uint32_t mode = 0;
                struct r_i_draw_list_t *draw_list = cmd->data;

                switch(cmd->sub_type)
                {
                    case R_I_DRAW_CMD_LINE_LIST:
                        if(draw_list->size)
                        {
                            glLineWidth(draw_list->size);
                        }
                        mode = GL_LINES;
                    break;

                    case R_I_DRAW_CMD_LINE_STRIP:
                        if(draw_list->size)
                        {
                            glLineWidth(draw_list->size);
                        }
                        mode = GL_LINE_STRIP;
                    break;

                    case R_I_DRAW_CMD_POINT_LIST:
                        if(draw_list->size)
                        {
                            glPointSize(draw_list->size);
                        }
                        mode = GL_POINTS;
                    break;

                    case R_I_DRAW_CMD_TRIANGLE_LIST:
                        if(draw_list->size)
                        {
                            glLineWidth(draw_list->size);
                        }
                        mode = GL_TRIANGLES;
                    break;
                }

                if(immediate_verts)
                {
                    glBufferSubData(GL_ARRAY_BUFFER, R_IMMEDIATE_VERTEX_BUFFER_OFFSET, sizeof(struct r_vert_t) * immediate_verts->count, immediate_verts->verts);
                    immediate_verts = NULL;
                }

                if(immediate_indices)
                {
                    for(uint32_t index = 0; index < immediate_indices->count; index++)
                    {
                        immediate_indices->indices[index] += R_IMMEDIATE_VERTEX_BUFFER_OFFSET / sizeof(struct r_vert_t);
                    }

                    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, R_IMMEDIATE_INDEX_BUFFER_OFFSET, sizeof(uint32_t) * immediate_indices->count, immediate_indices->indices);
                    immediate_indices = NULL;
                }

                if(draw_list->indexed)
                {
                    for(uint32_t cmd_index = 0; cmd_index < draw_list->command_count; cmd_index++)
                    {
                        struct r_i_draw_cmd_t *draw_cmd = draw_list->commands + cmd_index;
                        glDrawElements(mode, draw_cmd->count, GL_UNSIGNED_INT, (void *)((draw_cmd->start + cmd_index_start_offset) * sizeof(uint32_t)));
                        r_renderer_state.draw_call_count++;
                    }
                }
                else
                {
                    for(uint32_t cmd_index = 0; cmd_index < draw_list->command_count; cmd_index++)
                    {
                        struct r_i_draw_cmd_t *draw_cmd = draw_list->commands + cmd_index;
                        glDrawArrays(mode, draw_cmd->start + cmd_vert_start_offset, draw_cmd->count);
                        r_renderer_state.draw_call_count++;
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

//    glDisable(GL_BLEND);
//    glFlush();
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

        uint32_t data_index = ds_list_add_element(&r_immediate_data, NULL);
        data = ds_list_get_element(&r_immediate_data, data_index);
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
    uint32_t cmd_index = ds_list_add_element(&r_immediate_cmds, NULL);
    struct r_i_cmd_t *cmd = ds_list_get_element(&r_immediate_cmds, cmd_index);

    cmd->data = data;
    cmd->type = type;
    cmd->sub_type = sub_type;
}

struct r_i_draw_list_t *r_i_AllocDrawList(uint32_t cmd_count)
{
    struct r_i_draw_list_t *list = r_i_AllocImmediateData(sizeof(struct r_i_draw_list_t));
    list->commands = r_i_AllocImmediateData(sizeof(struct r_i_draw_cmd_t) * cmd_count);
    list->command_count = cmd_count;

    return list;
}

struct r_i_verts_t *r_i_AllocVerts(uint32_t vert_count)
{
    struct r_i_verts_t *verts = r_i_AllocImmediateData(sizeof(struct r_i_verts_t) + sizeof(struct r_vert_t) * vert_count);
    verts->count = vert_count;
    return verts;
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

void r_i_SetUniform(struct r_named_uniform_t *uniform, uint32_t count, void *value)
{
    if(uniform)
    {
        r_i_SetCurrentState();
        struct r_i_uniform_t *uniform_data = r_i_AllocImmediateData(sizeof(struct r_i_uniform_t));
        uint32_t size = r_uniform_type_sizes[uniform->type] * count;
        uniform_data->value = r_i_AllocImmediateData(size);
        uniform_data->count = count;
        uniform_data->uniform = uniform;
        memcpy(uniform_data->value, value, size);
        r_i_ImmediateCmd(R_I_CMD_SET_UNIFORM, 0, uniform_data);
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
    struct r_i_state_t *state = r_i_GetCurrentState();

    if(!state->depth)
    {
        state->depth = r_i_AllocImmediateData(sizeof(struct r_i_depth_t));
    }

    state->depth->enable = enable;
    state->depth->func = func;
}

void r_i_SetCullFace(uint16_t enable, uint16_t cull_face)
{
    r_i_SetRasterizer(enable, cull_face, GL_DONT_CARE);
//    struct r_i_state_t *state = r_i_GetCurrentState();
//
//    if(!state->cull_face)
//    {
//        state->cull_face = r_i_AllocImmediateData(sizeof(struct r_i_cull_face_t));
//    }
//
//    state->cull_face->enable = enable;
//    state->cull_face->cull_face = cull_face;
}

void r_i_SetRasterizer(uint16_t cull_face_enable, uint16_t cull_face, uint16_t polygon_mode)
{
    struct r_i_state_t *state = r_i_GetCurrentState();

    if(!state->rasterizer)
    {
        state->rasterizer = r_i_AllocImmediateData(sizeof(struct r_i_raster_t));
    }

    state->rasterizer->cull_enable = cull_face_enable;
    state->rasterizer->cull_face = cull_face;
    state->rasterizer->polygon_mode = polygon_mode;
}

void r_i_SetStencil(uint16_t enable, uint16_t sfail, uint16_t dfail, uint16_t dpass, uint16_t func, uint8_t mask, uint8_t ref)
{
    struct r_i_state_t *state = r_i_GetCurrentState();

    if(!state->stencil)
    {
        state->stencil = r_i_AllocImmediateData(sizeof(struct r_i_stencil_t));
    }

    state->stencil->enable = enable;
    state->stencil->stencil_fail = sfail;
    state->stencil->depth_fail = dfail;
    state->stencil->depth_pass = dpass;
    state->stencil->func = func;
    state->stencil->mask = mask;
    state->stencil->ref = ref;
}

void r_i_SetScissor(uint16_t enable, uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
    struct r_i_state_t *state = r_i_GetCurrentState();

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

void r_i_SetDrawMask(uint16_t red, uint16_t green, uint16_t blue, uint16_t alpha, uint16_t depth, uint16_t stencil)
{
    struct r_i_state_t *state = r_i_GetCurrentState();

    if(!state->draw_mask)
    {
        state->draw_mask = r_i_AllocImmediateData(sizeof(struct r_i_draw_mask_t));
    }

    state->draw_mask->red = red;
    state->draw_mask->green = green;
    state->draw_mask->blue = blue;
    state->draw_mask->alpha = alpha;
    state->draw_mask->depth = depth;
    state->draw_mask->stencil = stencil;
}

void r_i_SetTextures(uint32_t texture_count, struct r_i_texture_t *textures)
{
    struct r_i_state_t *state = r_i_GetCurrentState();
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

void r_i_DrawVerts(uint16_t sub_type, struct r_i_verts_t *verts, float size)
{
    struct r_i_draw_list_t *draw_list = r_i_AllocImmediateData(sizeof(struct r_i_draw_list_t));

    draw_list->commands = r_i_AllocImmediateData(sizeof(struct r_i_draw_cmd_t));
    draw_list->commands[0].start = 0;
    draw_list->commands[0].count = verts->count;
    draw_list->command_count = 1;
    draw_list->indexed = 0;
    draw_list->size = size;

    r_i_SetBuffers(verts, NULL);
    r_i_DrawImmediate(sub_type, draw_list);
}

void r_i_DrawVertsIndexed(uint16_t sub_type, struct r_i_verts_t *verts, struct r_i_indices_t *indices, float size)
{
    struct r_i_draw_list_t *draw_list = r_i_AllocImmediateData(sizeof(struct r_i_draw_list_t));
    draw_list->commands = r_i_AllocImmediateData(sizeof(struct r_i_draw_cmd_t));
    draw_list->commands[0].start = 0;
    draw_list->commands[0].count = indices->count;
    draw_list->command_count = 1;
    draw_list->indexed = 1;
    draw_list->size = size;
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

    r_i_DrawVerts(R_I_DRAW_CMD_POINT_LIST, verts, size);
}

void r_i_DrawLine(vec3_t *start, vec3_t *end, vec4_t *color, float width)
{
    struct r_i_verts_t *verts = r_i_AllocImmediateData(sizeof(struct r_i_verts_t) + sizeof(struct r_vert_t) * 2);

    verts->count = 2;

    verts->verts[0].pos = *start;
    verts->verts[0].color = *color;
    verts->verts[1].pos = *end;
    verts->verts[1].color = *color;

    r_i_DrawVerts(R_I_DRAW_CMD_LINE_LIST, verts, width);
}

#ifdef __cplusplus
}
#endif



