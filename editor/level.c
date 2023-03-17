#include <float.h>
#include "level.h"
#include "ed_pick.h"
#include "obj/obj.h"
#include "obj/brush.h"
#include "obj/ent.h"
#include "obj/light.h"
#include "ed_main.h"
#include "../engine/r_main.h"
#include "../engine/r_draw_i.h"
#include "../lib/dstuff/ds_buffer.h"
#include "../lib/dstuff/ds_path.h"
#include "../lib/dstuff/ds_dir.h"
#include "../lib/dstuff/ds_file.h"
#include "../engine/gui.h"
#include "../engine/g_main.h"
#include "../engine/g_enemy.h"
#include "../engine/input.h"
#include "../engine/l_defs.h"
#include "../engine/ent.h"
#include "../engine/level.h"

//extern struct ed_context_t ed_contexts[];
extern struct ed_editor_t ed_editors[];
struct ed_level_state_t ed_level_state;
//struct ed_context_t *ed_world_context;
extern mat4_t r_camera_matrix;
extern mat4_t r_view_projection_matrix;

extern struct p_shape_def_t *l_world_shape;
extern struct p_col_def_t l_world_col_def;
extern struct p_collider_t *l_world_collider;
extern char *p_col_type_names[];
extern char *r_light_type_names[];
extern char *ed_obj_names[];
extern char *ed_brush_element_names[];
extern char *ed_transform_operator_transform_mode_names[];
extern char *ed_transform_operator_transform_type_names[];
extern struct r_model_t *l_world_model;
extern struct ed_obj_funcs_t ed_obj_funcs[];
extern struct r_texture_t *r_default_albedo_texture;

struct r_texture_t *ed_l_extrude_tool_texture;
struct r_texture_t *ed_l_shape_tool_texture;
struct r_texture_t *ed_l_entity_tab_texture;
struct r_texture_t *ed_l_transform_operator_transform_type_textures[ED_TRANSFORM_OPERATOR_TRANSFORM_TYPE_LAST];
char *ed_l_transform_operator_transform_type_tooltips[ED_TRANSFORM_OPERATOR_TRANSFORM_TYPE_LAST] = {
    [ED_TRANSFORM_OPERATOR_TRANSFORM_TYPE_TRANSLATE] = "Translate selections. Shortcut: T",
    [ED_TRANSFORM_OPERATOR_TRANSFORM_TYPE_ROTATE] = "Rotate selections. Shortcut: R",
    [ED_TRANSFORM_OPERATOR_TRANSFORM_TYPE_SCALE] = "Scale selections. Shortcut: S",
};
//struct r_texture_t *ed_l_scale_texture;

//struct ed_obj_context_t ed_l_obj_context;

struct r_shader_t *ed_center_grid_shader;
//struct r_shader_t *ed_picking_shader;
//struct r_model_t *ed_translation_widget_model;
//struct r_model_t *ed_rotation_widget_model;
struct r_model_t *ed_light_pickable_model;
struct r_model_t *ed_ball_widget_model;
//struct ed_pickable_t *ed_translation_widget;
//struct r_shader_t *ed_outline_shader;
struct r_shader_t *ed_zero_depth_shader;

#define ED_GRID_DIVS 301
#define ED_GRID_QUAD_SIZE 250.0
#define ED_W_BRUSH_BOX_CROSSHAIR_DIM 0.3

struct r_vert_t ed_grid[] =
{
    [0] = {
        .pos = vec3_t_c(-ED_GRID_QUAD_SIZE, 0.0, -ED_GRID_QUAD_SIZE),
        .tex_coords = vec2_t_c(0.0, 0.0),
        .color = vec4_t_c(1.0, 0.0, 0.0, 1.0)
    },
    [1] = {
        .pos = vec3_t_c(-ED_GRID_QUAD_SIZE, 0.0, ED_GRID_QUAD_SIZE),
        .tex_coords = vec2_t_c(1.0, 0.0),
        .color = vec4_t_c(0.0, 1.0, 0.0, 1.0)
    },
    [2] = {
        .pos = vec3_t_c(ED_GRID_QUAD_SIZE, 0.0, ED_GRID_QUAD_SIZE),
        .tex_coords = vec2_t_c(1.0, 1.0),
        .color = vec4_t_c(0.0, 0.0, 1.0, 1.0)
    },
    [3] = {
        .pos = vec3_t_c(ED_GRID_QUAD_SIZE, 0.0, ED_GRID_QUAD_SIZE),
        .tex_coords = vec2_t_c(1.0, 1.0),
        .color = vec4_t_c(0.0, 0.0, 1.0, 1.0)
    },
    [4] = {
        .pos = vec3_t_c(ED_GRID_QUAD_SIZE, 0.0, -ED_GRID_QUAD_SIZE),
        .tex_coords = vec2_t_c(0.0, 1.0),
        .color = vec4_t_c(0.0, 0.0, 1.0, 1.0)
    },
    [5] = {
        .pos = vec3_t_c(-ED_GRID_QUAD_SIZE, 0.0, -ED_GRID_QUAD_SIZE),
        .tex_coords = vec2_t_c(0.0, 0.0),
        .color = vec4_t_c(1.0, 0.0, 0.0, 1.0)
    }
};
//struct r_i_verts_t *ed_grid;

extern struct ds_slist_t r_lights[];
extern struct ds_slist_t r_materials;
extern struct ds_slist_t r_textures;
extern struct ds_slist_t e_entities;
extern struct ds_slist_t e_ent_defs[];
extern struct ds_list_t e_components[];
extern struct ds_slist_t g_enemies[];
extern struct ds_list_t e_root_transforms;

extern mat4_t r_projection_matrix;
extern struct r_shader_t *r_immediate_shader;
extern mat4_t r_camera_matrix;
extern mat4_t r_view_matrix;
extern uint32_t r_width;
extern uint32_t r_height;
extern float r_fov;
extern float r_z_near;

extern char *g_enemy_names[];

char *ed_l_transform_type_texts[] =
{
    [ED_L_TRANSFORM_TYPE_TRANSLATION] = "Translation",
    [ED_L_TRANSFORM_TYPE_ROTATION] = "Rotation"
};

char *ed_l_transform_mode_texts[] =
{
    [ED_L_TRANSFORM_MODE_WORLD] = "World",
    [ED_L_TRANSFORM_MODE_LOCAL] = "Local",
};

float ed_w_linear_snap_values[] =
{
    0.0,
    0.001,
    0.01,
    0.1,
    0.2,
    0.25,
    0.5,
    1.0,
    5.0,
    10.0,
};

float ed_w_angular_snap_values[] =
{
    0.0,
    0.1,
    0.2,
    0.25,
    0.5
};

#define ED_LEVEL_CAMERA_PITCH (-0.15)
#define ED_LEVEL_CAMERA_YAW (-0.3)
#define ED_LEVEL_CAMERA_POS (vec3_t_c(-6.0, 4.0, 4.0))

//vec4_t ed_selection_outline_colors[][2] =
//{
//    [ED_PICKABLE_TYPE_BRUSH][0] = vec4_t_c(1.0, 0.2, 0.0, 1.0),
//    [ED_PICKABLE_TYPE_BRUSH][1] = vec4_t_c(1.0, 0.5, 0.0, 1.0),
//
//    [ED_PICKABLE_TYPE_LIGHT][0] = vec4_t_c(1.0, 0.2, 0.0, 1.0),
//    [ED_PICKABLE_TYPE_LIGHT][1] = vec4_t_c(1.0, 0.5, 0.0, 1.0),
//
//    [ED_PICKABLE_TYPE_ENTITY][0] = vec4_t_c(1.0, 0.2, 0.0, 1.0),
//    [ED_PICKABLE_TYPE_ENTITY][1] = vec4_t_c(1.0, 0.5, 0.0, 1.0),
//
//    [ED_PICKABLE_TYPE_FACE][0] = vec4_t_c(0.3, 0.4, 1.0, 1.0),
//    [ED_PICKABLE_TYPE_FACE][1] = vec4_t_c(0.3, 0.4, 1.0, 1.0),
//};

//extern struct e_ent_def_t *g_ent_def;

enum ED_L_MAIN_TOOL_TABS
{
//    ED_L_MAIN_TOOL_TAB_VERT,
//    ED_L_MAIN_TOOL_TAB_EDGE,
//    ED_L_MAIN_TOOL_TAB_FACE,
    ED_L_MAIN_TOOL_TAB_OBJECT,
    ED_L_MAIN_TOOL_TAB_MATERIAL,
    ED_L_MAIN_TOOL_TAB_SCRIPT,
    ED_L_MAIN_TOOL_TAB_LAST
};

char *ed_l_main_tool_tab_names[] =
{
//    [ED_L_MAIN_TOOL_TAB_VERT]       = "Vert"
    [ED_L_MAIN_TOOL_TAB_OBJECT]     = "Object",
    [ED_L_MAIN_TOOL_TAB_MATERIAL]   = "Material",
    [ED_L_MAIN_TOOL_TAB_SCRIPT]     = "Script",
};

uint32_t ed_l_active_main_tool_tab = ED_L_MAIN_TOOL_TAB_OBJECT;
uint32_t ed_l_tool_tab_passthrough = 0;
ImGuiID ed_l_tool_tab_root_docking_id;
ImGuiID ed_l_tool_tab_top_docking_id;
ImGuiID ed_l_tool_tab_selections_docking_id;
ImGuiWindowFlags ed_l_tool_tab_window_flags;

//struct e_ent_def_t *ed_l_selected_def = NULL;
struct r_material_t *ed_l_selected_material = NULL;

ImGuiID ed_l_root_docking_id = 0;

char *ed_l_project_folders[] =
{
    "entities",
    "levels",
    "models",
    "sounds",
    "animations",
    "textures",
    NULL
};

enum ED_L_TOOLS
{
    ED_L_TOOL_DELETE_OBJECT = 0,
    ED_L_TOOL_FLY_CAMERA,
    ED_L_TOOL_PLACE_OBJECT,
    ED_L_TOOL_LEFT_CLICK,
    ED_L_TOOL_TRANSFORM_MODE,
    ED_L_TOOL_LAST,
};

struct ed_l_delete_tool_state_t
{
    uint32_t    open_pop_up;
    int32_t     mouse_x;
    int32_t     mouse_y;
} ed_l_delete_tool_state = {};

struct ed_l_fly_camera_tool_state_t
{
    vec3_t      position;
    float       pitch;
    float       yaw;

} ed_l_fly_camera_state = {
    .position = ED_LEVEL_CAMERA_POS,
    .pitch = ED_LEVEL_CAMERA_PITCH,
    .yaw = ED_LEVEL_CAMERA_YAW
};

//struct ed_l_fly_camera_tool_state_t ed_l_fly_camera_state = {
//    .position = ED_LEVEL_CAMERA_POS,
//    .pitch = ED_LEVEL_CAMERA_PITCH,
//    .yaw = ED_LEVEL_CAMERA_YAW
//};

enum ED_L_OBJ_TABS
{
    ED_L_OBJ_TAB_BRUSH = 0,
    ED_L_OBJ_TAB_LIGHT,
    ED_L_OBJ_TAB_ENTITY,
    ED_L_OBJ_TAB_ENEMY,
    ED_L_OBJ_TAB_BARRIER,
    ED_L_OBJ_TAB_TRIGGER,
    ED_L_OBJ_TAB_LAST
};

char *ed_l_obj_tab_names[] =
{
    [ED_L_OBJ_TAB_BRUSH] = "Brush",
    [ED_L_OBJ_TAB_LIGHT] = "Light",
    [ED_L_OBJ_TAB_ENTITY] = "Entity",
    [ED_L_OBJ_TAB_ENEMY] = "Enemy",
    [ED_L_OBJ_TAB_BARRIER] = "Barrier",
    [ED_L_OBJ_TAB_TRIGGER] = "Trigger",
};

#define ED_L_PLACEMENT_CROSSHAIR_SIZE 0.3

struct ed_l_obj_placement_state_t
{
     uint32_t                   active_tab;
     vec3_t                     plane_point;
     mat3_t                     plane_orientation;


     struct
     {
         vec3_t                 box_start;
         vec3_t                 prev_intersection;
         float                  box_width;
         float                  box_height;
         float                  box_depth;
         uint32_t               stage;

         uint32_t               brush_element;
         uint32_t               prev_brush_element;
     } brush;

     struct
     {
        struct e_ent_def_t *    selected_def;
     } entity;

     struct
     {
        struct ed_light_args_t args;
     } light;
};

struct ed_l_obj_placement_state_t ed_l_obj_placement_state = {
    .plane_point = vec3_t_c(0.0, 0.0, 0.0),
    .plane_orientation = mat3_t_c_id(),
    .active_tab = ED_L_OBJ_TAB_BRUSH,

    .light = {
        .args = {
            .type = R_LIGHT_TYPE_POINT,
            .color = vec3_t_c(1, 1, 1),
            .range = 10,
            .energy = 10,
            .outer_angle = R_SPOT_LIGHT_MAX_ANGLE / 2,
            .inner_angle = R_SPOT_LIGHT_MAX_ANGLE / 2
        }
    }
};

struct ed_brush_pick_args_t ed_l_brush_pick_args = {};

struct ed_pick_args_t ed_l_pick_args = {
    .context = &ed_level_state.obj.objects
};

struct ed_tool_context_t ed_level_tool_context = {
    .idle_state = ed_l_IdleState,
    .current_state = ed_l_IdleState
};
//    .tool_count = ED_L_TOOL_LAST,
//    .tools = (struct ed_tool_t []){
//        [ED_L_TOOL_DELETE_OBJECT] = {.entry_state = ed_l_DeleteSelectionEntryState, .data = &(struct ed_l_delete_tool_state_t){}},
//        [ED_L_TOOL_FLY_CAMERA] = {.entry_state = ed_l_FlyCameraEntryState, .data = &ed_l_fly_camera_state},
//        [ED_L_TOOL_PLACE_OBJECT] = {.entry_state = ed_l_PlacementCrosshairEntryState, .data = &ed_l_obj_placement_state},
//        [ED_L_TOOL_LEFT_CLICK] = {.entry_state = ed_l_LeftClickEntryState, .data = &ed_level_state.obj.objects},
//        [ED_L_TOOL_TRANSFORM_MODE] = {.entry_state = ed_l_TransformOperatorModeEntryState, .data = &ed_level_state.obj.objects}
//    }

void ed_l_Init(struct ed_editor_t *editor)
{
//    ed_world_context = ed_contexts + ED_CONTEXT_WORLD;
//    ed_world_context->update = ed_w_Update;
//    ed_world_context->next_state = ed_w_Idle;
//    ed_world_context->context_data = &ed_level_state;

//    editor->next_state = ed_l_Idle;

//    ed_l_obj_context = ed_CreateObjContext();

    void *operator_data[ED_OPERATOR_LAST] = {
        [ED_OPERATOR_TRANSFORM] = &ed_level_state.obj.transform_operator_data
    };

    ed_level_state.obj.objects = ed_CreateObjContext(operator_data);
    ed_level_state.obj.objects.operators[ED_OPERATOR_TRANSFORM].visible = 1;
    ed_level_state.obj.objects.operators[ED_OPERATOR_TRANSFORM].type = ED_OPERATOR_TRANSFORM;
    ed_level_state.obj.transform_operator_data.transform_type = ED_TRANSFORM_OPERATOR_TRANSFORM_TYPE_TRANSLATE;
    ed_level_state.obj.transform_operator_data.transform_mode = ED_TRANSFORM_OPERATOR_TRANSFORM_MODE_LOCAL;
    ed_level_state.obj.transform_operator_data.linear_snap = ed_w_linear_snap_values[2];

//    ed_level_state.pickables.last_selected = NULL;
//    ed_level_state.pickables.selections = ds_list_create(sizeof(struct ed_pickable_t *), 512);
//    ed_level_state.pickables.pickables = ds_slist_create(sizeof(struct ed_pickable_t), 512);
//    ed_level_state.pickables.modified_brushes = ds_list_create(sizeof(struct ed_brush_t *), 512);
//    ed_level_state.pickables.modified_pickables = ds_list_create(sizeof(struct ed_pickable_t *), 512);
    ed_level_state.selected_tools_tab = ED_L_TOOL_TAB_BRUSH;
    ed_level_state.selected_material = r_GetDefaultMaterial();

//    for(uint32_t game_pickable_type = ED_PICKABLE_TYPE_ENTITY; game_pickable_type < ED_PICKABLE_TYPE_LAST_GAME_PICKABLE; game_pickable_type++)
//    {
//        ed_level_state.pickables.game_pickables[game_pickable_type] = ds_list_create(sizeof(struct ed_pickable_t *), 512);
//    }


//    ed_level_state.pickable_ranges = ds_slist_create(sizeof(struct ed_pickable_range_t), 512);
//    ed_level_state.widgets = ds_slist_create(sizeof(struct ed_widget_t), 16);

//    ed_level_state.brush.bsp_nodes = ds_slist_create(sizeof(struct ed_bsp_node_t), 512);
//    ed_level_state.brush.bsp_polygons = ds_slist_create(sizeof(struct ed_bsp_polygon_t), 512);
    ed_level_state.brush.brushes = ds_slist_create(sizeof(struct ed_brush_t), 512);
    ed_level_state.brush.brush_edges = ds_slist_create(sizeof(struct ed_edge_t), 512);
    ed_level_state.brush.brush_faces = ds_slist_create(sizeof(struct ed_face_t), 512);
//    ed_level_state.brush.brush_face_polygons = ds_slist_create(sizeof(struct ed_face_polygon_t), 512);
    ed_level_state.brush.brush_materials = ds_slist_create(sizeof(struct ed_brush_material_t), 512);
    ed_level_state.brush.brush_verts = ds_slist_create(sizeof(struct ed_vert_t), 512);

//    ed_level_state.brush.polygon_buffer = ds_buffer_create(sizeof(struct ed_bsp_polygon_t *), 0);
    ed_level_state.brush.vertex_buffer = ds_buffer_create(sizeof(struct r_vert_t), 0);
    ed_level_state.brush.index_buffer = ds_buffer_create(sizeof(uint32_t), 0);
    ed_level_state.brush.batch_buffer = ds_buffer_create(sizeof(struct r_batch_t), 0);
    ed_level_state.brush.face_buffer = ds_buffer_create(sizeof(struct ed_face_t *), 0);

    ed_level_state.camera_pitch = ED_LEVEL_CAMERA_PITCH;
    ed_level_state.camera_yaw = ED_LEVEL_CAMERA_YAW;
    ed_level_state.camera_pos = ED_LEVEL_CAMERA_POS;

    struct r_shader_desc_t shader_desc;
    shader_desc = (struct r_shader_desc_t){
        .vertex_code = "shaders/ed_grid.vert",
        .fragment_code = "shaders/ed_grid.frag",
        .vertex_layout = &R_DEFAULT_VERTEX_LAYOUT,
        .uniforms = R_DEFAULT_UNIFORMS,
        .uniform_count = sizeof(R_DEFAULT_UNIFORMS) / sizeof(R_DEFAULT_UNIFORMS[0])
    };
    ed_center_grid_shader = r_LoadShader(&shader_desc);

//    shader_desc = (struct r_shader_desc_t){
//        .vertex_code = "shaders/ed_pick.vert",
//        .fragment_code = "shaders/ed_pick.frag",
//        .vertex_layout = &R_DEFAULT_VERTEX_LAYOUT,
//        .uniforms = R_DEFAULT_UNIFORMS,
//        .uniform_count = sizeof(R_DEFAULT_UNIFORMS) / sizeof(R_DEFAULT_UNIFORMS[0])
//    };
//    ed_picking_shader = r_LoadShader(&shader_desc);
//
//    shader_desc = (struct r_shader_desc_t){
//        .vertex_code = "shaders/ed_outline.vert",
//        .fragment_code = "shaders/ed_outline.frag",
//        .vertex_layout = &R_DEFAULT_VERTEX_LAYOUT,
//        .uniforms = R_DEFAULT_UNIFORMS,
//        .uniform_count = sizeof(R_DEFAULT_UNIFORMS) / sizeof(R_DEFAULT_UNIFORMS[0])
//    };
//    ed_outline_shader = r_LoadShader(&shader_desc);

    shader_desc = (struct r_shader_desc_t){
        .vertex_code = "shaders/ed_zero_depth.vert",
        .fragment_code = "shaders/ed_zero_depth.frag",
        .vertex_layout = &R_DEFAULT_VERTEX_LAYOUT,
        .uniforms = R_DEFAULT_UNIFORMS,
        .uniform_count = sizeof(R_DEFAULT_UNIFORMS) / sizeof(R_DEFAULT_UNIFORMS[0])
    };
    ed_zero_depth_shader = r_LoadShader(&shader_desc);

//    ed_center_grid_shader = r_LoadShader("shaders/ed_grid.vert", "shaders/ed_grid.frag");
//    ed_picking_shader = r_LoadShader("shaders/ed_pick.vert", "shaders/ed_pick.frag");
//    ed_outline_shader = r_LoadShader("shaders/ed_outline.vert", "shaders/ed_outline.frag");
//    ed_zero_depth_shader = r_LoadShader("shaders/ed_zero_depth.vert", "shaders/ed_zero_depth.frag");


//    ed_translation_widget_model = r_LoadModel("models/twidget.mof");
//    ed_rotation_widget_model = r_LoadModel("models/rwidget.mof");
    ed_ball_widget_model = r_LoadModel("models/bwidget.mof");

    mat4_t_identity(&ed_level_state.manipulator.transform);
    ed_level_state.manipulator.linear_snap = 0.25;
    ed_level_state.manipulator.transform_type = ED_L_TRANSFORM_TYPE_TRANSLATION;
//    ed_level_state.manipulator.transform_space = ED_L_TRANSFORM_SPACE_WORLD;
//    ed_level_state.manipulator.widgets[ED_L_TRANSFORM_TYPE_TRANSLATION] = ed_CreateWidget(NULL);
//    struct ed_widget_t *widget = ed_level_state.manipulator.widgets[ED_L_TRANSFORM_TYPE_TRANSLATION];
//    widget->setup_ds_fn = ed_w_ManipulatorWidgetSetupPickableDrawState;


    struct ds_buffer_t vert_buffer = ds_buffer_create(sizeof(struct r_vert_t), 1);
    struct ds_buffer_t index_buffer = ds_buffer_create(sizeof(uint32_t), 1);
    struct ds_buffer_t batch_buffer = ds_buffer_create(sizeof(struct r_batch_t), 1);

    ((struct r_vert_t *)vert_buffer.buffer)[0].pos = vec3_t_c(0.0, 0.0, 0.0);
    ((uint32_t *)index_buffer.buffer)[0] = 0;
    ((struct r_batch_t *)batch_buffer.buffer)[0] = (struct r_batch_t){.start = 0, .count = 0, .material = NULL};

    struct r_model_geometry_t geometry = {};

    geometry.batches = batch_buffer.buffer;
    geometry.batch_count = 1;
    geometry.verts = vert_buffer.buffer;
    geometry.vert_count = 1;
    geometry.indices = index_buffer.buffer;
    geometry.index_count = 1;

    ed_light_pickable_model = r_CreateModel(&geometry, NULL, "light_pickable");
    ds_buffer_destroy(&vert_buffer);
    ds_buffer_destroy(&index_buffer);
    ds_buffer_destroy(&batch_buffer);

    struct r_texture_t *diffuse;
    struct r_texture_t *normal;

    ed_l_transform_operator_transform_type_textures[ED_TRANSFORM_OPERATOR_TRANSFORM_TYPE_TRANSLATE] = r_LoadTexture("textures/move.png");
    ed_l_transform_operator_transform_type_textures[ED_TRANSFORM_OPERATOR_TRANSFORM_TYPE_ROTATE] = r_LoadTexture("textures/rotate.png");
    ed_l_transform_operator_transform_type_textures[ED_TRANSFORM_OPERATOR_TRANSFORM_TYPE_SCALE] = r_LoadTexture("textures/scale.png");
    ed_l_extrude_tool_texture = r_LoadTexture("textures/extrude.png");
    ed_l_shape_tool_texture = r_LoadTexture("textures/chainsaw.png");
    ed_l_entity_tab_texture = r_LoadTexture("textures/chair.png");


//    ed_CreateEntityPickable(g_ent_def, &vec3_t_c(0.0, 0.0, 0.0), &vec3_t_c(1.0, 1.0, 1.0), &mat3_t_c_id(), NULL);
//
//    diffuse = r_LoadTexture("textures/bathroomtile2-basecolor.png", "bathroomtile2_diffuse");
//    normal = r_LoadTexture("textures/bathroomtile2-normal.png", "bathroomtile2_normal");
//    r_CreateMaterial("bathroomtile2", diffuse, normal, NULL);
//
//    diffuse = r_LoadTexture("textures/oakfloor_basecolor.png", "oakfloor_diffuse");
//    normal = r_LoadTexture("textures/oakfloor_normal.png", "oakfloor_normal");
//    r_CreateMaterial("oakfloor", diffuse, normal, NULL);
//
//    diffuse = r_LoadTexture("textures/sci-fi-panel1-albedo.png", "scifi_diffuse");
//    normal = r_LoadTexture("textures/sci-fi-panel1-normal-ogl.png", "scifi_normal");
//    r_CreateMaterial("scifi", diffuse, normal, NULL);

//    struct p_capsule_shape_t *capsule_shape = p_CreateCapsuleCollisionShape(0.2, 1.0);
//    p_CreateCollider(P_COLLIDER_TYPE_MOVABLE, &vec3_t_c(0.0, 0.0, 0.0), NULL, capsule_shape);

//    mat3_t orientation;
//    mat3_t_identity(&orientation);
//    mat3_t_rotate_x(&orientation, -0.35);
//    r_CreateSpotLight(&vec3_t_c(0.0, 2.2, 0.0), &vec3_t_c(1.0, 1.0, 1.0), &orientation, 30.0, 5.0, 0.2, 0.1);
}

