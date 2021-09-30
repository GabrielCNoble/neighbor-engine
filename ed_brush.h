#ifndef ED_BRUSH_H
#define ED_BRUSH_H

#include "ed_world.h"

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

//struct ed_clipped_polygon_t
//{
//    struct ed_polygon_t *next;
//    struct ed_polygon_t *prev;
//    struct ed_brush_t *brush;
//    struct ds_buffer_t vertices;
//    struct r_material_t *material;
//    vec3_t normal;
//    uint32_t index;
//    uint32_t mesh_start;
//    uint32_t mesh_count;
//};

struct ed_edge_t
{
    struct
    {
        struct ed_edge_t *next;
        struct ed_edge_t *prev;
        struct ed_face_polygon_t *polygon;

    }polygons[2];

    uint32_t verts[2];

    struct ed_brush_t *brush;
    uint32_t index;

//    uint32_t vert0;
//    uint32_t vert1;

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
};

struct ed_face_t
{
    struct ed_face_t *next;
    struct ed_face_t *prev;
    struct ed_brush_t *brush;
    uint32_t index;

    struct r_material_t *material;
    struct ed_face_polygon_t *polygons;
//    struct ed_face_polygon_t *clipped_polygons;
    struct ed_bsp_polygon_t *clipped_polygons;

    uint32_t clipped_vert_count;
    uint32_t clipped_index_count;
    uint32_t clipped_polygon_count;

    vec2_t tex_coords_scale;
    float tex_coords_rot;
};

struct ed_brush_t
{
    mat3_t orientation;
    vec3_t position;
    uint32_t index;
    struct ds_slist_t polygons;
    struct ds_slist_t bsp_nodes;
    struct ed_face_t *faces;
    struct ds_slist_t vertices;
    struct r_model_t *model;



    uint32_t clipped_vert_count;
    uint32_t clipped_index_count;
    uint32_t clipped_polygon_count;
};


struct ed_brush_t *ed_CreateBrush(vec3_t *position, mat3_t *orientation, vec3_t *size);

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

void ed_UpdateBrush(struct ed_brush_t *brush);

void ed_TranslateBrushFace(struct ed_brush_t *brush, uint32_t face_index, vec3_t *translation);

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
