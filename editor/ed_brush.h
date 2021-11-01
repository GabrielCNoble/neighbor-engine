#ifndef ED_BRUSH_H
#define ED_BRUSH_H

#include "ed_brush_defs.h"

struct ed_brush_t *ed_AllocBrush();

struct ed_brush_t *ed_CreateBrush(vec3_t *position, mat3_t *orientation, vec3_t *size);

struct ed_brush_t *ed_CopyBrush(struct ed_brush_t *brush);

void ed_DestroyBrush(struct ed_brush_t *brush);

struct ed_brush_batch_t *ed_GetGlobalBrushBatch(struct r_material_t *material);

struct ed_brush_t *ed_GetBrush(uint32_t index);

struct ed_face_t *ed_AllocFace(struct ed_brush_t *brush);

struct ed_face_t *ed_GetFace(uint32_t index);

void ed_FreeFace(struct ed_brush_t *brush, struct ed_face_t *face);

struct ed_face_polygon_t *ed_AllocFacePolygon(struct ed_brush_t *brush);

void ed_FreeFacePolygon(struct ed_brush_t *brush, struct ed_face_polygon_t *polygon);

struct ed_edge_t *ed_AllocEdge(struct ed_brush_t *brush);

struct ed_edge_t *ed_GetEdge(uint32_t index);

void ed_FreeEdge(struct ed_brush_t *brush, struct ed_edge_t *edge);

struct ed_vert_t *ed_AllocVert(struct ed_brush_t *brush);

struct ed_vert_t *ed_GetVert(struct ed_brush_t *brush, uint32_t index);

void ed_FreeVert(struct ed_brush_t *brush, uint32_t index);
//
//uint32_t ed_AllocVertex(struct ed_brush_t *brush);
//
//vec3_t *ed_GetVertex(struct ed_brush_t *brush, uint32_t index);
//
//void ed_FreeVertex(struct ed_brush_t *brush, uint32_t index);

struct ed_vert_transform_t *ed_FindVertTransform(struct ed_brush_t *brush, uint32_t vert_index);

void ed_ExtrudeBrushFace(struct ed_brush_t *brush, uint32_t face_index);

void ed_DeleteBrushFace(struct ed_brush_t *brush, uint32_t face_index);

void ed_SetFaceMaterial(struct ed_brush_t *brush, uint32_t face_index, struct r_material_t *material);

void ed_TranslateBrushFace(struct ed_brush_t *brush, uint32_t face_index, vec3_t *translation);

void ed_RotateBrushFace(struct ed_brush_t *brush, uint32_t face_index, mat3_t *rotation);

void ed_UpdateBrush(struct ed_brush_t *brush);

void ed_BuildWorldGeometry();

/*
=============================================================
=============================================================
=============================================================
*/

struct ed_bsp_node_t *ed_AllocBspNode();

void ed_FreeBspNode(struct ed_bsp_node_t *node);

struct ed_bsp_polygon_t *ed_BspPolygonFromBrushFace(struct ed_face_t *face);

struct ed_bsp_polygon_t *ed_AllocBspPolygon(uint32_t vert_count);

void ed_FreeBspPolygon(struct ed_bsp_polygon_t *polygon, uint32_t free_verts);

void ed_UnlinkPolygon(struct ed_bsp_polygon_t *polygon, struct ed_bsp_polygon_t **first_polygon);

uint32_t ed_PolygonOnSplitter(struct ed_bsp_polygon_t *polygon, vec3_t *point, vec3_t *normal);

struct ed_bsp_polygon_t *ed_BestSplitter(struct ed_bsp_polygon_t *polygons);

struct ed_bsp_node_t *ed_BrushBspFromPolygons(struct ed_bsp_polygon_t *polygons);




#endif // ED_BRUSH_H