void ed_l_Shutdown()
{

}

void ed_l_Suspend()
{
    ed_l_SaveGameLevelSnapshot();
    ed_l_ClearBrushEntities();
    l_ClearLevel();
}

void ed_l_Resume()
{
    r_SetClearColor(0.05, 0.05, 0.05, 1.0);
    ed_l_LoadGameLevelSnapshot();
//    ed_l_RestoreBrushEntities();
}

/*
=============================================================
=============================================================
=============================================================
*/

//void ed_LevelEditorRotateSelected(mat3_t *rotation, vec3_t *pivot, uint32_t transform_mode)
//{
//    struct ds_list_t *selections = &ed_level_state.pickables.selections;
//
//    for(uint32_t selection_index = 0; selection_index < selections->cursor; selection_index++)
//    {
//        struct ed_pickable_t *pickable = *(struct ed_pickable_t **)ds_list_get_element(selections, selection_index);
//        pickable->rotation = *rotation;
//        pickable->transform_flags |= ED_PICKABLE_TRANSFORM_FLAG_ROTATION;
//
//        if(pivot)
//        {
//            vec3_t pivot_pickable_vec;
//            vec3_t_sub(&pivot_pickable_vec, &pickable->transform.rows[3].xyz, pivot);
//            mat3_t_vec3_t_mul(&pivot_pickable_vec, &pivot_pickable_vec, rotation);
//            vec3_t_add(&pivot_pickable_vec, &pivot_pickable_vec, pivot);
//            vec3_t_sub(&pickable->translation, &pivot_pickable_vec, &pickable->transform.rows[3].xyz);
//            pickable->transform_flags |= ED_PICKABLE_TRANSFORM_FLAG_TRANSLATION;
//        }
//
//        ed_w_MarkPickableModified(pickable);
//    }
//}

//void ed_w_MarkPickableModified(struct ed_pickable_t *pickable)
//{
//    if(pickable && pickable->modified_index == 0xffffffff)
//    {
//        pickable->modified_index = ds_list_add_element(&ed_level_state.pickables.modified_pickables, &pickable);
//    }
//}

//void ed_w_MarkBrushModified(struct ed_brush_t *brush)
//{
//    if(brush && brush->modified_index == 0xffffffff)
//    {
//        brush->modified_index = ds_list_add_element(&ed_level_state.pickables.modified_brushes, &brush);
//    }
//}

/*
=============================================================
=============================================================
=============================================================
*/

#define ED_L_TOP_TABS_HEIGHT 22
#define ED_L_FOOTER_HEIGHT 24

void ed_l_UpdateUI()
{
    int32_t mouse_x;
    int32_t mouse_y;

    in_GetMousePos(&mouse_x, &mouse_y);

//    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize;
//    window_flags |= ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar;

    igSetNextWindowSize((ImVec2){r_width, r_height}, 0);
    igSetNextWindowPos((ImVec2){0, 0}, 0, (ImVec2){0, 0});
    ImGuiWindowFlags level_editor_docking_flags = ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoInputs;
    level_editor_docking_flags |= ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar;
    igPushStyleVar_Vec2(ImGuiStyleVar_FramePadding, (ImVec2){4, 0});
    igPushStyleVar_Vec2(ImGuiStyleVar_WindowPadding, (ImVec2){1, 0});
    igBegin("##level_editor_docking", NULL, level_editor_docking_flags);
    igPopStyleVar(2);

    igPushStyleVar_Vec2(ImGuiStyleVar_FramePadding, (ImVec2){4, 5});
    igPushStyleVar_Vec2(ImGuiStyleVar_WindowPadding, (ImVec2){1, 0});
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysUseWindowPadding;
    igSetNextWindowBgAlpha(0.5);
    igSetNextWindowPos((ImVec2){0, 16}, 0, (ImVec2){0, 0});
    uint32_t top_bar_visible = igBeginChild_Str("##top_bar", (ImVec2){r_width, ED_L_TOP_TABS_HEIGHT}, 0, ImGuiWindowFlags_NoScrollbar);

    if(top_bar_visible)
    {
        igPushStyleVar_Float(ImGuiStyleVar_TabRounding, 0.0);
        igPushStyleColor_Vec4(ImGuiCol_Tab, (ImVec4){0.2, 0.2, 0.2, 1.0});
        igPushStyleColor_Vec4(ImGuiCol_TabActive, (ImVec4){0.8, 0.8, 0.8, 1.0});

        igAlignTextToFramePadding();
        igText("Brush sel:");

        ImGuiTabItemFlags tab_flags[3] = {0, 0, 0};

        if(ed_l_obj_placement_state.brush.prev_brush_element != ed_l_obj_placement_state.brush.brush_element)
        {
            tab_flags[ed_l_obj_placement_state.brush.brush_element] = ImGuiTabItemFlags_SetSelected;
        }

        igSameLine(0, -1);
        if(igBeginTabBar("##brush_element_tab_bar", 0))
        {
            for(uint32_t tab = ED_BRUSH_ELEMENT_FACE; tab < ED_BRUSH_ELEMENT_BODY; tab++)
            {
                uint32_t selected = ed_l_obj_placement_state.brush.brush_element == tab;

                if(selected)
                {
                    igPushStyleColor_Vec4(ImGuiCol_Text, (ImVec4){0.1, 0.1, 0.1, 1.0});
                }

                char tab_text[64];
                sprintf(tab_text, "%s(%d)", ed_brush_element_names[tab], tab + 1);

                if(igBeginTabItem(tab_text, NULL, tab_flags[tab]))
                {
                    ed_l_obj_placement_state.brush.brush_element = tab;
                    ed_l_obj_placement_state.brush.prev_brush_element = ed_l_obj_placement_state.brush.brush_element;
                    igEndTabItem();
                }

                if(selected)
                {
                    igPopStyleColor(1);
                }
            }
            igEndTabBar();
        }

        igSameLine(0, -1);
        igSeparatorEx(ImGuiSeparatorFlags_Vertical);
        igSameLine(0, -1);

        if(igBeginTabBar("##top_tab_bar", 0))
        {
            for(uint32_t tab = ED_L_MAIN_TOOL_TAB_OBJECT; tab < ED_L_MAIN_TOOL_TAB_LAST; tab++)
            {
                uint32_t change_color = ed_l_active_main_tool_tab == tab;
                if(change_color)
                {
                    igPushStyleColor_Vec4(ImGuiCol_Text, (ImVec4){0.1, 0.1, 0.1, 1.0});
                }

                if(igBeginTabItem(ed_l_main_tool_tab_names[tab], NULL, 0))
                {
                    ed_l_active_main_tool_tab = tab;
                    igEndTabItem();
                }

                if(change_color)
                {
                    igPopStyleColor(1);
                }
            }
            igEndTabBar();
        }
        igPopStyleVar(1);
        igPopStyleColor(2);
    }
    igEndChild();
    ImGuiStyle *style = igGetStyle();
    float start_cursor_y = igGetCursorPosY() - style->WindowPadding.y * 0.5 - style->FramePadding.y;

    igSetNextWindowBgAlpha(0.5);
    igSetNextWindowPos((ImVec2){0, start_cursor_y}, 0, (ImVec2){0, 0});
//    igPushStyleVar_Vec2(ImGuiStyleVar_WindowPadding, (ImVec2){0, 0});
//    igPushStyleVar_Vec2(ImGuiStyleVar_FramePadding, (ImVec2){0, 0});

    if(igBeginChild_Str("##left_side_tab", (ImVec2){34, 0}, 0, 0))
    {
        igSeparator();
        igDummy((ImVec2){1, 2});

        igBeginGroup();
        igPushStyleVar_Vec2(ImGuiStyleVar_WindowPadding, (ImVec2){2, 2});
        igPushStyleVar_Float(ImGuiStyleVar_ChildRounding, 4.0);

        if(igBeginChild_Str("##extrude", (ImVec2){34, 34}, 1, 0))
        {
            float cursor_y = igGetCursorPosY();
            if(igSelectable_Bool("", 0, 0, (ImVec2){30, 30}))
            {

            }
            igSetCursorPosY(cursor_y);
            igImage(ed_l_extrude_tool_texture, (ImVec2){30, 30}, (ImVec2){0, 0}, (ImVec2){1, 1}, (ImVec4){1, 1, 1, 1}, (ImVec4){0, 0, 0, 0});
            if(igIsItemHovered(0))
            {
                igBeginTooltip();
                igTextUnformatted("Extrude tool. Shortcut: E", NULL);
                igEndTooltip();
            }
        }
        igEndChild();

        if(igBeginChild_Str("##shape", (ImVec2){34, 34}, 1, 0))
        {
            float cursor_y = igGetCursorPosY();
            if(igSelectable_Bool("", 0, 0, (ImVec2){30, 30}))
            {

            }
            igSetCursorPosY(cursor_y);
            igImage(ed_l_shape_tool_texture, (ImVec2){30, 30}, (ImVec2){0, 0}, (ImVec2){1, 1}, (ImVec4){1, 1, 1, 1}, (ImVec4){0, 0, 0, 0});
            if(igIsItemHovered(0))
            {
                igBeginTooltip();
                igTextUnformatted("Shaping tool. Shortcut: C", NULL);
                igEndTooltip();
            }
        }
        igEndChild();

        igEndGroup();
        igPopStyleVar(2);

        igSeparator();
        igDummy((ImVec2){1, 2});
        igPushStyleColor_Vec4(ImGuiCol_Header, (ImVec4){0.8, 0.8, 0.8, 1.0});
//        igPushStyleColor_Vec4(ImGuiCol_Border, (ImVec4){0.8, 0.0, 0.0, 1.0});
        igPushStyleVar_Vec2(ImGuiStyleVar_SelectableTextAlign, (ImVec2){0.5, 0.5});

        igIndent(4);
        for(uint32_t mode = ED_TRANSFORM_OPERATOR_TRANSFORM_MODE_WORLD; mode < ED_TRANSFORM_OPERATOR_TRANSFORM_MODE_LAST; mode++)
        {
            char label[2];
            label[0] = ed_transform_operator_transform_mode_names[mode][0];
            label[1] = '\0';
            uint32_t selected = ed_level_state.obj.transform_operator_data.transform_mode == mode;
            if(selected)
            {
                igPushStyleColor_Vec4(ImGuiCol_Text, (ImVec4){0.1, 0.1, 0.1, 1.0});
            }

            if(igSelectable_Bool(label, selected, 0, (ImVec2){8, 0}))
            {
                ed_level_state.obj.transform_operator_data.transform_mode = mode;
                ed_UpdateOperators(&ed_level_state.obj.objects);
            }

            if(selected)
            {
                igPopStyleColor(1);
            }

            igSameLine(0, -1);
        }

        igUnindent(4);
        igNewLine();
        igDummy((ImVec2){1, 2});

        igBeginGroup();
        igPushStyleVar_Vec2(ImGuiStyleVar_WindowPadding, (ImVec2){2, 2});
        igPushStyleVar_Float(ImGuiStyleVar_ChildRounding, 4.0);
        for(uint32_t type = ED_TRANSFORM_OPERATOR_TRANSFORM_TYPE_TRANSLATE; type < ED_TRANSFORM_OPERATOR_TRANSFORM_TYPE_LAST; type++)
        {
            char label[2];
            label[0] = ed_transform_operator_transform_type_names[type][0];
            label[1] = '\0';
            uint32_t selected = ed_level_state.obj.transform_operator_data.transform_type == type;
            ImVec4 tint_color = (ImVec4){1, 1, 1, 1};

            if(selected)
            {
                tint_color = (ImVec4){0, 0, 0, 1};
            }

            igPushID_Int(type);
            igBeginChild_Str("##border", (ImVec2){34, 34}, 1, ImGuiWindowFlags_NoScrollbar);
            float cursor_y = igGetCursorPosY();
            if(igSelectable_Bool("", selected, 0, (ImVec2){30, 30}))
            {
                ed_level_state.obj.transform_operator_data.transform_type = type;
            }
            igSetCursorPosY(cursor_y);
            igImage(ed_l_transform_operator_transform_type_textures[type], (ImVec2){30, 30}, (ImVec2){0, 0}, (ImVec2){1, 1}, tint_color, (ImVec4){0, 0, 0, 0});
            if(igIsItemHovered(0))
            {
                igBeginTooltip();
                igTextUnformatted(ed_l_transform_operator_transform_type_tooltips[type], NULL);
                igEndTooltip();
            }
            igEndChild();
            igPopID();
//            igDummy((ImVec2){1, 1});
        }
        igEndGroup();

        igSeparator();

        igPopStyleColor(1);
        igPopStyleVar(3);
    }
    igEndChild();

    igPopStyleVar(2);



//
//    window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoCollapse;
//    igSetNextWindowBgAlpha(0.5);
////    igPushStyleVar_Float(ImGuiStyleVar_WindowBorderSize, 0.0);
////    igPushStyleVar_Vec2(ImGuiStyleVar_FramePadding, (ImVec2){0, 0});
//    igSetNextWindowPos((ImVec2){0, r_height}, 0, (ImVec2){0, 1});
//    if(igBeginChild_Str("##footer", (ImVec2){r_width, ED_L_FOOTER_HEIGHT}, 0, 0))
//    {
//
////        if(igBeginChild_Str("left_side", (ImVec2){0, 24}, 0, 0))
////        {
//            igPushStyleVar_Float(ImGuiStyleVar_Alpha, 0.5);
//
//            if(igBeginTable("Stats table", 3, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_Hideable, (ImVec2){0.0, 0.0}, 0.0))
//            {
//                igTableNextRow(0, 0.0);
//
//                uint32_t transform_type = ed_level_state.manipulator.transform_type;
//                uint32_t transform_mode = ed_level_state.manipulator.transform_mode;
//
//                igTableNextColumn();
//                igText("Manipulator: %s", ed_l_transform_type_texts[transform_type]);
//                igSameLine(0, -1);
//                if(igBeginCombo("##tranfsorm_mode", ed_l_transform_mode_texts[transform_mode], 0))
//                {
//                    for(uint32_t mode = ED_L_TRANSFORM_MODE_WORLD; mode < ED_L_TRANSFORM_MODE_LAST; mode++)
//                    {
//                        if(igSelectable_Bool(ed_l_transform_mode_texts[mode], 0, 0, (ImVec2){0, 0}))
//                        {
//                            ed_level_state.manipulator.transform_mode = mode;
//                        }
//                    }
//                    igEndCombo();
//                }
//                igTableNextColumn();
//
//                char snap_label[32];
//                char snap_preview[32];
//                igText("Snap: ");
//                igSameLine(0.0, -1.0);
////                if(ed_level_state.manipulator.transform_type == ED_L_TRANSFORM_TYPE_ROTATION)
//                struct ed_transform_operator_data_t *operator_data = ed_level_state.obj.objects.operators[ED_OPERATOR_TRANSFORM].data;
//                if(operator_data->transform_type == ED_TRANSFORM_OPERATOR_TRANSFORM_TYPE_ROTATE)
//                {
//                    sprintf(snap_preview, "%0.4f deg", ed_level_state.manipulator.angular_snap * 180.0);
//                    igSetNextItemWidth(120.0);
//                    if(igBeginCombo("##angular_snap", snap_preview, 0))
//                    {
//                        for(uint32_t index = 0; index < sizeof(ed_w_angular_snap_values) / sizeof(ed_w_angular_snap_values[0]); index++)
//                        {
//                            sprintf(snap_label, "%f", ed_w_angular_snap_values[index] * 180.0);
//
//                            if(igSelectable_Bool(snap_label, 0, 0, (ImVec2){0.0, 0.0}))
//                            {
//                                ed_level_state.manipulator.angular_snap = ed_w_angular_snap_values[index];
//                            }
//                        }
//
//                        igEndCombo();
//                    }
//                }
//                else
//                {
//                    sprintf(snap_preview, "%0.4f m", ed_level_state.obj.transform_operator_data.linear_snap);
//                    igSetNextItemWidth(120.0);
//                    if(igBeginCombo("##linear_snap", snap_preview, 0))
//                    {
//                        for(uint32_t index = 0; index < sizeof(ed_w_linear_snap_values) / sizeof(ed_w_linear_snap_values[0]); index++)
//                        {
//                            sprintf(snap_label, "%f", ed_w_linear_snap_values[index]);
//
//                            if(igSelectable_Bool(snap_label, 0, 0, (ImVec2){0.0, 0.0}))
//                            {
//                                ed_level_state.obj.transform_operator_data.linear_snap = ed_w_linear_snap_values[index];
//                            }
//                        }
//
//                        igEndCombo();
//                    }
//                }
//
//                igTableNextColumn();
//
//                igEndTable();
//            }
//            igPopStyleVar(1);
//    }
//    igEndChild();
//    igPopStyleVar(1);

//    float end_cursor_y = igGetCursorPosY();
    igSetCursorPosY(start_cursor_y);
//
//
    ed_l_root_docking_id = igGetID_Str("level_editor_docking");
    float size = r_height - start_cursor_y - style->WindowPadding.y * 0.5;

    if(igDockBuilderGetNode(ed_l_root_docking_id) == NULL)
    {
        igDockBuilderAddNode(ed_l_root_docking_id, ImGuiDockNodeFlags_DockSpace | ImGuiWindowFlags_NoBackground);
        igDockBuilderSetNodeSize(ed_l_root_docking_id, (ImVec2){r_width, size});
        igDockBuilderSetNodePos(ed_l_root_docking_id, (ImVec2){0, start_cursor_y});

        ImGuiID side_a_docking_id;
        ImGuiID side_b_docking_id;

        igDockBuilderSplitNode(ed_l_root_docking_id, ImGuiDir_Right, 0.3, &side_a_docking_id, &side_b_docking_id);

        igDockBuilderSplitNode(side_a_docking_id, ImGuiDir_Up, 0.5, &side_a_docking_id, &side_b_docking_id);
        igDockBuilderDockWindow("##selected_top_tab", side_a_docking_id);
        igDockBuilderDockWindow("Selections", side_b_docking_id);
    }
//
    igDockSpace(ed_l_root_docking_id, (ImVec2){0, size}, ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_AutoHideTabBar, NULL);
//
    igEnd();


    window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground;
//
    if(!ed_l_tool_tab_passthrough)
    {
//        window_flags |=  ImGuiWindowFlags_NoInputs;
        if(igBegin("##selected_top_tab", NULL, window_flags))
        {
            switch(ed_l_active_main_tool_tab)
            {
                case ED_L_MAIN_TOOL_TAB_OBJECT:
                    ed_l_ObjectUI();
                break;

                case ED_L_MAIN_TOOL_TAB_MATERIAL:
                    ed_l_MaterialUI();
                break;
            }
        }
        igEnd();

    //    igSetNextWindowDockID(ed_l_tool_tab_selections_docking_id, 0);
        ImGuiWindowFlags selection_window_flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground;
        if(igBegin("Selections", NULL, selection_window_flags))
        {
            struct ds_list_t *selections = &ed_level_state.obj.objects.selections;
            igText("Selections: %d", selections->cursor);
            igSeparator();
            for(uint32_t selection_index = 0; selection_index < selections->cursor; selection_index++)
            {
                struct ed_obj_result_t *result = ds_list_get_element(selections, selection_index);
                igPushID_Int(selection_index);
                if(igCollapsingHeader_BoolPtr(ed_obj_names[result->object->type], NULL, 0))
                {
                    vec3_t position = result->object->transform.rows[3].xyz;
                    igPushID_Int(selection_index);
                    if(igDragFloat3("Position", position.comps, 1.0, -FLT_MAX, FLT_MAX, "%f", 0))
                    {
                        vec3_t disp;
                        vec3_t_sub(&disp, &position, &result->object->transform.rows[3].xyz);
                        ed_TranslateSelection(&ed_level_state.obj.objects, &disp, selection_index);
                    }
                    igPopID();

                    switch(result->object->type)
                    {
                        case ED_OBJ_TYPE_LIGHT:
                        {
                            struct r_light_t *light = result->object->base_obj;

                            struct ed_light_args_t args = {
                                .type = light->type,
                                .range = light->range,
                                .color = light->color,
                                .energy = light->energy
                            };

//                            if(light->type == R_LIGHT_TYPE_SPOT)
//                            {
//                                struct r_spot_light_t *spot_light = (struct r_spot_light_t *)light;
//                                args.inner_angle = spot_light->
//                            }

                            ed_l_LightUI(&args);

                            light->color = args.color;
                            light->range = args.range;
                            light->energy = args.energy;
                        }
                        break;
                    }

                }
                igPopID();
            }
        }
        igEnd();
    }



//    if(ed_l_tool_tab_passthrough)
//    {
//        igPopStyleVar(1);
//    }

}

