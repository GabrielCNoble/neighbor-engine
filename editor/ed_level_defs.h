#ifndef ED_LEVEL_DEFS_H
#define ED_LEVEL_DEFS_H

#include "ed_defs.h"

enum ED_LEVEL_EDIT_MODES
{
    ED_LEVEL_EDIT_MODE_OBJECT = 0,
    ED_LEVEL_EDIT_MODE_BRUSH,
    ED_LEVEL_EDIT_MODE_LAST
};

enum ED_LEVEL_MANIP_MODES
{
    ED_LEVEL_MANIP_MODE_TRANSLATION = 0,
    ED_LEVEL_MANIP_MODE_ROTATION,
    ED_LEVEL_MANIP_MODE_SCALE,
};

enum ED_LEVEL_SECONDARY_CLICK_FUNCS
{
    ED_LEVEL_SECONDARY_CLICK_FUNC_BRUSH = 0,
    ED_LEVEL_SECONDARY_CLICK_FUNC_LIGHT
};

struct ed_level_state_t
{

    struct
    {
        vec3_t box_start;
        vec3_t box_end;
        vec3_t plane_point;
        vec2_t box_size;
        mat3_t plane_orientation;
        uint32_t drawing;

        struct ds_slist_t bsp_nodes;
        struct ds_slist_t bsp_polygons;

        struct ds_slist_t brushes;
        struct ds_slist_t brush_faces;
        struct ds_slist_t brush_face_polygons;
        struct ds_slist_t brush_edges;

        struct ds_buffer_t polygon_buffer;
        struct ds_buffer_t vertex_buffer;
        struct ds_buffer_t index_buffer;
        struct ds_buffer_t batch_buffer;

        uint32_t brush_vert_count;

        uint32_t brush_model_vert_count;
        uint32_t brush_model_index_count;

        struct ds_list_t brush_batches;
    } brush;

    struct
    {
        uint32_t ignore_types;
        uint32_t secondary_click_function;
        uint32_t selections_window_open;
        struct ds_slist_t pickables;
        struct ds_list_t selections;
        struct ds_list_t modified_brushes;
        struct ds_list_t modified_pickables;
        struct ed_pickable_t *last_selected;

    } pickables;

    struct ds_slist_t pickable_ranges;
    struct ds_slist_t widgets;

    struct
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

    } manipulator;

    struct ed_widget_t *ball_widget;
    mat4_t ball_transform;

    float info_window_alpha;
    uint32_t open_delete_selections_popup;

    float camera_pitch;
    float camera_yaw;
    vec3_t camera_pos;

    void *game_level_buffer;
    size_t game_level_buffer_size;
    uint32_t world_data_stale;

    struct p_collider_t *world_collider;
    struct p_shape_def_t *world_shape;
    struct r_model_t *world_model;

    struct
    {
        char folder[PATH_MAX];
        char level_name[PATH_MAX];

    } project;
};

enum ED_LEVEL_RESOURCE_TYPES
{
    ED_LEVEL_RESOURCE_TYPE_TEXTURE = 0,
    ED_LEVEL_RESOURCE_TYPE_MODEL,
    ED_LEVEL_RESOURCE_TYPE_ENT_DEF,
    ED_LEVEL_RESOURCE_TYPE_SOUND,
    ED_LEVEL_RESOURCE_TYPE_MATERIAL,
    ED_LEVEL_RESOURCE_TYPE_LAST,
};

struct ed_level_resource_t
{
    uint32_t type;
    uint32_t index;
    void *resource;
    char *src_path;
};

#define ED_LEVEL_SECTION_MAGIC0 0x4749454e
#define ED_LEVEL_SECTION_MAGIC1 0x524f4248


struct ed_level_section_t
{
    uint32_t magic0;
    uint32_t magic1;

    char name[32];

    vec3_t camera_pos;
    float camera_pitch;
    float camera_yaw;

    size_t brush_section_start;
    size_t brush_section_size;

    size_t light_section_start;
    size_t light_section_size;

    size_t entity_section_start;
    size_t entity_section_size;

    size_t ent_def_section_start;
    size_t ent_def_section_size;

    size_t material_section_start;
    size_t material_section_size;

    size_t world_section_start;
    size_t world_section_size;

    size_t reserved[32];
};

#endif




