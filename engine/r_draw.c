#include "r_draw.h"
#include "r_draw_i.h"
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
//extern struct ds_list_t r_entity_cmds;
extern struct r_cmd_buffer_t r_entity_cmd_buffer;
extern struct r_cmd_buffer_t r_shadow_cmd_buffer;
extern struct r_i_cmd_buffer_t r_immediate_cmd_buffer;
//extern struct ds_list_t r_shadow_cmds;
//extern struct ds_list_t r_immediate_cmds;
//extern struct ds_list_t r_immediate_data;
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
extern struct r_vert_t *r_vert_copy_buffer;

extern uint32_t r_vao;
extern struct r_shader_t *r_z_prepass_shader;
extern struct r_shader_t *r_lit_shader;
extern struct r_shader_t *r_immediate_shader;
extern struct r_shader_t *r_current_shader;
extern struct r_shader_t *r_shadow_shader;
extern struct r_shader_t *r_volumetric_shader;
extern struct r_shader_t *r_bilateral_blend_shader;
extern struct r_shader_t *r_full_screen_blend_shader;
extern struct r_shader_t *r_blend_ui_debug_shader;

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
extern uint32_t r_max_parallax_samples;
extern vec4_t r_clear_color;

//extern uint32_t r_main_framebuffer;
extern struct r_framebuffer_t *r_main_framebuffer;
//extern struct r_framebuffer_t *r_debug_framebuffer;
extern struct r_framebuffer_t *r_volume_framebuffer;
extern struct r_framebuffer_t *r_debug_framebuffer;
extern struct r_framebuffer_t *r_ui_framebuffer;
extern uint32_t r_screen_tri_start;
extern uint32_t r_z_prepass_framebuffer;

//uint32_t r_use_z_prepass = 1;
//uint32_t r_draw_call_count = 0;
//uint32_t r_prev_draw_call_count = 0;

struct r_renderer_state_t r_renderer_state;
uint32_t r_immediate_verts_offset = 0;
uint32_t r_immediate_indices_offset = 0;
extern uint32_t r_test_query;


extern uint32_t r_uniform_type_sizes[];

