#ifndef R_DRAW_I_H
#define R_DRAW_I_H

#include "r_main.h"

//void *r_i_AllocImmediateData(uint32_t size);
//
//void *r_i_AllocImmediateExternData(uint32_t size);
//
//struct r_i_draw_list_t *r_i_AllocDrawList(uint32_t cmd_count);
//
//struct r_i_verts_t *r_i_AllocVerts(uint32_t vert_count);
//
//void r_i_ImmediateCmd(uint16_t type, uint16_t sub_type, void *data);
//
//struct r_i_state_t *r_i_GetCurrentState();
//
//void r_i_SetCurrentState();
//
//void r_i_SetShader(struct r_shader_t *shader);
//
//void r_i_SetUniform(struct r_named_uniform_t *uniform, uint32_t count, void *value);
//
//void r_i_SetBlending(uint16_t enable, uint16_t src_factor, uint16_t dst_factor);
//
//void r_i_SetDepth(uint16_t enable, uint16_t func);
//
//void r_i_SetCullFace(uint16_t enable, uint16_t cull_face);
//
//void r_i_SetRasterizer(uint16_t cull_face_enable, uint16_t cull_face, uint16_t polygon_mode);
//
//void r_i_SetStencil(uint16_t enable, uint16_t sfail, uint16_t dfail, uint16_t dpass, uint16_t func, uint8_t mask, uint8_t ref);
//
//void r_i_SetScissor(uint16_t enable, uint16_t x, uint16_t y, uint16_t width, uint16_t height);
//
//void r_i_SetDrawMask(uint16_t red, uint16_t green, uint16_t blue, uint16_t alpha, uint16_t depth, uint16_t stencil);
//
//void r_i_SetTextures(uint32_t texture_count, struct r_i_texture_t *textures);
//
//void r_i_SetTexture(struct r_texture_t *texture, uint32_t tex_unit);
//
//void r_i_SetBuffers(struct r_i_verts_t *verts, struct r_i_indices_t *indices);
//
//void r_i_SetModelMatrix(mat4_t *model_matrix);
//
//void r_i_SetViewProjectionMatrix(mat4_t *view_projection_matrix);
//
//void r_i_DrawImmediate(uint16_t sub_type, struct r_i_draw_list_t *list);
//
//void r_i_DrawVerts(uint16_t sub_type, struct r_i_verts_t *verts, float size);
//
//void r_i_DrawVertsIndexed(uint16_t sub_type, struct r_i_verts_t *verts, struct r_i_indices_t *indices, float size);
//
//void r_i_DrawPoint(vec3_t *position, vec4_t *color, float size);
//
//void r_i_DrawLine(vec3_t *start, vec3_t *end, vec4_t *color, float width);
//
//void r_i_DrawBox(vec3_t *half_extents, vec4_t *color);
//
//void r_i_DrawCylinder(float radius, float height);
//
//void r_i_DrawCapsule(float radius, float height);

//void r_i_DrawTriangles()

//void r_i_DrawLineStrip()

//void r_CopyImmediateVerts(void *verts, uint32_t size);

//uint32_t r_AppendImmediateVerts(void *verts, uint32_t size);

//void r_ResetImmediateVertsBuffer();

//void r_CopyImmediateIndices(void *indices, uint32_t size);

struct r_i_draw_state_t *r_i_DrawState(struct r_i_cmd_buffer_t *cmd_buffer, struct r_i_draw_range_t *range);

void r_i_ApplyDrawState(struct r_i_draw_state_t *draw_state);

void r_i_ApplyUniforms(struct r_i_uniform_list_t *uniform_list);

void r_i_SetShader(struct r_i_cmd_buffer_t *cmd_buffer, struct r_shader_t *shader);

void r_i_SetUniforms(struct r_i_cmd_buffer_t *cmd_buffer, struct r_i_draw_range_t *range, struct r_i_uniform_t *uniforms, uint32_t count);

void r_i_SetBlending(struct r_i_cmd_buffer_t *cmd_buffer, struct r_i_draw_range_t *range, struct r_i_blending_t *blending);

void r_i_SetDepth(struct r_i_cmd_buffer_t *cmd_buffer, struct r_i_draw_range_t *range, struct r_i_depth_t *depth);

void r_i_SetStencil(struct r_i_cmd_buffer_t *cmd_buffer, struct r_i_draw_range_t *range, struct r_i_stencil_t *stencil);

void r_i_SetRasterizer(struct r_i_cmd_buffer_t *cmd_buffer, struct r_i_draw_range_t *range, struct r_i_raster_t *rasterizer);

void r_i_SetDrawMask(struct r_i_cmd_buffer_t *cmd_buffer, struct r_i_draw_range_t *range, struct r_i_draw_mask_t *draw_mask);

void r_i_SetScissor(struct r_i_cmd_buffer_t *cmd_buffer, struct r_i_draw_range_t *range, struct r_i_scissor_t *scissor);

void r_i_SetFramebuffer(struct r_i_cmd_buffer_t *cmd_buffer, struct r_i_draw_range_t *range, struct r_i_framebuffer_t *framebuffer);

void r_i_Clear(struct r_i_cmd_buffer_t *cmd_buffer, struct r_i_draw_range_t *range, struct r_i_clear_t *clear);

void r_i_DrawList(struct r_i_cmd_buffer_t *cmd_buffer, struct r_i_draw_list_t *draw_list);

struct r_i_draw_list_t *r_i_AllocDrawList(struct r_i_cmd_buffer_t *cmd_buffer, uint32_t range_count);

struct r_i_mesh_t *r_i_AllocMesh(struct r_i_cmd_buffer_t *cmd_buffer, uint32_t vert_size, uint32_t vert_count, uint32_t index_count);

//struct r_i_indices_t *r_i_AllocIndices(struct r_i_cmd_buffer_t *cmd_buffer, uint32_t index_count);
//
//struct r_i_verts_t *r_i_AllocVerts(struct r_i_cmd_buffer_t *cmd_buffer, uint32_t vert_count, uint32_t vert_size);

//void r_i_SetTexture(struct r_i_cmd_buffer_t *cmd_buffer, )
//    struct r_i_texture_t *              textures;
//    uint32_t                            texture_count;






void r_i_DrawVerts(struct r_i_cmd_buffer_t *cmd_buffer, struct r_vert_t *verts, uint32_t vert_count, uint32_t *indices, uint32_t index_count, uint32_t mode);

void r_i_DrawBox(vec3_t *half_extents, vec4_t *color);

void r_i_DrawLine(struct r_i_cmd_buffer_t *cmd_buffer, vec3_t *start, vec3_t *end, vec4_t *color);

void r_i_DrawPoint(vec3_t *pos, vec4_t *color);

//void r_FlushState();

#endif // R_DRAW_I_H
