#ifndef ED_COM_H
#define ED_COM_H

#include <stdint.h>
#include "dstuff/ds_buffer.h"
#include "dstuff/ds_vector.h"
#include "dstuff/ds_matrix.h"
#include "r_com.h"

struct ed_brush_batch_t
{
    struct r_batch_t batch;
    uint32_t index;
};

struct ed_brush_edge_t
{
    struct ed_brush_edge_t *sibling;
    struct ed_brush_edge_t *next;
    struct ed_brush_edge_t *prev;
    struct ed_brush_face_t *face;
};

struct ed_brush_face_t
{
    struct ed_brush_edge_t *edges;
    struct ed_brush_face_t *next;
    struct ed_brush_face_t *prev;
};

struct ed_brush_t
{
    mat3_t orientation;
    vec3_t position;
    uint32_t index;
    struct ds_slist_t polygons;
    struct ds_slist_t bsp_nodes;
    struct ds_list_t faces;
    struct ds_buffer_t vertices;
    struct r_model_t *model;
    struct ed_bspn_t *bsp;
    uint32_t clipped_vert_count;
    uint32_t clipped_index_count;
    uint32_t clipped_polygon_count;
};

struct ed_polygon_t
{
    struct ed_polygon_t *next;
    struct ed_polygon_t *prev;
    struct ed_brush_t *brush;
    struct ds_buffer_t vertices;
    struct r_material_t *material;
    vec3_t normal;
    uint32_t index;
    uint32_t mesh_start;
    uint32_t mesh_count;
};

struct ed_face_t
{
    struct ed_polygon_t *polygon;
    struct ed_polygon_t *clipped_polygons;
    struct r_material_t *material;
    struct ds_buffer_t indices;
    uint32_t clipped_vert_count;
    uint32_t clipped_index_count;
    uint32_t clipped_polygon_count;
    vec3_t normal;
    vec3_t tangent;
};

struct ed_bspn_t
{
    struct ed_bspn_t *front;
    struct ed_bspn_t *back;
    struct ed_polygon_t *splitter;
    struct ed_brush_t *brush;
    uint32_t index;
};

struct ed_context_t;

struct ed_state_t
{
    void (*update)(struct ed_context_t *context, uint32_t just_changed);
};

enum ED_CONTEXTS
{
    ED_CONTEXT_WORLD = 0,
    ED_CONTEXT_LAST,
};

struct ed_context_t
{
    void (*update)();
    uint32_t current_state;
    uint32_t next_state;
    struct ed_state_t *states;
    void *context_data;
};


#endif // ED_COM_H
