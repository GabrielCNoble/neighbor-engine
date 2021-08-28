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

enum ED_PICKABLE_TYPE
{
    ED_PICKABLE_TYPE_BRUSH = 1,
    ED_PICKABLE_TYPE_ENTITY,
    ED_PICKABLE_TYPE_LIGHT,
    ED_PICKABLE_TYPE_FACE,
    ED_PICKABLE_TYPE_WIDGET,
};

struct ed_pickable_t
{
    uint32_t type;
    uint32_t index;

    mat4_t transform;
    uint32_t mode;
    uint32_t start;
    uint32_t count;

    uint32_t primary_index;
    uint32_t secondary_index;
};

struct ed_pick_result_t
{
    uint32_t type;
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



enum ED_WORLD_CONTEXT_STATES
{
    ED_WORLD_CONTEXT_STATE_IDLE = 0,
    ED_WORLD_CONTEXT_STATE_LEFT_CLICK,
    ED_WORLD_CONTEXT_STATE_WIDGET_SELECTED,
//    ED_WORLD_CONTEXT_STATE_RIGHT_CLICK,
    ED_WORLD_CONTEXT_STATE_BRUSH_BOX,
//    ED_WORLD_CONTEXT_STATE_CREATE_BRUSH,
//    ED_WORLD_CONTEXT_STATE_PROCESS_SELECTION,
    ED_WORLD_CONTEXT_STATE_ENTER_OBJECT_EDIT_MODE,
    ED_WORLD_CONTEXT_STATE_ENTER_BRUSH_EDIT_MODE,
    ED_WORLD_CONTEXT_STATE_LAST
};

enum ED_WORLD_CONTEXT_EDIT_MODES
{
    ED_WORLD_CONTEXT_EDIT_MODE_OBJECT = 0,
    ED_WORLD_CONTEXT_EDIT_MODE_BRUSH,
};

struct ed_world_context_data_t
{
    vec3_t box_start;
    vec3_t box_end;
    struct ed_pickable_t *last_selected;
    uint32_t edit_mode;
    uint32_t active_list;

    struct ds_slist_t pickables[3];
    struct ds_list_t selections[2];

    struct ds_slist_t *active_pickables;
    struct ds_list_t *active_selections;

//    struct ds_slist_t brush_pickables;
//    struct ds_list_t brush_selections;


    struct ds_slist_t brushes;
    uint32_t global_brush_vert_count;
    uint32_t global_brush_index_count;
    struct ds_list_t global_brush_batches;


    float info_window_alpha;
    uint32_t open_delete_selections_popup;


    float camera_pitch;
    float camera_yaw;
    vec3_t camera_pos;
};


#endif // ED_COM_H
