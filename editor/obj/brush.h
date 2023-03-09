#ifndef ED_BRUSH_H
#define ED_BRUSH_H

#include "../engine/e_defs.h"
#include "../ed_defs.h"
#include "obj.h"

struct ed_brush_material_t
{
    struct r_batch_t batch;
    uint32_t index;
};

struct ed_face_edge_t
{
    struct ed_face_edge_t *     next;
    struct ed_face_edge_t *     prev;
    struct ed_edge_t *          edge;
    struct ed_face_t *          face;
};

struct ed_vert_edge_t
{
    struct ed_vert_edge_t *     next;
    struct ed_vert_edge_t *     prev;
    struct ed_edge_t *          edge;
    struct ed_vert_t *          vert;
};

struct ed_vert_t
{
    vec3_t                      vert;
    uint32_t                    index;
    uint32_t                    s_index;
    struct ed_vert_edge_t *     edges;
    struct ed_vert_edge_t *     last_edge;
};

struct ed_edge_t
{
    uint32_t                        index;
    uint32_t                        s_index;
    struct ed_edge_t *              brush_next;
    struct ed_edge_t *              brush_prev;
    struct ed_obj_t *               object;

    struct ed_face_edge_t           faces[2];
    struct ed_vert_edge_t           verts[2];
    struct ed_brush_t *             brush;
    uint32_t                        model_start_flag;
    uint32_t                        model_start;
};
//
//struct ed_face_polygon_t
//{
//    struct ed_edge_t *edges;
//    struct ed_edge_t *last_edge;
//    struct ed_face_polygon_t *next;
//    struct ed_face_polygon_t *prev;
//    struct ed_face_t *face;
//    struct ed_brush_t *brush;
//    uint32_t index;
//    uint32_t s_index;
//
//    uint32_t edge_count;
//
//    vec3_t tangent;
//    vec3_t normal;
//    vec3_t center;
//    vec2_t center_uv;
//};

enum ED_FACE_FLAGS
{
    ED_FACE_FLAG_GEOMETRY_MODIFIED = 1,
    ED_FACE_FLAG_MATERIAL_MODIFIED = 1 << 1,
};

struct ed_face_t
{
    struct ed_face_t *              next;
    struct ed_face_t *              prev;
    struct ed_brush_t *             brush;
    uint32_t                        index;

    struct ed_obj_t *               object;
    struct ed_face_edge_t *         edges;
    struct ed_face_edge_t *         last_edge;
    struct ed_brush_material_t *    material;
//    struct r_material_t *       material;

    uint32_t                        edge_count;
    mat3_t                          orientation;
    vec3_t                          normal;
    vec3_t                          tangent;
    vec3_t                          center;
    vec3_t                          center_uv;

    uint32_t                        first_index;
    uint32_t                        index_count;

//    struct ed_brush_batch_t *material;
//    struct ed_face_polygon_t *polygons;
//    struct ed_face_polygon_t *last_polygon;
//    struct ed_bsp_polygon_t *clipped_polygons;

//    uint32_t clipped_vert_count;
//    uint32_t clipped_index_count;
//    uint32_t clipped_polygon_count;

    uint32_t                        flags;

//    vec3_t center;
    vec2_t                          tex_coords_scale;
    float                           tex_coords_rot;
};

struct ed_vert_transform_t
{
    vec3_t translation;
    vec3_t rotation;
    uint32_t index;
};

enum ED_BRUSH_FLAGS
{
    ED_BRUSH_FLAG_GEOMETRY_MODIFIED = 1,
    ED_BRUSH_FLAG_MATERIAL_MODIFIED = 1 << 1,
};

enum ED_BRUSH_UPDATE_FLAGS
{
    ED_BRUSH_UPDATE_FLAG_TRANSFORM_VERTS = 1,
    ED_BRUSH_UPDATE_FLAG_FACE_POLYGONS = 1 << 1,
//    ED_BRUSH_UPDATE_FLAG_CLIPPED_POLYGONS = 1 << 2,
    ED_BRUSH_UPDATE_FLAG_UV_COORDS = 1 << 2,
    ED_BRUSH_UPDATE_FLAG_MODEL = 1 << 3,
};

enum ED_BRUSH_ELEMENTS
{
    ED_BRUSH_ELEMENT_BODY = 0,
    ED_BRUSH_ELEMENT_FACE = 1,
    ED_BRUSH_ELEMENT_EDGE = 2,
    ED_BRUSH_ELEMENT_VERT = 3,
};

