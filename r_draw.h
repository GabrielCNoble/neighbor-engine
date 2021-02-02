#ifndef R_DRAW_H
#define R_DRAW_H

#include "r_com.h"
#include "r_main.h"


void r_BeginFrame();

void r_EndFrame();

void r_SetViewPos(vec3_t *pos);

void r_TranslateView(vec3_t *disp);

void r_SetViewPitchYaw(float pitch, float yaw);

//void r_SetViewYaw(float yaw);

void r_UpdateViewProjectionMatrix();

void r_DrawModel(mat4_t *transform, struct r_model_t *model);

void r_DrawRange(mat4_t *transform, struct r_material_t *material, uint32_t start, uint32_t count);

void r_SortBatches();

void r_DrawSortedBatches();

void r_DrawImmediateBatches();

struct r_imm_batch_t *r_i_BeginBatch();

struct r_imm_batch_t *r_i_GetCurrentBatch();

void r_i_SetTransform(mat4_t *transform);

void r_i_SetPolygonMode(uint16_t polygon_mode);

void r_i_SetSize(float size);

void r_i_SetPrimitiveType(uint32_t primitive_type);

void r_i_DrawImmediate(struct r_vert_t *verts, uint32_t count);

void r_i_DrawPoint(vec3_t *position, vec3_t *color, float radius);

void r_i_DrawLine(vec3_t *from, vec3_t *to, vec3_t *color, float width);

void r_i_DrawVerts(struct r_vert_t *verts, uint32_t count);

//void r_DrawLine(vec3_t *from, vec3_t *to, vec3_t *color);

//void r_DrawLines(struct r_vert_t *verts, uint32_t count);

#endif // R_DRAW_H