void ed_l_LightUI(struct ed_light_args_t *light_args)
{
    if(igBeginCombo("Type", r_light_type_names[light_args->type], 0))
    {
        for(uint32_t type = R_LIGHT_TYPE_POINT; type < R_LIGHT_TYPE_LAST; type++)
        {
            if(igSelectable_Bool(r_light_type_names[type], light_args->type == type, 0, (ImVec2){0, 0}))
            {
                light_args->type = type;
            }
        }
        igEndCombo();
    }
    vec3_t color = light_args->color;
    if(igDragFloat3("Color", color.comps, 0.005, 0.0, 1.0, "%.03f", 0))
    {
        light_args->color.x = CLAMPF(color.x, 0.0, 1.0);
        light_args->color.y = CLAMPF(color.y, 0.0, 1.0);
        light_args->color.z = CLAMPF(color.z, 0.0, 1.0);
    }

    float energy = light_args->energy;

    if(igDragFloat("Energy", &energy, 0.005, 0.0, R_LIGHT_MAX_ENERGY, "%.03f", 0))
    {
        light_args->energy = CLAMPF(energy, 0.0, R_LIGHT_MAX_ENERGY);
    }

    igSeparator();

    float range = light_args->range;

    switch(light_args->type)
    {
        case R_LIGHT_TYPE_POINT:
        {
            igDragFloat("Radius", &range, 0.05, 0.0, R_LIGHT_MAX_RANGE, "%.02f", 0);
        }
        break;

        case R_LIGHT_TYPE_SPOT:
        {
            igDragFloat("Range", &range, 0.05, 0.0, R_LIGHT_MAX_RANGE, "%.02f", 0);

            float outer_angle = light_args->outer_angle;
            float inner_angle = light_args->inner_angle;

            igDragFloat("Outer angle", &outer_angle, 0.1, R_SPOT_LIGHT_MIN_ANGLE, R_SPOT_LIGHT_MAX_ANGLE, "%.02f", 0);
            igDragFloat("Inner angle", &inner_angle, 0.1, R_SPOT_LIGHT_MIN_ANGLE, R_SPOT_LIGHT_MAX_ANGLE, "%.02f", 0);
            light_args->outer_angle = CLAMPF(outer_angle, R_SPOT_LIGHT_MIN_ANGLE, R_SPOT_LIGHT_MAX_ANGLE);
            light_args->inner_angle = CLAMPF(inner_angle, R_SPOT_LIGHT_MIN_ANGLE, R_SPOT_LIGHT_MAX_ANGLE);

        }
        break;
    }

    light_args->range = CLAMPF(range, 0.0, R_LIGHT_MAX_RANGE);
}

void ed_l_ObjectUI()
{
    if(igBeginChild_Str("##object_window", (ImVec2){0, 0}, 0, 0))
    {
        igPushStyleVar_Vec2(ImGuiStyleVar_FramePadding, (ImVec2){2, 2});
        igPushStyleVar_Float(ImGuiStyleVar_TabRounding, 0);

        if(igBeginTabBar("##object_type", 0))
        {
            for(uint32_t tab = ED_L_OBJ_TAB_BRUSH; tab < ED_L_OBJ_TAB_LAST; tab++)
            {
                if(igBeginTabItem(ed_l_obj_tab_names[tab], NULL, 0))
                {
                    ed_l_obj_placement_state.active_tab = tab;

                    if(igBeginChild_Str("##object_list", (ImVec2){0, 0}, 0, 0))
                    {
                        switch(tab)
                        {
                            case ED_L_OBJ_TAB_ENTITY:
                            {
//                                ImVec2 region_size;
//                                ImVec2 item_size = (ImVec2){60, 60};
//                                ImGuiStyle *style = igGetStyle();
//                                igGetContentRegionAvail(&region_size);
//                                float cur_cursor_x = 0;
                                for(uint32_t def_index = 0; def_index < e_ent_defs[E_ENT_DEF_TYPE_ROOT].cursor; def_index++)
                                {
                                    struct e_ent_def_t *def = e_GetEntDef(E_ENT_DEF_TYPE_ROOT, def_index);

                                    if(def != NULL)
                                    {
                                        ImVec2 cursor_pos;
                                        igPushID_Int(def_index);
                                        igTextUnformatted(def->name, NULL);
                                        igGetCursorPos(&cursor_pos);
//                                        igBeginGroup();

                                        if(igSelectable_Bool("##item", ed_l_obj_placement_state.entity.selected_def == def, 0, (ImVec2){0, 64}))
                                        {
                                            ed_l_obj_placement_state.entity.selected_def = def;
                                        }
//                                        igSetCursorPosX(cursor_pos.x);
                                        igSetCursorPos(cursor_pos);
                                        igImage(r_default_albedo_texture, (ImVec2){64, 64}, (ImVec2){0, 0}, (ImVec2){1, 1}, (ImVec4){1, 1, 1, 1}, (ImVec4){0, 0, 0, 0});
                                        igSameLine(0, -1);
                                        igBeginGroup();
                                        igText("Nodes: %d", def->node_count);
                                        igText("Instances: %d", def->ref_count);
                                        igText("Collides: %s", def->collider_count ? "Yes" : "No");
                                        if(def->collider_count)
                                        {
                                            igText("Collider type: %s", p_col_type_names[def->collider.type]);
                                        }
                                        igEndGroup();
                                        igPopID();
                //                            ImVec2 current_cursor;
                //                            igGetCursorPos(&current_cursor);
//                                        float next_cursor_x = cur_cursor_x + style->ItemSpacing.x + item_size.x;
//
//                                        if(next_cursor_x < region_size.x)
//                                        {
//                                            igSameLine(0.0, -1.0);
//                                        }
//                                        else
//                                        {
//                                            next_cursor_x -= cur_cursor_x;
//                                        }
//
//                                        cur_cursor_x = next_cursor_x;
//
//                                        igPushID_Int(def_index);
//                                        igBeginGroup();
//                                        if(igSelectable_Bool("##item", 0, 0, item_size))
//                                        {
//
//                                        }
//                                        igTextUnformatted(def->name, NULL);
//                                        igEndGroup();
//                                        igPopID();
                                    }
                                }
                            }
                            break;

//                            case ED_L_OBJ_TAB_BRUSH:
//                            {
//                                for(uint32_t index = ED_BRUSH_ELEMENT_FACE; index < ED_BRUSH_ELEMENT_BODY; index++)
//                                {
//                                    if(igSelectable_Bool(ed_brush_element_names[index], index == ed_l_obj_placement_state.brush.brush_element, 0, (ImVec2){8, 0}))
//                                    {
//                                        ed_l_obj_placement_state.brush.brush_element = index;
//                                    }
//
//                                    igSameLine(0, -1);
//                                }
//                            }
//                            break;

                            case ED_L_OBJ_TAB_LIGHT:
                                ed_l_LightUI(&ed_l_obj_placement_state.light.args);
                            break;
                        }
                    }
                    igEndChild();
                    igEndTabItem();
                }
            }

            igEndTabBar();
        }
        igPopStyleVar(2);
    }
    igEndChild();
//    igPopStyleVar(1);
}

void ed_l_MaterialUI()
{
    if(igBeginChild_Str("##materials", (ImVec2){0, 0}, 0, 0))
    {
        char *preview = "Select...";

        if(ed_l_selected_material != NULL)
        {
            preview = ed_l_selected_material->name;
        }

        if(igBeginCombo("Material", preview, 0))
        {
            for(uint32_t material_index = 0; material_index < r_materials.cursor; material_index++)
            {
                struct r_material_t *material = r_GetMaterial(material_index);
                if(material != NULL)
                {
                    bool selected = material == ed_l_selected_material;
                    if(igSelectable_Bool(material->name, selected, 0, (ImVec2){0, 0}))
                    {
                        ed_l_selected_material = material;
                    }
                }
            }

            igEndCombo();
        }

        if(ed_l_selected_material != NULL)
        {
            char *preview = "Select";
            if(ed_l_selected_material->diffuse_texture != NULL)
            {
                preview = ed_l_selected_material->diffuse_texture->name;
            }

            ImGuiStyle *style = igGetStyle();

            if(igBeginCombo("Diffuse", preview, 0))
            {
                for(uint32_t texture_index = 0; texture_index < r_textures.cursor; texture_index++)
                {
                    struct r_texture_t *texture = r_GetTexture(texture_index);

                    if(texture != NULL)
                    {
                        bool selected = texture == ed_l_selected_material->diffuse_texture;
                        igPushID_Ptr(texture);
                        igBeginGroup();
                        float cursor_y = igGetCursorPosY();
                        if(igSelectable_Bool("##texture", selected, 0, (ImVec2){64, 64}))
                        {

                        }
                        igSetCursorPosY(cursor_y);
                        igImage(texture, (ImVec2){64 - style->ItemInnerSpacing.x, 64 - style->ItemInnerSpacing.y}, (ImVec2){0, 0}, (ImVec2){1, 1}, (ImVec4){1, 1, 1, 1}, (ImVec4){0, 0, 0, 0});
                        igTextUnformatted(texture->name, NULL);
                        igEndGroup();
                        igPopID();
                    }
                }

                igEndCombo();
            }
        }
    }
    igEndChild();
}

//struct ed_pickable_range_t *ed_UpdateEntityPickableRanges(struct ed_pickable_t *pickable, struct e_entity_t *entity, mat4_t *parent_transform, struct ed_pickable_range_t **cur_range)
//{
//    struct e_node_t *node = entity->node;
//    struct e_model_t *model = entity->model;
//
//    struct ed_pickable_range_t *range = *cur_range;
//    pickable->range_count++;
//
//    if(!range)
//    {
//        range = ed_AllocPickableRange();
//    }
//
//    if(!parent_transform)
//    {
//        range->offset = mat4_t_c_id();
//    }
//    else
//    {
//        mat4_t scale = mat4_t_c_id();
//
//        scale.rows[0].x = node->scale.x;
//        scale.rows[1].y = node->scale.y;
//        scale.rows[2].z = node->scale.z;
//
//        mat4_t_comp(&range->offset, &node->orientation, &node->position);
//        mat4_t_mul(&range->offset, &range->offset, parent_transform);
//        mat4_t_mul(&range->offset, &scale, &range->offset);
//    }
//
//    range->start = model->model->model_start;
//    range->count = model->model->model_count;
//
//    *cur_range = range;
//    struct e_node_t *child = node->children;
//
//    if(child)
//    {
//        struct ed_pickable_range_t *next_range = ed_UpdateEntityPickableRanges(pickable, child->entity, &range->offset, &(*cur_range)->next);
//        next_range->prev = range;
//        child = child->next;
//
//        while(child)
//        {
//            ed_UpdateEntityPickableRanges(pickable, child->entity, &range->offset, &(*cur_range)->next);
//            child = child->next;
//        }
//    }
//
//    return range;
//}

