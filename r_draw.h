#ifndef R_DRAW_H
#define R_DRAW_H

#include "r_com.h"
#include "r_main.h"


void r_BeginFrame();

void r_EndFrame();

void r_SetViewPos(vec3_t *pos);

void r_TranslateView(vec3_t *disp);

void r_SetViewPitchYaw(float pitch, float yaw);

void r_UpdateViewProjectionMatrix();

void r_DrawElements(uint32_t mode, uint32_t count, uint32_t type, const void *indices);

void r_DrawEntity(mat4_t *model_matrix, struct r_model_t *model);

void r_DrawShadow(mat4_t *model_view_projection_matrix, uint32_t shadow_map, uint32_t start, uint32_t count);

void r_DrawWorld(struct r_material_t *material, uint32_t start, uint32_t count);

void r_DrawCmds();

void *r_i_AllocImmediateData(uint32_t size);

void *r_i_AllocImmediateExternData(uint32_t size);

void r_i_ImmediateCmd(uint16_t type, uint16_t sub_type, void *data);

struct r_i_state_t *r_i_GetCurrentState();

void r_i_SetCurrentState();

void r_i_SetShader(struct r_shader_t *shader);

void r_i_SetBlending(uint16_t enable, uint16_t src_factor, uint16_t dst_factor);

void r_i_SetDepth(uint16_t enable, uint16_t func);

void r_i_SetCullFace(uint16_t enable, uint16_t cull_face);

void r_i_SetStencil(uint16_t enable, uint16_t sfail, uint16_t dfail, uint16_t dpass, uint16_t op, uint8_t mask, uint8_t ref);

void r_i_SetScissor(uint16_t enable, uint16_t x, uint16_t y, uint16_t width, uint16_t height);

void r_i_SetTextures(uint32_t texture_count, struct r_i_texture_t *textures);

void r_i_SetTexture(struct r_texture_t *texture, uint32_t tex_unit);

void r_i_SetBuffers(struct r_i_verts_t *verts, struct r_i_indices_t *indices);

void r_i_SetModelMatrix(mat4_t *model_matrix);

void r_i_SetViewProjectionMatrix(mat4_t *view_projection_matrix);

void r_i_DrawImmediate(uint16_t sub_type, struct r_i_draw_list_t *list);

void r_i_DrawVerts(uint16_t sub_type, struct r_i_verts_t *verts);

void r_i_DrawVertsIndexed(uint16_t sub_type, struct r_i_verts_t *verts, struct r_i_indices_t *indices);

void r_i_DrawPoint(vec3_t *position, vec4_t *color, float size);

void r_i_DrawLine(vec3_t *start, vec3_t *end, vec4_t *color, float width);

//void r_i_DrawTriangles()

//void r_i_DrawLineStrip()


//void r_SortBatches();
//
//void r_DrawSortedBatches();
//
//void r_DrawImmediateBatches();
//
//struct r_imm_batch_t *r_i_BeginBatch();
//
//struct r_imm_batch_t *r_i_GetCurrentBatch();
//
//void r_i_SetTransform(mat4_t *transform);
//
//void r_i_SetPolygonMode(uint16_t polygon_mode);
//
//void r_i_SetSize(float size);
//
//void r_i_SetPrimitiveType(uint32_t primitive_type);
//
//void r_i_DrawImmediate(struct r_vert_t *verts, uint32_t count);
//
//void r_i_DrawPoint(vec3_t *position, vec3_t *color, float radius);
//
//void r_i_DrawLine(vec3_t *from, vec3_t *to, vec3_t *color, float width);
//
//void r_i_DrawVerts(struct r_vert_t *verts, uint32_t count);

//void r_DrawLine(vec3_t *from, vec3_t *to, vec3_t *color);

//void r_DrawLines(struct r_vert_t *verts, uint32_t count);

#endif // R_DRAW_H