#ifdef __cplusplus
extern "C"
{
#endif

void r_BeginFrame()
{
//    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, r_main_framebuffer->handle);
//    glViewport(0, 0, r_width, r_height);
    r_BindFramebuffer(r_main_framebuffer);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glBindVertexArray(r_vao);

//    if(r_point_light_buffer_cursor)
//    {
//        glBindBuffer(GL_SHADER_STORAGE_BUFFER, r_point_light_data_uniform_buffer);
//        glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, r_point_light_buffer_cursor * sizeof(struct r_point_data_t), r_point_light_buffer);
//    }
//
//    if(r_spot_light_buffer_cursor)
//    {
//        glBindBuffer(GL_SHADER_STORAGE_BUFFER, r_spot_light_data_uniform_buffer);
//        glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, r_spot_light_buffer_cursor * sizeof(struct r_spot_data_t), r_spot_light_buffer);
//    }

//    glBindBuffer(GL_SHADER_STORAGE_BUFFER, r_light_index_uniform_buffer);
//    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, R_MAX_CLUSTER_LIGHTS * R_CLUSTER_COUNT * sizeof(uint32_t), r_light_index_buffer);
//
//    glBindBuffer(GL_SHADER_STORAGE_BUFFER, r_shadow_map_uniform_buffer);
//    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, r_shadow_map_buffer_cursor * sizeof(struct r_shadow_map_t), r_shadow_map_buffer);
//
//    glActiveTexture(GL_TEXTURE0 + R_CLUSTERS_TEX_UNIT);
//    glBindTexture(GL_TEXTURE_3D, r_cluster_texture);
//    glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, R_CLUSTER_ROW_WIDTH, R_CLUSTER_ROWS, R_CLUSTER_SLICES, GL_RGB_INTEGER, GL_UNSIGNED_INT, r_clusters);
//    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, R_POINT_LIGHT_UNIFORM_BUFFER_BINDING, r_point_light_data_uniform_buffer);
//    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, R_SPOT_LIGHT_UNIFORM_BUFFER_BINDING, r_spot_light_data_uniform_buffer);
//    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, R_LIGHT_INDICES_UNIFORM_BUFFER_BINDING, r_light_index_uniform_buffer);
//    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, R_SHADOW_INDICES_BUFFER_BINDING, r_shadow_map_uniform_buffer);
//
//    glActiveTexture(GL_TEXTURE0 + R_SHADOW_ATLAS_TEX_UNIT);
//    glBindTexture(GL_TEXTURE_2D, r_shadow_atlas_texture);
//
//    glActiveTexture(GL_TEXTURE0 + R_INDIRECT_TEX_UNIT);
//    glBindTexture(GL_TEXTURE_CUBE_MAP, r_indirect_texture);
//
//    r_denom = log(r_z_far / r_z_near);
//
//    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
//
    glBindBuffer(GL_ARRAY_BUFFER, r_vertex_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, r_index_buffer);

    r_renderer_state.draw_call_count = 0;
    r_renderer_state.indice_count = 0;
    r_renderer_state.material_swaps = 0;
    r_renderer_state.shader_swaps = 0;
    r_renderer_state.vert_count = 0;

//    r_BeginCmdBuffer(&r_entity_cmd_buffer);
}

void r_SetClearColor(float r, float g, float b, float a)
{
    r_clear_color.x = r;
    r_clear_color.y = g;
    r_clear_color.z = b;
    r_clear_color.w = a;
}

void r_EndFrame()
{
    r_point_light_buffer_cursor = 0;
    r_light_index_buffer_cursor = 0;

    r_ResetCmdBuffer(&r_entity_cmd_buffer);
    r_ResetCmdBuffer(&r_shadow_cmd_buffer);
    r_ResetImmediateCmdBuffer(&r_immediate_cmd_buffer);
    r_world_cmds.cursor = 0;
//    r_entity_cmds.cursor = 0;
//    r_ResetCmdBuffer(&r_entity_cmd_buffer);
//    r_shadow_cmds.cursor = 0;
//    r_immediate_cmds.cursor = 0;
//    r_immediate_data.cursor = 0;
    r_i_current_state = NULL;
//    glEnable(GL_FRAMEBUFFER_SRGB);
    r_PresentFramebuffer(r_main_framebuffer);
//    glDisable(GL_FRAMEBUFFER_SRGB);
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
//            uint32_t index = ds_list_add_element(&r_entity_cmds, NULL);
//            struct r_entity_cmd_t *cmd = ds_list_get_element(&r_entity_cmds, index);
            struct r_entity_cmd_t *cmd = (struct r_entity_cmd_t *)r_AllocCmd(&r_entity_cmd_buffer);
            cmd->model_view_matrix = model_view_matrix;
            cmd->start = batch->start;
            cmd->count = batch->count;
            cmd->material = batch->material;
        }
    }
}

