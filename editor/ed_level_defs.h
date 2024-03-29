#ifndef ED_LEVEL_DEFS_H
#define ED_LEVEL_DEFS_H

#include "ed_defs.h"
#include "ed_pick_defs.h"
#include "obj/obj.h"

enum ED_LEVEL_EDIT_MODES
{
    ED_LEVEL_EDIT_MODE_OBJECT = 0,
    ED_LEVEL_EDIT_MODE_BRUSH,
    ED_LEVEL_EDIT_MODE_LAST
};

enum ED_L_TRANSFORM_TYPES
{
    ED_L_TRANSFORM_TYPE_TRANSLATION = 0,
    ED_L_TRANSFORM_TYPE_ROTATION,
    ED_L_TRANSFORM_TYPE_SCALE,
    ED_L_TRANSFORM_TYPE_LAST
};

enum ED_L_TRANSFORM_MODES
{
    ED_L_TRANSFORM_MODE_WORLD = 0,
    ED_L_TRANSFORM_MODE_LOCAL = 1,
    ED_L_TRANSFORM_MODE_LAST
};

//enum ED_LEVEL_SECONDARY_CLICK_FUNCS
//{
//    ED_LEVEL_SECONDARY_CLICK_FUNC_BRUSH = 0,
//    ED_LEVEL_SECONDARY_CLICK_FUNC_LIGHT,
//    ED_LEVEL_SECONDARY_CLICK_FUNC_ENTITY
//};

enum ED_L_TOOL_TABS
{
    ED_L_TOOL_TAB_BRUSH = 0,
    ED_L_TOOL_TAB_LIGHT,
    ED_L_TOOL_TAB_ENTITY,
    ED_L_TOOL_TAB_ENEMY,
    ED_L_TOOL_TAB_MATERIAL,
};

enum ED_LEVEL_BRUSH_TOOLS
{
    ED_LEVEL_BRUSH_TOOL_CREATE = 0,
};

enum ED_LEVEL_LIGHT_TYPES
{
    ED_LEVEL_LIGHT_TYPE_POINT = 0,
    ED_LEVEL_LIGHT_TYPE_SPOT
};

struct ed_level_state_t
{
    struct
    {
        vec3_t              box_start;
        vec3_t              box_end;
        vec2_t              box_size;
        uint32_t            drawing;
        uint32_t            selected_tool;

//        struct ds_slist_t bsp_nodes;
//        struct ds_slist_t bsp_polygons;

        struct ds_slist_t   brushes;
        struct ds_slist_t   brush_faces;
//        struct ds_slist_t brush_face_polygons;
        struct ds_slist_t   brush_edges;
        struct ds_slist_t   brush_verts;
        struct ds_slist_t   brush_materials;

//        struct ds_buffer_t polygon_buffer;
        struct ds_buffer_t  face_buffer;
        struct ds_buffer_t  vertex_buffer;
        struct ds_buffer_t  index_buffer;
        struct ds_buffer_t  batch_buffer;

        uint32_t            brush_vert_count;

        uint32_t            brush_model_vert_count;
        uint32_t            brush_model_index_count;


    } brush;

    struct
    {
        struct ed_obj_context_t objects;
    }obj;

    struct
    {
        vec3_t plane_point;
        mat3_t plane_orientation;
        struct e_ent_def_t *ent_def;
        uint32_t enemy_type;
        uint32_t light_type;
        uint32_t ignore_types;
        uint32_t selections_window_open;
        struct ds_list_t game_pickables[ED_PICKABLE_TYPE_LAST_GAME_PICKABLE];
        struct ds_slist_t pickables;
        struct ds_list_t selections;
        struct ds_list_t modified_brushes;
        struct ds_list_t modified_pickables;
        struct ed_pickable_t *last_selected;

    } pickables;

    uint32_t selected_tools_tab;
    struct r_material_t *selected_material;

    struct ds_slist_t pickable_ranges;
    struct ds_slist_t widgets;

    struct
    {
        uint32_t transform_type;
        uint32_t transform_mode;
        uint32_t space;
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
        char base_folder[PATH_MAX];
        char level_name[PATH_MAX];

    } project;
};

//enum ED_LEVEL_RESOURCE_TYPES
//{
//    ED_LEVEL_RESOURCE_TYPE_TEXTURE = 0,
//    ED_LEVEL_RESOURCE_TYPE_MODEL,
//    ED_LEVEL_RESOURCE_TYPE_ENT_DEF,
//    ED_LEVEL_RESOURCE_TYPE_SOUND,
//    ED_LEVEL_RESOURCE_TYPE_MATERIAL,
//    ED_LEVEL_RESOURCE_TYPE_LAST,
//};
//
//struct ed_level_resource_t
//{
//    uint32_t type;
//    uint32_t index;
//    void *resource;
//    char *src_path;
//};

struct ed_l_section_t
{
    char name[32];

    vec3_t camera_pos;
    float camera_pitch;
    float camera_yaw;

    uint64_t brush_section_start;
    uint64_t brush_section_size;
};

#endif




