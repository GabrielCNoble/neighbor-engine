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
//        uint32_t brush_index_count;

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
};

//static char ed_level_section_start_label[] = "[LEVEL SECTION START]";
//#define ED_LEVEL_SECTION_START_LABEL_LEN ((sizeof(ed_level_section_start_label) + 3) & (~3))

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

    size_t material_section_start;
    size_t material_section_size;

    size_t reserved[32];
};

//static char ed_level_section_end_label[] = "[LEVEL SECTION END]";
//#define ED_LEVEL_SECTION_END_LABEL_LEN ((sizeof(ed_level_section_end_label) + 3) & (~3))
//
//struct ed_level_section_end_t
//{
//    char label[ED_LEVEL_SECTION_END_LABEL_LEN];
//};

#endif