void r_DrawShadow(mat4_t *model_view_projection_matrix, uint32_t shadow_map, uint32_t start, uint32_t count)
{
//    uint32_t index = ds_list_add_element(&r_shadow_cmds, NULL);
//    struct r_shadow_cmd_t *shadow_cmd = ds_list_get_element(&r_shadow_cmds, index);
    struct r_shadow_cmd_t *shadow_cmd = (struct r_shadow_cmd_t *)r_AllocCmd(&r_shadow_cmd_buffer);
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

//void r_CopyImmediateVerts(void *verts, uint32_t size)
//{
//    r_ResetImmediateVertsBuffer();
//    r_AppendImmediateVerts(verts, size);
//}

//uint32_t r_AppendImmediateVerts(void *verts, uint32_t size)
//{
//    uint32_t offset = 0;
//
//    if(verts && size)
//    {
//        if(size < R_IMMEDIATE_VERTEX_BUFFER_SIZE && r_immediate_verts_offset < R_VERTEX_BUFFER_SIZE)
//        {
//            offset = r_immediate_verts_offset;
//            glBufferSubData(GL_ARRAY_BUFFER, r_immediate_verts_offset, size, verts);
//            r_immediate_verts_offset += size;
//        }
//    }
//
//    return offset;
//}

//void r_ResetImmediateVertsBuffer()
//{
//    r_immediate_verts_offset = R_IMMEDIATE_VERTEX_BUFFER_OFFSET;
//}

//void r_CopyImmediateIndices(void *indices, uint32_t size)
//{
//    if(indices && size)
//    {
//        if(size < R_IMMEDIATE_INDEX_BUFFER_SIZE)
//        {
//            glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, R_IMMEDIATE_INDEX_BUFFER_OFFSET, size, indices);
//        }
//    }
//}

void r_DrawVerts(struct r_vert_t *verts, uint32_t count, uint32_t mode)
{
//    uint32_t vert_start = R_IMMEDIATE_VERTEX_BUFFER_OFFSET / sizeof(struct r_vert_t);
//    r_CopyImmediateVerts(verts, sizeof(struct r_vert_t) * count);
//    glDrawArrays(mode, vert_start, count);
}

void r_DrawBox(vec3_t *half_extents, vec4_t *color)
{
//    vec3_t min = vec3_t_c(-half_extents->x, -half_extents->y, -half_extents->z);
//    vec3_t max = vec3_t_c(half_extents->x, half_extents->y, half_extents->z);
////
//    struct r_vert_t verts[] = {
//        {.pos = vec3_t_c(min.x, max.y, min.z), .color = *color},
//        {.pos = vec3_t_c(min.x, max.y, max.z), .color = *color},
//        {.pos = vec3_t_c(max.x, max.y, max.z), .color = *color},
//        {.pos = vec3_t_c(max.x, max.y, min.z), .color = *color},
//
//        {.pos = vec3_t_c(min.x, min.y, min.z), .color = *color},
//        {.pos = vec3_t_c(min.x, min.y, max.z), .color = *color},
//        {.pos = vec3_t_c(max.x, min.y, max.z), .color = *color},
//        {.pos = vec3_t_c(max.x, min.y, min.z), .color = *color},
////        {.pos = vec3_t_c(min.x, max.y, max.z), .color = *color}, {.pos = vec3_t_c(min.x, min.y, max.z), .color = *color},
////        {.pos = vec3_t_c(max.x, max.y, max.z), .color = *color}, {.pos = vec3_t_c(max.x, min.y, max.z), .color = *color},
////        {.pos = vec3_t_c(min.x, max.y, min.z), .color = *color}, {.pos = vec3_t_c(min.x, min.y, min.z), .color = *color},
////        {.pos = vec3_t_c(max.x, max.y, min.z), .color = *color}, {.pos = vec3_t_c(max.x, min.y, min.z), .color = *color},
////
////        {.pos = vec3_t_c(min.x, max.y, max.z), .color = *color}, {.pos = vec3_t_c(max.x, max.y, max.z), .color = *color},
////        {.pos = vec3_t_c(min.x, min.y, max.z), .color = *color}, {.pos = vec3_t_c(max.x, min.y, max.z), .color = *color},
////        {.pos = vec3_t_c(min.x, max.y, min.z), .color = *color}, {.pos = vec3_t_c(max.x, max.y, min.z), .color = *color},
////        {.pos = vec3_t_c(min.x, min.y, min.z), .color = *color}, {.pos = vec3_t_c(max.x, min.y, min.z), .color = *color},
//
////        {.pos = vec3_t_c(min.x, max.y, max.z), .color = *color}, {.pos = vec3_t_c(min.x, max.y, min.z), .color = *color},
////        {.pos = vec3_t_c(min.x, min.y, max.z), .color = *color}, {.pos = vec3_t_c(min.x, min.y, min.z), .color = *color},
////        {.pos = vec3_t_c(max.x, max.y, max.z), .color = *color}, {.pos = vec3_t_c(max.x, max.y, min.z), .color = *color},
////        {.pos = vec3_t_c(max.x, min.y, max.z), .color = *color}, {.pos = vec3_t_c(max.x, min.y, min.z), .color = *color},
//    };
//
//    uint32_t indices[] = {
//        0, 1,
//        1, 2,
//        2, 3,
//        3, 0,
//
//        4, 5,
//        5, 6,
//        6, 7,
//        7, 4,
//
//        0, 4,
//        1, 5,
//        2, 6,
//        3, 7
//    };
//
//    struct r_i_draw_list_t *draw_list = r_i_AllocDrawList(NULL, 1);
//    draw_list->mesh = r_i_AllocMesh(NULL, sizeof(struct r_vert_t), 8, 24);
//    memcpy(draw_list->mesh->verts.verts, verts, sizeof(verts));
//    memcpy(draw_list->mesh->indices.indices, indices, sizeof(indices));
//    draw_list->ranges[0].count = 24;
//    draw_list->ranges[0].start = 0;
//    draw_list->indexed = 1;
//    draw_list->mode = GL_LINES;
//
//    r_i_DrawList(NULL, draw_list);


//
//    uint32_t first_vert = R_IMMEDIATE_VERTEX_BUFFER_OFFSET / sizeof(struct r_vert_t);
//
//    r_CopyImmediateVerts(verts, sizeof(struct r_vert_t) * 24);
//    glDrawArrays(GL_LINES, first_vert, 24);

//    r_i_DrawLine(&vec3_t_c(min.x, max.y, max.z), &vec3_t_c(min.x, min.y, max.z), color, 1.0);
//    r_i_DrawLine(&vec3_t_c(max.x, max.y, max.z), &vec3_t_c(max.x, min.y, max.z), color, 1.0);
//    r_i_DrawLine(&vec3_t_c(min.x, max.y, min.z), &vec3_t_c(min.x, min.y, min.z), color, 1.0);
//    r_i_DrawLine(&vec3_t_c(max.x, max.y, min.z), &vec3_t_c(max.x, min.y, min.z), color, 1.0);
//
//    r_i_DrawLine(&vec3_t_c(min.x, max.y, max.z), &vec3_t_c(max.x, max.y, max.z), color, 1.0);
//    r_i_DrawLine(&vec3_t_c(min.x, min.y, max.z), &vec3_t_c(max.x, min.y, max.z), color, 1.0);
//    r_i_DrawLine(&vec3_t_c(min.x, max.y, min.z), &vec3_t_c(max.x, max.y, min.z), color, 1.0);
//    r_i_DrawLine(&vec3_t_c(min.x, min.y, min.z), &vec3_t_c(max.x, min.y, min.z), color, 1.0);
//
//    r_i_DrawLine(&vec3_t_c(min.x, max.y, max.z), &vec3_t_c(min.x, max.y, min.z), color, 1.0);
//    r_i_DrawLine(&vec3_t_c(min.x, min.y, max.z), &vec3_t_c(min.x, min.y, min.z), color, 1.0);
//    r_i_DrawLine(&vec3_t_c(max.x, max.y, max.z), &vec3_t_c(max.x, max.y, min.z), color, 1.0);
//    r_i_DrawLine(&vec3_t_c(max.x, min.y, max.z), &vec3_t_c(max.x, min.y, min.z), color, 1.0);
}

void r_DrawLine(vec3_t *start, vec3_t *end, vec4_t *color)
{
//    struct r_vert_t verts[2];
//    uint32_t vert_start = R_IMMEDIATE_VERTEX_BUFFER_OFFSET / sizeof(struct r_vert_t);
//
//    verts[0].pos = *start;
//    verts[0].color = *color;
//    verts[1].pos = *end;
//    verts[1].color = *color;
//
//    r_CopyImmediateVerts(verts, sizeof(struct r_vert_t) * 2);
//    glDrawArrays(GL_LINES, vert_start, 2);


}

void r_DrawPoint(vec3_t *pos, vec4_t *color)
{
//    struct r_vert_t vert;
//    uint32_t vert_start = R_IMMEDIATE_VERTEX_BUFFER_OFFSET / sizeof(struct r_vert_t);
//
//    vert.pos = *pos;
//    vert.color = *color;
//
//    r_CopyImmediateVerts(&vert, sizeof(struct r_vert_t));
//    glDrawArrays(GL_LINES, vert_start, 1);
}

void r_RunImmCmdBuffer(struct r_i_cmd_buffer_t *cmd_buffer)
{
    if(cmd_buffer && cmd_buffer->base.cmds.cursor)
    {
        for(uint32_t cmd_index = 0; cmd_index < cmd_buffer->base.cmds.cursor; cmd_index++)
        {
            struct r_i_cmd_t *cmd = ds_list_get_element(&cmd_buffer->base.cmds, cmd_index);

            switch(cmd->type)
            {
                case R_I_CMD_SET_SHADER:
                {
                    struct r_i_shader_t *shader = cmd->data;
                    r_BindShader(shader->shader);
                }
                break;

                case R_I_CMD_SET_UNIFORM:
                {
                    struct r_i_uniform_list_t *uniform_list = cmd->data;
                    r_i_ApplyUniforms(uniform_list);
                }
                break;

                case R_I_CMD_SET_DRAW_STATE:
                {
                    struct r_i_draw_state_t *draw_state = cmd->data;
                    r_i_ApplyDrawState(draw_state);
                }
                break;

                case R_I_CMD_CLEAR:
                {
                    struct r_i_clear_t *clear = cmd->data;

                    if(clear->bitmask & GL_COLOR_BUFFER_BIT)
                    {
                        glClearColor(clear->r, clear->g, clear->b, clear->a);
                    }

                    if(clear->bitmask & GL_DEPTH_BUFFER_BIT)
                    {
                        glClearDepth(clear->depth);
                    }

                    glClear(clear->bitmask);
                }
                break;

                case R_I_CMD_DRAW:
                {
                    struct r_i_draw_list_t *draw_list = cmd->data;

                    uint32_t index_offset = 0;
                    uint32_t start_offset = 0;

                    if(draw_list->mesh)
                    {
                        struct r_i_mesh_t *mesh = draw_list->mesh;

                        if(mesh->verts.count)
                        {
                            struct r_i_verts_t *verts = &mesh->verts;
                            glBufferSubData(GL_ARRAY_BUFFER, R_IMMEDIATE_VERTEX_BUFFER_OFFSET, verts->stride * verts->count, verts->verts);
                            index_offset = R_IMMEDIATE_VERTEX_BUFFER_OFFSET / verts->stride;
                        }

                        if(mesh->indices.count)
                        {
                            struct r_i_indices_t *indices = &mesh->indices;
                            glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, R_IMMEDIATE_INDEX_BUFFER_OFFSET, sizeof(uint32_t) * indices->count, indices->indices);
                            start_offset = R_IMMEDIATE_INDEX_BUFFER_OFFSET;
                        }
                    }

                    if(draw_list->indexed)
                    {
                        for(uint32_t range_index = 0; range_index < draw_list->range_count; range_index++)
                        {
                            struct r_i_draw_range_t *range = draw_list->ranges + range_index;

                            if(range->draw_state)
                            {
                                r_i_ApplyDrawState(range->draw_state);
                            }

                            if(range->uniforms)
                            {
                                r_i_ApplyUniforms(range->uniforms);
                            }

                            void *start = (void *)(sizeof(uint32_t) * range->start + start_offset);
                            glDrawElementsBaseVertex(draw_list->mode, range->count, GL_UNSIGNED_INT, start, index_offset);
                        }
                    }
                    else
                    {
                        for(uint32_t range_index = 0; range_index < draw_list->range_count; range_index++)
                        {
                            struct r_i_draw_range_t *range = draw_list->ranges + range_index;

                            if(range->draw_state)
                            {
                                r_i_ApplyDrawState(range->draw_state);
                            }

                            if(range->uniforms)
                            {
                                r_i_ApplyUniforms(range->uniforms);
                            }

                            glDrawArrays(draw_list->mode, range->start + index_offset, range->count);
                        }
                    }
                }
                break;
            }
        }
    }
}
//
//void r_BeginImmediateMode()
//{
//
//}
//
//void r_EndImmediateMode()
//{
//
//}

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

