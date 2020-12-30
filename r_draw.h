#ifndef R_DRAW_H
#define R_DRAW_H

#include "r_com.h"
#include "r_main.h"


void r_BeginFrame();

void r_EndFrame();

void r_SetViewPos(vec3_t *pos);

void r_TranslateView(vec3_t *disp);

void r_SetViewPitch(float pitch);

void r_SetViewYaw(float yaw);

void r_UpdateViewProjectionMatrix();

void r_DrawModel(mat4_t *transform, struct r_model_t *model);

void r_DrawRange(mat4_t *transform, struct r_material_t *material, uint32_t start, uint32_t count);

void r_DrawBatches();

void r_DrawImmediateBatches();

void r_DrawImmediate(struct r_vert_t *verts, uint32_t count, uint32_t mode, float size);

void r_DrawPoint(vec3_t *position, vec3_t *color, float radius);

void r_DrawLine(vec3_t *from, vec3_t *to, vec3_t *color);

void r_DrawLines(struct r_vert_t *verts, uint32_t count);

#endif // R_DRAW_H