struct ed_brush_t
{
    struct ed_brush_t *     next;
    struct ed_brush_t *     prev;
    struct ed_brush_t *     last;

//    struct ed_brush_t *main_brush;
    struct ed_obj_t *       object;

    mat3_t                  orientation;
    vec3_t                  position;
    uint32_t                index;

    struct ds_slist_t       vertices;
    struct ds_list_t        vert_transforms;
    struct r_model_t *      model;
    struct e_entity_t *     entity;
    uint32_t                entity_index;

//    uint32_t                vert_count;

    struct ed_face_t *      faces;
    struct ed_face_t *      last_face;
    uint32_t                face_count;
//    uint32_t                polygon_count;


    struct ed_edge_t *      edges;
    struct ed_edge_t *      last_edge;
    uint32_t                edge_count;

//    uint32_t clipped_vert_count;
//    uint32_t clipped_index_count;
//    uint32_t clipped_polygon_count;
    uint32_t                flags;
    uint32_t                update_flags;

//    uint32_t modified_index;
};

struct ed_vert_record_t
{
    vec3_t vert;
    /* used by the deserializer to map between a serialized vertex
    and the allocated vertex */
    uint32_t d_index;
};

struct ed_edge_record_t
{
    uint64_t polygons[2];
    uint64_t vertices[2];
    /* used by the deserializer to map between a serialized edge
    and the allocated edge */
    uint32_t d_index;
};

//struct ed_polygon_edge_record_t
//{
//    size_t edge;
//};

struct ed_polygon_record_t
{
    uint64_t edge_count;
    uint64_t edges[];
};

struct ed_face_record_t
{
    uint64_t polygon_start;
    uint64_t polygon_count;

    vec2_t uv_scale;
    float uv_rot;

    char material[32];
};

struct ed_brush_record_t
{
    vec3_t position;
    mat3_t orientation;

    uint64_t record_size;

    uint64_t face_start;
    uint64_t face_count;

    uint64_t edge_start;
    uint64_t edge_count;

    uint64_t vert_start;
    uint64_t vert_count;

    uint32_t uuid;
};

struct ed_brush_section_t
{
    uint64_t brush_record_start;
    uint64_t brush_record_count;

    size_t reserved[32];
};

struct ed_brush_obj_args_t
{
    mat3_t orientation;
    vec3_t position;
    vec3_t size;
};

void ed_InitBrushObjectFuncs();

struct ed_brush_t *ed_AllocBrush();

struct ed_brush_t *ed_CreateBrush(vec3_t *position, mat3_t *orientation, vec3_t *size);

struct ed_brush_t *ed_CopyBrush(struct ed_brush_t *brush);

void ed_DestroyBrush(struct ed_brush_t *brush);

struct ed_brush_material_t *ed_GetBrushMaterial(struct r_material_t *material);

struct ed_brush_t *ed_GetBrush(uint32_t index);

struct ed_face_t *ed_AllocFace(struct ed_brush_t *brush);

struct ed_face_t *ed_GetFace(uint32_t index);

void ed_FreeFace(struct ed_brush_t *brush, struct ed_face_t *face);

//struct ed_face_polygon_t *ed_AllocFacePolygon(struct ed_brush_t *brush, struct ed_face_t *face);

//void ed_FreeFacePolygon(struct ed_brush_t *brush, struct ed_face_polygon_t *polygon);

void ed_LinkFaceEdge(struct ed_face_t *face, struct ed_edge_t *edge);

//void ed_LinkFacePolygonEdge(struct ed_face_polygon_t *polygon, struct ed_edge_t *edge);

//void ed_UnlinkFacePolygonEdge(struct ed_face_polygon_t *polygon, struct ed_edge_t *edge);

//void ed_RemoveFacePolygon(struct ed_brush_t *brush, struct ed_face_polygon_t *polygon);

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

//int ed_CompareBspPolygons(const void *a, const void *b);

void ed_ExtrudeBrushFace(struct ed_brush_t *brush, uint32_t face_index);

void ed_DeleteBrushFace(struct ed_brush_t *brush, uint32_t face_index);

void ed_SetFaceMaterial(struct ed_brush_t *brush, uint32_t face_index, struct ed_brush_material_t *material);

void ed_TranslateBrushFace(struct ed_brush_t *brush, uint32_t face_index, vec3_t *translation);