//void ed_w_UpdatePickableObjects()
//{
//    struct ds_list_t *pickables = &ed_level_state.pickables.modified_pickables;
////    struct e_entity_t *entity;
//
//    uint32_t pickable_index = 0;
//    uint32_t pickable_count = pickables->cursor;
//
//    /* here we iterate over pickables marked as modified, so we can update the objects they reference. During
//    update, it may be necessary to mark additional pickables as modified. This currently happens only with
//    pickables that reference brushes, and is particularly noticeable in the case when a pickable that references
//    a brush face/edge/vertex gets modified by the user. Those directly modified pickables will be process twice
//    by the loop -- first, their transforms will be applied to brush geometry; then, their transforms will be
//    recomputed based on the up to date brush geometry. */
//
//    do
//    {
//        for(; pickable_index < pickable_count; pickable_index++)
//        {
//            struct ed_pickable_t *pickable = *(struct ed_pickable_t **)ds_list_get_element(pickables, pickable_index);
//
//            switch(pickable->type)
//            {
//                case ED_PICKABLE_TYPE_BRUSH:
//                {
//                    struct ed_brush_t *brush = ed_GetBrush(pickable->primary_index);
//                    struct r_batch_t *first_batch = brush->model->batches.buffer;
//                    pickable->ranges->start = first_batch->start;
//                    pickable->ranges->count = brush->model->indices.buffer_size;
//
//                    ed_level_state.world_data_stale = 1;
//
//                    if(pickable->transform_flags)
//                    {
//                        /* this pickable got directly modified by the user, so we'll apply the
//                        transforms here */
//                        if(pickable->transform_flags & ED_PICKABLE_TRANSFORM_FLAG_TRANSLATION)
//                        {
//                            vec3_t_add(&brush->position, &brush->position, &pickable->translation);
//                        }
//
//                        if(pickable->transform_flags & ED_PICKABLE_TRANSFORM_FLAG_ROTATION)
//                        {
//                            mat3_t_mul(&brush->orientation, &brush->orientation, &pickable->rotation);
//                        }
//
//                        brush->update_flags |= ED_BRUSH_UPDATE_FLAG_UV_COORDS;
//
//                        /* update brush to update its entity transform */
//                        ed_w_MarkBrushModified(brush);
//                    }
//
//                    struct ed_face_t *face = brush->faces;
//
//                    while(face)
//                    {
//                        /* face/edge/vertice pickables also depend on the up to date brush transform,
//                        so mark those as modified so they can have their transforms updated as well */
//                        ed_w_MarkPickableModified(face->pickable);
//
//                        struct ed_face_polygon_t *polygon = face->polygons;
//
//                        while(polygon)
//                        {
//
//                            struct ed_edge_t *edge = polygon->edges;
//
//                            while(edge)
//                            {
//                                uint32_t polygon_side = edge->polygons[1].polygon == polygon;
//                                ed_w_MarkPickableModified(edge->pickable);
//                                edge = edge->polygons[polygon_side].next;
//                            }
//
//                            polygon = polygon->next;
//                        }
//
//                        face = face->next;
//                    }
//
//                    mat4_t_comp(&pickable->transform, &brush->orientation, &brush->position);
////                    pickable->draw_transform = pickable->transform;
//                }
//                break;
//
//                case ED_PICKABLE_TYPE_FACE:
//                {
//                    struct ed_brush_t *brush = ed_GetBrush(pickable->primary_index);
//                    struct ed_face_t *face = ed_GetFace(pickable->secondary_index);
//                    struct r_batch_t *first_batch = brush->model->batches.buffer;
//
//                    if(pickable->range_count > face->clipped_polygon_count)
//                    {
//                        while(pickable->range_count > face->clipped_polygon_count)
//                        {
//                            struct ed_pickable_range_t *next_range = pickable->ranges->next;
//                            next_range->prev = NULL;
//
//                            ed_FreePickableRange(pickable->ranges);
//                            pickable->range_count--;
//                            pickable->ranges = next_range;
//                        }
//                    }
//                    else if(pickable->range_count < face->clipped_polygon_count)
//                    {
//                        while(pickable->range_count < face->clipped_polygon_count)
//                        {
//                            struct ed_pickable_range_t *new_range = ed_AllocPickableRange();
//                            new_range->next = pickable->ranges;
//                            if(pickable->ranges)
//                            {
//                                pickable->ranges->prev = new_range;
//                            }
//                            pickable->ranges = new_range;
//                            pickable->range_count++;
//                        }
//                    }
//
//                    struct ed_bsp_polygon_t *polygon = face->clipped_polygons;
//                    struct ed_pickable_range_t *range = pickable->ranges;
//
//                    while(polygon)
//                    {
//                        range->start = polygon->model_start + first_batch->start;
//                        range->count = polygon->model_count;
//
//                        polygon = polygon->next;
//                        range = range->next;
//                    }
//
//                    if(pickable->transform_flags)
//                    {
//                        /* this pickable was directly modified by the user, so we'll apply the transforms
//                        here*/
//
//                        if(pickable->transform_flags & ED_PICKABLE_TRANSFORM_FLAG_TRANSLATION)
//                        {
//                            ed_TranslateBrushFace(brush, pickable->secondary_index, &pickable->translation);
//                        }
//
//                        if(pickable->transform_flags & ED_PICKABLE_TRANSFORM_FLAG_ROTATION)
//                        {
//                            ed_RotateBrushFace(brush, pickable->secondary_index, &pickable->rotation);
//                        }
//
//                        /* we're modifying brush geometry here, so mark the brush as modified so things
//                        like face polygon normals/centers/uvs/mesh can be recomputed */
//                        ed_w_MarkBrushModified(brush);
//
//                        /* the brush pickable position depends on the position of its vertices (the brush origin
//                        needs to be recomputed every time some vertice gets moved), so mark it as modified,
//                        so it can have its transform recomputed */
//                        ed_w_MarkPickableModified(brush->pickable);
//                    }
//                    else
//                    {
//                        /* this pickable wasn't modified directly, but was marked as modified because its transform
//                        was affected by some brush geometry/transform change */
//                        vec3_t face_position = face->center;
//                        mat3_t_vec3_t_mul(&face_position, &face_position, &brush->orientation);
//
//                        mat4_t_comp(&pickable->transform, &brush->orientation, &brush->position);
//                        vec3_t_add(&pickable->transform.rows[3].xyz, &pickable->transform.rows[3].xyz, &face_position);
//
//                        mat4_t draw_offset;
//                        mat4_t_identity(&draw_offset);
//                        vec3_t_mul(&draw_offset.rows[3].xyz, &face->center, -1.0);
////                        mat4_t_mul(&draw_offset, &draw_offset, &pickable->transform);
//
//                        struct ed_pickable_range_t *range = pickable->ranges;
//
//                        while(range)
//                        {
//                            range->offset = draw_offset;
//                            range = range->next;
//                        }
//                    }
//                }
//                break;
//
//                case ED_PICKABLE_TYPE_EDGE:
//                {
//                    struct ed_brush_t *brush = ed_GetBrush(pickable->primary_index);
//                    struct ed_edge_t *edge = ed_GetEdge(pickable->secondary_index);
//                    struct r_model_t *model = brush->model;
//                    struct r_batch_t *first_batch = model->batches.buffer;
//
//                    if(!pickable->range_count)
//                    {
//                        pickable->ranges = ed_AllocPickableRange();
//                        pickable->range_count = 1;
//                    }
//
//                    pickable->ranges->start = first_batch->start + edge->model_start;
//                    pickable->ranges->count = 2;
//
//                    if(pickable->transform_flags)
//                    {
//                        if(pickable->transform_flags & ED_PICKABLE_TRANSFORM_FLAG_TRANSLATION)
//                        {
//                            ed_TranslateBrushEdge(brush, pickable->secondary_index, &pickable->translation);
//                        }
//
//                        ed_w_MarkBrushModified(brush);
//
//                        ed_w_MarkPickableModified(brush->pickable);
//                    }
//                    else
//                    {
//                        struct ed_vert_t *vert0 = edge->verts[0].vert;
//                        struct ed_vert_t *vert1 = edge->verts[1].vert;
//
//                        vec3_t edge_center;
//                        vec3_t_add(&edge_center, &vert0->vert, &vert1->vert);
//                        vec3_t_mul(&edge_center, &edge_center, 0.5);
//
//                        mat3_t_vec3_t_mul(&edge_center, &edge_center, &brush->orientation);
//                        mat4_t_comp(&pickable->transform, &brush->orientation, &brush->position);
//                        vec3_t_add(&pickable->transform.rows[3].xyz, &pickable->transform.rows[3].xyz, &edge_center);
//                        vec3_t_mul(&pickable->ranges->offset.rows[3].xyz, &edge_center, -1.0);
//
//                    }
//                }
//                break;
//
//                case ED_PICKABLE_TYPE_LIGHT:
//                {
//                    struct r_light_t *light = r_GetLight(pickable->primary_index);
//                    mat3_t rot;
//
//                    if(pickable->transform_flags & ED_PICKABLE_TRANSFORM_FLAG_TRANSLATION)
//                    {
//                        vec3_t_add(&light->position, &light->position, &pickable->translation);
//                    }
//
//                    if(light->type == R_LIGHT_TYPE_SPOT && (pickable->transform_flags & ED_PICKABLE_TRANSFORM_FLAG_ROTATION))
//                    {
//                        struct r_spot_light_t *spot_light = (struct r_spot_light_t *)light;
//                        mat3_t_mul(&spot_light->orientation, &spot_light->orientation, &pickable->rotation);
//                    }
//
//                    mat3_t_identity(&rot);
//                    mat4_t_comp(&pickable->transform, &rot, &light->position);
//                }
//                break;
//
//                case ED_PICKABLE_TYPE_ENTITY:
//                {
//                    struct e_entity_t *entity = e_GetEntity(pickable->primary_index);
//
//                    if(pickable->transform_flags)
//                    {
//                        if(pickable->transform_flags & ED_PICKABLE_TRANSFORM_FLAG_TRANSLATION)
//                        {
//                            e_TranslateEntity(entity, &pickable->translation);
//                        }
//
//                        if(pickable->transform_flags & ED_PICKABLE_TRANSFORM_FLAG_ROTATION)
//                        {
//                            e_RotateEntity(entity, &pickable->rotation);
//                        }
//                    }
//
//                    ed_UpdateEntityPickableRanges(pickable, entity);
//                    e_UpdateEntityNode(entity->node, &mat4_t_c_id());
//                    pickable->transform = entity->transform->transform;
//                }
//                break;
//
//                case ED_PICKABLE_TYPE_ENEMY:
//                {
//                    struct g_enemy_t *enemy = g_GetEnemy(pickable->secondary_index, pickable->primary_index);
//
//                    if(pickable->transform_flags & ED_PICKABLE_TRANSFORM_FLAG_TRANSLATION)
//                    {
//                        g_TranslateEnemy(enemy, &pickable->translation);
//                    }
//
//                    if(pickable->transform_flags & ED_PICKABLE_TRANSFORM_FLAG_ROTATION)
//                    {
//                        g_RotateEnemy(enemy, &pickable->rotation);
//                    }
//
//                    ed_UpdateEntityPickableRanges(pickable, enemy->thing.entity);
//                    e_UpdateEntityNode(enemy->thing.entity->node, &mat4_t_c_id());
//                    pickable->transform = enemy->thing.entity->transform->transform;
//                }
//                break;
//            }
//
//            pickable->transform_flags = 0;
//            pickable->modified_index = 0xffffffff;
//        }
//
//        struct ds_list_t *modified_brushes = &ed_level_state.pickables.modified_brushes;
//
//        for(uint32_t brush_index = 0; brush_index < modified_brushes->cursor; brush_index++)
//        {
//            struct ed_brush_t *brush = *(struct ed_brush_t **)ds_list_get_element(modified_brushes, brush_index);
//            ed_UpdateBrush(brush);
//            brush->modified_index = 0xffffffff;
//        }
//
//        modified_brushes->cursor = 0;
//
//        pickable_count = pickables->cursor;
//    }
//    while(pickable_index < pickable_count);
//
//    pickables->cursor = 0;
//}

void ed_l_Update()
{
    r_SetViewPos(&ed_l_fly_camera_state.position);
    r_SetViewPitchYaw(ed_l_fly_camera_state.pitch, ed_l_fly_camera_state.yaw);

    ed_l_UpdateUI();
//    ed_w_UpdatePickableObjects();
//    ed_w_UpdateManipulator();

    if(ed_level_state.world_data_stale && l_world_collider)
    {
        ed_level_state.world_data_stale = 0;
        ed_l_ClearWorldData();
        ed_l_RestoreBrushEntities();
    }

    ed_l_DrawGrid();

    ed_UpdateTools(&ed_level_tool_context);
    ed_UpdateObjectContext(&ed_level_state.obj.objects, NULL);

    if(in_GetKeyState(SDL_SCANCODE_P) & IN_KEY_STATE_JUST_PRESSED)
    {
        ed_l_PlayGame();
    }
}

/*
=============================================================
=============================================================
=============================================================
*/

void ed_l_DrawGrid()
{
//    r_i_SetModelMatrix(NULL);
//    r_i_SetViewProjectionMatrix(NULL);
//    r_i_SetBlending(GL_TRUE, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//    r_i_SetStencil(GL_FALSE, GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE);
//    r_i_SetRasterizer(GL_FALSE, GL_BACK, GL_FILL);
//    r_i_SetShader(ed_center_grid_shader);
//    r_i_DrawVerts(R_I_DRAW_CMD_TRIANGLE_LIST, ed_grid, 1.0);
//    r_i_SetShader(NULL);
//    r_i_SetModelMatrix(NULL);
//    r_i_SetViewProjectionMatrix(NULL);
//    r_i_SetBlending(GL_FALSE, GL_ONE, GL_ZERO);

//    r_UpdateViewProjectionMatrix();

    mat4_t view_projection_matrix;
    mat4_t_identity(&view_projection_matrix);

    view_projection_matrix.rows[3].y = -2;

    mat4_t_mul(&view_projection_matrix, &view_projection_matrix, &r_projection_matrix);

    r_i_SetShader(NULL, ed_center_grid_shader);
    struct r_i_uniform_t uniforms[] = {
        (struct r_i_uniform_t){
            .uniform = R_UNIFORM_MODEL_VIEW_PROJECTION_MATRIX,
            .count = 1,
            .value = &r_view_projection_matrix
        }
    };
    r_i_SetUniforms(NULL, NULL, uniforms, 1);

    struct r_i_blending_t blend_state = {
        .enable = R_I_ENABLE,
        .src_factor = GL_SRC_ALPHA,
        .dst_factor = GL_ONE_MINUS_SRC_ALPHA
    };
    r_i_SetBlending(NULL, NULL, &blend_state);

    struct r_i_depth_t depth_state = {
        .enable = R_I_ENABLE,
        .func = GL_LESS
    };
    r_i_SetDepth(NULL, NULL, &depth_state);

    struct r_i_raster_t raster_state = {
        .cull_enable = R_I_DISABLE,
        .polygon_mode = GL_FILL
    };
    r_i_SetRasterizer(NULL, NULL, &raster_state);


    struct r_i_draw_list_t *draw_list = r_i_AllocDrawList(NULL, 1);
    struct r_i_mesh_t *mesh = r_i_AllocMesh(NULL, sizeof(struct r_vert_t), 6, 0);
    memcpy(mesh->verts.verts, ed_grid, sizeof(ed_grid));

    draw_list->mode = GL_TRIANGLES;
    draw_list->mesh = mesh;
    draw_list->indexed = 0;

    draw_list->ranges[0].start = 0;
    draw_list->ranges[0].count = 6;
    draw_list->ranges[0].draw_state = NULL;
    draw_list->ranges[0].uniforms = NULL;

    r_i_DrawList(NULL, draw_list);


//    r_BindShader(ed_center_grid_shader);
//    r_SetDefaultUniformMat4(R_UNIFORM_MODEL_VIEW_PROJECTION_MATRIX, &r_view_projection_matrix);
//    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//    glEnable(GL_BLEND);
//    glEnable(GL_DEPTH_TEST);
//    glDepthFunc(GL_LESS);
//    glDisable(GL_CULL_FACE);
//    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
//    r_DrawVerts(ed_grid, 6, GL_TRIANGLES);
//
//
//
//    glLineWidth(4.0);
//    r_BindShader(r_immediate_shader);
//    r_SetDefaultUniformMat4(R_UNIFORM_MODEL_VIEW_PROJECTION_MATRIX, &r_view_projection_matrix);
//    r_DrawLine(&vec3_t_c(-10000.0, 0.0, 0.0), &vec3_t_c(10000.0, 0.0, 0.0), &vec4_t_c(1.0, 0.0, 0.0, 1.0));
//    r_DrawLine(&vec3_t_c(0.0, 0.0, -10000.0), &vec3_t_c(0.0, 0.0, 10000.0), &vec4_t_c(0.0, 0.0, 1.0, 1.0));
//    glDisable(GL_BLEND);
//    glEnable(GL_CULL_FACE);
}

void ed_LevelEditorDrawSelections()
{
//    ed_DrawSelections(&ed_level_state.obj.objects, NULL);
//    struct ds_list_t *selections = &ed_level_state.pickables.selections;
//
//    if(selections->cursor)
//    {
//        r_BindShader(ed_outline_shader);
//        glEnable(GL_DEPTH_TEST);
//        glDepthFunc(GL_LESS);
////        r_i_SetViewProjectionMatrix(NULL);
////        r_i_SetShader(ed_outline_shader);
////        r_i_SetBuffers(NULL, NULL);
////        r_i_SetRasterizer(GL_TRUE, GL_FRONT, GL_LINE);
////        r_i_SetModelMatrix(NULL);
//
//        uint32_t selection_count = selections->cursor - 1;
//        uint32_t selection_index = 0;
//        uint8_t stencil_value = 1;
//
//        struct r_named_uniform_t *color_uniform = r_GetNamedUniform(ed_outline_shader, "ed_color");
//
//        for(uint32_t index = 0; index < 2; index++)
//        {
//            for(; selection_index < selection_count; selection_index++)
//            {
//                struct ed_pickable_t *pickable = *(struct ed_pickable_t **)ds_list_get_element(selections, selection_index);
//
//                mat4_t base_model_view_projection_matrix;
//                mat4_t model_view_projection_matrix;
//                ed_PickableModelViewProjectionMatrix(pickable, NULL, &base_model_view_projection_matrix);
////                r_i_SetViewProjectionMatrix(&model_view_projection_matrix);
//                struct r_i_draw_list_t *draw_list = NULL;
//
//                switch(pickable->type)
//                {
//                    case ED_PICKABLE_TYPE_FACE:
//                    {
//                        struct ed_pickable_range_t *range = pickable->ranges;
//                        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
//                        glEnable(GL_STENCIL_TEST);
//                        glEnable(GL_DEPTH_TEST);
//                        glDepthFunc(GL_LESS);
//                        glDepthMask(GL_TRUE);
//                        glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
//                        glStencilFunc(GL_ALWAYS, 0xff, 0xff);
//                        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
//                        glEnable(GL_POLYGON_OFFSET_FILL);
//                        glEnable(GL_POLYGON_OFFSET_LINE);
//                        glPolygonOffset(-1.0, 1.0);
//                        while(range)
//                        {
//                            mat4_t_mul(&model_view_projection_matrix, &range->offset, &base_model_view_projection_matrix);
//                            r_SetDefaultUniformMat4(R_UNIFORM_MODEL_VIEW_PROJECTION_MATRIX, &model_view_projection_matrix);
//                            glDrawElements(GL_TRIANGLES, range->count, GL_UNSIGNED_INT, (void *)(range->start * sizeof(uint32_t)));
////                            draw_list = r_i_AllocDrawList(1);
////                            draw_list->commands->start = range->start;
////                            draw_list->commands->count = range->count;
////                            draw_list->size = 6.0;
////                            draw_list->indexed = 1;
////
////                            r_i_SetViewProjectionMatrix(&model_view_projection_matrix);
////                            r_i_SetDrawMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE, 0xff);
////                            r_i_SetStencil(GL_TRUE, GL_KEEP, GL_KEEP, GL_REPLACE, GL_ALWAYS, 0xff, 0xff);
////                            r_i_SetDepth(GL_TRUE, GL_ALWAYS);
////                            r_i_SetRasterizer(GL_TRUE, GL_BACK, GL_FILL);
////                            r_i_DrawImmediate(R_I_DRAW_CMD_TRIANGLE_LIST, draw_list);
//                            range = range->next;
//                        }
//
//                        r_SetNamedUniform(color_uniform, &vec4_t_c(0.2, 0.7, 0.4, 1.0));
//                        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
//                        glDepthMask(GL_TRUE);
//                        glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
//                        glStencilFunc(GL_EQUAL, 0x00, 0xff);
//                        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
//                        range = pickable->ranges;
//                        while(range)
//                        {
//                            mat4_t_mul(&model_view_projection_matrix, &range->offset, &base_model_view_projection_matrix);
//                            r_SetDefaultUniformMat4(R_UNIFORM_MODEL_VIEW_PROJECTION_MATRIX, &model_view_projection_matrix);
//                            glDrawElements(GL_TRIANGLES, range->count, GL_UNSIGNED_INT, (void *)(range->start * sizeof(uint32_t)));
////                            draw_list = r_i_AllocDrawList(1);
////                            draw_list->commands->start = range->start;
////                            draw_list->commands->count = range->count;
////                            draw_list->size = 6.0;
////                            draw_list->indexed = 1;
////
////                            r_i_SetViewProjectionMatrix(&model_view_projection_matrix);
////                            r_i_SetDrawMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
////                            r_i_SetStencil(GL_TRUE, GL_KEEP, GL_KEEP, GL_KEEP, GL_EQUAL, 0xff, 0x00);
////                            r_i_SetRasterizer(GL_TRUE, GL_BACK, GL_LINE);
////                            r_i_DrawImmediate(R_I_DRAW_CMD_TRIANGLE_LIST, draw_list);
//
//                            range = range->next;
//                        }
//
//                        r_SetNamedUniform(color_uniform, &vec4_t_c(0.3, 0.4, 1.0, 0.4));
////                        glEnable(GL_BLEND);
////                        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
//                        glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
//                        glStencilFunc(GL_ALWAYS, 0x00, 0xff);
//                        glDepthFunc(GL_EQUAL);
//                        glDepthMask(GL_FALSE);
//                        glEnable(GL_DEPTH_TEST);
//                        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
////                        glEnable(GL_POLYGON_OFFSET_FILL);
////                        glPolygonOffset(-1.0, 1.0);
//                        range = pickable->ranges;
//                        while(range)
//                        {
//                            mat4_t_mul(&model_view_projection_matrix, &range->offset, &base_model_view_projection_matrix);
//                            r_SetDefaultUniformMat4(R_UNIFORM_MODEL_VIEW_PROJECTION_MATRIX, &model_view_projection_matrix);
//                            glDrawElements(GL_TRIANGLES, range->count, GL_UNSIGNED_INT, (void *)(range->start * sizeof(uint32_t)));
////                            draw_list = r_i_AllocDrawList(1);
////                            draw_list->commands->start = range->start;
////                            draw_list->commands->count = range->count;
////                            draw_list->size = 4.0;
////                            draw_list->indexed = 1;
////
////                            r_i_SetViewProjectionMatrix(&model_view_projection_matrix);
////                            r_i_SetBlending(GL_TRUE, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
////                            r_i_SetDrawMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE, 0xff);
////                            r_i_SetStencil(GL_TRUE, GL_KEEP, GL_KEEP, GL_REPLACE, GL_ALWAYS, 0xff, 0x00);
////                            r_i_SetDepth(GL_TRUE, GL_ALWAYS);
////                            r_i_SetRasterizer(GL_TRUE, GL_BACK, GL_FILL);
////                            r_i_DrawImmediate(R_I_DRAW_CMD_TRIANGLE_LIST, draw_list);
//
//                            range = range->next;
//                        }
//                    }
//                    break;
//
//
////                    case ED_PICKABLE_TYPE_EDGE:
////                    {
////                        draw_list = r_i_AllocDrawList(1);
////                        struct ed_pickable_range_t *range = pickable->ranges;
////
////                        mat4_t_mul(&model_view_projection_matrix, &range->offset, &base_model_view_projection_matrix);
////                        r_i_SetUniform(color_uniform, 1, &vec4_t_c(1.0, 0.7, 0.4, 1.0));
////
////                        draw_list->commands->start = range->start;
////                        draw_list->commands->count = range->count;
////                        draw_list->size = 4.0;
////                        draw_list->indexed = 1;
////
////                        r_i_SetViewProjectionMatrix(&model_view_projection_matrix);
////                        r_i_SetDrawMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE, 0xff);
////                        r_i_SetDepth(GL_TRUE, GL_ALWAYS);
////                        r_i_DrawImmediate(R_I_DRAW_CMD_LINE_LIST, draw_list);
////                    }
////                    break;
//
//                    default:
//                        if(index)
//                        {
//                            r_SetNamedUniform(color_uniform, &vec4_t_c(1.0, 0.4, 0.0, 1.0));
//                        }
//                        else
//                        {
//                            r_SetNamedUniform(color_uniform, &vec4_t_c(1.0, 0.2, 0.0, 1.0));
//                        }
//
////                        r_i_SetViewProjectionMatrix(&base_model_view_projection_matrix);
//
//                        uint32_t start = pickable->ranges->start * sizeof(uint32_t);
//                        uint32_t count = pickable->ranges->count;
//
//                        r_SetDefaultUniformMat4(R_UNIFORM_MODEL_VIEW_PROJECTION_MATRIX, &base_model_view_projection_matrix);
//                        glLineWidth(2.0);
//                        glPolygonOffset(-1.0, 1.0);
//                        glEnable(GL_POLYGON_OFFSET_LINE);
//                        glStencilMask(0xff);
//                        glEnable(GL_STENCIL_TEST);
//                        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
//                        glDepthFunc(GL_LESS);
//                        glDepthMask(GL_TRUE);
//                        glCullFace(GL_FRONT);
//
//                        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
//                        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
//                        glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
//                        glStencilFunc(GL_EQUAL, 0x00, 0xff);
//                        glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, (void *)start);
//
//                        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
//                        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
//                        glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
//                        glStencilFunc(GL_NOTEQUAL, 0xff, 0xff);
//                        glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, (void *)start);
//
//                        glCullFace(GL_BACK);
//                        glDepthFunc(GL_ALWAYS);
//                        glDepthMask(GL_FALSE);
//                        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
//                        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
//                        glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
//                        glStencilFunc(GL_ALWAYS, 0x00, 0xff);
//                        glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, (void *)start);
//
//                    break;
//                }
//            }
//
//            selection_count++;
//        }
//
////        r_i_SetRasterizer(GL_TRUE, GL_BACK, GL_FILL);
////        r_i_SetDepth(GL_TRUE, GL_LESS);
////        r_i_SetBlending(GL_FALSE, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
////        r_i_SetDrawMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE, 0xff);
////        r_i_SetStencil(GL_FALSE, GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE);
////        r_i_SetBlending(GL_FALSE, GL_NONE, GL_NONE);
//        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
//        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
//        glEnable(GL_DEPTH_TEST);
//        glDepthFunc(GL_LESS);
//        glDepthMask(GL_TRUE);
//        glDisable(GL_STENCIL_TEST);
//        glDisable(GL_BLEND);
//        glCullFace(GL_BACK);
//        glDisable(GL_POLYGON_OFFSET_LINE);
//    }
//
//    struct ds_list_t *enemy_list = &ed_level_state.pickables.game_pickables[ED_PICKABLE_TYPE_ENEMY];
//
//    for(uint32_t index = 0; index < enemy_list->cursor; index++)
//    {
//        struct ed_pickable_t *pickable = *(struct ed_pickable_t **)ds_list_get_element(enemy_list, index);
//        struct g_enemy_t *enemy = g_GetEnemy(pickable->secondary_index, pickable->primary_index);
//
//        switch(enemy->type)
//        {
//            case G_ENEMY_TYPE_CAMERA:
//            {
//                struct g_camera_t *camera = (struct g_camera_t *)enemy;
////                r_i_SetModelMatrix(&camera->entity->transform->transform);
//
////                r_i_
//            }
//            break;
//        }
//    }
}