void r_DrawFrame()
{
    mat4_t model_view_projection_matrix;
    mat4_t model_view_matrix;

    if(r_point_light_buffer_cursor)
    {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, r_point_light_data_uniform_buffer);
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, r_point_light_buffer_cursor * sizeof(struct r_point_data_t), r_point_light_buffer);
    }

    if(r_spot_light_buffer_cursor)
    {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, r_spot_light_data_uniform_buffer);
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, r_spot_light_buffer_cursor * sizeof(struct r_spot_data_t), r_spot_light_buffer);
    }

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, r_light_index_uniform_buffer);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, R_MAX_CLUSTER_LIGHTS * R_CLUSTER_COUNT * sizeof(uint32_t), r_light_index_buffer);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, r_shadow_map_uniform_buffer);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, r_shadow_map_buffer_cursor * sizeof(struct r_shadow_map_t), r_shadow_map_buffer);

    glActiveTexture(GL_TEXTURE0 + R_CLUSTERS_TEX_UNIT);
    glBindTexture(GL_TEXTURE_3D, r_cluster_texture);
    glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, R_CLUSTER_ROW_WIDTH, R_CLUSTER_ROWS, R_CLUSTER_SLICES, GL_RGB_INTEGER, GL_UNSIGNED_INT, r_clusters);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, R_POINT_LIGHT_UNIFORM_BUFFER_BINDING, r_point_light_data_uniform_buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, R_SPOT_LIGHT_UNIFORM_BUFFER_BINDING, r_spot_light_data_uniform_buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, R_LIGHT_INDICES_UNIFORM_BUFFER_BINDING, r_light_index_uniform_buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, R_SHADOW_INDICES_BUFFER_BINDING, r_shadow_map_uniform_buffer);

    glActiveTexture(GL_TEXTURE0 + R_SHADOW_ATLAS_TEX_UNIT);
    glBindTexture(GL_TEXTURE_2D, r_shadow_atlas_texture);

    glActiveTexture(GL_TEXTURE0 + R_INDIRECT_TEX_UNIT);
    glBindTexture(GL_TEXTURE_CUBE_MAP, r_indirect_texture);

    r_denom = log(r_z_far / r_z_near);


    ds_list_qsort(&r_world_cmds, r_CompareWorldCmds);
    ds_list_qsort(&r_entity_cmd_buffer.cmds, r_CompareEntityCmds);
    ds_list_qsort(&r_shadow_cmd_buffer.cmds, r_CompareShadowCmds);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glDisable(GL_STENCIL_TEST);
    glDepthMask(GL_TRUE);
    glCullFace(GL_BACK);
    glDisable(GL_BLEND);
    glDepthFunc(GL_LESS);


    /********** shadow maps **********/

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, r_shadow_map_framebuffer);
    glEnable(GL_SCISSOR_TEST);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.0, 1.0);
    r_BindShader(r_shadow_shader);

    struct r_shadow_map_t *cur_shadow_map = NULL;
    for(uint32_t cmd_index = 0; cmd_index < r_shadow_cmd_buffer.cmds.cursor; cmd_index++)
    {
        struct r_shadow_cmd_t *cmd = ds_list_get_element(&r_shadow_cmd_buffer.cmds, cmd_index);
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
    glDisable(GL_SCISSOR_TEST);


    /********** z prepass **********/

    r_BindFramebuffer(r_main_framebuffer);
    glClearColor(r_clear_color.x, r_clear_color.y, r_clear_color.z, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glScissor(0, 0, r_width, r_height);
    glViewport(0, 0, r_width, r_height);

    if(r_renderer_state.use_z_prepass)
    {
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        r_BindShader(r_z_prepass_shader);
        glDepthFunc(GL_LESS);

        r_SetDefaultUniformMat4(R_UNIFORM_MODEL_VIEW_PROJECTION_MATRIX, &r_view_projection_matrix);
        for(uint32_t cmd_index = 0; cmd_index < r_world_cmds.cursor; cmd_index++)
        {
            struct r_world_cmd_t *cmd = ds_list_get_element(&r_world_cmds, cmd_index);
            glDrawElements(GL_TRIANGLES, cmd->count, GL_UNSIGNED_INT, (void *)(cmd->start * sizeof(uint32_t)));
            r_renderer_state.draw_call_count++;
        }

        for(uint32_t cmd_index = 0; cmd_index < r_entity_cmd_buffer.cmds.cursor; cmd_index++)
        {
            struct r_entity_cmd_t *cmd = ds_list_get_element(&r_entity_cmd_buffer.cmds, cmd_index);
            mat4_t_mul(&model_view_projection_matrix, &cmd->model_view_matrix, &r_projection_matrix);
            r_SetDefaultUniformMat4(R_UNIFORM_MODEL_VIEW_PROJECTION_MATRIX, &model_view_projection_matrix);
            glDrawElements(GL_TRIANGLES, cmd->count, GL_UNSIGNED_INT, (void *)(cmd->start * sizeof(uint32_t)));
            r_renderer_state.draw_call_count++;
        }

        glDepthFunc(GL_EQUAL);
        glDepthMask(GL_FALSE);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    }
    /****************************/
    /********** opaque **********/
    /****************************/

    r_BindShader(r_lit_shader);
    r_SetDefaultUniformI(R_UNIFORM_TEX_CLUSTERS, R_CLUSTERS_TEX_UNIT);
    r_SetDefaultUniformI(R_UNIFORM_TEX_SHADOW_ATLAS, R_SHADOW_ATLAS_TEX_UNIT);
    r_SetDefaultUniformI(R_UNIFORM_TEX_INDIRECT, R_INDIRECT_TEX_UNIT);
    r_SetDefaultUniformI(R_UNIFORM_MAX_PARALLAX_SAMPLES, r_max_parallax_samples);
    r_SetDefaultUniformMat4(R_UNIFORM_CAMERA_MATRIX, &r_camera_matrix);
    r_SetDefaultUniformVec2(R_UNIFORM_POINT_PROJ_PARAMS, &r_point_shadow_projection_params);
    r_SetDefaultUniformMat4(R_UNIFORM_MODEL_VIEW_PROJECTION_MATRIX, &r_view_projection_matrix);
    r_SetDefaultUniformMat4(R_UNIFORM_MODEL_VIEW_MATRIX, &r_view_matrix);

    /* world geometry */
    struct r_material_t *current_material = NULL;
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

    /* entity geometry */
    current_material = NULL;
    for(uint32_t cmd_index = 0; cmd_index < r_entity_cmd_buffer.cmds.cursor; cmd_index++)
    {
        struct r_entity_cmd_t *cmd = ds_list_get_element(&r_entity_cmd_buffer.cmds, cmd_index);

        if(cmd->material != current_material)
        {
            current_material = cmd->material;
            r_BindMaterial(current_material);
        }

//        if(cmd_index == 0)
//        {
//            glBeginQuery(GL_ANY_SAMPLES_PASSED, r_test_query);
//        }

        /* TODO: precompute this matrix */
        mat4_t_mul(&model_view_projection_matrix, &cmd->model_view_matrix, &r_projection_matrix);
        r_SetDefaultUniformMat4(R_UNIFORM_MODEL_VIEW_PROJECTION_MATRIX, &model_view_projection_matrix);
        r_SetDefaultUniformMat4(R_UNIFORM_MODEL_VIEW_MATRIX, &cmd->model_view_matrix);
        glDrawElements(GL_TRIANGLES, cmd->count, GL_UNSIGNED_INT, (void *)(cmd->start * sizeof(uint32_t)));
        r_renderer_state.draw_call_count++;

//        if(cmd_index == 0)
//        {
//            glEndQuery(GL_ANY_SAMPLES_PASSED);
//            int32_t result;
//            glGetQueryObjectiv(r_test_query, GL_QUERY_RESULT, &result);
//            if(!result)
//            {
//                printf("first object is occluded\n");
//            }
//            else
//            {
//                printf("first object is not occluded\n");
//            }
//        }
    }
//    glDepthMask(GL_TRUE);
//    glDepthFunc(GL_LESS);
//    r_BindShader(r_immediate_shader);
//    r_BindFramebuffer(r_main_framebuffer);
//    r_RunImmCmdBuffer(&r_immediate_cmd_buffer);

//    glDisable(GL_STENCIL_TEST);
    /***************************************/
    /********** volumetric lights **********/
    /***************************************/

    r_BindFramebuffer(r_volume_framebuffer);
    r_BindShader(r_volumetric_shader);
    r_BindTexture(r_main_framebuffer->depth_attachment, GL_TEXTURE0);
    r_SetDefaultUniformI(R_UNIFORM_TEX_SHADOW_ATLAS, R_SHADOW_ATLAS_TEX_UNIT);
    r_SetDefaultUniformMat4(R_UNIFORM_VIEW_PROJECTION_MATRIX, &r_view_projection_matrix);
    r_SetDefaultUniformMat4(R_UNIFORM_PROJECTION_MATRIX, &r_projection_matrix);
    r_SetDefaultUniformUI(R_UNIFORM_SPOT_LIGHT_COUNT, r_spot_light_buffer_cursor);
    r_SetDefaultUniformUI(R_UNIFORM_TEX0, 0);
    glDrawArrays(GL_TRIANGLES, r_screen_tri_start, 3);

    /**********************************************/
    /********** mix in volumetric lights **********/
    /**********************************************/

    r_BindFramebuffer(r_main_framebuffer);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_SCISSOR_TEST);
    r_BindShader(r_full_screen_blend_shader);
    r_BindTexture(r_volume_framebuffer->color_attachments[0], GL_TEXTURE0);
    r_BindTexture(r_main_framebuffer->depth_attachment, GL_TEXTURE1);
//    r_BindShader(r_bilateral_blend_shader);

    r_SetDefaultUniformUI(R_UNIFORM_TEX0, 0);
    r_SetDefaultUniformUI(R_UNIFORM_TEX1, 1);
    glDrawArrays(GL_TRIANGLES, r_screen_tri_start, 3);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

//    mat4_t view_projection_matrix;
//    mat4_t model_matrix;

//    mat4_t_identity(&view_projection_matrix);
//    mat4_t_identity(&model_matrix);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
    r_BindShader(r_immediate_shader);
    r_BindFramebuffer(r_main_framebuffer);
    r_RunImmCmdBuffer(&r_immediate_cmd_buffer);
}

#ifdef __cplusplus
}
#endif



