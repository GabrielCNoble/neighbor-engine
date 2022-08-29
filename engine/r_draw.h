#ifndef R_DRAW_H
#define R_DRAW_H

#include "r_defs.h"
#include "r_main.h"

#ifdef __cplusplus
extern "C"
{
#endif

void r_BeginFrame();

void r_EndFrame();

void r_SetClearColor(float r, float g, float b, float a);

void r_SetViewPos(vec3_t *pos);

void r_TranslateView(vec3_t *disp);

void r_SetViewPitchYaw(float pitch, float yaw);

void r_UpdateViewProjectionMatrix();

void r_DrawElements(uint32_t mode, uint32_t count, uint32_t type, const void *indices);

void r_DrawEntity(mat4_t *model_matrix, struct r_model_t *model);

void r_DrawShadow(mat4_t *model_view_projection_matrix, uint32_t shadow_map, uint32_t start, uint32_t count);

void r_DrawWorld(struct r_material_t *material, uint32_t start, uint32_t count);

void r_DrawVerts(struct r_vert_t *verts, uint32_t count, uint32_t mode);

void r_DrawBox(vec3_t *half_extents, vec4_t *color);

void r_DrawLine(vec3_t *start, vec3_t *end, vec4_t *color);

void r_DrawPoint(vec3_t *pos, vec4_t *color);

void r_RunImmCmdBuffer(struct r_i_cmd_buffer_t *cmd_buffer);

void r_DrawFrame();

#ifdef __cplusplus
}
#endif

#endif // R_DRAW_H
