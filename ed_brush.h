#ifndef ED_BRUSH_H
#define ED_BRUSH_H

#include "ed_com.h"


struct ed_brush_t *ed_CreateBrush(vec3_t *position, mat3_t *orientation, vec3_t *size);

void ed_DestroyBrush(struct ed_brush_t *brush);

struct ed_brush_batch_t *ed_GetGlobalBrushBatch(struct r_material_t *material);

struct ed_brush_t *ed_GetBrush(uint32_t index);

struct ed_polygon_t *ed_AllocPolygon(struct ed_brush_t *brush);

struct ed_polygon_t *ed_CopyPolygon(struct ed_polygon_t *src);

void ed_FreePolygon(struct ed_polygon_t *polygon);

struct ed_bspn_t *ed_AllocBspNode(struct ed_brush_t *brush);

void ed_FreeBspNode(struct ed_bspn_t *node);

void ed_UpdateBrush(struct ed_brush_t *brush);

void ed_GeometryFromPolygons(struct r_model_geometry_t *geometry, struct ed_polygon_t *polygons);

void ed_BuildWorldGeometry();




#endif // ED_BRUSH_H