void ed_TranslateBrushEdge(struct ed_brush_t *brush, uint32_t edge_index, vec3_t *translation);

void ed_RotateBrushFace(struct ed_brush_t *brush, uint32_t face_index, mat3_t *rotation);

void ed_UpdateBrushEntity(struct ed_brush_t *brush);

void ed_UpdateBrush(struct ed_brush_t *brush);



//void *ed_CreateBrushObject(vec3_t *position, mat3_t *orientation, vec3_t *scale, void *args);

//void ed_DestroyBrushObject(void *base_obj);

//struct r_i_draw_list_t *ed_RenderPickBrushObject(struct ed_obj_t *object, struct r_i_cmd_buffer_t *cmd_buffer);

//struct r_i_draw_list_t *ed_RenderDrawBrushObject(struct ed_obj_result_t *object, struct r_i_cmd_buffer_t *cmd_buffer);

//void ed_UpdateBrushHandleObject(struct ed_obj_t *object);

//void ed_UpdateBrushBaseObject(struct ed_obj_t *object);



void *ed_CreateFaceObject(vec3_t *position, mat3_t *orientation, vec3_t *scale, void *args);

void ed_DestroyFaceObject(void *base_obj);

//struct r_i_draw_list_t *ed_RenderPickFaceObject(struct ed_obj_t *object, struct r_i_cmd_buffer_t *cmd_buffer);

struct r_i_draw_list_t *ed_RenderDrawFaceObject(struct ed_obj_result_t *object, struct r_i_cmd_buffer_t *cmd_buffer);

void ed_UpdateFaceHandleObject(struct ed_obj_t *object);

void ed_UpdateFaceBaseObject(struct ed_obj_t *object);

void ed_FaceObjectDrawTransform(struct ed_obj_t *object, mat4_t *model_view_projection_matrix);


//void ed_BuildWorldGeometry();

/*
=============================================================
=============================================================
=============================================================
*/

//void ed_InitBrushPickItem(struct ed_pick_item_t *pick_item);

/*
=============================================================
=============================================================
=============================================================
*/

//struct ed_bsp_node_t *ed_AllocBspNode();
//
//void ed_FreeBspNode(struct ed_bsp_node_t *node);
//
//void ed_FreeBspTree(struct ed_bsp_node_t *bsp);
//
//struct ed_bsp_polygon_t *ed_BspPolygonsFromBrush(struct ed_brush_t *brush);
//
//struct ed_bsp_polygon_t *ed_BspPolygonFromBrushFace(struct ed_face_t *face);
//
//struct ed_bsp_polygon_t *ed_AllocBspPolygon(uint32_t vert_count);
//
//struct ed_bsp_polygon_t *ed_CopyBspPolygons(struct ed_bsp_polygon_t *src_polygons);
//
//void ed_FreeBspPolygon(struct ed_bsp_polygon_t *polygon, uint32_t free_verts);
//
//void ed_FreeBspPolygons(struct ed_bsp_polygon_t *polygons);
//
//void ed_UnlinkPolygon(struct ed_bsp_polygon_t *polygon, struct ed_bsp_polygon_t **first_polygon);
//
//uint32_t ed_PolygonOnSplitter(struct ed_bsp_polygon_t *polygon, vec3_t *point, vec3_t *normal);
//
//uint32_t ed_PointOnSplitter(vec3_t *point, vec3_t *plane_point, vec3_t *plane_normal);
//
//struct ed_bsp_polygon_t *ed_BestSplitter(struct ed_bsp_polygon_t *polygons);
//
//void ed_SplitPolygon(struct ed_bsp_polygon_t *polygon, vec3_t *point, vec3_t *normal, struct ed_bsp_polygon_t **front, struct ed_bsp_polygon_t **back);
//
//struct ed_bsp_node_t *ed_SolidBspFromPolygons(struct ed_bsp_polygon_t *polygons);
//
//struct ed_bsp_node_t *ed_LeafBspFromPolygons(struct ed_bsp_polygon_t *polygons);
//
//struct ed_bsp_polygon_t *ed_ClipPolygonToBsp(struct ed_bsp_polygon_t *polygons, struct ed_bsp_node_t *bsp);
//
//struct ed_bsp_polygon_t *ed_ClipPolygonLists(struct ed_bsp_polygon_t *polygons_a, struct ed_bsp_polygon_t *polygons_b);




#endif // ED_BRUSH_H
