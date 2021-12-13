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

struct ed_face_polygon_t *ed_AllocFacePolygon(struct ed_brush_t *brush, struct ed_face_t *face);

void ed_FreeFacePolygon(struct ed_brush_t *brush, struct ed_face_polygon_t *polygon);

void ed_LinkFacePolygonEdge(struct ed_face_polygon_t *polygon, struct ed_edge_t *edge);

void ed_UnlinkFacePolygonEdge(struct ed_face_polygon_t *polygon, struct ed_edge_t *edge);

void ed_RemoveFacePolygon(struct ed_brush_t *brush, struct ed_face_polygon_t *polygon);

struct ed_edge_t *ed_AllocEdge(struct ed_brush_t *brush);

struct ed_edge_t *ed_GetEdge(uint32_t index);

void ed_FreeEdge(struct ed_brush_t *brush, struct ed_edge_t *edge);

struct ed_vert_t *ed_AllocVert(struct ed_brush_t *brush);

struct ed_vert_t *ed_GetVert(struct ed_brush_t *brush, uint32_t index);

void ed_LinkVertEdge(struct ed_vert_t *vert, struct ed_edge_t *edge);

void ed_FreeVert(struct ed_brush_t *brush, uint32_t index);
//
//uint32_t ed_AllocVertex(struct ed_brush_t *brush);
//
//vec3_t *ed_GetVertex(struct ed_brush_t *brush, uint32_t index);
//
//void ed_FreeVertex(struct ed_brush_t *brush, uint32_t index);

struct ed_vert_transform_t *ed_FindVertTransform(struct ed_brush_t *brush, uint32_t vert_index);

int ed_CompareBspPolygons(const void *a, const void *b);

void ed_ExtrudeBrushFace(struct ed_brush_t *brush, uint32_t face_index);

void ed_DeleteBrushFace(struct ed_brush_t *brush, uint32_t face_index);

void ed_SetFaceMaterial(struct ed_brush_t *brush, uint32_t face_index, struct r_material_t *material);

void ed_TranslateBrushFace(struct ed_brush_t *brush, uint32_t face_index, vec3_t *translation);

void ed_TranslateBrushEdge(struct ed_brush_t *brush, uint32_t edge_index, vec3_t *translation);

void ed_RotateBrushFace(struct ed_brush_t *brush, uint32_t face_index, mat3_t *rotation);

void ed_UpdateBrushEntity(struct ed_brush_t *brush);

void ed_UpdateBrush(struct ed_brush_t *brush);

//void ed_BuildWorldGeometry();

/*
=============================================================
=============================================================
=============================================================
*/

struct ed_bsp_node_t *ed_AllocBspNode();

void ed_FreeBspNode(struct ed_bsp_node_t *node);

void ed_FreeBspTree(struct ed_bsp_node_t *bsp);

struct ed_bsp_polygon_t *ed_BspPolygonsFromBrush(struct ed_brush_t *brush);

struct ed_bsp_polygon_t *ed_BspPolygonFromBrushFace(struct ed_face_t *face);

struct ed_bsp_polygon_t *ed_AllocBspPolygon(uint32_t vert_count);

struct ed_bsp_polygon_t *ed_CopyBspPolygons(struct ed_bsp_polygon_t *src_polygons);

void ed_FreeBspPolygon(struct ed_bsp_polygon_t *polygon, uint32_t free_verts);

void ed_FreeBspPolygons(struct ed_bsp_polygon_t *polygons);

void ed_UnlinkPolygon(struct ed_bsp_polygon_t *polygon, struct ed_bsp_polygon_t **first_polygon);

uint32_t ed_PolygonOnSplitter(struct ed_bsp_polygon_t *polygon, vec3_t *point, vec3_t *normal);

uint32_t ed_PointOnSplitter(vec3_t *point, vec3_t *plane_point, vec3_t *plane_normal);

struct ed_bsp_polygon_t *ed_BestSplitter(struct ed_bsp_polygon_t *polygons);

void ed_SplitPolygon(struct ed_bsp_polygon_t *polygon, vec3_t *point, vec3_t *normal, struct ed_bsp_polygon_t **front, struct ed_bsp_polygon_t **back);

struct ed_bsp_node_t *ed_SolidBspFromPolygons(struct ed_bsp_polygon_t *polygons);

struct ed_bsp_node_t *ed_LeafBspFromPolygons(struct ed_bsp_polygon_t *polygons);

struct ed_bsp_polygon_t *ed_ClipPolygonToBsp(struct ed_bsp_polygon_t *polygons, struct ed_bsp_node_t *bsp);

struct ed_bsp_polygon_t *ed_ClipPolygonLists(struct ed_bsp_polygon_t *polygons_a, struct ed_bsp_polygon_t *polygons_b);




#endif // ED_BRUSH_H