void ed_w_PingInfoWindow()
{
    ed_level_state.info_window_alpha = 1.0;
}

uint32_t ed_l_IdleState(struct ed_tool_context_t *context, void *state_data, uint32_t just_changed)
{
    if(in_GetKeyState(SDL_SCANCODE_DELETE) & IN_KEY_STATE_JUST_PRESSED)
    {
        ed_NextToolState(context, ed_l_DeleteSelectionState, &ed_l_delete_tool_state);
    }
    else if(in_GetKeyState(SDL_SCANCODE_LALT) & IN_KEY_STATE_PRESSED)
    {
        ed_NextToolState(context, ed_l_FlyCameraState, &ed_l_fly_camera_state);
    }
    else if(in_GetKeyState(SDL_SCANCODE_LCTRL) & IN_KEY_STATE_PRESSED)
    {
        ed_NextToolState(context, ed_l_PlacementCrosshairState, &ed_l_obj_placement_state);
    }
    else if(in_GetMouseButtonState(SDL_BUTTON_LEFT) & IN_KEY_STATE_JUST_PRESSED)
    {
        ed_NextToolState(context, ed_l_LeftClickState, &ed_l_pick_args);
    }
    else if(in_GetMouseButtonState(SDL_BUTTON_RIGHT) & IN_KEY_STATE_JUST_PRESSED)
    {
        ed_NextToolState(context, ed_l_RightClickState, &ed_l_pick_args);
    }
    else if(in_GetKeyState(SDL_SCANCODE_1) & IN_KEY_STATE_JUST_PRESSED)
    {
        ed_l_obj_placement_state.brush.brush_element = ED_BRUSH_ELEMENT_FACE;
    }
    else if(in_GetKeyState(SDL_SCANCODE_2) & IN_KEY_STATE_JUST_PRESSED)
    {
        ed_l_obj_placement_state.brush.brush_element = ED_BRUSH_ELEMENT_EDGE;
    }
    else if(in_GetKeyState(SDL_SCANCODE_3) & IN_KEY_STATE_JUST_PRESSED)
    {
        ed_l_obj_placement_state.brush.brush_element = ED_BRUSH_ELEMENT_VERT;
    }


    struct ed_transform_operator_data_t *operator_data = ed_level_state.obj.objects.operators[ED_OPERATOR_TRANSFORM].data;

    if(in_GetKeyState(SDL_SCANCODE_T) & IN_KEY_STATE_JUST_PRESSED)
    {
        operator_data->transform_type = ED_TRANSFORM_OPERATOR_TRANSFORM_TYPE_TRANSLATE;
    }
    else if(in_GetKeyState(SDL_SCANCODE_R) & IN_KEY_STATE_JUST_PRESSED)
    {
        operator_data->transform_type = ED_TRANSFORM_OPERATOR_TRANSFORM_TYPE_ROTATE;
    }
    else if(in_GetKeyState(SDL_SCANCODE_S) & IN_KEY_STATE_JUST_PRESSED)
    {
        operator_data->transform_type = ED_TRANSFORM_OPERATOR_TRANSFORM_TYPE_SCALE;
    }

    return 0;
}

uint32_t ed_l_DeleteSelectionState(struct ed_tool_context_t *context, void *state_data, uint32_t just_changed)
{
    struct ed_l_delete_tool_state_t *state = (struct ed_l_delete_tool_state_t *)state_data;

    if(just_changed)
    {
        igOpenPopup_Str("Delete selections", 0);
        in_GetMousePos(&state->mouse_x, &state->mouse_y);
    }


//    {
//        if(state->open_pop_up)
//        {
//            igOpenPopup_Str("Delete selections", 0);
//            state->open_pop_up = 0;
//        }
//
//        igSetNextWindowPos((ImVec2){state->mouse_x, state->mouse_y}, ImGuiCond_Once, (ImVec2){0.0, 0.0});
//        if(igBeginPopup("Delete selections", 0))
//        {
//            if(igMenuItem_Bool("Delete selections?", NULL, 0, 1))
//            {
//                ed_w_DeleteSelections();
//            }
//            igEndPopup();
//        }
//        else
//        {
//            ed_NextToolState(context, NULL);
//        }
//    }

    ed_NextToolState(context, NULL, NULL);

    return 0;
}

uint32_t ed_l_FlyCameraState(struct ed_tool_context_t *context, void *state_data, uint32_t just_changed)
{
    uint32_t key_state = in_GetKeyState(SDL_SCANCODE_LALT);
    struct ed_l_fly_camera_tool_state_t *state = (struct ed_l_fly_camera_tool_state_t *)state_data;

    if(just_changed)
    {
//        if(key_state & IN_KEY_STATE_PRESSED)
//        {
        in_SetMouseWarp(1);
//            return 1;
//        }
    }
//    else
    {
        ed_l_tool_tab_passthrough = 1;

        if(!(key_state & IN_KEY_STATE_PRESSED))
        {
            ed_l_tool_tab_passthrough = 0;
            ed_NextToolState(context, NULL, NULL);
            in_SetMouseWarp(0);
            return 0;
        }

        float dx;
        float dy;

        in_GetMouseDelta(&dx, &dy);

        state->pitch += dy;
        state->yaw -= dx;

        if(state->pitch > 0.5)
        {
            state->pitch = 0.5;
        }
        else if(state->pitch < -0.5)
        {
            state->pitch = -0.5;
        }

        vec4_t translation = {};

        if(in_GetKeyState(SDL_SCANCODE_W) & IN_KEY_STATE_PRESSED)
        {
            translation.z -= 0.05;
        }
        if(in_GetKeyState(SDL_SCANCODE_S) & IN_KEY_STATE_PRESSED)
        {
            translation.z += 0.05;
        }

        if(in_GetKeyState(SDL_SCANCODE_A) & IN_KEY_STATE_PRESSED)
        {
            translation.x -= 0.05;
        }
        if(in_GetKeyState(SDL_SCANCODE_D) & IN_KEY_STATE_PRESSED)
        {
            translation.x += 0.05;
        }

        mat4_t_vec4_t_mul_fast(&translation, &r_camera_matrix, &translation);
        vec3_t_add(&state->position, &state->position, &vec3_t_c(translation.x, translation.y, translation.z));
    }

    return 0;
}

uint32_t ed_l_PlacementCrosshairState(struct ed_tool_context_t *context, void *state_data, uint32_t just_changed)
{
    struct ed_l_obj_placement_state_t *state = (struct ed_l_obj_placement_state_t *)state_data;

//    if(just_changed)
//    {
//        if(in_GetKeyState(SDL_SCANCODE_LCTRL) & IN_KEY_STATE_PRESSED)
//        {
//            return 1;
//        }
//    }
//    else
    {
        ed_l_tool_tab_passthrough = 1;

        if(!(in_GetKeyState(SDL_SCANCODE_LCTRL) & IN_KEY_STATE_PRESSED))
        {
            ed_NextToolState(context, NULL, NULL);
            ed_l_tool_tab_passthrough = 0;
            return 0;
        }

        int32_t mouse_x;
        int32_t mouse_y;
//        struct ed_level_state_t *context_data = &ed_level_state;
        in_GetMousePos(&mouse_x, &mouse_y);
        vec3_t intersection = {};

        vec3_t *plane_point = &state->plane_point;
        mat3_t *plane_orientation = &state->plane_orientation;

        ed_l_SurfaceUnderMouse(mouse_x, mouse_y, plane_point, plane_orientation);
        ed_CameraRay(mouse_x, mouse_y, plane_point, &plane_orientation->rows[1], plane_point);
        ed_l_LinearSnapValueOnSurface(&ed_level_state.obj.objects, plane_point, plane_orientation, plane_point);

        vec3_t start = *plane_point;
        vec3_t end = *plane_point;
        vec3_t normal = plane_orientation->rows[1];

        vec3_t u_axis;
        vec3_t v_axis;

        vec3_t_mul(&u_axis, &plane_orientation->rows[0], ED_L_PLACEMENT_CROSSHAIR_SIZE);
        vec3_t_mul(&v_axis, &plane_orientation->rows[2], ED_L_PLACEMENT_CROSSHAIR_SIZE);

        int32_t window_x;
        int32_t window_y;

        ed_PointPixelCoords(&window_x, &window_y, plane_point);

        igSetNextWindowPos((ImVec2){window_x, window_y}, 0, (ImVec2){0.0, 0.0});
        igSetNextWindowBgAlpha(0.25);
        if(igBegin("crosshair", NULL, ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoTitleBar |
                                 ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDecoration))
        {
            igText("pos: [%f, %f, %f]", plane_point->x, plane_point->y, plane_point->z);
            igText("norm: [%f, %f, %f]", normal.x, normal.y, normal.z);
        }
        igEnd();

//        r_i_SetModelMatrix(NULL);
//        r_i_SetViewProjectionMatrix(NULL);

        r_i_SetShader(NULL, NULL);
        struct r_i_uniform_t view_projection_matrix = {
            .uniform = R_UNIFORM_MODEL_VIEW_PROJECTION_MATRIX,
            .count = 1,
            .value = &r_view_projection_matrix
        };
        r_i_SetUniforms(NULL, NULL, &view_projection_matrix, 1);

        struct r_i_raster_t rasterizer = {
            .line_width = 4.0,
        };
        r_i_SetRasterizer(NULL, NULL, &rasterizer);

        struct r_i_depth_t depth_state = {
            .enable = R_I_ENABLE,
            .func = GL_LESS
        };
        r_i_SetDepth(NULL, NULL, &depth_state);


        struct r_i_draw_mask_t draw_mask_state = {
            .red = GL_TRUE,
            .green = GL_TRUE,
            .blue = GL_TRUE,
            .alpha = GL_TRUE,
            .depth = GL_TRUE,
            .stencil = 0xff,
        };
        r_i_SetDrawMask(NULL, NULL, &draw_mask_state);

        r_i_DrawLine(NULL, &vec3_t_c(start.x + u_axis.x, start.y + u_axis.y, start.z + u_axis.z),
                     &vec3_t_c(end.x - u_axis.x, end.y - u_axis.y, end.z - u_axis.z),
                     &vec4_t_c(1.0, 1.0, 1.0, 1.0));

        r_i_DrawLine(NULL, &vec3_t_c(start.x + v_axis.x, start.y + v_axis.y, start.z + v_axis.z),
                     &vec3_t_c(end.x - v_axis.x, end.y - v_axis.y, end.z - v_axis.z),
                     &vec4_t_c(1.0, 1.0, 1.0, 1.0));

//        r_BindShader(r_immediate_shader);
//        glLineWidth(4.0);
//        r_SetDefaultUniformMat4(R_UNIFORM_MODEL_VIEW_PROJECTION_MATRIX, &r_view_projection_matrix);
//        r_DrawLine(&vec3_t_c(start.x + u_axis.x, start.y + u_axis.y, start.z + u_axis.z),
//                     &vec3_t_c(end.x - u_axis.x, end.y - u_axis.y, end.z - u_axis.z),
//                     &vec4_t_c(1.0, 1.0, 1.0, 1.0));

//        r_DrawLine(&vec3_t_c(start.x + v_axis.x, start.y + v_axis.y, start.z + v_axis.z),
//                     &vec3_t_c(end.x - v_axis.x, end.y - v_axis.y, end.z - v_axis.z),
//                     &vec4_t_c(1.0, 1.0, 1.0, 1.0));

        if(in_GetMouseButtonState(SDL_BUTTON_LEFT) & IN_KEY_STATE_JUST_PRESSED)
        {
            switch(state->active_tab)
            {
                case ED_L_OBJ_TAB_BRUSH:
                    ed_NextToolState(context, ed_l_BrushBoxState, state_data);
                break;

                case ED_L_OBJ_TAB_LIGHT:
                    ed_NextToolState(context, ed_l_PlaceLightAtCursorState, state_data);
                break;

                case ED_L_OBJ_TAB_ENTITY:
                    ed_NextToolState(context, ed_l_PlaceEntityAtCursorState, state_data);
                break;

                case ED_L_OBJ_TAB_ENEMY:
//                    ed_SetNextState(ed_l_PlaceEnemyAtCursor);
                break;
            }
        }

    }

    return 0;
}

//uint32_t ed_l_TransformOperatorModeEntryState(struct ed_tool_context_t *context, void *state_data, uint32_t just_changed)
//{
//    struct ed_obj_context_t *obj_context = tool->data;
//
//    struct ed_transform_operator_data_t *operator_data = obj_context->operators[ED_OPERATOR_TRANSFORM].data;
//
//    if(in_GetKeyState(SDL_SCANCODE_G) & IN_KEY_STATE_JUST_PRESSED)
//    {
//        operator_data->mode = ED_TRANSFORM_OPERATOR_MODE_TRANSLATE;
//    }
//    else if(in_GetKeyState(SDL_SCANCODE_R) & IN_KEY_STATE_JUST_PRESSED)
//    {
//        operator_data->mode = ED_TRANSFORM_OPERATOR_MODE_ROTATE;
//    }
//
//    return 0;
//}

