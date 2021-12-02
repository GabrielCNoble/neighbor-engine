#ifndef ED_PICK_DEFS_H
#define ED_PICK_DEFS_H

#include "ed_defs.h"

enum ED_PICKABLE_TYPE
{
    /* game specific pickables */
    ED_PICKABLE_TYPE_ENTITY = 0,
    ED_PICKABLE_TYPE_LIGHT,
    ED_PICKABLE_TYPE_GAME_ENTITY,
    ED_PICKABLE_TYPE_LAST_GAME_PICKABLE,

    /* level editor specific pickables */
    ED_PICKABLE_TYPE_BRUSH = ED_PICKABLE_TYPE_LAST_GAME_PICKABLE,
    ED_PICKABLE_TYPE_FACE,
    ED_PICKABLE_TYPE_EDGE,
    ED_PICKABLE_TYPE_VERT,
    ED_PICKABLE_TYPE_WIDGET,
};

enum ED_PICKABLE_TYPE_MASKS
{
    ED_PICKABLE_TYPE_MASK_BRUSH = 1 << ED_PICKABLE_TYPE_BRUSH,
    ED_PICKABLE_TYPE_MASK_ENTITY = 1 << ED_PICKABLE_TYPE_ENTITY,
    ED_PICKABLE_TYPE_MASK_LIGHT = 1 << ED_PICKABLE_TYPE_LIGHT,
    ED_PICKABLE_TYPE_MASK_FACE = 1 << ED_PICKABLE_TYPE_FACE,
    ED_PICKABLE_TYPE_MASK_EDGE = 1 << ED_PICKABLE_TYPE_EDGE,
    ED_PICKABLE_TYPE_MASK_VERT = 1 << ED_PICKABLE_TYPE_VERT,
};

#define ED_PICKABLE_BRUSH_PART_MASK (ED_PICKABLE_TYPE_MASK_FACE | ED_PICKABLE_TYPE_MASK_EDGE | ED_PICKABLE_TYPE_MASK_VERT)
#define ED_PICKABLE_OBJECT_MASK (ED_PICKABLE_TYPE_MASK_BRUSH | ED_PICKABLE_TYPE_MASK_ENTITY | ED_PICKABLE_TYPE_MASK_LIGHT)

struct ed_pickable_range_t
{
    struct ed_pickable_range_t *prev;
    struct ed_pickable_range_t *next;
    mat4_t offset;
    uint32_t index;
    uint32_t start;
    uint32_t count;
};

enum ED_PICKABLE_DRAW_RENDER_FLAGS
{
    ED_PICKABLE_DRAW_RENDER_FLAG_OUTLINE = 1,
    ED_PICKABLE_DRAW_RENDER_FLAG_NO_DEPTH_TEST = 1 << 1,
    ED_PICKABLE_DRAW_RENDER_FLAG_BLEND = 1 << 2,
};

enum ED_PICKABLE_DRAW_TRANSF_FLAGS
{
    ED_PICKABLE_DRAW_TRANSF_FLAG_BILLBOARD = 1,
    ED_PICKABLE_DRAW_TRANSF_FLAG_FIXED_CAM_DIST = 1 << 1
};

enum ED_PICKABLE_TRANSFORM_FLAGS
{
    ED_PICKABLE_TRANSFORM_FLAG_TRANSLATION = 1,
    ED_PICKABLE_TRANSFORM_FLAG_ROTATION = 1 << 1,
    ED_PICKABLE_TRANSFORM_FLAG_SCALING = 1 << 2,
};

struct ed_pickable_t
{
    uint32_t type;
    uint32_t index;
    struct ds_slist_t *list;
    uint32_t selection_index;
    uint32_t modified_index;
    uint32_t game_pickable_index;

    uint32_t transform_flags;
    uint32_t draw_render_flags;
    uint32_t draw_transf_flags;
    float camera_distance;

    vec3_t translation;
    vec3_t scale;
    mat3_t rotation;
    mat4_t transform;
//    mat4_t draw_transform;

    uint32_t mode;
    uint32_t range_count;
    struct ed_pickable_range_t *ranges;

    uint32_t primary_index;
    uint32_t secondary_index;
    uint32_t update_index;
};

struct ed_manipulator_t
{
    uint32_t mode;
    uint32_t visible;
    float prev_angle;
    vec3_t start_pos;
    vec3_t prev_offset;
    mat4_t transform;
    vec2_t screen_pos;
    float linear_snap;
    float angular_snap;
    struct ed_widget_t *widgets[3];
};

//struct ed_pick_dep_t
//{
//    struct
//    {
//        struct ed_pick_dep_t *next;
//        struct ed_pick_dep_t *prev;
//        struct ed_pickable_t *pickable;
//    } pickables[2];
//
//    uint32_t index;
//};

struct ed_widget_t
{
    uint32_t index;
    uint32_t stencil_layer;
    struct ds_slist_t pickables;

//    void (*mvp_mat_fn)(mat4_t *view_projection_matrix, mat4_t *widget_transform);
    void (*setup_ds_fn)(uint32_t pickable_index, struct ed_pickable_t *pickable);
};

#endif
