#ifndef ED_BRUSH_H
#define ED_BRUSH_H

#include "ed_com.h"


struct ed_brush_t *ed_CreateBrush(vec3_t *position, mat3_t *orientation, vec3_t *size);

void ed_DestroyBrush(struct ed_brush_t *brush);

struct ed_brush_batch_t *ed_GetGlobalBrushBatch(struct r_material_t *material);

struct ed_brush_t *ed_GetBrush(uint32_t index);

void ed_UpdateBrush(struct ed_brush_t *brush);

void ed_BuildWorldGeometry();




#endif // ED_BRUSH_H