uint32_t ed_l_BrushBoxState(struct ed_tool_context_t *context, void *state_data, uint32_t just_changed)
{
//    struct ed_level_state_t *context_data = &ed_level_state;
    struct ed_l_obj_placement_state_t *state = (struct ed_l_obj_placement_state_t *)state_data;
//    uint32_t right_button_down = in_GetMouseButtonState(SDL_BUTTON_LEFT) & IN_KEY_STATE_PRESSED;
    uint32_t ctrl_down = in_GetKeyState(SDL_SCANCODE_LCTRL) & IN_KEY_STATE_PRESSED;

    if(just_changed)
    {
//        context_data->brush.drawing = 0;
        state->brush.box_start = state->plane_point;
        state->brush.prev_intersection = state->plane_point;
        state->brush.box_width = 0.0;
        state->brush.box_height = 1.0;
        state->brush.box_depth = 0.0;
        state->brush.stage = 0xffffffff;
    }

    if(in_GetMouseButtonState(SDL_BUTTON_LEFT) & IN_KEY_STATE_JUST_PRESSED)
    {
        state->brush.stage++;
        state->plane_point = state->brush.prev_intersection;
    }

    if((in_GetKeyState(SDL_SCANCODE_LCTRL) & IN_KEY_STATE_PRESSED) && state->brush.stage < 2)
    {
//        if(in_GetMouseButtonState(SDL_BUTTON_LEFT) & IN_KEY_STATE_PRESSED)
//        {
        if(in_GetKeyState(SDL_SCANCODE_ESCAPE) & IN_KEY_STATE_PRESSED)
        {
            ed_NextToolState(context, NULL, NULL);
            return 0;
        }
        else
        {
            mat3_t plane_orientation;

            switch(state->brush.stage)
            {
                case 0:
                    plane_orientation = state->plane_orientation;
                break;

                case 1:
                {
                    plane_orientation.rows[0] = state->plane_orientation.rows[1];

                    vec3_t plane_camera_vec;
                    vec3_t_sub(&plane_camera_vec, &r_camera_matrix.rows[3].xyz, &state->plane_point);
                    float proj_a = fabsf(vec3_t_dot(&plane_camera_vec, &state->plane_orientation.rows[0]));
                    float proj_b = fabsf(vec3_t_dot(&plane_camera_vec, &state->plane_orientation.rows[2]));

                    if(proj_a > proj_b)
                    {
                        plane_orientation.rows[1] = state->plane_orientation.rows[0];
                        plane_orientation.rows[2] = state->plane_orientation.rows[2];
                    }
                    else
                    {
                        plane_orientation.rows[2] = state->plane_orientation.rows[0];
                        plane_orientation.rows[1] = state->plane_orientation.rows[2];
                    }

                }
                break;
            }

            int32_t mouse_x;
            int32_t mouse_y;

            in_GetMousePos(&mouse_x, &mouse_y);

            vec3_t intersection = {};

            vec3_t plane_point = state->plane_point;


            if(ed_CameraRay(mouse_x, mouse_y, &plane_point, &plane_orientation.rows[1], &intersection))
            {
                ed_l_LinearSnapValueOnSurface(&ed_level_state.obj.objects, &plane_point, &plane_orientation, &intersection);

//                if(state->brush.stage == 1)
//                {
//                    intersection.x = state->brush.box_end.x;
//                    intersection.z = state->brush.box_end.z;
//                }

                vec3_t diagonal;
                vec3_t_sub(&diagonal, &intersection, &state->brush.box_start);

//                printf("%f %f %f\n", diagonal.x, diagonal.y, diagonal.z );
                vec4_t box_base_color;

//                float proj_x = vec3_t_dot(&diagonal, &plane_orientation.rows[0]);
//                float proj_y = vec3_t_dot(&diagonal, &plane_orientation.rows[1]);
//                float proj_z = vec3_t_dot(&diagonal, &plane_orientation.rows[2]);

                if(state->brush.stage == 0)
                {
                    state->brush.box_width = vec3_t_dot(&diagonal, &state->plane_orientation.rows[0]);
                    state->brush.box_depth = vec3_t_dot(&diagonal, &state->plane_orientation.rows[2]);
                    box_base_color = vec4_t_c(0.0, 1.0, 0.0, 1.0);
                    state->brush.prev_intersection = intersection;
                }
                else
                {
                    state->brush.box_height = vec3_t_dot(&diagonal, &state->plane_orientation.rows[1]);
                    box_base_color = vec4_t_c(1.0, 0.5, 0.0, 1.0);
                }

                vec3_t corners[8];
                corners[0] = state->brush.box_start;
                vec3_t_fmadd(&corners[1], &state->brush.box_start, &state->plane_orientation.rows[0], state->brush.box_width);
                vec3_t_fmadd(&corners[2], &corners[1], &state->plane_orientation.rows[2], state->brush.box_depth);
                vec3_t_fmadd(&corners[3], &state->brush.box_start, &state->plane_orientation.rows[2], state->brush.box_depth);

                vec3_t_fmadd(&corners[4], &corners[0], &state->plane_orientation.rows[1], state->brush.box_height);
                vec3_t_fmadd(&corners[5], &corners[1], &state->plane_orientation.rows[1], state->brush.box_height);
                vec3_t_fmadd(&corners[6], &corners[2], &state->plane_orientation.rows[1], state->brush.box_height);
                vec3_t_fmadd(&corners[7], &corners[3], &state->plane_orientation.rows[1], state->brush.box_height);


                r_i_SetShader(NULL, NULL);
                struct r_i_uniform_t model_view_projection_matrix = {
                    .uniform = R_UNIFORM_MODEL_VIEW_PROJECTION_MATRIX,
                    .count = 1,
                    .value = &r_view_projection_matrix
                };
                r_i_SetUniforms(NULL, NULL, &model_view_projection_matrix, 1);

                struct r_i_raster_t rasterizer = {
                    .line_width = 4.0
                };
                r_i_SetRasterizer(NULL, NULL, &rasterizer);

                struct r_i_depth_t depth_state = {
                    .enable = R_I_ENABLE,
                    .func = GL_LESS
                };
                r_i_SetDepth(NULL, NULL, &depth_state);


                struct r_i_draw_mask_t draw_mask_state = {
                    .red = GL_TRUE,
                    .green = GL_TRUE,
                    .blue = GL_TRUE,
                    .alpha = GL_TRUE,
                    .depth = GL_TRUE,
                    .stencil = 0xff,
                };
                r_i_SetDrawMask(NULL, NULL, &draw_mask_state);


                r_i_DrawLine(NULL, &corners[0], &corners[1], &vec4_t_c(0.2, 0.2, 1.0, 1.0));
                r_i_DrawLine(NULL, &corners[1], &corners[2], &vec4_t_c(1.0, 0.2, 0.2, 1.0));
                r_i_DrawLine(NULL, &corners[2], &corners[3], &vec4_t_c(0.2, 0.2, 1.0, 1.0));
                r_i_DrawLine(NULL, &corners[3], &corners[0], &vec4_t_c(1.0, 0.2, 0.2, 1.0));

                vec3_t edge_center;
                int32_t window_x;
                int32_t window_y;

                ImGuiWindowFlags dim_window_flags = ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoTitleBar |
                                                    ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDecoration;

                if(state->brush.stage)
                {
                    r_i_DrawLine(NULL, &corners[0], &corners[4], &vec4_t_c(0.0, 1.0, 0.0, 1.0));
                    r_i_DrawLine(NULL, &corners[1], &corners[5], &vec4_t_c(0.0, 1.0, 0.0, 1.0));
                    r_i_DrawLine(NULL, &corners[2], &corners[6], &vec4_t_c(0.0, 1.0, 0.0, 1.0));
                    r_i_DrawLine(NULL, &corners[3], &corners[7], &vec4_t_c(0.0, 1.0, 0.0, 1.0));

                    r_i_DrawLine(NULL, &corners[4], &corners[5], &vec4_t_c(0.2, 0.2, 1.0, 1.0));
                    r_i_DrawLine(NULL, &corners[5], &corners[6], &vec4_t_c(1.0, 0.2, 0.2, 1.0));
                    r_i_DrawLine(NULL, &corners[6], &corners[7], &vec4_t_c(0.2, 0.2, 1.0, 1.0));
                    r_i_DrawLine(NULL, &corners[7], &corners[4], &vec4_t_c(1.0, 0.2, 0.2, 1.0));

                    vec3_t_add(&edge_center, &corners[4], &corners[6]);
                    vec3_t_mul(&edge_center, &edge_center, 0.5);
                    ed_PointPixelCoords(&window_x, &window_y, &edge_center);

                    igSetNextWindowPos((ImVec2){window_x, window_y}, 0, (ImVec2){0.5, 0.5});
                    igSetNextWindowBgAlpha(0.5);
                    igPushStyleColor_Vec4(ImGuiCol_Text, (ImVec4){0.2, 1.0, 0.2, 1.0});
                    if(igBegin("dimh", NULL, dim_window_flags))
                    {
                        igText("%f m", fabsf(state->brush.box_height));
                    }
                    igEnd();
                    igPopStyleColor(1);
                }


                vec3_t_add(&edge_center, &corners[3], &corners[0]);
                vec3_t_mul(&edge_center, &edge_center, 0.5);
                ed_PointPixelCoords(&window_x, &window_y, &edge_center);

                igSetNextWindowPos((ImVec2){window_x, window_y}, 0, (ImVec2){0.5, 0.5});
                igSetNextWindowBgAlpha(0.5);
                igPushStyleColor_Vec4(ImGuiCol_Text, (ImVec4){1.0, 0.2, 0.2, 1.0});
                if(igBegin("dimw", NULL, dim_window_flags))
                {
                    igText("%f m", fabsf(state->brush.box_width));
                }
                igEnd();
                igPopStyleColor(1);


                vec3_t_add(&edge_center, &corners[3], &corners[2]);
                vec3_t_mul(&edge_center, &edge_center, 0.5);
                ed_PointPixelCoords(&window_x, &window_y, &edge_center);

                igSetNextWindowPos((ImVec2){window_x, window_y}, 0, (ImVec2){0.5, 0.5});
                igSetNextWindowBgAlpha(0.5);
                igPushStyleColor_Vec4(ImGuiCol_Text, (ImVec4){0.2, 0.2, 1.0, 1.0});
                if(igBegin("dimd", NULL, dim_window_flags))
                {
                    igText("%f m", fabsf(state->brush.box_depth));
                }
                igEnd();
                igPopStyleColor(1);
            }
        }
//        }
    }
    else
    {
        vec3_t position;
        vec3_t size;
        mat3_t orientation;

        if(state->brush.box_height == 0.0)
        {
            state->brush.box_height = 1.0;
        }

        size.x = fabsf(state->brush.box_width);
        size.y = fabsf(state->brush.box_height);
        size.z = fabsf(state->brush.box_depth);

//        size = vec3_t_c(1.0, 1.0, 1.0);
//        mat3_t_identity(&orientation);

        if(size.x != 0.0 || size.z != 0.0)
        {
//            vec3_t_add(&position, &state->brush.box_start, &state->brush.box_end);
//            vec3_t_mul(&position, &position, 0.5);
//            position = vec3_t_c(0.0, 0.0, 0.0);
            vec3_t_fmadd(&position, &state->brush.box_start, &state->plane_orientation.rows[0], state->brush.box_width * 0.5);
            vec3_t_fmadd(&position, &position, &state->plane_orientation.rows[1], state->brush.box_height * 0.5);
            vec3_t_fmadd(&position, &position, &state->plane_orientation.rows[2], state->brush.box_depth * 0.5);
//            vec3_t_fmadd(&position, &position, &state->plane_orientation.rows[1], size.y * 0.5);
//            ed_CreateObj(&ed_l_obj_context, ED_OBJ_TYPE_BRUSH, &position, &context_data->pickables.plane_orientation, &size, NULL);
            struct ed_obj_t *brush_obj = ed_CreateObj(&ed_level_state.obj.objects, ED_OBJ_TYPE_BRUSH, &position, &state->plane_orientation, &size, NULL);
            struct ed_brush_t *brush = (struct ed_brush_t *)brush_obj->base_obj;
            brush->object = brush_obj;

//            ed_AddObjToSelections(&ed_level_state.obj.objects, 0, &(struct ed_obj_result_t){.object = brush_obj});
        }
        ed_NextToolState(context, NULL, NULL);
    }

    return 0;
}

uint32_t ed_l_LeftClickState(struct ed_tool_context_t *context, void *state_data, uint32_t just_changed)
{
    struct ed_pick_args_t *pick_args = state_data;
    pick_args->args[ED_OBJ_TYPE_BRUSH] = NULL;
    ed_NextToolState(context, ed_ClickPickState, state_data);

    return 0;
}

uint32_t ed_l_RightClickState(struct ed_tool_context_t *context, void *state_data, uint32_t just_changed)
{
    struct ed_pick_args_t *pick_args = state_data;
    pick_args->args[ED_OBJ_TYPE_BRUSH] = &ed_l_brush_pick_args;
    ed_l_brush_pick_args.pick_faces = 1;
    ed_NextToolState(context, ed_ClickPickState, state_data);
}

uint32_t ed_l_PlaceEntityAtCursorState(struct ed_tool_context_t *context, void *state_data, uint32_t just_changed)
{
//    struct ed_level_state_t *context_data = &ed_level_state;
//    struct e_ent_def_t *ent_def = ed_level_state.pickables.ent_def;

    struct ed_l_obj_placement_state_t *data = state_data;

    if(data->entity.selected_def != NULL)
    {
        vec3_t position = data->plane_point;
        mat3_t orientation = data->plane_orientation;

        vec3_t_fmadd(&position, &position, &orientation.rows[2], 0.2);
        struct ed_ent_args_t args = {
            .def = data->entity.selected_def
        };
        ed_CreateObj(&ed_level_state.obj.objects, ED_OBJ_TYPE_ENTITY, &position, &orientation, &vec3_t_c(1, 1, 1), &args);
//        ed_CreateEntityPickable(data->entity.selected_def, &position, &vec3_t_c(1.0, 1.0, 1.0), &orientation, NULL);
    }

    ed_NextToolState(context, NULL, NULL);

    return 0;
}

uint32_t ed_l_PlaceLightAtCursorState(struct ed_tool_context_t *context, void *state_data, uint32_t just_changed)
{
    struct ed_l_obj_placement_state_t *data = state_data;

    vec3_t position;
    vec3_t_fmadd(&position, &data->plane_point, &data->plane_orientation.rows[1], 0.2);
    ed_CreateObj(&ed_level_state.obj.objects, ED_OBJ_TYPE_LIGHT, &position, NULL, NULL, &data->light.args);
    ed_NextToolState(context, NULL, NULL);
    return 0;
}

void ed_l_PickObject(uint32_t just_changed)
{
//    struct ed_level_state_t *context_data = &ed_level_state;
//    uint32_t button_state = in_GetMouseButtonState(SDL_BUTTON_LEFT) | in_GetMouseButtonState(SDL_BUTTON_RIGHT);
//    int32_t mouse_x;
//    int32_t mouse_y;
//
//    in_GetMousePos(&mouse_x, &mouse_y);
//
//    if(!(button_state & IN_KEY_STATE_PRESSED))
//    {
//        ed_level_state.obj.last_picked = ed_PickObject(&ed_level_state.obj.objects, mouse_x, mouse_y, (1 << ED_OBJ_TYPE_FACE));
//
//        if(ed_level_state.obj.last_picked.object != NULL)
//        {
//            uint32_t shift_state = in_GetKeyState(SDL_SCANCODE_LSHIFT);
//
//            if(ed_level_state.obj.last_picked.object->type == ED_OBJ_TYPE_BRUSH)
//            {
//                struct ed_brush_t *brush = (struct ed_brush_t *)ed_level_state.obj.last_picked.object->base_obj;
//
//                switch(ed_level_state.obj.last_picked.extra0)
//                {
//                    case ED_BRUSH_ELEMENT_BODY:
//                    {
//                        /* we picked the brush itself */
//                        struct ed_face_t *face = brush->faces;
//
//                        while(face)
//                        {
//                            if(face->object != NULL)
//                            {
//                                /* drop all the selected faces of this brush from the selection list */
//                                ed_DropObjFromSelections(&ed_level_state.obj.objects, &(struct ed_obj_result_t){.object = face->object});
//                            }
//
//                            face = face->next;
//                        }
//                    }
//                    break;
//
//                    case ED_BRUSH_ELEMENT_FACE:
//                    {
//                        /* we picked a brush face. To avoid having a billion face objects to update,
//                        we create a face object once a face gets picked, and destroy it once the face
//                        isn't selected anymore. */
//                        struct ed_face_t *face = ed_GetFace(ed_level_state.obj.last_picked.data1);
//
//                        if(!face->object)
//                        {
//                            face->object = ed_CreateObj(&ed_level_state.obj.objects, ED_OBJ_TYPE_FACE,
//                                                                 &brush->position,
//                                                                 &brush->orientation,
//                                                                 &vec3_t_c(1.0, 1.0, 1.0), &ed_level_state.obj.last_picked);
//
//                        }
//
//                        ed_level_state.obj.last_picked.object = face->object;
//
//                        if(brush->object->selection_index != ED_INVALID_OBJ_SELECTION_INDEX)
//                        {
//                            /* if the brush this face belongs to is currently selected, drop it from the selection list */
//                            ed_DropObjFromSelections(&ed_level_state.obj.objects, &(struct ed_obj_result_t){.object = brush->object});
//                        }
//                    }
//                    break;
//
//                }
//            }
//
//            ed_AddObjToSelections(&ed_level_state.obj.objects, shift_state & IN_KEY_STATE_PRESSED, &ed_level_state.obj.last_picked);
//
//            for(uint32_t index = 0; index < ed_level_state.obj.objects.objects[ED_OBJ_TYPE_FACE].cursor; index++)
//            {
//                /* destroy all face objects which aren't selected */
//                struct ed_obj_t *object = ed_GetObject(&ed_level_state.obj.objects, (struct ed_obj_h){.type = ED_OBJ_TYPE_FACE, .index = index});
//
//                if(object && object->selection_index == ED_INVALID_OBJ_SELECTION_INDEX)
//                {
//                    ed_DestroyObj(&ed_level_state.obj.objects, object);
//                    struct ed_face_t *face = (struct ed_face_t *)object->base_obj;
//                    face->object = NULL;
//                }
//            }
//        }
//
//        ed_SetNextState(ed_l_Idle);
//    }
}

void ed_l_PlaceEnemyAtCursor(uint32_t just_changed)
{
    struct ed_level_state_t *context_data = &ed_level_state;
    uint32_t type = ed_level_state.pickables.enemy_type;
    vec3_t position;
    vec3_t_fmadd(&position, &context_data->pickables.plane_point, &context_data->pickables.plane_orientation.rows[1], 0.2);
//    ed_CreateEnemyPickable(type, &position, &mat3_t_c_id(), NULL);
//    ed_SetNextState(ed_l_Idle);
}

