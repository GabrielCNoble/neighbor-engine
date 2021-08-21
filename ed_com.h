#ifndef ED_COM_H
#define ED_COM_H

#include <stdint.h>
#include "dstuff/ds_buffer.h"
#include "dstuff/ds_vector.h"
#include "dstuff/ds_matrix.h"
#include "r_com.h"

struct ed_polygon_t
{
    struct ed_polygon_t *next;
    struct ed_polygon_t *prev;
    struct ds_buffer_t vertices;
    struct r_material_t *material;
    uint32_t index;
    vec3_t normal;
};

struct ed_bspn_t
{
    struct ed_bspn_t *front;
    struct ed_bspn_t *back;
    struct ed_polygon_t *splitter;
    uint32_t index;
};

struct ed_face_t
{
    struct r_material_t *material;
    struct ds_buffer_t indices;
    vec3_t normal;
    vec3_t tangent;
};

struct ed_brush_batch_t
{
    struct r_batch_t batch;
    uint32_t index;
};

struct ed_brush_t
{
    mat3_t orientation;
    vec3_t position;
    uint32_t index;
    struct ds_list_t faces;
    struct ds_buffer_t vertices;
    struct r_model_t *model;
    struct ed_bspn_t *bsp;
};

enum ED_PICKABLE_TYPE
{
    ED_PICKABLE_TYPE_BRUSH = 1,
    ED_PICKABLE_TYPE_ENTITY,
    ED_PICKABLE_TYPE_LIGHT,
    ED_PICKABLE_TYPE_FACE,
    ED_PICKABLE_TYPE_MANIPULATOR,
};

struct ed_pickable_t
{
    uint32_t type;
    uint32_t index;

    mat4_t transform;
    uint32_t mode;
    uint32_t start;
    uint32_t count;
    uint32_t pick_index;
};


struct ed_context_t;

struct ed_state_t
{
    void (*update)(struct ed_context_t *context, uint32_t just_changed);
};

struct ed_context_t
{
    void (*update)();
    uint32_t current_state;
    uint32_t next_state;
    struct ed_state_t *states;
    void *context_data;
};

struct ed_world_context_data_t
{
    vec3_t box_start;
    vec3_t box_end;
    struct ed_pickable_t *last_selected;
    struct ds_slist_t pickables;
    struct ds_slist_t brushes;
    uint32_t global_brush_vert_count;
    uint32_t global_brush_index_count;
    struct ds_list_t global_brush_batches;
    struct ds_list_t selections;
};

enum ED_CONTEXTS
{
    ED_CONTEXT_WORLD = 0,
    ED_CONTEXT_LAST,
};


enum ED_WORLD_CONTEXT_STATES
{
    ED_WORLD_CONTEXT_STATE_IDLE = 0,
    ED_WORLD_CONTEXT_STATE_LEFT_CLICK,
    ED_WORLD_CONTEXT_STATE_RIGHT_CLICK,
    ED_WORLD_CONTEXT_STATE_BRUSH_BOX,
    ED_WORLD_CONTEXT_STATE_CREATE_BRUSH,
    ED_WORLD_CONTEXT_STATE_PROCESS_SELECTION,
    ED_WORLD_CONTEXT_STATE_LAST
};

#endif // ED_COM_H
