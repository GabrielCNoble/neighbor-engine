#ifndef ED_BRUSH_H
#define ED_BRUSH_H

#include "ed_lev_editor.h"

struct ed_bsp_polygon_t
{
    struct ed_bsp_polygon_t *next;
    struct ed_bsp_polygon_t *prev;
    struct ed_face_polygon_t *face_polygon;
    struct ds_list_t vertices;
    uint32_t model_start;
    uint32_t model_count;
    uint32_t index;
    vec3_t normal;
    vec3_t point;
};

struct ed_bsp_node_t
{
    struct ed_bsp_node_t *front;
    struct ed_bsp_node_t *back;
    struct ed_bsp_polygon_t *splitter;
    uint32_t index;
};

enum ED_SPLITTER_SIDES
{
    ED_SPLITTER_SIDE_FRONT = 0,
    ED_SPLITTER_SIDE_BACK = 1,
    ED_SPLITTER_SIDE_STRADDLE = 2,
    ED_SPLITTER_SIDE_ON_FRONT = 3,
    ED_SPLITTER_SIDE_ON_BACK = 4,
};



struct ed_brush_batch_t
{
    struct r_batch_t batch;
    uint32_t index;
};

struct ed_edge_t
{
    uint32_t index;
    struct ed_edge_t *init_next;
    struct ed_pickable_t *pickable;

    struct
    {
        struct ed_edge_t *next;
        struct ed_edge_t *prev;
        struct ed_face_polygon_t *polygon;

    }polygons[2];

    uint32_t verts[2];

    struct ed_brush_t *brush;
    uint32_t model_start;
};

struct ed_face_polygon_t
{
    struct ed_edge_t *edges;
    struct ed_edge_t *last_edge;
    struct ed_face_polygon_t *next;
    struct ed_face_polygon_t *prev;
    struct ed_face_t *face;
    struct ed_brush_t *brush;
    uint32_t index;

    uint32_t edge_count;

    vec3_t tangent;
    vec3_t normal;
    vec3_t center;
};

enum ED_FACE_FLAGS
{
    ED_FACE_FLAG_GEOMETRY_MODIFIED = 1,
    ED_FACE_FLAG_MATERIAL_MODIFIED = 1 << 1,
};

struct ed_face_t
{
    struct ed_face_t *next;
    struct ed_face_t *prev;
    struct ed_brush_t *brush;
    uint32_t index;

    struct ed_pickable_t *pickable;

    struct r_material_t *material;
    struct ed_face_polygon_t *polygons;
    struct ed_bsp_polygon_t *clipped_polygons;

    uint32_t clipped_vert_count;
    uint32_t clipped_index_count;
    uint32_t clipped_polygon_count;

    uint32_t flags;

    vec3_t center;
    vec2_t tex_coords_scale;
    float tex_coords_rot;
};

struct ed_vert_transform_t
{
    vec3_t translation;
    uint32_t index;
};

enum ED_BRUSH_FLAGS
{
    ED_BRUSH_FLAG_GEOMETRY_MODIFIED = 1,
    ED_BRUSH_FLAG_MATERIAL_MODIFIED = 1 << 1,
};

struct ed_brush_t
{
    struct ed_brush_t *next;
    struct ed_brush_t *prev;
    struct ed_brush_t *last;

    struct ed_brush_t *main_brush;
    struct ed_pickable_t *pickable;

    mat3_t orientation;
    vec3_t position;
    uint32_t index;
    struct ed_face_t *faces;
    struct ds_slist_t vertices;
    struct ds_list_t vert_transforms;
    struct r_model_t *model;
    struct g_entity_t *entity;

    uint32_t clipped_vert_count;
    uint32_t clipped_index_count;
    uint32_t clipped_polygon_count;
    uint32_t flags;

    uint32_t modified_index;
};

struct ed_brush_t *ed_AllocBrush();

struct ed_brush_t *ed_CreateBrush(vec3_t *position, mat3_t *orientation, vec3_t *size);

struct ed_brush_t *ed_CopyBrush(struct ed_brush_t *brush);

void ed_DestroyBrush(struct ed_brush_t *brush);

struct ed_brush_batch_t *ed_GetGlobalBrushBatch(struct r_material_t *material);

struct ed_brush_t *ed_GetBrush(uint32_t index);

struct ed_face_t *ed_AllocFace();

struct ed_face_t *ed_GetFace(uint32_t index);

void ed_FreeFace(struct ed_face_t *face);

struct ed_face_polygon_t *ed_AllocFacePolygon();

void ed_FreeFacePolygon(struct ed_face_polygon_t *polygon);

struct ed_edge_t *ed_AllocEdge();

struct ed_edge_t *ed_GetEdge(uint32_t index);

void ed_FreeEdge(struct ed_edge_t *edge);

uint32_t ed_AllocVertex(struct ed_brush_t *brush);

vec3_t *ed_GetVertex(struct ed_brush_t *brush, uint32_t index);

void ed_FreeVertex(struct ed_brush_t *brush, uint32_t index);

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