void ed_SerializeLevel(void **level_buffer, size_t *buffer_size, uint32_t serialize_editor)
{
    size_t out_buffer_size = sizeof(struct l_level_header_t);

    struct ds_list_t *light_list = &ed_level_state.pickables.game_pickables[ED_PICKABLE_TYPE_LIGHT];
    struct ds_list_t *entity_list = &ed_level_state.pickables.game_pickables[ED_PICKABLE_TYPE_ENTITY];
    struct ds_list_t *enemy_list = &ed_level_state.pickables.game_pickables[ED_PICKABLE_TYPE_ENEMY];

    size_t level_editor_section_size = sizeof(struct ed_l_section_t);

    if(serialize_editor)
    {
        level_editor_section_size += sizeof(struct ed_brush_section_t);
        level_editor_section_size += sizeof(struct ed_brush_record_t) * ed_level_state.brush.brushes.used;
        level_editor_section_size += sizeof(struct ed_vert_record_t) * ed_level_state.brush.brush_vert_count;
        level_editor_section_size += sizeof(struct ed_edge_record_t) * ed_level_state.brush.brush_edges.used;
//        level_editor_section_size += sizeof(struct ed_polygon_record_t) * ed_level_state.brush.brush_face_polygons.used;
        /* each edge is referenced by two polygons, so its index will be serialized twice */
        level_editor_section_size += sizeof(size_t) * ed_level_state.brush.brush_edges.used * 2;
        level_editor_section_size += sizeof(struct ed_face_t) * ed_level_state.brush.brush_faces.used;
    }

    size_t light_section_size = 0;
    if(light_list->cursor)
    {
        light_section_size = sizeof(struct l_light_section_t);
        light_section_size += sizeof(struct l_light_record_t) * light_list->cursor;
    }

    size_t entity_section_size = 0;
    if(entity_list->cursor)
    {
        entity_section_size = sizeof(struct l_entity_section_t);
        entity_section_size += sizeof(struct l_entity_record_t) * entity_list->cursor;
    }

    size_t ent_def_section_size = 0;
    if(e_ent_defs[E_ENT_DEF_TYPE_ROOT].used)
    {
        ent_def_section_size = sizeof(struct l_ent_def_section_t);
        ent_def_section_size += sizeof(struct l_ent_def_record_t) * e_ent_defs[E_ENT_DEF_TYPE_ROOT].used;
    }

    size_t material_section_size = sizeof(struct l_material_section_t);
    material_section_size += sizeof(struct l_material_record_t) * r_materials.used;

    size_t enemy_section_size = 0;
    if(enemy_list->cursor)
    {
        enemy_section_size = sizeof(union g_enemy_record_t) * enemy_list->cursor;
    }

    size_t game_section_size = enemy_section_size;
    if(game_section_size)
    {
        game_section_size += sizeof(struct g_game_section_t);
    }

    size_t world_section_size = 0;
    if(l_world_model)
    {
        world_section_size = sizeof(struct l_world_section_t);
        world_section_size += sizeof(struct l_batch_record_t) * l_world_model->batches.buffer_size;
        world_section_size += sizeof(struct r_vert_t) * l_world_model->verts.buffer_size;
        world_section_size += sizeof(uint32_t) * l_world_model->indices.buffer_size;
    }

    out_buffer_size += level_editor_section_size + light_section_size + entity_section_size;
    out_buffer_size += ent_def_section_size + world_section_size + material_section_size + game_section_size;

    char *start_out_buffer = mem_Calloc(1, out_buffer_size);
    char *cur_out_buffer = start_out_buffer;
    *level_buffer = start_out_buffer;
    *buffer_size = out_buffer_size;

    struct l_level_header_t *level_header = (struct l_level_header_t *)cur_out_buffer;
    cur_out_buffer += sizeof(struct l_level_header_t);
    level_header->magic0 = L_LEVEL_HEADER_MAGIC0;
    level_header->magic1 = L_LEVEL_HEADER_MAGIC1;

    level_header->level_editor_start = cur_out_buffer - start_out_buffer;
    level_header->level_editor_size = level_editor_section_size;
    struct ed_l_section_t *level_editor_section = (struct ed_l_section_t *)cur_out_buffer;
    cur_out_buffer += sizeof(struct ed_l_section_t );

    /* level editor stuff */
    level_editor_section->camera_pos = ed_level_state.camera_pos;
    level_editor_section->camera_pitch = ed_level_state.camera_pitch;
    level_editor_section->camera_yaw = ed_level_state.camera_yaw;

    if(serialize_editor)
    {
        /* brush stuff */
        level_editor_section->brush_section_start = cur_out_buffer - start_out_buffer;

        struct ed_brush_section_t *brush_section = (struct ed_brush_section_t *)cur_out_buffer;
        cur_out_buffer += sizeof(struct ed_brush_section_t);
        brush_section->brush_record_start = cur_out_buffer - start_out_buffer;
        brush_section->brush_record_count = ed_level_state.brush.brushes.used;

        for(uint32_t brush_index = 0; brush_index < ed_level_state.brush.brushes.cursor; brush_index++)
        {
            struct ed_brush_t *brush = ed_GetBrush(brush_index);

            if(brush)
            {
                struct ed_brush_record_t *brush_record = (struct ed_brush_record_t *)cur_out_buffer;
                brush_record->record_size = (uint64_t)cur_out_buffer;
                brush_record->position = brush->position;
                brush_record->orientation = brush->orientation;
                brush_record->uuid = brush->index;
                cur_out_buffer += sizeof(struct ed_brush_record_t);

//                brush_record->vert_start = cur_out_buffer - start_out_buffer;
//                cur_out_buffer += sizeof(struct ed_vert_record_t) * brush->vertices.used;
//                brush_record->edge_start = cur_out_buffer - start_out_buffer;
//                cur_out_buffer += sizeof(struct ed_edge_record_t) * brush->edge_count;
//                brush_record->face_start = cur_out_buffer - start_out_buffer;
//                cur_out_buffer += sizeof(struct ed_face_record_t) * brush->face_count;

                struct ed_vert_record_t *vert_records = (struct ed_vert_record_t *)(start_out_buffer + brush_record->vert_start);
                struct ed_edge_record_t *edge_records = (struct ed_edge_record_t *)(start_out_buffer + brush_record->edge_start);
                struct ed_face_record_t *face_records = (struct ed_face_record_t *)(start_out_buffer + brush_record->face_start);

                /* serialize vertices and edges */
//                for(uint32_t vert_index = 0; vert_index < brush->vertices.cursor; vert_index++)
//                {
//                    struct ed_vert_t *vert = ed_GetVert(brush, vert_index);
//
//                    if(vert)
//                    {
//                        struct ed_vert_record_t *vert_record = vert_records + brush_record->vert_count;
//                        vert->s_index = brush_record->vert_count;
//                        brush_record->vert_count++;
//                        vert_record->vert = vert->vert;
//
//                        /* go over the edges of this vertex, and serialize them if they haven't been already */
//                        struct ed_vert_edge_t *vert_edge = vert->edges;
//
//                        while(vert_edge)
//                        {
//                            struct ed_edge_t *edge = vert_edge->edge;
//                            struct ed_edge_record_t *edge_record;
//
//                            if(edge->s_index == 0xffffffff)
//                            {
//                                /* this edge hasn't been serialized yet, so create a record */
//                                edge_record = edge_records + brush_record->edge_count;
//                                /* store the serialization index, so the record for this edge can
//                                be quickly found later */
//                                edge->s_index = brush_record->edge_count;
//                                brush_record->edge_count++;
//
//                                edge_record->polygons[0] = 0xffffffff;
//                                edge_record->polygons[1] = 0xffffffff;
//                                /* This constant value is used by the deserializer to know whether an edge hasn't been
//                                "seen" already. Could be done in the deserializer before processing polygons, but we're
//                                already touching the record here, might as well fill this now. */
//                                edge_record->d_index = 0xffffffff;
//                            }
//                            else
//                            {
//                                edge_record = edge_records + edge->s_index;
//                            }
//
//                            uint32_t vert_index = edge->verts[1].vert == vert;
//                            edge_record->vertices[vert_index] = vert->s_index;
//                            vert_edge = vert_edge->next;
//                        }
//                    }
//                }

                /* for serialization purposes we generate sequential polygon ids here. Those ids are generated the same
                during deserialization, so everything is fine. Edges reference those sequential ids */
                uint32_t polygon_id = 0;
                struct ed_face_t *face = brush->faces;
                /* serialize faces and polygons */
//                while(face)
//                {
//                    struct ed_face_record_t *face_record = face_records + brush_record->face_count;
//                    brush_record->face_count++;
//
//                    strcpy(face_record->material, face->material->name);
//                    face_record->uv_rot = face->tex_coords_rot;
//                    face_record->uv_scale = face->tex_coords_scale;
//                    face_record->polygon_start = cur_out_buffer - start_out_buffer;
//
//                    struct ed_face_polygon_t *polygon = face->polygons;
//
//                    while(polygon)
//                    {
//                        struct ed_polygon_record_t *polygon_record = (struct ed_polygon_record_t *)cur_out_buffer;
//                        cur_out_buffer += sizeof(struct ed_polygon_record_t);
//                        cur_out_buffer += sizeof(size_t) * polygon->edge_count;
//
//                        /* go over all the edges of this polygon and store connectivity data. The indices
//                        stored are for the serialized records */
//                        struct ed_edge_t *edge = polygon->edges;
//
//                        while(edge)
//                        {
//                            uint32_t polygon_side = edge->polygons[1].polygon == polygon;
//                            struct ed_edge_record_t *edge_record = edge_records + edge->s_index;
//
//                            /* store the serialization index of this edge in the edge list of this polygon
//                            record */
//                            polygon_record->edges[polygon_record->edge_count] = edge->s_index;
//                            /* store on which side of this edge the polygon is */
//                            edge_record->polygons[polygon_side] = polygon_id;
//                            polygon_record->edge_count++;
//
//                            if(edge_record->polygons[0] != 0xffffffff && edge_record->polygons[1] != 0xffffffff)
//                            {
//                                /* this edge has been referenced twice, so we can clear its serialization index. This
//                                is necessary for other serializations to properly happen in the future. */
//                                edge->s_index = 0xffffffff;
//                            }
//
//                            edge = edge->polygons[polygon_side].next;
//                        }
//
//                        face_record->polygon_count++;
//                        polygon = polygon->next;
//                        polygon_id++;
//                    }
//
//                    face = face->next;
//                }

                brush_record->record_size = cur_out_buffer - (char *)brush_record->record_size;
            }
        }

        level_editor_section->brush_section_size = (uint64_t)cur_out_buffer - level_editor_section->brush_section_start;
    }

    if(light_section_size)
    {
        /* light stuff */
        level_header->light_section_size = light_section_size;
        level_header->light_section_start = cur_out_buffer - start_out_buffer;

        struct l_light_section_t *light_section = (struct l_light_section_t *)cur_out_buffer;
        cur_out_buffer += sizeof(struct l_light_section_t);
        light_section->record_start = cur_out_buffer - start_out_buffer;
        struct l_light_record_t *light_records = (struct l_light_record_t *)cur_out_buffer;
        cur_out_buffer += sizeof(struct l_light_record_t ) * light_list->cursor;

        for(uint32_t light_index = 0; light_index < light_list->cursor; light_index++)
        {
            struct ed_pickable_t *pickable = *(struct ed_pickable_t **)ds_list_get_element(light_list, light_index);
            struct r_light_t *light = r_GetLight(pickable->primary_index);

            if(light)
            {
                struct l_light_record_t *light_record = light_records + light_section->record_count;
                light_section->record_count++;
                mat3_t_identity(&light_record->orientation);
                light_record->position = light->position;
                light_record->color = light->color;
                light_record->energy = light->energy;
                light_record->radius = light->range;
                light_record->size = vec2_t_c(0.0, 0.0);
                light_record->vert_start = 0;
                light_record->vert_count = 0;
                light_record->type = light->type;
                light_record->s_index = light->index;

                if(light->type == R_LIGHT_TYPE_SPOT)
                {
                    struct r_spot_light_t *spot_light = (struct r_spot_light_t *)light;
                    light_record->softness = spot_light->softness;
                    light_record->angle = spot_light->angle;
                    light_record->orientation = spot_light->orientation;
                }
            }
        }
    }

    /* enemy stuff */
    if(game_section_size)
    {
        level_header->game_section_start = cur_out_buffer - start_out_buffer;
        level_header->game_section_size = game_section_size;
        struct g_game_section_t *game_section = (struct g_game_section_t *)cur_out_buffer;
        cur_out_buffer += sizeof(struct g_game_section_t);

        if(enemy_list->cursor)
        {
            game_section->enemy_start = cur_out_buffer - start_out_buffer;
            game_section->enemy_count = enemy_list->cursor;

            union g_enemy_record_t *enemy_records = (union g_enemy_record_t *)cur_out_buffer;
            cur_out_buffer += sizeof(union g_enemy_record_t) * enemy_list->cursor;

            for(uint32_t enemy_index = 0; enemy_index < game_section->enemy_count; enemy_index++)
            {
                union g_enemy_record_t *record = enemy_records + enemy_index;

                struct ed_pickable_t *pickable = *(struct ed_pickable_t **)ds_list_get_element(enemy_list, enemy_index);
                struct g_enemy_t *enemy = g_GetEnemy(pickable->secondary_index, pickable->primary_index);
                struct e_node_t *node = enemy->thing.entity->node;

                record->thing.position = node->position;
                record->thing.orientation = node->orientation;
                record->thing.scale = vec3_t_c(1.0, 1.0, 1.0);
                record->thing.type = G_THING_TYPE_ENEMY;
                record->def.type = enemy->type;
                record->thing.s_index = enemy->thing.index;

                switch(enemy->type)
                {
                    case G_ENEMY_TYPE_CAMERA:
                    {
                        struct g_camera_t *camera = (struct g_camera_t *)enemy;
                        record->def.camera = camera->def;
                    }
                    break;
                }
            }
        }


    }

    level_header->material_section_start = cur_out_buffer - start_out_buffer;
    level_header->material_section_size = material_section_size;

    struct l_material_section_t *material_section = (struct l_material_section_t *)cur_out_buffer;
    cur_out_buffer += sizeof(struct l_material_section_t);
    material_section->record_start = cur_out_buffer - start_out_buffer;
    struct l_material_record_t *material_records = (struct l_material_record_t *)cur_out_buffer;
    cur_out_buffer += sizeof(struct l_material_record_t) * r_materials.used;

    for(uint32_t material_index = 0; material_index < r_materials.cursor; material_index++)
    {
        struct r_material_t *material = r_GetMaterial(material_index);

        if(material)
        {
            struct l_material_record_t *record = material_records + material_section->record_count;
            material->s_index = material_section->record_count;
            material_section->record_count++;

            strcpy(record->name, material->name);
            strcpy(record->diffuse_texture, material->diffuse_texture->name);
            strcpy(record->normal_texture, material->normal_texture->name);
//            strcpy(record->height_texture, material->height_texture->name);
            strcpy(record->roughness_texture, material->roughness_texture->name);
        }
    }

    if(ent_def_section_size)
    {
        level_header->ent_def_section_size = ent_def_section_size;
        level_header->ent_def_section_start = cur_out_buffer - start_out_buffer;

        struct l_ent_def_section_t *ent_def_section = (struct l_ent_def_section_t *)cur_out_buffer;
        cur_out_buffer += sizeof(struct l_ent_def_section_t);

        ent_def_section->record_start = cur_out_buffer - start_out_buffer;
        struct l_ent_def_record_t *ent_def_records = (struct l_ent_def_record_t *)cur_out_buffer;
        cur_out_buffer += sizeof(struct l_ent_def_record_t) * e_ent_defs[E_ENT_DEF_TYPE_ROOT].cursor;

        for(uint32_t ent_def_index = 0; ent_def_index < e_ent_defs[E_ENT_DEF_TYPE_ROOT].cursor; ent_def_index++)
        {
            struct e_ent_def_t *ent_def = e_GetEntDef(E_ENT_DEF_TYPE_ROOT, ent_def_index);

            if(ent_def)
            {
                struct l_ent_def_record_t *ent_def_record = ent_def_records + ent_def_section->record_count;
                ent_def->s_index = ent_def_section->record_count;
                ent_def_section->record_count++;
                strcpy(ent_def_record->name, ent_def->name);
            }
        }
    }

    if(entity_section_size)
    {
        level_header->entity_section_start = cur_out_buffer - start_out_buffer;

        struct l_entity_section_t *entity_section = (struct l_entity_section_t *)cur_out_buffer;
        cur_out_buffer += sizeof(struct l_entity_section_t);
        entity_section->record_start = cur_out_buffer - start_out_buffer;
        struct l_entity_record_t *entity_records = (struct l_entity_record_t *)cur_out_buffer;

        for(uint32_t entity_index = 0; entity_index < entity_list->cursor; entity_index++)
        {
            struct ed_pickable_t *pickable = *(struct ed_pickable_t **)ds_list_get_element(entity_list, entity_index);
            struct e_entity_t *entity = e_GetEntity(pickable->primary_index);
            struct e_node_t *transform = entity->node;

            /* don't serialize entities without a valid ent def (mostly brush entities) */
            if(transform->entity->def)
            {
                struct e_ent_def_t *ent_def = transform->entity->def;
                struct l_entity_record_t *entity_record = entity_records + entity_section->record_count;
                entity_section->record_count++;

                entity_record->child_start = 0;
                entity_record->child_count = 0;
                entity_record->ent_def = transform->entity->def->s_index;
                entity_record->position = transform->position;
                entity_record->orientation = transform->orientation;
                entity_record->scale.x = transform->scale.x / ent_def->scale.x;
                entity_record->scale.y = transform->scale.y / ent_def->scale.y;
                entity_record->scale.z = transform->scale.z / ent_def->scale.z;
                entity_record->s_index = transform->entity->index;
            }
        }

        cur_out_buffer += sizeof(struct l_entity_record_t) * entity_section->record_count;
        level_header->entity_section_size = (cur_out_buffer - start_out_buffer) - level_header->entity_section_start;
    }

    if(l_world_model)
    {
        level_header->world_section_start = cur_out_buffer - start_out_buffer;
        level_header->world_section_size = world_section_size;

        struct l_world_section_t *world_section = (struct l_world_section_t *)cur_out_buffer;
        cur_out_buffer += sizeof(struct l_world_section_t);

        world_section->vert_start = cur_out_buffer - start_out_buffer;
        world_section->vert_count = l_world_model->verts.buffer_size;
        struct r_vert_t *verts = (struct r_vert_t *)cur_out_buffer;
        cur_out_buffer += sizeof(struct r_vert_t ) * world_section->vert_count;
        memcpy(verts, l_world_model->verts.buffer, sizeof(struct r_vert_t) * world_section->vert_count);

        world_section->index_start = cur_out_buffer - start_out_buffer;
        world_section->index_count = l_world_model->indices.buffer_size;
        uint32_t *indices = (uint32_t *)cur_out_buffer;
        cur_out_buffer += sizeof(uint32_t) * world_section->index_count;
        memcpy(indices, l_world_model->indices.buffer, sizeof(uint32_t) * world_section->index_count);

        world_section->batch_start = cur_out_buffer - start_out_buffer;
        world_section->batch_count = l_world_model->batches.buffer_size;
        struct l_batch_record_t *batch_records = (struct l_batch_record_t *)cur_out_buffer;
        cur_out_buffer += sizeof(struct l_batch_record_t) * world_section->batch_count;
        for(uint32_t batch_index = 0; batch_index < world_section->batch_count; batch_index++)
        {
            struct r_batch_t *batch = ((struct r_batch_t *)l_world_model->batches.buffer) + batch_index;
            struct l_batch_record_t *record = batch_records + batch_index;

            record->start = batch->start - l_world_model->model_start;
            record->count = batch->count;
            record->material = batch->material->s_index;
        }
    }
}

void ed_DeserializeLevel(void *level_buffer, size_t buffer_size)
{
    if(!l_DeserializeLevel(level_buffer, buffer_size))
    {
        return;
    }

    char *start_in_buffer = level_buffer;
    char *cur_in_buffer = start_in_buffer;
    struct l_level_header_t *level_header = (struct l_level_header_t *)cur_in_buffer;


    if(level_header->light_section_size)
    {
        struct l_light_section_t *light_section = (struct l_light_section_t *)(cur_in_buffer + level_header->light_section_start);
        struct l_light_record_t *light_records = (struct l_light_record_t *)(cur_in_buffer + light_section->record_start);

        for(uint32_t record_index = 0; record_index < light_section->record_count; record_index++)
        {
            struct l_light_record_t *record = light_records + record_index;
            struct r_light_t *light = r_GetLight(R_LIGHT_INDEX(record->type, record->d_index));
//            ed_CreateLightPickable(NULL, NULL, 0, 0, 0, light);
        }
    }

    if(level_header->entity_section_size)
    {
        struct l_entity_section_t *entity_section = (struct l_entity_section_t *)(cur_in_buffer + level_header->entity_section_start);
        struct l_entity_record_t *entity_records = (struct l_entity_record_t *)(cur_in_buffer + entity_section->record_start);
        for(uint32_t record_index = 0; record_index < entity_section->record_count; record_index++)
        {
            struct l_entity_record_t *record = entity_records + record_index;
            struct e_entity_t *entity = e_GetEntity(record->d_index);
//            ed_CreateEntityPickable(NULL, NULL, NULL, NULL, entity);
        }
    }

    if(level_header->game_section_size)
    {
        struct g_game_section_t *game_section = (struct g_game_section_t *)(cur_in_buffer + level_header->game_section_start);
        union g_enemy_record_t *enemy_records = (union g_enemy_record_t *)(cur_in_buffer + game_section->enemy_start);
        for(uint32_t record_index = 0; record_index < game_section->enemy_count; record_index++)
        {
            union g_enemy_record_t *record = enemy_records + record_index;
            struct g_enemy_t *enemy = g_GetEnemy(record->def.type, record->thing.d_index);
//            ed_CreateEnemyPickable(0, NULL, NULL, enemy);
        }
    }

//    for(uint32_t entity_index = 0; entity_index < e_root_transforms.cursor; entity_index++)
//    {
//        struct e_node_t *node = *(struct e_node_t **)ds_list_get_element(&e_root_transforms, entity_index);
//
//        if(node->entity->def)
//        {
//            ed_CreateEntityPickable(NULL, NULL, NULL, NULL, node->entity);
//        }
//    }



    struct ed_l_section_t *level_editor_section = (struct ed_l_section_t *)(cur_in_buffer + level_header->level_editor_start);

    /* level editor stuff */
    ed_level_state.camera_pitch = level_editor_section->camera_pitch;
    ed_level_state.camera_yaw = level_editor_section->camera_yaw;
    ed_level_state.camera_pos = level_editor_section->camera_pos;

    /* brush stuff */
    struct ed_brush_section_t *brush_section = (struct ed_brush_section_t *)(start_in_buffer + level_editor_section->brush_section_start);
    cur_in_buffer = start_in_buffer + brush_section->brush_record_start;

    for(uint32_t record_index = 0; record_index < brush_section->brush_record_count; record_index++)
    {
        struct ed_brush_record_t *brush_record = (struct ed_brush_record_t *)cur_in_buffer;
        cur_in_buffer += brush_record->record_size;

        struct ed_vert_record_t *vert_records = (struct ed_vert_record_t *)(start_in_buffer + brush_record->vert_start);
        struct ed_edge_record_t *edge_records = (struct ed_edge_record_t *)(start_in_buffer + brush_record->edge_start);
        struct ed_face_record_t *face_records = (struct ed_face_record_t *)(start_in_buffer + brush_record->face_start);

        struct ed_brush_t *brush = ed_AllocBrush();
        brush->update_flags |= ED_BRUSH_UPDATE_FLAG_FACE_POLYGONS;
        brush->position = brush_record->position;
        brush->orientation = brush_record->orientation;
//        brush->main_brush = brush;

        /* first read all verts of this brush, and store the allocation index back into the vertex record,
        so we can map between a record and a vertex further down */
        for(uint32_t vert_index = 0; vert_index < brush_record->vert_count; vert_index++)
        {
            struct ed_vert_record_t *vert_record = vert_records + vert_index;
            struct ed_vert_t *vert = ed_AllocVert(brush);
            vert_record->d_index = vert->index;
            vert->vert = vert_record->vert;
        }

        uint32_t polygon_id = 0;

        /* deserialize faces */
//        for(uint32_t face_index = 0; face_index < brush_record->face_count; face_index++)
//        {
//            struct ed_face_record_t *face_record = face_records + face_index;
//            struct ed_face_t *face = ed_AllocFace(brush);
//
//            face->material = r_FindMaterial(face_record->material);
//            face->tex_coords_rot = face_record->uv_rot;
//            face->tex_coords_scale = face_record->uv_scale;
//
//            struct ed_face_polygon_t *last_polygon = NULL;
//
//            struct ed_polygon_record_t *polygon_records = (struct ed_polygon_record_t *)(start_in_buffer + face_record->polygon_start);
//            for(uint32_t polygon_index = 0; polygon_index < face_record->polygon_count; polygon_index++)
//            {
//                struct ed_polygon_record_t *polygon_record = polygon_records + polygon_index;
//                struct ed_face_polygon_t *polygon = ed_AllocFacePolygon(brush, face);
//
//                for(uint32_t edge_index = 0; edge_index < polygon_record->edge_count; edge_index++)
//                {
//                    struct ed_edge_record_t *edge_record = edge_records + polygon_record->edges[edge_index];
//                    /* find in which side of the serialized edge the serialized polygon is */
//                    uint32_t polygon_side = edge_record->polygons[1] == polygon_id;
//                    struct ed_edge_t *edge;
//
//                    if(edge_record->d_index == 0xffffffff)
//                    {
//                        edge = ed_AllocEdge(brush);
//                        /* we store the allocated index for the edge in the serialized data, so
//                        we can quickly map to the allocated edge from the serialized edge */
//                        edge_record->d_index = edge->index;
//                        struct ed_vert_record_t *vert0_record = vert_records + edge_record->vertices[0];
//                        struct ed_vert_record_t *vert1_record = vert_records + edge_record->vertices[1];
//                        struct ed_vert_t *vert0 = ed_GetVert(brush, vert0_record->d_index);
//                        struct ed_vert_t *vert1 = ed_GetVert(brush, vert1_record->d_index);
//
//                        edge->verts[0].vert = vert0;
//                        edge->verts[1].vert = vert1;
//
//                        ed_LinkVertEdge(vert0, edge);
//                        ed_LinkVertEdge(vert1, edge);
//                    }
//                    else
//                    {
//                        edge = ed_GetEdge(edge_record->d_index);
//                        /* clearing this here allows us to deserialize the contents of this
//                        buffer as many times as we want. Not terribly useful, but ehh. */
//                        edge_record->d_index = 0xffffffff;
//                    }
//
//                    edge->polygons[polygon_side].polygon = polygon;
//                    ed_LinkFacePolygonEdge(polygon, edge);
//                }
//
//                polygon_id++;
//            }
//        }

        ed_UpdateBrush(brush);
//        ed_CreateBrushPickable(NULL, NULL, NULL, brush);
    }

//    ed_w_UpdatePickableObjects();
    ed_l_ClearBrushEntities();
    ed_level_state.world_data_stale = 0;
}

void ed_l_SurfaceUnderMouse(int32_t mouse_x, int32_t mouse_y, vec3_t *plane_point, mat3_t *plane_orientation)
{
//    uint32_t ignore_types = ED_PICKABLE_OBJECT_MASK | ED_PICKABLE_TYPE_MASK_EDGE | ED_PICKABLE_TYPE_MASK_VERT;
//    struct ds_slist_t *pickables = &ed_level_state.pickables.pickables;
//    struct ed_pickable_t *surface = ed_SelectPickable(mouse_x, mouse_y, pickables, NULL, ignore_types);
//
//    if(surface)
//    {
//        struct ed_brush_t *brush = ed_GetBrush(surface->primary_index);
//        struct ed_face_t *face = ed_GetFace(surface->secondary_index);
//        plane_orientation->rows[1] = face->polygons->normal;
//        *plane_point = *(vec3_t *)ds_list_get_element(&face->clipped_polygons->vertices, 0);
//        mat3_t_vec3_t_mul(&plane_orientation->rows[1], &plane_orientation->rows[1], &brush->orientation);
//        mat3_t_vec3_t_mul(plane_point, plane_point, &brush->orientation);
//        vec3_t_add(plane_point, plane_point, &brush->position);
//
//        float max_axis_proj = -FLT_MAX;
//        uint32_t j_axis_index = 0;
////        mat3_t *plane_orientation = &context_data->brush.plane_orientation;
//        vec3_t axes[] =
//        {
//            vec3_t_c(1.0, 0.0, 0.0),
//            vec3_t_c(0.0, 1.0, 0.0),
//            vec3_t_c(0.0, 0.0, 1.0),
//        };
//
//        for(uint32_t comp_index = 0; comp_index < 3; comp_index++)
//        {
//            float axis_proj = fabsf(plane_orientation->rows[1].comps[comp_index]);
//
//            if(axis_proj > max_axis_proj)
//            {
//                max_axis_proj = axis_proj;
//                j_axis_index = comp_index;
//            }
//        }
//
//        uint32_t k_axis_index = (j_axis_index + 1) % 3;
//        vec3_t_cross(&plane_orientation->rows[0], &axes[k_axis_index], &plane_orientation->rows[1]);
//        vec3_t_normalize(&plane_orientation->rows[0], &plane_orientation->rows[0]);
//
//        vec3_t_cross(&plane_orientation->rows[2], &plane_orientation->rows[1], &plane_orientation->rows[0]);
//        vec3_t_normalize(&plane_orientation->rows[2], &plane_orientation->rows[2]);
//    }
//    else
//    {
        *plane_point = vec3_t_c(0.0, 0.0, 0.0);
        *plane_orientation = mat3_t_c_id();
//    }
}

void ed_l_LinearSnapValueOnSurface(struct ed_obj_context_t *context, vec3_t *plane_point, mat3_t *plane_orientation, vec3_t *snapped_value)
{
    vec3_t plane_origin;
//    struct ed_obj_context_t *context = tool->data;
    struct ed_transform_operator_data_t *operator_data = context->operators[ED_OPERATOR_TRANSFORM].data;
    /* compute where the world origin projects onto the plane */
    vec3_t_mul(&plane_origin, &plane_orientation->rows[1], vec3_t_dot(plane_point, &plane_orientation->rows[1]));

    /* transform intersection point from world space to plane space */
    vec3_t_sub(snapped_value, snapped_value, &plane_origin);
    vec3_t transformed_intersection;
    transformed_intersection.x = vec3_t_dot(snapped_value, &plane_orientation->rows[0]);
    transformed_intersection.y = vec3_t_dot(snapped_value, &plane_orientation->rows[1]);
    transformed_intersection.z = vec3_t_dot(snapped_value, &plane_orientation->rows[2]);
    *snapped_value = transformed_intersection;

    float linear_snap = operator_data->linear_snap;
    if(linear_snap)
    {
//        float linear_snap = context_data->manipulator.linear_snap;
        float x_a = ceilf(snapped_value->x / linear_snap) * linear_snap;
        float x_b = floorf(snapped_value->x / linear_snap) * linear_snap;

        float z_a = ceilf(snapped_value->z / linear_snap) * linear_snap;
        float z_b = floorf(snapped_value->z / linear_snap) * linear_snap;

        if(snapped_value->x > 0.0)
        {
            float t = x_a;
            x_a = x_b;
            x_b = t;
        }

        if(snapped_value->z > 0.0)
        {
            float t = z_a;
            z_a = z_b;
            z_b = t;
        }

        float snapped_x;
        float snapped_z;

        if(fabsf(fabsf(snapped_value->x) - fabsf(x_a)) < fabsf(fabsf(x_b) - fabsf(snapped_value->x)))
        {
            snapped_x = x_a;
        }
        else
        {
            snapped_x = x_b;
        }

        if(fabsf(fabsf(snapped_value->z) - fabsf(z_a)) < fabsf(fabsf(z_b) - fabsf(snapped_value->z)))
        {
            snapped_z = z_a;
        }
        else
        {
            snapped_z = z_b;
        }

        snapped_value->x = snapped_x;
        snapped_value->z = snapped_z;
    }

    mat3_t_vec3_t_mul(snapped_value, snapped_value, plane_orientation);
    vec3_t_add(snapped_value, snapped_value, &plane_origin);
}

uint32_t ed_l_SaveLevel(char *path, char *file)
{
    void *buffer;
    size_t buffer_size;
    char file_path[PATH_MAX];
    char file_no_ext[PATH_MAX];

    if(strcmp(ed_level_state.project.base_folder, path))
    {
        strcpy(ed_level_state.project.base_folder, path);
        uint32_t folder_index = 0;
        while(ed_l_project_folders[folder_index])
        {
            if(strstr(ed_level_state.project.base_folder, ed_l_project_folders[folder_index]))
            {
                ds_path_drop_end(ed_level_state.project.base_folder, ed_level_state.project.base_folder, PATH_MAX);
                break;
            }

            folder_index++;
        }

//        g_SetBasePath(ed_level_state.project.base_folder);

        folder_index = 0;
        while(ed_l_project_folders[folder_index])
        {
            ds_path_append_end(ed_level_state.project.base_folder, ed_l_project_folders[folder_index], file_path, PATH_MAX);
            ds_dir_make_dir(file_path);
            folder_index++;
        }
    }

    ds_path_drop_ext(file, file_no_ext, PATH_MAX);
    strcpy(ed_level_state.project.level_name, file);

    ed_l_BuildWorldData();
    ed_SerializeLevel(&buffer, &buffer_size, 1);
    ds_path_append_end(ed_level_state.project.base_folder, "levels", file_path, PATH_MAX);
    ds_path_append_end(file_path, file, file_path, PATH_MAX);
    ds_path_set_ext(file_path, "nlf", file_path, PATH_MAX);

    FILE *fp = fopen(file_path, "wb");
    fwrite(buffer, buffer_size, 1, fp);
    fclose(fp);

    return 1;
}

uint32_t ed_l_LoadLevel(char *path, char *file)
{
    void *buffer;
    size_t buffer_size;
    char file_name[PATH_MAX];

    ds_path_append_end(path, file, file_name, PATH_MAX);

    if(strstr(file, ".nlf"))
    {
        FILE *fp = fopen(file_name, "rb");

        if(!fp)
        {
            printf("couldn't find level file %s\n", file_name);
            return 0;
        }

        ed_l_ResetEditor();

        strcpy(ed_level_state.project.level_name, file);
        ds_path_drop_end(path, ed_level_state.project.base_folder, PATH_MAX);
//        g_SetBasePath(ed_level_state.project.base_folder);

        ds_file_read(fp, &buffer, &buffer_size);
        fclose(fp);
        ed_DeserializeLevel(buffer, buffer_size);
        mem_Free(buffer);

        return 1;
    }

    return 0;
}

uint32_t ed_l_SelectFolder(char *path, char *file)
{
    ds_path_append_end(path, file, ed_level_state.project.base_folder, PATH_MAX);
//    g_SetBasePath(ed_level_state.project.base_folder);
    return 1;
}

void ed_l_OpenExplorerSave(struct ed_explorer_state_t *explorer_state)
{
    if(ed_level_state.project.base_folder[0])
    {
        ds_path_append_end(ed_level_state.project.base_folder, "levels", explorer_state->current_path, PATH_MAX);

        if(ed_level_state.project.level_name[0])
        {
            strcpy(explorer_state->current_file, ed_level_state.project.level_name);
            ds_path_set_ext(explorer_state->current_file, "nlf", explorer_state->current_file, PATH_MAX);
        }
    }
}

void ed_l_OpenExplorerLoad(struct ed_explorer_state_t *explorer_state)
{
    if(ed_level_state.project.base_folder[0])
    {
        ds_path_append_end(ed_level_state.project.base_folder, "levels", explorer_state->current_path, PATH_MAX);
        explorer_state->current_file[0] = '\0';
    }
}

void ed_l_ClearBrushEntities()
{
    for(uint32_t brush_index = 0; brush_index < ed_level_state.brush.brushes.cursor; brush_index++)
    {
        struct ed_brush_t *brush = ed_GetBrush(brush_index);

        if(brush)
        {
            e_DestroyEntity(brush->entity);
            brush->entity = NULL;
        }
    }
}

void ed_l_RestoreBrushEntities()
{
    for(uint32_t brush_index = 0; brush_index < ed_level_state.brush.brushes.cursor; brush_index++)
    {
        struct ed_brush_t *brush = ed_GetBrush(brush_index);

        if(brush)
        {
            ed_UpdateBrushEntity(brush);
        }
    }
}

void ed_l_SaveGameLevelSnapshot()
{
    ed_l_BuildWorldData();
    ed_SerializeLevel(&ed_level_state.game_level_buffer, &ed_level_state.game_level_buffer_size, 0);
}

void ed_l_LoadGameLevelSnapshot()
{
    if(ed_level_state.game_level_buffer)
    {
        l_DeserializeLevel(ed_level_state.game_level_buffer, ed_level_state.game_level_buffer_size);

        char *level_buffer = ed_level_state.game_level_buffer;
        struct l_level_header_t *level_header = (struct l_level_header_t *)level_buffer;

        if(level_header->light_section_size)
        {
            struct l_light_section_t *light_section = (struct l_light_section_t *)(level_buffer + level_header->light_section_start);
            struct l_light_record_t *light_records = (struct l_light_record_t *)(level_buffer + light_section->record_start);

            struct ds_list_t *light_list = &ed_level_state.pickables.game_pickables[ED_PICKABLE_TYPE_LIGHT];

            for(uint32_t light_index = 0; light_index < light_list->cursor; light_index++)
            {
                struct ed_pickable_t *pickable = *(struct ed_pickable_t **)ds_list_get_element(light_list, light_index);
                for(uint32_t record_index = 0; record_index < light_section->record_count; record_index++)
                {
                    struct l_light_record_t *record = light_records + record_index;
                    if(pickable->primary_index == R_LIGHT_INDEX(record->type, record->s_index))
                    {
                        pickable->primary_index = R_LIGHT_INDEX(record->type, record->d_index);

                        if(record_index < light_section->record_count - 1)
                        {
                            *record = light_records[light_section->record_count - 1];
                        }
                        light_section->record_count--;
                        break;
                    }
                }
            }
        }

        if(level_header->game_section_size)
        {
            struct g_game_section_t *game_section = (struct g_game_section_t *)(level_buffer + level_header->game_section_start);

            if(game_section->enemy_count)
            {
                struct ds_list_t *enemy_list = &ed_level_state.pickables.game_pickables[ED_PICKABLE_TYPE_ENEMY];
                union g_enemy_record_t *enemy_records = (union g_enemy_record_t *)(level_buffer + game_section->enemy_start);

                for(uint32_t enemy_index = 0; enemy_index < enemy_list->cursor; enemy_index++)
                {
                    struct ed_pickable_t *pickable = *(struct ed_pickable_t **)ds_list_get_element(enemy_list, enemy_index);

                    for(uint32_t record_index = 0; record_index < game_section->enemy_count; record_index++)
                    {
                        union g_enemy_record_t *record = enemy_records + record_index;

                        if(record->thing.s_index == pickable->primary_index && record->thing.type == pickable->secondary_index)
                        {
                            pickable->primary_index = record->thing.d_index;

                            if(record_index < game_section->enemy_count - 1)
                            {
                                *record = enemy_records[game_section->enemy_count - 1];
                            }

                            game_section->enemy_count--;
                            break;
                        }
                    }
                }
            }
        }

        if(level_header->entity_section_size)
        {
            struct l_entity_section_t *entity_section = (struct l_entity_section_t *)(level_buffer + level_header->entity_section_start);
            struct l_entity_record_t *entity_records = (struct l_entity_record_t *)(level_buffer + entity_section->record_start);
            struct ds_list_t *entity_list = &ed_level_state.pickables.game_pickables[ED_PICKABLE_TYPE_ENTITY];

            for(uint32_t entity_index = 0; entity_index < entity_list->cursor; entity_index++)
            {
                struct ed_pickable_t *pickable = *(struct ed_pickable_t **)ds_list_get_element(entity_list, entity_index);

                for(uint32_t record_index = 0; record_index < entity_section->record_count; record_index++)
                {
                    struct l_entity_record_t *record = entity_records + record_index;

                    if(pickable->primary_index == record->s_index)
                    {
                        pickable->primary_index = record->d_index;
                        /* FIXME: this will break once the level format allows to store
                        child entities in entity records that were parented in the level
                        editor */
                        if(record_index < entity_section->record_count - 1)
                        {
                            *record = entity_records[entity_section->record_count - 1];
                        }
                        entity_section->record_count--;
                        break;
                    }
                }
            }
        }

        mem_Free(ed_level_state.game_level_buffer);
        ed_level_state.game_level_buffer = NULL;
    }
}

void ed_l_PlayGame()
{
    ed_l_SaveGameLevelSnapshot();
    ed_l_ClearBrushEntities();
    g_BeginGame();
}

void ed_l_GamePaused()
{
    igSetNextWindowBgAlpha(0.05);
    igSetNextWindowPos((ImVec2){0, r_height / 2}, 0, (ImVec2){0, 0});
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;
    window_flags |= ImGuiWindowFlags_NoDecoration;
    if(igBegin("Pause", NULL, window_flags))
    {
        igSetWindowFontScale(2.0);
        if(igSelectable_Bool("Resume game", NULL, 0, (ImVec2){0, 0}))
        {
            g_ResumeGame();
        }

        if(igSelectable_Bool("Stop game", NULL, 0, (ImVec2){0, 0}))
        {
            g_StopGame();
        }
        igSetWindowFontScale(1.0);
    }
    igEnd();
}

void ed_l_StopGame()
{
    ed_l_LoadGameLevelSnapshot();
}

void ed_l_ResetEditor()
{
    ed_level_state.camera_pitch = ED_LEVEL_CAMERA_PITCH;
    ed_level_state.camera_yaw = ED_LEVEL_CAMERA_YAW;
    ed_level_state.camera_pos = ED_LEVEL_CAMERA_POS;

    ed_level_state.project.base_folder[0] = '\0';
    ed_level_state.project.level_name[0] = '\0';

//    g_SetBasePath("");

    l_DestroyWorld();

//    for(uint32_t pickable_index = 0; pickable_index < ed_level_state.pickables.pickables.cursor; pickable_index++)
//    {
//        struct ed_pickable_t *pickable = ed_GetPickable(pickable_index);
//
//        if(pickable)
//        {
//            ed_DestroyPickable(pickable);
//        }
//    }

//    ed_level_state.pickables.pickables.cursor = 0;
//    ed_level_state.pickables.pickables.free_stack_top = 0xffffffff;
//    ed_level_state.pickables.selections.cursor = 0;
}

void ed_l_BuildWorldData()
{
//    ed_l_ClearBrushEntities();

//    if((ed_level_state.world_data_stale || !l_world_collider) && ed_level_state.brush.brushes.used)
//    {
//        printf("ed_l_BuildWorldData\n");
//        ed_level_state.world_data_stale = 0;
//
//        float start = g_GetDeltaTime();
//        struct ed_bsp_polygon_t *clipped_polygons = NULL;
//
//        for(uint32_t brush_index = 0; brush_index < ed_level_state.brush.brushes.cursor; brush_index++)
//        {
//            struct ed_brush_t *brush = ed_GetBrush(brush_index);
//
//            if(brush)
//            {
//                struct ed_bsp_polygon_t *brush_polygons = ed_BspPolygonsFromBrush(brush);
//                clipped_polygons = ed_ClipPolygonLists(clipped_polygons, brush_polygons);
//            }
//        }
//
//        float end = g_GetDeltaTime();
//        printf("bsp took %f seconds\n", end - start);
//
//        struct ds_buffer_t *polygon_buffer = &ed_level_state.brush.polygon_buffer;
//        struct ds_buffer_t *batch_buffer = &ed_level_state.brush.batch_buffer;
//        struct ds_buffer_t *vertex_buffer = &ed_level_state.brush.vertex_buffer;
//        struct ds_buffer_t *index_buffer = &ed_level_state.brush.index_buffer;
//
//        struct ed_bsp_polygon_t *polygon = clipped_polygons;
//        uint32_t polygon_count = 0;
//        uint32_t index_count = 0;
//        uint32_t vert_count = 0;
//
//        while(polygon)
//        {
//            if(polygon_count >= polygon_buffer->buffer_size)
//            {
//                ds_buffer_resize(polygon_buffer, polygon_buffer->buffer_size + 16);
//            }
//
//            ((struct ed_bsp_polygon_t **)polygon_buffer->buffer)[polygon_count] = polygon;
//            index_count += (polygon->vertices.cursor - 2) * 3;
//            vert_count += polygon->vertices.cursor;
//            polygon_count++;
//            polygon = polygon->next;
//        }
//
//        qsort(polygon_buffer->buffer, polygon_count, polygon_buffer->elem_size, ed_CompareBspPolygons);
//
//        if(index_count > index_buffer->buffer_size)
//        {
//            ds_buffer_resize(index_buffer, index_count);
//        }
//
//        if(vert_count > vertex_buffer->buffer_size)
//        {
//            ds_buffer_resize(vertex_buffer, vert_count);
//        }
//
//        uint32_t batch_count = 0;
//        struct r_material_t *cur_material = NULL;
//        struct ds_buffer_t col_vertex_buffer = ds_buffer_create(sizeof(vec3_t), vert_count);
//
//        uint32_t *world_indices = index_buffer->buffer;
//        struct r_vert_t *world_verts = vertex_buffer->buffer;
//        vec3_t *world_col_verts = col_vertex_buffer.buffer;
//
//
//        vert_count = 0;
//
//        struct r_batch_t *batch = NULL;
//
//        for(uint32_t polygon_index = 0; polygon_index < polygon_count; polygon_index++)
//        {
//            struct ed_bsp_polygon_t *polygon = ((struct ed_bsp_polygon_t **)polygon_buffer->buffer)[polygon_index];
//
//            if(polygon->face_polygon->face->material != cur_material)
//            {
//                cur_material = polygon->face_polygon->face->material;
//
//                if(batch_count >= batch_buffer->buffer_size)
//                {
//                    ds_buffer_resize(batch_buffer, batch_count + 1);
//                }
//
//                batch = ((struct r_batch_t *)batch_buffer->buffer) + batch_count;
//                batch->material = cur_material;
//                batch->count = 0;
//                batch->start = 0;
//
//                if(batch_count)
//                {
//                    struct r_batch_t *prev_batch = ((struct r_batch_t *)batch_buffer->buffer) + batch_count - 1;
//                    batch->start = prev_batch->start + prev_batch->count;
//                }
//
//                batch_count++;
//            }
//
//            for(uint32_t vert_index = 1; vert_index < polygon->vertices.cursor - 1;)
//            {
//                world_indices[batch->start + batch->count] = vert_count;
//                batch->count++;
//
//                world_indices[batch->start + batch->count] = vert_count + vert_index;
//                vert_index++;
//                batch->count++;
//
//                world_indices[batch->start + batch->count] = vert_count + vert_index;
//                batch->count++;
//            }
//
//            for(uint32_t vert_index = 0; vert_index < polygon->vertices.cursor; vert_index++)
//            {
//                struct r_vert_t *vert = ds_list_get_element(&polygon->vertices, vert_index);
//                world_verts[vert_index + vert_count] = *vert;
//                world_col_verts[vert_index + vert_count] = vert->pos;
//            }
//
//            vert_count += polygon->vertices.cursor;
//            polygon = polygon->next;
//        }
//
//        struct ds_buffer_t col_index_buffer = ds_buffer_copy(index_buffer);
//
//        if(vert_count)
//        {
//            l_world_shape->itri_mesh.verts = world_col_verts;
//            l_world_shape->itri_mesh.vert_count = vert_count;
//            l_world_shape->itri_mesh.indices = col_index_buffer.buffer;
//            l_world_shape->itri_mesh.index_count = index_count;
//            l_world_collider = p_CreateCollider(&l_world_col_def, &vec3_t_c(1.0, 1.0, 1.0), &vec3_t_c(0.0, 0.0, 0.0), &mat3_t_c_id());
//
//            struct r_model_geometry_t model_geometry = {};
//
//            model_geometry.batches = batch_buffer->buffer;
//            model_geometry.batch_count = batch_count;
//            model_geometry.verts = vertex_buffer->buffer;
//            model_geometry.vert_count = vert_count;
//            model_geometry.indices = index_buffer->buffer;
//            model_geometry.index_count = index_count;
//            l_world_model = r_CreateModel(&model_geometry, NULL, "world_model");
//        }
//    }
}

void ed_l_ClearWorldData()
{
    printf("ed_l_ClearWorldData\n");
    l_DestroyWorld();
}











