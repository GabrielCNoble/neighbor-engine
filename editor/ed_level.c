#include <float.h>
#include "ed_level.h"
#include "ed_pick.h"
#include "ed_brush.h"
#include "ed_main.h"
#include "../engine/r_main.h"
#include "dstuff/ds_buffer.h"
#include "../lib/dstuff/ds_path.h"
#include "../lib/dstuff/ds_dir.h"
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
extern struct r_model_t *l_world_model;

struct r_shader_t *ed_center_grid_shader;
struct r_shader_t *ed_picking_shader;
struct r_model_t *ed_translation_widget_model;
struct r_model_t *ed_rotation_widget_model;
struct r_model_t *ed_light_pickable_model;
struct r_model_t *ed_ball_widget_model;
//struct ed_pickable_t *ed_translation_widget;
struct r_shader_t *ed_outline_shader;
struct r_i_verts_t *ed_grid;

extern struct ds_slist_t r_lights[];
extern struct ds_slist_t r_materials;
extern struct ds_slist_t r_textures;
extern struct ds_slist_t e_entities;
extern struct ds_slist_t e_ent_defs[];
extern struct ds_list_t e_components[];
extern struct ds_slist_t g_enemies[];
extern struct ds_list_t e_root_transforms;

extern mat4_t r_projection_matrix;
extern mat4_t r_camera_matrix;
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

#define ED_GRID_DIVS 301
#define ED_GRID_QUAD_SIZE 250.0
#define ED_W_BRUSH_BOX_CROSSHAIR_DIM 0.3

#define ED_LEVEL_CAMERA_PITCH (-0.15)
#define ED_LEVEL_CAMERA_YAW (-0.3)
#define ED_LEVEL_CAMERA_POS (vec3_t_c(-6.0, 4.0, 4.0))

vec4_t ed_selection_outline_colors[][2] =
{
    [ED_PICKABLE_TYPE_BRUSH][0] = vec4_t_c(1.0, 0.2, 0.0, 1.0),
    [ED_PICKABLE_TYPE_BRUSH][1] = vec4_t_c(1.0, 0.5, 0.0, 1.0),

    [ED_PICKABLE_TYPE_LIGHT][0] = vec4_t_c(1.0, 0.2, 0.0, 1.0),
    [ED_PICKABLE_TYPE_LIGHT][1] = vec4_t_c(1.0, 0.5, 0.0, 1.0),

    [ED_PICKABLE_TYPE_ENTITY][0] = vec4_t_c(1.0, 0.2, 0.0, 1.0),
    [ED_PICKABLE_TYPE_ENTITY][1] = vec4_t_c(1.0, 0.5, 0.0, 1.0),

    [ED_PICKABLE_TYPE_FACE][0] = vec4_t_c(0.3, 0.4, 1.0, 1.0),
    [ED_PICKABLE_TYPE_FACE][1] = vec4_t_c(0.3, 0.4, 1.0, 1.0),
};

//extern struct e_ent_def_t *g_ent_def;

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

void ed_l_Init(struct ed_editor_t *editor)
{
//    ed_world_context = ed_contexts + ED_CONTEXT_WORLD;
//    ed_world_context->update = ed_w_Update;
//    ed_world_context->next_state = ed_w_Idle;
//    ed_world_context->context_data = &ed_level_state;

    editor->next_state = ed_LevelEditorIdle;

    ed_level_state.pickables.last_selected = NULL;
    ed_level_state.pickables.selections = ds_list_create(sizeof(struct ed_pickable_t *), 512);
    ed_level_state.pickables.pickables = ds_slist_create(sizeof(struct ed_pickable_t), 512);
    ed_level_state.pickables.modified_brushes = ds_list_create(sizeof(struct ed_brush_t *), 512);
    ed_level_state.pickables.modified_pickables = ds_list_create(sizeof(struct ed_pickable_t *), 512);
    ed_level_state.selected_tools_tab = ED_L_TOOL_TAB_BRUSH;
    ed_level_state.selected_material = r_GetDefaultMaterial();

    for(uint32_t game_pickable_type = ED_PICKABLE_TYPE_ENTITY; game_pickable_type < ED_PICKABLE_TYPE_LAST_GAME_PICKABLE; game_pickable_type++)
    {
        ed_level_state.pickables.game_pickables[game_pickable_type] = ds_list_create(sizeof(struct ed_pickable_t *), 512);
    }


    ed_level_state.pickable_ranges = ds_slist_create(sizeof(struct ed_pickable_range_t), 512);
    ed_level_state.widgets = ds_slist_create(sizeof(struct ed_widget_t), 16);

    ed_level_state.brush.bsp_nodes = ds_slist_create(sizeof(struct ed_bsp_node_t), 512);
    ed_level_state.brush.bsp_polygons = ds_slist_create(sizeof(struct ed_bsp_polygon_t), 512);
    ed_level_state.brush.brushes = ds_slist_create(sizeof(struct ed_brush_t), 512);
    ed_level_state.brush.brush_edges = ds_slist_create(sizeof(struct ed_edge_t), 512);
    ed_level_state.brush.brush_faces = ds_slist_create(sizeof(struct ed_face_t), 512);
    ed_level_state.brush.brush_face_polygons = ds_slist_create(sizeof(struct ed_face_polygon_t), 512);
    ed_level_state.brush.brush_batches = ds_list_create(sizeof(struct ed_brush_batch_t), 512);

    ed_level_state.brush.polygon_buffer = ds_buffer_create(sizeof(struct ed_bsp_polygon_t *), 0);
    ed_level_state.brush.vertex_buffer = ds_buffer_create(sizeof(struct r_vert_t), 0);
    ed_level_state.brush.index_buffer = ds_buffer_create(sizeof(uint32_t), 0);
    ed_level_state.brush.batch_buffer = ds_buffer_create(sizeof(struct r_batch_t), 0);

    ed_level_state.camera_pitch = ED_LEVEL_CAMERA_PITCH;
    ed_level_state.camera_yaw = ED_LEVEL_CAMERA_YAW;
    ed_level_state.camera_pos = ED_LEVEL_CAMERA_POS;

    ed_center_grid_shader = r_LoadShader("shaders/ed_grid.vert", "shaders/ed_grid.frag");
    ed_picking_shader = r_LoadShader("shaders/ed_pick.vert", "shaders/ed_pick.frag");
    ed_outline_shader = r_LoadShader("shaders/ed_outline.vert", "shaders/ed_outline.frag");
    ed_translation_widget_model = r_LoadModel("models/twidget.mof");
    ed_rotation_widget_model = r_LoadModel("models/rwidget.mof");
    ed_ball_widget_model = r_LoadModel("models/bwidget.mof");

    mat4_t_identity(&ed_level_state.manipulator.transform);
    ed_level_state.manipulator.linear_snap = 0.25;
    ed_level_state.manipulator.transform_type = ED_L_TRANSFORM_TYPE_TRANSLATION;
//    ed_level_state.manipulator.transform_space = ED_L_TRANSFORM_SPACE_WORLD;
    ed_level_state.manipulator.widgets[ED_L_TRANSFORM_TYPE_TRANSLATION] = ed_CreateWidget(NULL);
    struct ed_widget_t *widget = ed_level_state.manipulator.widgets[ED_L_TRANSFORM_TYPE_TRANSLATION];
    widget->setup_ds_fn = ed_w_ManipulatorWidgetSetupPickableDrawState;

    struct ed_pickable_t *translation_axis;
    translation_axis = ed_CreatePickableOnList(ED_PICKABLE_TYPE_WIDGET, &widget->pickables);
    translation_axis->camera_distance = 50.0;
    translation_axis->mode = GL_TRIANGLES;
    translation_axis->range_count = 1;
    translation_axis->ranges = ed_AllocPickableRange();
    translation_axis->ranges->start = ((struct r_batch_t *)ed_translation_widget_model->batches.buffer)->start;
    translation_axis->ranges->count = ed_translation_widget_model->indices.buffer_size;
    translation_axis->primary_index = 0;
    translation_axis->draw_transf_flags = ED_PICKABLE_DRAW_TRANSF_FLAG_FIXED_CAM_DIST;
    mat4_t_identity(&translation_axis->transform);
    translation_axis->transform.rows[0].x = 1.0;
    translation_axis->transform.rows[1].y = 1.0;
    translation_axis->transform.rows[2].z = 1.6;
    mat4_t_rotate_y(&translation_axis->transform, 0.5);
//    translation_axis->draw_transform = translation_axis->transform;

    translation_axis = ed_CreatePickableOnList(ED_PICKABLE_TYPE_WIDGET, &widget->pickables);
    translation_axis->camera_distance = 50.0;
    translation_axis->mode = GL_TRIANGLES;
    translation_axis->range_count = 1;
    translation_axis->ranges = ed_AllocPickableRange();
    translation_axis->ranges->start = ((struct r_batch_t *)ed_translation_widget_model->batches.buffer)->start;
    translation_axis->ranges->count = ed_translation_widget_model->indices.buffer_size;
    translation_axis->primary_index = 0;
    translation_axis->draw_transf_flags = ED_PICKABLE_DRAW_TRANSF_FLAG_FIXED_CAM_DIST;
    mat4_t_identity(&translation_axis->transform);
    translation_axis->transform.rows[0].x = 1.0;
    translation_axis->transform.rows[1].y = 1.0;
    translation_axis->transform.rows[2].z = 1.6;
    mat4_t_rotate_x(&translation_axis->transform, -0.5);
//    translation_axis->draw_transform = translation_axis->transform;

    translation_axis = ed_CreatePickableOnList(ED_PICKABLE_TYPE_WIDGET, &widget->pickables);
    translation_axis->camera_distance = 50.0;
    translation_axis->mode = GL_TRIANGLES;
    translation_axis->range_count = 1;
    translation_axis->ranges = ed_AllocPickableRange();
    translation_axis->ranges->start = ((struct r_batch_t *)ed_translation_widget_model->batches.buffer)->start;
    translation_axis->ranges->count = ed_translation_widget_model->indices.buffer_size;
    translation_axis->primary_index = 0;
    translation_axis->draw_transf_flags = ED_PICKABLE_DRAW_TRANSF_FLAG_FIXED_CAM_DIST;
    mat4_t_identity(&translation_axis->transform);
    translation_axis->transform.rows[0].x = 1.0;
    translation_axis->transform.rows[1].y = 1.0;
    translation_axis->transform.rows[2].z = 1.6;
//    translation_axis->draw_transform = translation_axis->transform;








    ed_level_state.manipulator.widgets[ED_L_TRANSFORM_TYPE_ROTATION] = ed_CreateWidget(NULL);
    widget = ed_level_state.manipulator.widgets[ED_L_TRANSFORM_TYPE_ROTATION];
    widget->setup_ds_fn = ed_w_ManipulatorWidgetSetupPickableDrawState;

    struct ed_pickable_t *rotation_axis;
    rotation_axis = ed_CreatePickableOnList(ED_PICKABLE_TYPE_WIDGET, &widget->pickables);
    rotation_axis->camera_distance = 50.0;
    rotation_axis->mode = GL_TRIANGLES;
    rotation_axis->range_count = 1;
    rotation_axis->ranges = ed_AllocPickableRange();
    rotation_axis->ranges->start = ((struct r_batch_t *)ed_rotation_widget_model->batches.buffer)->start;
    rotation_axis->ranges->count = ed_rotation_widget_model->indices.buffer_size;
    rotation_axis->primary_index = 0;
    rotation_axis->draw_transf_flags = ED_PICKABLE_DRAW_TRANSF_FLAG_FIXED_CAM_DIST;
    mat4_t_identity(&rotation_axis->transform);
    rotation_axis->transform.rows[0].x = 8.0;
    rotation_axis->transform.rows[1].y = 8.0;
    rotation_axis->transform.rows[2].z = 8.0;
    mat4_t_rotate_y(&rotation_axis->transform, 0.5);
//    rotation_axis->draw_transform = rotation_axis->transform;

    rotation_axis = ed_CreatePickableOnList(ED_PICKABLE_TYPE_WIDGET, &widget->pickables);
    rotation_axis->camera_distance = 50.0;
    rotation_axis->mode = GL_TRIANGLES;
    rotation_axis->range_count = 1;
    rotation_axis->ranges = ed_AllocPickableRange();
    rotation_axis->ranges->start = ((struct r_batch_t *)ed_rotation_widget_model->batches.buffer)->start;
    rotation_axis->ranges->count = ed_rotation_widget_model->indices.buffer_size;
    rotation_axis->primary_index = 0;
    rotation_axis->draw_transf_flags = ED_PICKABLE_DRAW_TRANSF_FLAG_FIXED_CAM_DIST;
    mat4_t_identity(&rotation_axis->transform);
    rotation_axis->transform.rows[0].x = 8.0;
    rotation_axis->transform.rows[1].y = 8.0;
    rotation_axis->transform.rows[2].z = 8.0;
    mat4_t_rotate_x(&rotation_axis->transform, -0.5);
//    rotation_axis->draw_transform = rotation_axis->transform;

    rotation_axis = ed_CreatePickableOnList(ED_PICKABLE_TYPE_WIDGET, &widget->pickables);
    rotation_axis->camera_distance = 50.0;
    rotation_axis->mode = GL_TRIANGLES;
    rotation_axis->range_count = 1;
    rotation_axis->ranges = ed_AllocPickableRange();
    rotation_axis->ranges->start = ((struct r_batch_t *)ed_rotation_widget_model->batches.buffer)->start;
    rotation_axis->ranges->count = ed_rotation_widget_model->indices.buffer_size;
    rotation_axis->primary_index = 0;
    rotation_axis->draw_transf_flags = ED_PICKABLE_DRAW_TRANSF_FLAG_FIXED_CAM_DIST;
    mat4_t_identity(&rotation_axis->transform);
    rotation_axis->transform.rows[0].x = 8.0;
    rotation_axis->transform.rows[1].y = 8.0;
    rotation_axis->transform.rows[2].z = 8.0;
//    rotation_axis->draw_transform = rotation_axis->transform;



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

    ed_grid = r_i_AllocImmediateExternData(sizeof(struct r_i_verts_t) + sizeof(struct r_vert_t) * 6);

    ed_grid->count = 6;
    ed_grid->verts[0].pos = vec3_t_c(-ED_GRID_QUAD_SIZE, 0.0, -ED_GRID_QUAD_SIZE);
    ed_grid->verts[0].tex_coords = vec2_t_c(0.0, 0.0);
    ed_grid->verts[0].color = vec4_t_c(1.0, 0.0, 0.0, 1.0);

    ed_grid->verts[1].pos = vec3_t_c(-ED_GRID_QUAD_SIZE, 0.0, ED_GRID_QUAD_SIZE);
    ed_grid->verts[1].tex_coords = vec2_t_c(1.0, 0.0);
    ed_grid->verts[1].color = vec4_t_c(0.0, 1.0, 0.0, 1.0);

    ed_grid->verts[2].pos = vec3_t_c(ED_GRID_QUAD_SIZE, 0.0, ED_GRID_QUAD_SIZE);
    ed_grid->verts[2].tex_coords = vec2_t_c(1.0, 1.0);
    ed_grid->verts[2].color = vec4_t_c(0.0, 0.0, 1.0, 1.0);

    ed_grid->verts[3].pos = vec3_t_c(ED_GRID_QUAD_SIZE, 0.0, ED_GRID_QUAD_SIZE);
    ed_grid->verts[3].tex_coords = vec2_t_c(1.0, 1.0);
    ed_grid->verts[3].color = vec4_t_c(0.0, 0.0, 1.0, 1.0);

    ed_grid->verts[4].pos = vec3_t_c(ED_GRID_QUAD_SIZE, 0.0, -ED_GRID_QUAD_SIZE);
    ed_grid->verts[4].tex_coords = vec2_t_c(0.0, 1.0);
    ed_grid->verts[4].color = vec4_t_c(0.0, 0.0, 1.0, 1.0);

    ed_grid->verts[5].pos = vec3_t_c(-ED_GRID_QUAD_SIZE, 0.0, -ED_GRID_QUAD_SIZE);
    ed_grid->verts[5].tex_coords = vec2_t_c(0.0, 0.0);
    ed_grid->verts[5].color = vec4_t_c(1.0, 0.0, 0.0, 1.0);


    struct r_texture_t *diffuse;
    struct r_texture_t *normal;


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

void ed_w_ManipulatorWidgetSetupPickableDrawState(uint32_t pickable_index, struct ed_pickable_t *pickable)
{
    struct r_named_uniform_t *color_uniform = r_GetNamedUniform(ed_outline_shader, "ed_color");

    switch(pickable_index)
    {
        case 0:
            r_i_SetUniform(color_uniform, 1, &vec4_t_c(1.0, 0.0, 0.0, 1.0));
        break;

        case 1:
            r_i_SetUniform(color_uniform, 1, &vec4_t_c(0.0, 1.0, 0.0, 1.0));
        break;

        case 2:
            r_i_SetUniform(color_uniform, 1, &vec4_t_c(0.0, 0.0, 1.0, 1.0));
        break;
    }
}

/*
=============================================================
=============================================================
=============================================================
*/

void ed_w_AddSelection(struct ed_pickable_t *selection, uint32_t multiple_key_down, struct ds_list_t *selections)
{
    if(!selections)
    {
        selections = &ed_level_state.pickables.selections;
    }

    if(selection->selection_index != 0xffffffff)
    {
        uint32_t last_index = selections->cursor - 1;
        uint32_t selection_index = selection->selection_index;

        /* This selection already exists in the list. In this case, it can either be the
        main selection (last in the list), in which case it'll be dropped from the list, or
        it can be some other selection, in which case it'll be re-added at the back of the
        list, becoming the main selection. Either way, we need to remove it here. */
        ed_w_DropSelection(selection, selections);

        if(selection_index >= last_index)
        {
            /* pickable is the last in the list */
            if(multiple_key_down || !selections->cursor)
            {
                /* the behavior is, if this is the last pickable, there are more pickables
                in the list and the multiple selection key is down, this pickable gets dropped.
                If, instead, this is the last pickable, there are more pickables in the list
                but the key is not being held, then this pickable becomes the only pickable in
                the list. If, then, it gets selected again, without the multiple selection key
                down, it gets dropped. */
                return;
            }
        }
    }

    if(!multiple_key_down)
    {
        ed_w_ClearSelections(selections);
    }

    /* This is either a new selection, or an already existing selection becoming the main selection. */
    selection->selection_index = ds_list_add_element(selections, &selection);
}

void ed_w_DropSelection(struct ed_pickable_t *selection, struct ds_list_t *selections)
{
    if(!selections)
    {
        selections = &ed_level_state.pickables.selections;
    }

    if(selection && selection->selection_index != 0xffffffff)
    {
        uint32_t last_index = selections->cursor - 1;
        ds_list_remove_element(selections, selection->selection_index);

        if(selections->cursor && selection->selection_index != last_index)
        {
            struct ed_pickable_t *moved_pickable = *(struct ed_pickable_t **)ds_list_get_element(selections, selection->selection_index);
            moved_pickable->selection_index = selection->selection_index;
        }

        selection->selection_index = 0xffffffff;
    }
}

void ed_w_ClearSelections(struct ds_list_t *selections)
{
    if(!selections)
    {
        selections = &ed_level_state.pickables.selections;
    }

    for(uint32_t selection_index = 0; selection_index < selections->cursor; selection_index++)
    {
        struct ed_pickable_t *pickable = *(struct ed_pickable_t **)ds_list_get_element(selections, selection_index);
        pickable->selection_index = 0xffffffff;
    }

    selections->cursor = 0;
}

void ed_w_CopySelections(struct ds_list_t *selections)
{
    if(!selections)
    {
        selections = &ed_level_state.pickables.selections;
    }

    for(uint32_t selection_index = 0; selection_index < selections->cursor; selection_index++)
    {
        struct ed_pickable_t **selection_slot = ds_list_get_element(selections, selection_index);
        struct ed_pickable_t *src_pickable = *selection_slot;
        struct ed_pickable_t *dst_pickable = ed_CopyPickable(src_pickable);
        *selection_slot = dst_pickable;
        dst_pickable->selection_index = src_pickable->selection_index;
        src_pickable->selection_index = 0xffffffff;
    }
}

void ed_w_DeleteSelections()
{
    struct ds_list_t *selections = &ed_level_state.pickables.selections;
    for(uint32_t selection_index = 0; selection_index < selections->cursor; selection_index++)
    {
        struct ed_pickable_t *pickable = *(struct ed_pickable_t **)ds_list_get_element(selections, selection_index);
        ed_DestroyPickable(pickable);
    }

    ed_w_ClearSelections(selections);
}

void ed_LevelEditorTranslateSelected(vec3_t *translation, uint32_t transform_mode)
{
    struct ds_list_t *selections = &ed_level_state.pickables.selections;

    for(uint32_t selection_index = 0; selection_index < selections->cursor; selection_index++)
    {
        struct ed_pickable_t *pickable = *(struct ed_pickable_t **)ds_list_get_element(selections, selection_index);
        pickable->translation = *translation;
        pickable->transform_flags |= ED_PICKABLE_TRANSFORM_FLAG_TRANSLATION;
        ed_w_MarkPickableModified(pickable);
    }
}

void ed_LevelEditorRotateSelected(mat3_t *rotation, vec3_t *pivot, uint32_t transform_mode)
{
    struct ds_list_t *selections = &ed_level_state.pickables.selections;

    for(uint32_t selection_index = 0; selection_index < selections->cursor; selection_index++)
    {
        struct ed_pickable_t *pickable = *(struct ed_pickable_t **)ds_list_get_element(selections, selection_index);
        pickable->rotation = *rotation;
        pickable->transform_flags |= ED_PICKABLE_TRANSFORM_FLAG_ROTATION;

        if(pivot)
        {
            vec3_t pivot_pickable_vec;
            vec3_t_sub(&pivot_pickable_vec, &pickable->transform.rows[3].xyz, pivot);
            mat3_t_vec3_t_mul(&pivot_pickable_vec, &pivot_pickable_vec, rotation);
            vec3_t_add(&pivot_pickable_vec, &pivot_pickable_vec, pivot);
            vec3_t_sub(&pickable->translation, &pivot_pickable_vec, &pickable->transform.rows[3].xyz);
            pickable->transform_flags |= ED_PICKABLE_TRANSFORM_FLAG_TRANSLATION;
        }

        ed_w_MarkPickableModified(pickable);
    }
}

void ed_w_MarkPickableModified(struct ed_pickable_t *pickable)
{
    if(pickable && pickable->modified_index == 0xffffffff)
    {
        pickable->modified_index = ds_list_add_element(&ed_level_state.pickables.modified_pickables, &pickable);
    }
}

void ed_w_MarkBrushModified(struct ed_brush_t *brush)
{
    if(brush && brush->modified_index == 0xffffffff)
    {
        brush->modified_index = ds_list_add_element(&ed_level_state.pickables.modified_brushes, &brush);
    }
}

/*
=============================================================
=============================================================
=============================================================
*/

#define ED_LEVEL_FOOTER_WINDOW_HEIGHT 40

void ed_w_UpdateUI()
{
    int32_t mouse_x;
    int32_t mouse_y;

    in_GetMousePos(&mouse_x, &mouse_y);

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize;
    window_flags |= ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar;

    struct ds_list_t *selections = &ed_level_state.pickables.selections;

    if(ed_level_state.pickables.selections_window_open)
    {
        char node_label[32];
        igSetNextWindowPos((ImVec2){0.0, r_height - ED_LEVEL_FOOTER_WINDOW_HEIGHT}, 0, (ImVec2){0, 1});
        igSetNextWindowSizeConstraints((ImVec2){400, 0}, (ImVec2){400 , 400}, NULL, NULL);
        if(igBegin("Selections", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            for(uint32_t selection_index = 0; selection_index < selections->cursor; selection_index++)
            {
                struct ed_pickable_t *pickable = *(struct ed_pickable_t **)ds_list_get_element(selections, selection_index);
                static char *pickable_label[] =
                {
                    [ED_PICKABLE_TYPE_BRUSH] = "Brush",
                    [ED_PICKABLE_TYPE_LIGHT] = "Light",
                    [ED_PICKABLE_TYPE_ENTITY] = "Entity",
                    [ED_PICKABLE_TYPE_FACE] = "Face",
                    [ED_PICKABLE_TYPE_EDGE] = "Edge",
                    [ED_PICKABLE_TYPE_VERT] = "Vert",
                };

                sprintf(node_label, "%s##%d", pickable_label[pickable->type], pickable->index);

                if(igTreeNode_Str(node_label))
                {
                    igText("Pos: [%f %f %f]", pickable->transform.rows[3].x,
                                              pickable->transform.rows[3].y,
                                              pickable->transform.rows[3].z);
                    switch(pickable->type)
                    {
                        case ED_PICKABLE_TYPE_BRUSH:
                        {
                            struct ed_brush_t *brush = ed_GetBrush(pickable->primary_index);
                            igText("Verts: %d", brush->vertices.used);
                        }
                        break;

                        case ED_PICKABLE_TYPE_FACE:
                        {
                            struct ed_face_t *face = ed_GetFace(pickable->secondary_index);
                            if(igBeginCombo("Face material", face->material->name, 0))
                            {
                                for(uint32_t material_index = 0; material_index < r_materials.cursor; material_index++)
                                {
                                    struct r_material_t *material = r_GetMaterial(material_index);
                                    if(material)
                                    {
                                        if(igSelectable_Bool(material->name, material == face->material, 0, (ImVec2){0, 0}))
                                        {
                                            face->material = material;
                                            ed_w_MarkBrushModified(face->brush);
//                                            ed_SetFaceMaterial(face->brush, face->index, material);
                                        }
                                    }
                                }
                                igEndCombo();
                            }
                        }
                        break;

                        case ED_PICKABLE_TYPE_LIGHT:
                        {
                            struct r_light_t *light = r_GetLight(pickable->primary_index);

                            igSliderFloat3("Color", light->color.comps, 0.0, 1.0, "%0.2f", 0);
                            igSliderFloat("Radius", &light->range, 0.0, 100.0, "%0.2f", 0);
                            igSliderFloat("Energy", &light->energy, 0.0, 100.0, "%0.2f", 0);

                            if(light->type == R_LIGHT_TYPE_SPOT)
                            {
                                struct r_spot_light_t *spot_light = (struct r_spot_light_t *)light;
                                igSliderInt("Angle", &spot_light->angle, R_SPOT_LIGHT_MIN_ANGLE, R_SPOT_LIGHT_MAX_ANGLE, "%d", 0);
                                igSliderFloat("Softness", &spot_light->softness, 0.0, 1.0, "%0.2f", 0);
                            }
                        }
                        break;
                    }
                    igTreePop();
                }

                if(selection_index < selections->cursor - 1)
                {
                    igSeparator();
                }
            }
        }
        igEnd();
    }

    window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoCollapse;
    igSetNextWindowPos((ImVec2){r_width, 20}, 0, (ImVec2){1, 0});
    igSetNextWindowSize((ImVec2){500, 0}, 0);
    if(igBegin("Tools", NULL, window_flags))
    {
        if(igBeginTabBar("Tool tabs", 0))
        {
            if(igBeginTabItem("Brush", NULL, 0))
            {
                ed_level_state.selected_tools_tab = ED_L_TOOL_TAB_BRUSH;
                igEndTabItem();
            }

            if(igBeginTabItem("Light", NULL, 0))
            {
                ed_level_state.selected_tools_tab = ED_L_TOOL_TAB_LIGHT;
                uint32_t type = ed_level_state.pickables.light_type;

                if(igSelectable_Bool("Point", type == ED_LEVEL_LIGHT_TYPE_POINT, 0, (ImVec2){48, 0}))
                {
                    ed_level_state.pickables.light_type = ED_LEVEL_LIGHT_TYPE_POINT;
                }

                igSameLine(0, -1);

                if(igSelectable_Bool("Spot", type == ED_LEVEL_LIGHT_TYPE_SPOT, 0, (ImVec2){48, 0}))
                {
                    ed_level_state.pickables.light_type = ED_LEVEL_LIGHT_TYPE_SPOT;
                }
                igEndTabItem();
            }

            if(igBeginTabItem("Entity", NULL, 0))
            {
                ed_level_state.selected_tools_tab = ED_L_TOOL_TAB_ENTITY;
                uint32_t type = ed_level_state.pickables.light_type;

                struct e_ent_def_t *cur_def = ed_level_state.pickables.ent_def;
                for(uint32_t ent_def_index = 0; ent_def_index < e_ent_defs[E_ENT_DEF_TYPE_ROOT].cursor; ent_def_index++)
                {
                    struct e_ent_def_t *ent_def = e_GetEntDef(E_ENT_DEF_TYPE_ROOT, ent_def_index);

                    if(ent_def)
                    {
                        if(igSelectable_Bool(ent_def->name, ent_def == cur_def, 0, (ImVec2){48, 0}))
                        {
                            ed_level_state.pickables.ent_def = ent_def;
                        }
                    }
                }

                igEndTabItem();
            }

            if(igBeginTabItem("Enemies", NULL, 0))
            {
                ed_level_state.selected_tools_tab = ED_L_TOOL_TAB_ENEMY;
                uint32_t selected_enemy = ed_level_state.pickables.enemy_type;
                for(uint32_t enemy_type = G_ENEMY_TYPE_CAMERA; enemy_type < G_ENEMY_TYPE_LAST; enemy_type++)
                {
                    if(igSelectable_Bool(g_enemy_names[enemy_type], enemy_type == selected_enemy, 0, (ImVec2){48, 0}))
                    {
                        ed_level_state.pickables.enemy_type = enemy_type;
                    }
                }
                igEndTabItem();
            }

            if(igBeginTabItem("Material", NULL, 0))
            {
                ed_level_state.selected_tools_tab = ED_L_TOOL_TAB_MATERIAL;
                struct r_material_t *selected_material = ed_level_state.selected_material;
                struct r_material_t *default_material = r_GetDefaultMaterial();

                if(igBeginCombo("##materials", selected_material->name, 0))
                {
                    for(uint32_t material_index = 0; material_index < r_materials.cursor; material_index++)
                    {
                        struct r_material_t *material = r_GetMaterial(material_index);

                        if(material)
                        {
                            if(igSelectable_Bool(material->name, material == selected_material, ImGuiSelectableFlags_AllowItemOverlap, (ImVec2){0, 0}))
                            {
                                ed_level_state.selected_material = material;
                            }

                            if(material != default_material)
                            {
                                ImVec2 size;
                                igGetItemRectSize(&size);
                                igSameLine(size.x - 20, -1);
                                igPushID_Ptr(material);
                                if(igSmallButton("X"))
                                {
                                    r_DestroyMaterial(material);
                                }
                                igPopID();
                            }
                        }
                    }
                    igEndCombo();
                }

                igSameLine(0, -1);
                if(igSmallButton("+"))
                {
                    ed_level_state.selected_material = r_CreateMaterial("", default_material->diffuse_texture, default_material->normal_texture, default_material->roughness_texture);
                    selected_material = ed_level_state.selected_material;
                }

                igSeparator();

                ImGuiInputTextFlags text_input_flags = 0;

                if(selected_material == default_material)
                {
                    text_input_flags = ImGuiInputTextFlags_ReadOnly;
                }

                igInputText("Name", selected_material->name, sizeof(selected_material->name), text_input_flags, 0, NULL);
                if(igBeginCombo("Diffuse texture", selected_material->diffuse_texture->name, 0))
                {
                    if(selected_material != default_material)
                    {
                        struct r_texture_t *selected_texture = selected_material->diffuse_texture;
                        for(uint32_t texture_index = 0; texture_index < r_textures.cursor; texture_index++)
                        {
                            struct r_texture_t *texture = r_GetTexture(texture_index);

                            if(texture)
                            {
                                igPushID_Ptr(texture);
                                if(igSelectable_Bool(texture->name, selected_texture == texture, 0, (ImVec2){0, 0}))
                                {
                                    selected_material->diffuse_texture = texture;
                                }
                                igPopID();
                            }
                        }
                    }
                    igEndCombo();
                }

                if(igBeginCombo("Normal texture", selected_material->normal_texture->name, 0))
                {
                    if(selected_material != default_material)
                    {
                        struct r_texture_t *selected_texture = selected_material->normal_texture;
                        for(uint32_t texture_index = 0; texture_index < r_textures.cursor; texture_index++)
                        {
                            struct r_texture_t *texture = r_GetTexture(texture_index);

                            if(texture)
                            {
                                if(igSelectable_Bool(texture->name, selected_texture == texture, 0, (ImVec2){0, 0}))
                                {
                                    selected_material->normal_texture = texture;
                                }
                            }
                        }
                    }
                    igEndCombo();
                }

                if(igBeginCombo("Roughness texture", selected_material->roughness_texture->name, 0))
                {
                    if(selected_material != default_material)
                    {
                        struct r_texture_t *selected_texture = selected_material->roughness_texture;
                        for(uint32_t texture_index = 0; texture_index < r_textures.cursor; texture_index++)
                        {
                            struct r_texture_t *texture = r_GetTexture(texture_index);

                            if(texture)
                            {
                                if(igSelectable_Bool(texture->name, selected_texture == texture, 0, (ImVec2){0, 0}))
                                {
                                    selected_material->roughness_texture = texture;
                                }
                            }
                        }
                    }
                    igEndCombo();
                }

                igEndTabItem();
            }

            igEndTabBar();
        }
    }
    igEnd();

//    if(igBegin("Tools window", NULL, window_flags))
//    {
//        switch(ed_level_state.pickables.secondary_click_function)
//        {
//            case ED_LEVEL_SECONDARY_CLICK_FUNC_BRUSH:
//            {
//                uint32_t func = ed_level_state.brush.selected_tool;
//                if(igBeginChild_Str("Brush tools", (ImVec2){48, 0}, 0, 0))
//                {
//                    if(igSelectable_Bool("Create", func == ED_LEVEL_BRUSH_TOOL_CREATE, 0, (ImVec2){48, 48}))
//                    {
//                        ed_level_state.brush.selected_tool = ED_LEVEL_BRUSH_TOOL_CREATE;
//                    }
//                }
//                igEndChild();
//            }
//            break;
//
//            case ED_LEVEL_SECONDARY_CLICK_FUNC_LIGHT:
//            {
//                uint32_t type = ed_level_state.pickables.light_type;
//                if(igBeginChild_Str("Light types", (ImVec2){48, 0}, 0, 0))
//                {
//                    if(igSelectable_Bool("Point", type == ED_LEVEL_LIGHT_TYPE_POINT, 0, (ImVec2){48, 48}))
//                    {
//                        ed_level_state.pickables.light_type = ED_LEVEL_LIGHT_TYPE_POINT;
//                    }
//                    if(igSelectable_Bool("Spot", type == ED_LEVEL_LIGHT_TYPE_SPOT, 0, (ImVec2){48, 48}))
//                    {
//                        ed_level_state.pickables.light_type = ED_LEVEL_LIGHT_TYPE_SPOT;
//                    }
//                }
//                igEndChild();
//            }
//            break;
//
//            case ED_LEVEL_SECONDARY_CLICK_FUNC_ENTITY:
//            {
//                uint32_t type = ed_level_state.pickables.light_type;
//                if(igBeginChild_Str("Ent defs", (ImVec2){120, 0}, 0, 0))
//                {
//                    struct e_ent_def_t *cur_def = ed_level_state.pickables.ent_def;
//
//                    for(uint32_t ent_def_index = 0; ent_def_index < e_ent_defs[E_ENT_DEF_TYPE_ROOT].cursor; ent_def_index++)
//                    {
//                        struct e_ent_def_t *ent_def = e_GetEntDef(E_ENT_DEF_TYPE_ROOT, ent_def_index);
//
//                        if(ent_def)
//                        {
//                            if(igSelectable_Bool(ent_def->name, ent_def == cur_def, 0, (ImVec2){0, 32}))
//                            {
//                                ed_level_state.pickables.ent_def = ent_def;
//                            }
//                        }
//                    }
//                }
//                igEndChild();
//            }
//            break;
//        }
//
//        igSameLine(0.0, -1.0);
//        igSeparatorEx(ImGuiSeparatorFlags_Vertical);
//        igSameLine(0.0, -1.0);
//
//        if(igBeginChild_Str("Tool buttons", (ImVec2){48, 0}, 0, 0))
//        {
//            uint32_t func = ed_level_state.pickables.secondary_click_function;
//
//            if(igSelectable_Bool("Brush", func == ED_LEVEL_SECONDARY_CLICK_FUNC_BRUSH, 0, (ImVec2){0, 0}))
//            {
//                ed_level_state.pickables.secondary_click_function = ED_LEVEL_SECONDARY_CLICK_FUNC_BRUSH;
//            }
//
//            if(igSelectable_Bool("Light", func == ED_LEVEL_SECONDARY_CLICK_FUNC_LIGHT, 0, (ImVec2){0, 0}))
//            {
//                ed_level_state.pickables.secondary_click_function = ED_LEVEL_SECONDARY_CLICK_FUNC_LIGHT;
//            }
//
//            if(igSelectable_Bool("Entity", func == ED_LEVEL_SECONDARY_CLICK_FUNC_ENTITY, 0, (ImVec2){0, 0}))
//            {
//                ed_level_state.pickables.secondary_click_function = ED_LEVEL_SECONDARY_CLICK_FUNC_ENTITY;
//            }
//        }
//        igEndChild();
//    }
//    igEnd();

    igSetNextWindowBgAlpha(0.5);
    igPushStyleVar_Float(ImGuiStyleVar_WindowBorderSize, 0.0);
    igSetNextWindowSize((ImVec2){r_width, 0}, 0);
    igSetNextWindowPos((ImVec2){0.0, r_height}, 0, (ImVec2){0, 1});
    if(igBegin("Footer window", NULL, window_flags))
    {
//        char *transform_type_text = NULL;
//        char *transform_mode_text = NULL;

//        switch(ed_level_state.manipulator.transform_type)
//        {
//            case ED_L_TRANSFORM_TYPE_TRANSLATION:
//                transform_type_text = "Translation";
//            break;
//
//            case ED_L_TRANSFORM_TYPE_ROTATION:
//                transform_type_text = "Rotation";
//            break;
//        }
//
//        switch(ed_level_state.manipulator.transform_mode)
//        {
//            case ED_L_TRANSFORM_MODE_WORLD:
//                transform_mode_text = "World";
//            break;
//
//            case ED_L_TRANSFORM_MODE_LOCAL:
//                transform_mode_text = "Local";
//            break;
//        }


//        if(igBeginChild_Str("left_side", (ImVec2){0, 24}, 0, 0))
//        {
            igPushStyleVar_Float(ImGuiStyleVar_Alpha, 0.5);

            if(igBeginTable("Stats table", 4, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_Hideable, (ImVec2){0.0, 0.0}, 0.0))
            {
                igTableNextRow(0, 0.0);

                char selected_label[32];
                sprintf(selected_label, "Selected: %d  ", selections->cursor);

                igTableNextColumn();
                igSetNextItemWidth(100.0);
                if(igSelectable_Bool(selected_label, ed_level_state.pickables.selections_window_open, 0, (ImVec2){0, 0}))
                {
                    ed_level_state.pickables.selections_window_open = !ed_level_state.pickables.selections_window_open;
                }

                uint32_t transform_type = ed_level_state.manipulator.transform_type;
                uint32_t transform_mode = ed_level_state.manipulator.transform_mode;

                igTableNextColumn();
                igText("Manipulator: %s", ed_l_transform_type_texts[transform_type]);
                igSameLine(0, -1);
                if(igBeginCombo("##tranfsorm_mode", ed_l_transform_mode_texts[transform_mode], 0))
                {
                    for(uint32_t mode = ED_L_TRANSFORM_MODE_WORLD; mode < ED_L_TRANSFORM_MODE_LAST; mode++)
                    {
                        if(igSelectable_Bool(ed_l_transform_mode_texts[mode], 0, 0, (ImVec2){0, 0}))
                        {
                            ed_level_state.manipulator.transform_mode = mode;
                        }
                    }
                    igEndCombo();
                }
                igTableNextColumn();

                char snap_label[32];
                char snap_preview[32];
                igText("Snap: ");
                igSameLine(0.0, -1.0);
                if(ed_level_state.manipulator.transform_type == ED_L_TRANSFORM_TYPE_ROTATION)
                {
                    sprintf(snap_preview, "%0.4f deg", ed_level_state.manipulator.angular_snap * 180.0);
                    igSetNextItemWidth(120.0);
                    if(igBeginCombo("##angular_snap", snap_preview, 0))
                    {
                        for(uint32_t index = 0; index < sizeof(ed_w_angular_snap_values) / sizeof(ed_w_angular_snap_values[0]); index++)
                        {
                            sprintf(snap_label, "%f", ed_w_angular_snap_values[index] * 180.0);

                            if(igSelectable_Bool(snap_label, 0, 0, (ImVec2){0.0, 0.0}))
                            {
                                ed_level_state.manipulator.angular_snap = ed_w_angular_snap_values[index];
                            }
                        }

                        igEndCombo();
                    }
                }
                else
                {
                    sprintf(snap_preview, "%0.4f m", ed_level_state.manipulator.linear_snap);
                    igSetNextItemWidth(120.0);
                    if(igBeginCombo("##linear_snap", snap_preview, 0))
                    {
                        for(uint32_t index = 0; index < sizeof(ed_w_linear_snap_values) / sizeof(ed_w_linear_snap_values[0]); index++)
                        {
                            sprintf(snap_label, "%f", ed_w_linear_snap_values[index]);

                            if(igSelectable_Bool(snap_label, 0, 0, (ImVec2){0.0, 0.0}))
                            {
                                ed_level_state.manipulator.linear_snap = ed_w_linear_snap_values[index];
                            }
                        }

                        igEndCombo();
                    }
                }

                igTableNextColumn();

                igEndTable();
            }
            igPopStyleVar(1);
    }
    igEnd();
    igPopStyleVar(1);


//    ImGuiWindowFlags footer_flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
//    footer_flags |= ImGuiWindowFlags_NoCollapse;
//    footer_flags |= ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoTitleBar;
//
//    igSetNextWindowPos((ImVec2){0.0, r_height - 24}, 0, (ImVec2){0, 0});
//    igSetNextWindowSize((ImVec2){r_width, 24}, 0);
//    igPushStyleVar_Vec2(ImGuiStyleVar_WindowPadding, (ImVec2){2, 2});
//    if(igBegin("##Footer", NULL, footer_flags))
//    {
//        if(igButton("Brush", (ImVec2){0.0, 20.0}))
//        {
//            ed_level_state.pickables.secondary_click_function = ED_LEVEL_SECONDARY_CLICK_FUNC_BRUSH;
//        }
//
//        igSameLine(0.0, -1.0);
//
//        if(igButton("Light", (ImVec2){0.0, 20.0}))
//        {
//            ed_level_state.pickables.secondary_click_function = ED_LEVEL_SECONDARY_CLICK_FUNC_LIGHT;
//        }
//    }
//    igEnd();

//    igPopStyleVar(1);
}

void ed_w_UpdateManipulator()
{
    struct ds_list_t *selections = &ed_level_state.pickables.selections;
    ed_level_state.manipulator.visible = selections->cursor;
    mat4_t *transform = &ed_level_state.manipulator.transform;

    if(ed_level_state.manipulator.visible)
    {
        mat4_t_identity(transform);
        vec3_t *translation = &transform->rows[3].xyz;

        for(uint32_t selection_index = 0; selection_index < selections->cursor; selection_index++)
        {
            struct ed_pickable_t *pickable = *(struct ed_pickable_t **)ds_list_get_element(selections, selection_index);
            vec3_t_add(translation, translation, &pickable->transform.rows[3].xyz);
        }

        vec3_t_div(translation, translation, (float)selections->cursor);

        if(selections->cursor == 1 && ed_level_state.manipulator.transform_mode == ED_L_TRANSFORM_MODE_LOCAL)
        {
            struct ed_pickable_t *pickable = *(struct ed_pickable_t **)ds_list_get_element(selections, 0);
            transform->rows[0].xyz = pickable->transform.rows[0].xyz;
            transform->rows[1].xyz = pickable->transform.rows[1].xyz;
            transform->rows[2].xyz = pickable->transform.rows[2].xyz;
        }
    }
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

void ed_w_UpdatePickableObjects()
{
    struct ds_list_t *pickables = &ed_level_state.pickables.modified_pickables;
//    struct e_entity_t *entity;

    uint32_t pickable_index = 0;
    uint32_t pickable_count = pickables->cursor;

    /* here we iterate over pickables marked as modified, so we can update the objects they reference. During
    update, it may be necessary to mark additional pickables as modified. This currently happens only with
    pickables that reference brushes, and is particularly noticeable in the case when a pickable that references
    a brush face/edge/vertex gets modified by the user. Those directly modified pickables will be process twice
    by the loop -- first, their transforms will be applied to brush geometry; then, their transforms will be
    recomputed based on the up to date brush geometry. */

    do
    {
        for(; pickable_index < pickable_count; pickable_index++)
        {
            struct ed_pickable_t *pickable = *(struct ed_pickable_t **)ds_list_get_element(pickables, pickable_index);

            switch(pickable->type)
            {
                case ED_PICKABLE_TYPE_BRUSH:
                {
                    struct ed_brush_t *brush = ed_GetBrush(pickable->primary_index);
                    struct r_batch_t *first_batch = brush->model->batches.buffer;
                    pickable->ranges->start = first_batch->start;
                    pickable->ranges->count = brush->model->indices.buffer_size;

                    ed_level_state.world_data_stale = 1;

                    if(pickable->transform_flags)
                    {
                        /* this pickable got directly modified by the user, so we'll apply the
                        transforms here */
                        if(pickable->transform_flags & ED_PICKABLE_TRANSFORM_FLAG_TRANSLATION)
                        {
                            vec3_t_add(&brush->position, &brush->position, &pickable->translation);
                        }

                        if(pickable->transform_flags & ED_PICKABLE_TRANSFORM_FLAG_ROTATION)
                        {
                            mat3_t_mul(&brush->orientation, &brush->orientation, &pickable->rotation);
                        }

                        brush->update_flags |= ED_BRUSH_UPDATE_FLAG_UV_COORDS;

                        /* update brush to update its entity transform */
                        ed_w_MarkBrushModified(brush);
                    }

                    struct ed_face_t *face = brush->faces;

                    while(face)
                    {
                        /* face/edge/vertice pickables also depend on the up to date brush transform,
                        so mark those as modified so they can have their transforms updated as well */
                        ed_w_MarkPickableModified(face->pickable);

                        struct ed_face_polygon_t *polygon = face->polygons;

                        while(polygon)
                        {

                            struct ed_edge_t *edge = polygon->edges;

                            while(edge)
                            {
                                uint32_t polygon_side = edge->polygons[1].polygon == polygon;
                                ed_w_MarkPickableModified(edge->pickable);
                                edge = edge->polygons[polygon_side].next;
                            }

                            polygon = polygon->next;
                        }

                        face = face->next;
                    }

                    mat4_t_comp(&pickable->transform, &brush->orientation, &brush->position);
//                    pickable->draw_transform = pickable->transform;
                }
                break;

                case ED_PICKABLE_TYPE_FACE:
                {
                    struct ed_brush_t *brush = ed_GetBrush(pickable->primary_index);
                    struct ed_face_t *face = ed_GetFace(pickable->secondary_index);
                    struct r_batch_t *first_batch = brush->model->batches.buffer;

                    if(pickable->range_count > face->clipped_polygon_count)
                    {
                        while(pickable->range_count > face->clipped_polygon_count)
                        {
                            struct ed_pickable_range_t *next_range = pickable->ranges->next;
                            next_range->prev = NULL;

                            ed_FreePickableRange(pickable->ranges);
                            pickable->range_count--;
                            pickable->ranges = next_range;
                        }
                    }
                    else if(pickable->range_count < face->clipped_polygon_count)
                    {
                        while(pickable->range_count < face->clipped_polygon_count)
                        {
                            struct ed_pickable_range_t *new_range = ed_AllocPickableRange();
                            new_range->next = pickable->ranges;
                            if(pickable->ranges)
                            {
                                pickable->ranges->prev = new_range;
                            }
                            pickable->ranges = new_range;
                            pickable->range_count++;
                        }
                    }

                    struct ed_bsp_polygon_t *polygon = face->clipped_polygons;
                    struct ed_pickable_range_t *range = pickable->ranges;

                    while(polygon)
                    {
                        range->start = polygon->model_start + first_batch->start;
                        range->count = polygon->model_count;

                        polygon = polygon->next;
                        range = range->next;
                    }

                    if(pickable->transform_flags)
                    {
                        /* this pickable was directly modified by the user, so we'll apply the transforms
                        here*/

                        if(pickable->transform_flags & ED_PICKABLE_TRANSFORM_FLAG_TRANSLATION)
                        {
                            ed_TranslateBrushFace(brush, pickable->secondary_index, &pickable->translation);
                        }

                        if(pickable->transform_flags & ED_PICKABLE_TRANSFORM_FLAG_ROTATION)
                        {
                            ed_RotateBrushFace(brush, pickable->secondary_index, &pickable->rotation);
                        }

                        /* we're modifying brush geometry here, so mark the brush as modified so things
                        like face polygon normals/centers/uvs/mesh can be recomputed */
                        ed_w_MarkBrushModified(brush);

                        /* the brush pickable position depends on the position of its vertices (the brush origin
                        needs to be recomputed every time some vertice gets moved), so mark it as modified,
                        so it can have its transform recomputed */
                        ed_w_MarkPickableModified(brush->pickable);
                    }
                    else
                    {
                        /* this pickable wasn't modified directly, but was marked as modified because its transform
                        was affected by some brush geometry/transform change */
                        vec3_t face_position = face->center;
                        mat3_t_vec3_t_mul(&face_position, &face_position, &brush->orientation);

                        mat4_t_comp(&pickable->transform, &brush->orientation, &brush->position);
                        vec3_t_add(&pickable->transform.rows[3].xyz, &pickable->transform.rows[3].xyz, &face_position);

                        mat4_t draw_offset;
                        mat4_t_identity(&draw_offset);
                        vec3_t_mul(&draw_offset.rows[3].xyz, &face->center, -1.0);
//                        mat4_t_mul(&draw_offset, &draw_offset, &pickable->transform);

                        struct ed_pickable_range_t *range = pickable->ranges;

                        while(range)
                        {
                            range->offset = draw_offset;
                            range = range->next;
                        }
                    }
                }
                break;

                case ED_PICKABLE_TYPE_EDGE:
                {
                    struct ed_brush_t *brush = ed_GetBrush(pickable->primary_index);
                    struct ed_edge_t *edge = ed_GetEdge(pickable->secondary_index);
                    struct r_model_t *model = brush->model;
                    struct r_batch_t *first_batch = model->batches.buffer;

                    if(!pickable->range_count)
                    {
                        pickable->ranges = ed_AllocPickableRange();
                        pickable->range_count = 1;
                    }

                    pickable->ranges->start = first_batch->start + edge->model_start;
                    pickable->ranges->count = 2;

                    if(pickable->transform_flags)
                    {
                        if(pickable->transform_flags & ED_PICKABLE_TRANSFORM_FLAG_TRANSLATION)
                        {
                            ed_TranslateBrushEdge(brush, pickable->secondary_index, &pickable->translation);
                        }

                        ed_w_MarkBrushModified(brush);

                        ed_w_MarkPickableModified(brush->pickable);
                    }
                    else
                    {
                        struct ed_vert_t *vert0 = edge->verts[0].vert;
                        struct ed_vert_t *vert1 = edge->verts[1].vert;

                        vec3_t edge_center;
                        vec3_t_add(&edge_center, &vert0->vert, &vert1->vert);
                        vec3_t_mul(&edge_center, &edge_center, 0.5);

                        mat3_t_vec3_t_mul(&edge_center, &edge_center, &brush->orientation);
                        mat4_t_comp(&pickable->transform, &brush->orientation, &brush->position);
                        vec3_t_add(&pickable->transform.rows[3].xyz, &pickable->transform.rows[3].xyz, &edge_center);
                        vec3_t_mul(&pickable->ranges->offset.rows[3].xyz, &edge_center, -1.0);

                    }
                }
                break;

                case ED_PICKABLE_TYPE_LIGHT:
                {
                    struct r_light_t *light = r_GetLight(pickable->primary_index);
                    mat3_t rot;

                    if(pickable->transform_flags & ED_PICKABLE_TRANSFORM_FLAG_TRANSLATION)
                    {
                        vec3_t_add(&light->position, &light->position, &pickable->translation);
                    }

                    if(light->type == R_LIGHT_TYPE_SPOT && (pickable->transform_flags & ED_PICKABLE_TRANSFORM_FLAG_ROTATION))
                    {
                        struct r_spot_light_t *spot_light = (struct r_spot_light_t *)light;
                        mat3_t_mul(&spot_light->orientation, &spot_light->orientation, &pickable->rotation);
                    }

                    mat3_t_identity(&rot);
                    mat4_t_comp(&pickable->transform, &rot, &light->position);
                }
                break;

                case ED_PICKABLE_TYPE_ENTITY:
                {
                    struct e_entity_t *entity = e_GetEntity(pickable->primary_index);

                    if(pickable->transform_flags)
                    {
                        if(pickable->transform_flags & ED_PICKABLE_TRANSFORM_FLAG_TRANSLATION)
                        {
                            e_TranslateEntity(entity, &pickable->translation);
                        }

                        if(pickable->transform_flags & ED_PICKABLE_TRANSFORM_FLAG_ROTATION)
                        {
                            e_RotateEntity(entity, &pickable->rotation);
                        }
                    }

                    ed_UpdateEntityPickableRanges(pickable, entity);
                    e_UpdateEntityNode(entity->node, &mat4_t_c_id());
                    pickable->transform = entity->transform->transform;
                }
                break;

                case ED_PICKABLE_TYPE_ENEMY:
                {
                    struct g_enemy_t *enemy = g_GetEnemy(pickable->secondary_index, pickable->primary_index);

                    if(pickable->transform_flags & ED_PICKABLE_TRANSFORM_FLAG_TRANSLATION)
                    {
                        g_TranslateEnemy(enemy, &pickable->translation);
                    }

                    if(pickable->transform_flags & ED_PICKABLE_TRANSFORM_FLAG_ROTATION)
                    {
                        g_RotateEnemy(enemy, &pickable->rotation);
                    }

                    ed_UpdateEntityPickableRanges(pickable, enemy->entity);
                    e_UpdateEntityNode(enemy->entity->node, &mat4_t_c_id());
                    pickable->transform = enemy->entity->transform->transform;
                }
                break;
            }

            pickable->transform_flags = 0;
            pickable->modified_index = 0xffffffff;
        }

        struct ds_list_t *modified_brushes = &ed_level_state.pickables.modified_brushes;

        for(uint32_t brush_index = 0; brush_index < modified_brushes->cursor; brush_index++)
        {
            struct ed_brush_t *brush = *(struct ed_brush_t **)ds_list_get_element(modified_brushes, brush_index);
            ed_UpdateBrush(brush);
            brush->modified_index = 0xffffffff;
        }

        modified_brushes->cursor = 0;

        pickable_count = pickables->cursor;
    }
    while(pickable_index < pickable_count);

    pickables->cursor = 0;
}

void ed_l_Update()
{
    r_SetViewPos(&ed_level_state.camera_pos);
    r_SetViewPitchYaw(ed_level_state.camera_pitch, ed_level_state.camera_yaw);

    ed_w_UpdateUI();
    ed_w_UpdatePickableObjects();
    ed_w_UpdateManipulator();

    if(ed_level_state.world_data_stale && l_world_collider)
    {
        ed_level_state.world_data_stale = 0;
        ed_l_ClearWorldData();
        ed_l_RestoreBrushEntities();
    }

    ed_LevelEditorDrawGrid();
    ed_LevelEditorDrawSelections();
    ed_LevelEditorDrawLights();
    ed_LevelEditorDrawWidgets();

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

void ed_LevelEditorDrawManipulator()
{
    if(ed_level_state.manipulator.visible)
    {
        struct ed_widget_t *manipulator = ed_level_state.manipulator.widgets[ed_level_state.manipulator.transform_type];
        ed_DrawWidget(manipulator, &ed_level_state.manipulator.transform);
    }
}

void ed_LevelEditorDrawWidgets()
{
    r_i_SetShader(ed_outline_shader);
    ed_LevelEditorDrawManipulator();
}

void ed_LevelEditorDrawGrid()
{
    r_i_SetModelMatrix(NULL);
    r_i_SetViewProjectionMatrix(NULL);
    r_i_SetBlending(GL_TRUE, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    r_i_SetStencil(GL_FALSE, GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE);
    r_i_SetRasterizer(GL_FALSE, GL_BACK, GL_FILL);
    r_i_SetShader(ed_center_grid_shader);
    r_i_DrawVerts(R_I_DRAW_CMD_TRIANGLE_LIST, ed_grid, 1.0);
    r_i_SetShader(NULL);
    r_i_SetModelMatrix(NULL);
    r_i_SetViewProjectionMatrix(NULL);
    r_i_SetBlending(GL_FALSE, GL_ONE, GL_ZERO);
    r_i_DrawLine(&vec3_t_c(-10000.0, 0.0, 0.0), &vec3_t_c(10000.0, 0.0, 0.0), &vec4_t_c(1.0, 0.0, 0.0, 1.0), 3.0);
    r_i_DrawLine(&vec3_t_c(0.0, 0.0, -10000.0), &vec3_t_c(0.0, 0.0, 10000.0), &vec4_t_c(0.0, 0.0, 1.0, 1.0), 3.0);
}

//void ed_LevelEditorDrawBrushes()
//{
////    return;
//
////    for(uint32_t brush_index = 0; brush_index < ed_level_state.brush.brushes.cursor; brush_index++)
////    {
////        struct ed_brush_t *brush = ed_GetBrush(brush_index);
////
////        if(brush)
////        {
////            mat4_t transform;
////            mat4_t_identity(&transform);
////            mat4_t_comp(&transform, &brush->orientation, &brush->position);
////            r_DrawEntity(&transform, brush->model);
////        }
////    }
//}

void ed_LevelEditorDrawLights()
{
    r_i_SetModelMatrix(NULL);
    r_i_SetViewProjectionMatrix(NULL);
    r_i_SetShader(NULL);

    for(uint32_t light_index = 0; light_index < r_lights[R_LIGHT_TYPE_POINT].cursor; light_index++)
    {
        struct r_light_t *light = r_GetLight(R_LIGHT_INDEX(R_LIGHT_TYPE_POINT, light_index));

        if(light)
        {
            vec3_t position = light->position;
            vec4_t color = vec4_t_c(light->color.x, light->color.y, light->color.z, 1.0);
            r_i_DrawPoint(&position, &color, 8.0);
        }
    }


    for(uint32_t light_index = 0; light_index < r_lights[R_LIGHT_TYPE_SPOT].cursor; light_index++)
    {
        struct r_light_t *light = r_GetLight(R_LIGHT_INDEX(R_LIGHT_TYPE_SPOT, light_index));

        if(light)
        {
            vec3_t position = light->position;
            vec4_t color = vec4_t_c(light->color.x, light->color.y, light->color.z, 1.0);
            r_i_DrawPoint(&position, &color, 8.0);
        }
    }
}

void ed_LevelEditorDrawSelections()
{
    struct ds_list_t *selections = &ed_level_state.pickables.selections;

    if(selections->cursor)
    {
        r_i_SetViewProjectionMatrix(NULL);
        r_i_SetShader(ed_outline_shader);
        r_i_SetBuffers(NULL, NULL);
        r_i_SetRasterizer(GL_TRUE, GL_FRONT, GL_LINE);
        r_i_SetModelMatrix(NULL);

        uint32_t selection_count = selections->cursor - 1;
        uint32_t selection_index = 0;
        uint8_t stencil_value = 1;

        struct r_named_uniform_t *color_uniform = r_GetNamedUniform(ed_outline_shader, "ed_color");

        for(uint32_t index = 0; index < 2; index++)
        {
            for(; selection_index < selection_count; selection_index++)
            {
                struct ed_pickable_t *pickable = *(struct ed_pickable_t **)ds_list_get_element(selections, selection_index);

                mat4_t base_model_view_projection_matrix;
                mat4_t model_view_projection_matrix;
                ed_PickableModelViewProjectionMatrix(pickable, NULL, &base_model_view_projection_matrix);
//                r_i_SetViewProjectionMatrix(&model_view_projection_matrix);
                struct r_i_draw_list_t *draw_list = NULL;

                switch(pickable->type)
                {
                    case ED_PICKABLE_TYPE_FACE:
                    {
                        struct ed_pickable_range_t *range = pickable->ranges;
                        while(range)
                        {
                            mat4_t_mul(&model_view_projection_matrix, &range->offset, &base_model_view_projection_matrix);
                            draw_list = r_i_AllocDrawList(1);
                            draw_list->commands->start = range->start;
                            draw_list->commands->count = range->count;
                            draw_list->size = 6.0;
                            draw_list->indexed = 1;

                            r_i_SetViewProjectionMatrix(&model_view_projection_matrix);
                            r_i_SetDrawMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE, 0xff);
                            r_i_SetStencil(GL_TRUE, GL_KEEP, GL_KEEP, GL_REPLACE, GL_ALWAYS, 0xff, 0xff);
                            r_i_SetDepth(GL_TRUE, GL_ALWAYS);
                            r_i_SetRasterizer(GL_TRUE, GL_BACK, GL_FILL);
                            r_i_DrawImmediate(R_I_DRAW_CMD_TRIANGLE_LIST, draw_list);
                            range = range->next;
                        }

                        r_i_SetUniform(color_uniform, 1, &vec4_t_c(0.2, 0.7, 0.4, 1.0));
                        range = pickable->ranges;
                        while(range)
                        {
                            mat4_t_mul(&model_view_projection_matrix, &range->offset, &base_model_view_projection_matrix);
                            draw_list = r_i_AllocDrawList(1);
                            draw_list->commands->start = range->start;
                            draw_list->commands->count = range->count;
                            draw_list->size = 6.0;
                            draw_list->indexed = 1;

                            r_i_SetViewProjectionMatrix(&model_view_projection_matrix);
                            r_i_SetDrawMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
                            r_i_SetStencil(GL_TRUE, GL_KEEP, GL_KEEP, GL_KEEP, GL_EQUAL, 0xff, 0x00);
                            r_i_SetRasterizer(GL_TRUE, GL_BACK, GL_LINE);
                            r_i_DrawImmediate(R_I_DRAW_CMD_TRIANGLE_LIST, draw_list);

                            range = range->next;
                        }

                        r_i_SetUniform(color_uniform, 1, &vec4_t_c(0.3, 0.4, 1.0, 0.4));
                        range = pickable->ranges;
                        while(range)
                        {
                            mat4_t_mul(&model_view_projection_matrix, &range->offset, &base_model_view_projection_matrix);
                            draw_list = r_i_AllocDrawList(1);
                            draw_list->commands->start = range->start;
                            draw_list->commands->count = range->count;
                            draw_list->size = 4.0;
                            draw_list->indexed = 1;

                            r_i_SetViewProjectionMatrix(&model_view_projection_matrix);
                            r_i_SetBlending(GL_TRUE, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                            r_i_SetDrawMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE, 0xff);
                            r_i_SetStencil(GL_TRUE, GL_KEEP, GL_KEEP, GL_REPLACE, GL_ALWAYS, 0xff, 0x00);
                            r_i_SetDepth(GL_TRUE, GL_ALWAYS);
                            r_i_SetRasterizer(GL_TRUE, GL_BACK, GL_FILL);
                            r_i_DrawImmediate(R_I_DRAW_CMD_TRIANGLE_LIST, draw_list);

                            range = range->next;
                        }
                    }
                    break;


                    case ED_PICKABLE_TYPE_EDGE:
                    {
                        draw_list = r_i_AllocDrawList(1);
                        struct ed_pickable_range_t *range = pickable->ranges;

                        mat4_t_mul(&model_view_projection_matrix, &range->offset, &base_model_view_projection_matrix);
                        r_i_SetUniform(color_uniform, 1, &vec4_t_c(1.0, 0.7, 0.4, 1.0));

                        draw_list->commands->start = range->start;
                        draw_list->commands->count = range->count;
                        draw_list->size = 4.0;
                        draw_list->indexed = 1;

                        r_i_SetViewProjectionMatrix(&model_view_projection_matrix);
                        r_i_SetDrawMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE, 0xff);
                        r_i_SetDepth(GL_TRUE, GL_ALWAYS);
                        r_i_DrawImmediate(R_I_DRAW_CMD_LINE_LIST, draw_list);
                    }
                    break;

                    default:
                        if(index)
                        {
                            r_i_SetUniform(color_uniform, 1, &vec4_t_c(1.0, 0.4, 0.0, 1.0));
                        }
                        else
                        {
                            r_i_SetUniform(color_uniform, 1, &vec4_t_c(1.0, 0.2, 0.0, 1.0));
                        }

                        r_i_SetViewProjectionMatrix(&base_model_view_projection_matrix);


                        draw_list = r_i_AllocDrawList(1);
                        draw_list->commands[0].start = pickable->ranges->start;
                        draw_list->commands[0].count = pickable->ranges->count;
                        draw_list->size = 4.0;
                        draw_list->indexed = 1;
//
                        r_i_SetDrawMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE, 0xff);
                        r_i_SetDepth(GL_TRUE, GL_ALWAYS);
                        r_i_SetStencil(GL_TRUE, GL_KEEP, GL_KEEP, GL_REPLACE, GL_ALWAYS, 0xff, 0xff);
                        r_i_SetRasterizer(GL_TRUE, GL_FRONT, GL_FILL);
                        r_i_DrawImmediate(R_I_DRAW_CMD_TRIANGLE_LIST, draw_list);



                        draw_list = r_i_AllocDrawList(1);
                        draw_list->commands[0].start = pickable->ranges->start;
                        draw_list->commands[0].count = pickable->ranges->count;
                        draw_list->size = 4.0;
                        draw_list->indexed = 1;

                        r_i_SetDrawMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE, 0xff);
                        r_i_SetDepth(GL_TRUE, GL_ALWAYS);
                        r_i_SetStencil(GL_TRUE, GL_KEEP, GL_KEEP, GL_KEEP, GL_EQUAL, 0xff, 0x00);
                        r_i_SetRasterizer(GL_TRUE, GL_FRONT, GL_LINE);
                        r_i_DrawImmediate(R_I_DRAW_CMD_TRIANGLE_LIST, draw_list);



                        draw_list = r_i_AllocDrawList(1);
                        draw_list->commands[0].start = pickable->ranges->start;
                        draw_list->commands[0].count = pickable->ranges->count;
                        draw_list->size = 4.0;
                        draw_list->indexed = 1;

                        r_i_SetDrawMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE, 0xff);
                        r_i_SetDepth(GL_TRUE, GL_ALWAYS);
                        r_i_SetStencil(GL_TRUE, GL_KEEP, GL_KEEP, GL_REPLACE, GL_ALWAYS, 0xff, 0x00);
                        r_i_SetRasterizer(GL_TRUE, GL_FRONT, GL_FILL);
                        r_i_DrawImmediate(R_I_DRAW_CMD_TRIANGLE_LIST, draw_list);
                    break;
                }
            }

            selection_count++;
        }

        r_i_SetRasterizer(GL_TRUE, GL_BACK, GL_FILL);
        r_i_SetDepth(GL_TRUE, GL_LESS);
        r_i_SetBlending(GL_FALSE, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        r_i_SetDrawMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE, 0xff);
        r_i_SetStencil(GL_FALSE, GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE);
        r_i_SetBlending(GL_FALSE, GL_NONE, GL_NONE);
    }

    struct ds_list_t *enemy_list = &ed_level_state.pickables.game_pickables[ED_PICKABLE_TYPE_ENEMY];

    for(uint32_t index = 0; index < enemy_list->cursor; index++)
    {
        struct ed_pickable_t *pickable = *(struct ed_pickable_t **)ds_list_get_element(enemy_list, index);
        struct g_enemy_t *enemy = g_GetEnemy(pickable->secondary_index, pickable->primary_index);

        switch(enemy->type)
        {
            case G_ENEMY_TYPE_CAMERA:
            {
                struct g_camera_t *camera = (struct g_camera_t *)enemy;
//                r_i_SetModelMatrix(&camera->entity->transform->transform);

//                r_i_
            }
            break;
        }
    }
}

void ed_w_PingInfoWindow()
{
    ed_level_state.info_window_alpha = 1.0;
}

uint32_t ed_w_IntersectPlaneFromCamera(int32_t mouse_x, int32_t mouse_y, vec3_t *plane_point, vec3_t *plane_normal, vec3_t *result)
{
    vec3_t mouse_pos;
    vec3_t camera_pos;
    vec4_t mouse_vec = {.z = 0.0, .w = 0.0};
    mouse_vec.x = ((float)mouse_x / (float)r_width) * 2.0 - 1.0;
    mouse_vec.y = 1.0 - ((float)mouse_y / (float)r_height) * 2.0;

    float aspect = (float)r_width / (float)r_height;
    float top = tan(r_fov) * r_z_near;
    float right = top * aspect;

    mouse_vec.x *= right;
    mouse_vec.y *= top;
    mouse_vec.z = -r_z_near;

    vec4_t_normalize(&mouse_vec, &mouse_vec);
    mat4_t_vec4_t_mul_fast(&mouse_vec, &r_camera_matrix, &mouse_vec);

    camera_pos = r_camera_matrix.rows[3].xyz;
    vec3_t_add(&mouse_pos, &camera_pos, &mouse_vec.xyz);

    vec3_t plane_vec;
    vec3_t_sub(&plane_vec, &camera_pos, plane_point);
    float dist_a = vec3_t_dot(&plane_vec, plane_normal);

    vec3_t_sub(&plane_vec, &mouse_pos, plane_point);
    float dist_b = vec3_t_dot(&plane_vec, plane_normal);
    float denom = (dist_a - dist_b);

    if(denom)
    {
        float frac = dist_a / denom;

        if(frac >= 0.0)
        {
            vec3_t_fmadd(result, &camera_pos, &mouse_vec.xyz, frac);
            return 1;
        }
    }

    return 0;
}

void ed_w_PointPixelCoords(int32_t *x, int32_t *y, vec3_t *point)
{
    vec4_t result;
    result.xyz = *point;
    result.w = 1.0;
    mat4_t_vec4_t_mul_fast(&result, &r_view_projection_matrix, &result);
    *x = r_width * ((result.x / result.w) * 0.5 + 0.5);
    *y = r_height * (1.0 - ((result.y / result.w) * 0.5 + 0.5));
}

void ed_LevelEditorIdle(uint32_t just_changed)
{
//    struct ed_level_state_t *context_data = &ed_level_state;
    struct ds_list_t *selections = &ed_level_state.pickables.selections;

//    igText("R Mouse down: fly camera");
//    igText("L Mouse down: select object...");
//    igText("Tab: switch edit mode");
//    igText("Delete: delete selections");

    int32_t mouse_x;
    int32_t mouse_y;

    in_SetMouseRelative(0);

    in_GetMousePos(&mouse_x, &mouse_y);

    if(ed_level_state.open_delete_selections_popup)
    {
        igOpenPopup_Str("Delete selections", 0);
        ed_level_state.open_delete_selections_popup = 0;
    }

    igSetNextWindowPos((ImVec2){mouse_x, mouse_y}, ImGuiCond_Once, (ImVec2){0.0, 0.0});
    if(igBeginPopup("Delete selections", 0))
    {
        if(igMenuItem_Bool("Delete selections?", NULL, 0, 1))
        {
            ed_w_DeleteSelections();
        }
        igEndPopup();
    }
    else if(in_GetKeyState(SDL_SCANCODE_LALT) & IN_KEY_STATE_PRESSED)
    {
        ed_SetNextState(ed_LevelEditorFlyCamera);
        in_SetMouseWarp(1);
    }
    else if(in_GetMouseButtonState(SDL_BUTTON_LEFT) & IN_KEY_STATE_JUST_PRESSED)
    {
        ed_SetNextState(ed_LevelEditorLeftClick);
    }
    else if(in_GetMouseButtonState(SDL_BUTTON_RIGHT) & IN_KEY_STATE_JUST_PRESSED)
    {
        ed_SetNextState(ed_LevelEditorRightClick);
    }
    else if(in_GetKeyState(SDL_SCANCODE_LCTRL) & IN_KEY_STATE_PRESSED)
    {
        ed_SetNextState(ed_l_PlacementCrosshair);
//        ed_SetNextState(ed_LevelEditorBrushBox);
    }
    else if(selections->cursor)
    {
        if(in_GetKeyState(SDL_SCANCODE_DELETE) & IN_KEY_STATE_JUST_PRESSED)
        {
            ed_level_state.open_delete_selections_popup = 1;
        }
        else if(in_GetKeyState(SDL_SCANCODE_LSHIFT) & IN_KEY_STATE_PRESSED)
        {
            if(in_GetKeyState(SDL_SCANCODE_D) & IN_KEY_STATE_JUST_PRESSED)
            {
                ed_w_CopySelections(NULL);
            }
        }
    }

    if(in_GetKeyState(SDL_SCANCODE_G) & IN_KEY_STATE_JUST_PRESSED)
    {
        ed_level_state.manipulator.transform_type = ED_L_TRANSFORM_TYPE_TRANSLATION;
    }

    else if(in_GetKeyState(SDL_SCANCODE_R) & IN_KEY_STATE_JUST_PRESSED)
    {
        ed_level_state.manipulator.transform_type = ED_L_TRANSFORM_TYPE_ROTATION;
    }

//    else if(in_GetKeyState(SDL_SCANCODE_L) & IN_KEY_STATE_JUST_PRESSED)
//    {
//        ed_CreateEntityPickable(g_ent_def, &vec3_t_c(0.0, 0.0, 0.0), &vec3_t_c(1.0, 1.0, 1.0), &mat3_t_c_id(), NULL);
//    }
}

void ed_LevelEditorFlyCamera(uint32_t just_changed)
{
    float dx;
    float dy;

    if(!(in_GetKeyState(SDL_SCANCODE_LALT) & IN_KEY_STATE_PRESSED))
    {
        ed_SetNextState(ed_LevelEditorIdle);
        in_SetMouseWarp(0);
        return;
    }

    in_GetMouseDelta(&dx, &dy);

    ed_level_state.camera_pitch += dy;
    ed_level_state.camera_yaw -= dx;

    if(ed_level_state.camera_pitch > 0.5)
    {
        ed_level_state.camera_pitch = 0.5;
    }
    else if(ed_level_state.camera_pitch < -0.5)
    {
        ed_level_state.camera_pitch = -0.5;
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
    vec3_t_add(&ed_level_state.camera_pos, &ed_level_state.camera_pos, &vec3_t_c(translation.x, translation.y, translation.z));
}

void ed_LevelEditorRightClick(uint32_t just_changed)
{
    struct ed_level_state_t *context_data = &ed_level_state;

    switch(context_data->selected_tools_tab)
    {
        case ED_L_TOOL_TAB_BRUSH:
            context_data->pickables.ignore_types = ED_PICKABLE_OBJECT_MASK;
            ed_l_PickObjectOrWidget(just_changed);
        break;

//        case ED_LEVEL_SECONDARY_CLICK_FUNC_LIGHT:
////            ed_SetNextState(ed_LevelEditorPlaceLightAtCursor);
//        break;
    }
}

void ed_l_PlacementCrosshair(uint32_t just_changed)
{
    if(in_GetKeyState(SDL_SCANCODE_LCTRL) & IN_KEY_STATE_PRESSED)
    {
        int32_t mouse_x;
        int32_t mouse_y;
        struct ed_level_state_t *context_data = &ed_level_state;
        in_GetMousePos(&mouse_x, &mouse_y);
        vec3_t intersection = {};

        vec3_t *plane_point = &context_data->pickables.plane_point;
        mat3_t *plane_orientation = &context_data->pickables.plane_orientation;

        ed_l_SurfaceUnderMouse(mouse_x, mouse_y, plane_point, plane_orientation);
        ed_w_IntersectPlaneFromCamera(mouse_x, mouse_y, plane_point, &plane_orientation->rows[1], plane_point);
        ed_l_LinearSnapValueOnSurface(plane_point, plane_orientation, plane_point);

        vec3_t start = *plane_point;
        vec3_t end = *plane_point;
        vec3_t normal = plane_orientation->rows[1];

        vec3_t u_axis;
        vec3_t v_axis;

        vec3_t_mul(&u_axis, &plane_orientation->rows[0], ED_W_BRUSH_BOX_CROSSHAIR_DIM);
        vec3_t_mul(&v_axis, &plane_orientation->rows[2], ED_W_BRUSH_BOX_CROSSHAIR_DIM);

        int32_t window_x;
        int32_t window_y;

        ed_w_PointPixelCoords(&window_x, &window_y, plane_point);

        igSetNextWindowPos((ImVec2){window_x, window_y}, 0, (ImVec2){0.0, 0.0});
        igSetNextWindowBgAlpha(0.25);
        if(igBegin("crosshair", NULL, ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoTitleBar |
                                 ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDecoration))
        {
            igText("pos: [%f, %f, %f]", plane_point->x, plane_point->y, plane_point->z);
            igText("norm: [%f, %f, %f]", normal.x, normal.y, normal.z);
        }
        igEnd();

        r_i_SetModelMatrix(NULL);
        r_i_SetViewProjectionMatrix(NULL);

        r_i_DrawLine(&vec3_t_c(start.x + u_axis.x, start.y + u_axis.y, start.z + u_axis.z),
                     &vec3_t_c(end.x - u_axis.x, end.y - u_axis.y, end.z - u_axis.z),
                     &vec4_t_c(1.0, 1.0, 1.0, 1.0), 4.0);

        r_i_DrawLine(&vec3_t_c(start.x + v_axis.x, start.y + v_axis.y, start.z + v_axis.z),
                     &vec3_t_c(end.x - v_axis.x, end.y - v_axis.y, end.z - v_axis.z),
                     &vec4_t_c(1.0, 1.0, 1.0, 1.0), 4.0);

        if(in_GetMouseButtonState(SDL_BUTTON_LEFT) & IN_KEY_STATE_JUST_PRESSED)
        {
            switch(context_data->selected_tools_tab)
            {
                case ED_L_TOOL_TAB_BRUSH:
                    ed_SetNextState(ed_LevelEditorBrushBox);
                break;

                case ED_L_TOOL_TAB_LIGHT:
                    ed_SetNextState(ed_l_PlaceLightAtCursor);
                break;

                case ED_L_TOOL_TAB_ENTITY:
                    if(ed_level_state.pickables.ent_def)
                    {
                        ed_SetNextState(ed_l_PlaceEntityAtCursor);
                    }
                break;

                case ED_L_TOOL_TAB_ENEMY:
                    ed_SetNextState(ed_l_PlaceEnemyAtCursor);
                break;
            }
        }
    }
    else
    {
        ed_SetNextState(ed_LevelEditorIdle);
    }
}

void ed_LevelEditorLeftClick(uint32_t just_changed)
{
    struct ed_level_state_t *context_data = &ed_level_state;
    context_data->pickables.ignore_types = ED_PICKABLE_BRUSH_PART_MASK;
    ed_l_PickObjectOrWidget(just_changed);
}

void ed_LevelEditorBrushBox(uint32_t just_changed)
{
    struct ed_level_state_t *context_data = &ed_level_state;
    uint32_t right_button_down = in_GetMouseButtonState(SDL_BUTTON_LEFT) & IN_KEY_STATE_PRESSED;
    uint32_t ctrl_down = in_GetKeyState(SDL_SCANCODE_LCTRL) & IN_KEY_STATE_PRESSED;

    if(just_changed)
    {
//        context_data->brush.drawing = 0;
        context_data->brush.box_start = context_data->pickables.plane_point;
    }

    if((ctrl_down && (!context_data->brush.drawing)) || right_button_down)
    {
        if(in_GetKeyState(SDL_SCANCODE_ESCAPE) & IN_KEY_STATE_PRESSED)
        {
            ed_SetNextState(ed_LevelEditorIdle);
        }
        else
        {
            int32_t mouse_x;
            int32_t mouse_y;

            in_GetMousePos(&mouse_x, &mouse_y);

            vec3_t intersection = {};

            vec3_t plane_point = context_data->pickables.plane_point;
            mat3_t plane_orientation = context_data->pickables.plane_orientation;

            if(ed_w_IntersectPlaneFromCamera(mouse_x, mouse_y, &plane_point, &plane_orientation.rows[1], &intersection))
            {
                r_i_SetModelMatrix(NULL);
                r_i_SetViewProjectionMatrix(NULL);
                r_i_SetShader(NULL);

                ed_l_LinearSnapValueOnSurface(&plane_point, &plane_orientation, &intersection);

                context_data->brush.box_end = intersection;
                vec3_t start = context_data->brush.box_start;
                vec3_t end = context_data->brush.box_end;
                vec3_t diagonal;
                vec3_t_sub(&diagonal, &end, &start);
                float proj_u = vec3_t_dot(&diagonal, &plane_orientation.rows[0]);
                float proj_v = vec3_t_dot(&diagonal, &plane_orientation.rows[2]);

                vec3_t corners[4];
                corners[0] = start;
                vec3_t_fmadd(&corners[1], &start, &plane_orientation.rows[0], proj_u);
                corners[2] = end;
                vec3_t_fmadd(&corners[3], &start, &plane_orientation.rows[2], proj_v);

                r_i_DrawLine(&corners[0], &corners[1], &vec4_t_c(0.0, 1.0, 0.0, 1.0), 2.0);
                r_i_DrawLine(&corners[1], &corners[2], &vec4_t_c(0.0, 1.0, 0.0, 1.0), 2.0);
                r_i_DrawLine(&corners[2], &corners[3], &vec4_t_c(0.0, 1.0, 0.0, 1.0), 2.0);
                r_i_DrawLine(&corners[3], &corners[0], &vec4_t_c(0.0, 1.0, 0.0, 1.0), 2.0);


                context_data->brush.box_size.x = fabsf(proj_u);
                context_data->brush.box_size.y = fabsf(proj_v);

                vec3_t edge_center;
                int32_t window_x;
                int32_t window_y;

                vec3_t_add(&edge_center, &corners[3], &corners[0]);
                vec3_t_mul(&edge_center, &edge_center, 0.5);
                ed_w_PointPixelCoords(&window_x, &window_y, &edge_center);

                igSetNextWindowPos((ImVec2){window_x, window_y}, 0, (ImVec2){0.5, 0.5});
                igSetNextWindowBgAlpha(0.15);
                if(igBegin("dimh", NULL, ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoTitleBar |
                                         ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDecoration))
                {
                    igText("%f m", context_data->brush.box_size.x);
                }

                igEnd();


                vec3_t_add(&edge_center, &corners[3], &corners[2]);
                vec3_t_mul(&edge_center, &edge_center, 0.5);
                ed_w_PointPixelCoords(&window_x, &window_y, &edge_center);

                igSetNextWindowPos((ImVec2){window_x, window_y}, 0, (ImVec2){0.5, 0.5});
                igSetNextWindowBgAlpha(0.15);
                if(igBegin("dimv", NULL, ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoTitleBar |
                                         ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDecoration))
                {
                    igText("%f m", context_data->brush.box_size.y);
                }

                igEnd();
            }
        }
    }
    else
    {
        vec3_t position;
        vec3_t size;

        size.x = context_data->brush.box_size.x;
        size.y = 1.0;
        size.z = context_data->brush.box_size.y;

        if(size.x != 0.0 || size.z != 0.0)
        {
            vec3_t_add(&position, &context_data->brush.box_start, &context_data->brush.box_end);
            vec3_t_mul(&position, &position, 0.5);
            vec3_t_fmadd(&position, &position, &context_data->pickables.plane_orientation.rows[1], size.y * 0.5);

            ed_CreateBrushPickable(&position, &context_data->pickables.plane_orientation, &size, NULL);
        }
        ed_SetNextState(ed_LevelEditorIdle);
    }
}

void ed_l_PickObjectOrWidget(uint32_t just_changed)
{
    struct ed_level_state_t *context_data = &ed_level_state;
    int32_t mouse_x;
    int32_t mouse_y;

    in_GetMousePos(&mouse_x, &mouse_y);

    if(just_changed)
    {
        context_data->pickables.last_selected = NULL;

        if(context_data->pickables.selections.cursor)
        {
            struct ed_widget_t *manipulator = context_data->manipulator.widgets[context_data->manipulator.transform_type];
            context_data->pickables.last_selected = ed_SelectWidget(mouse_x, mouse_y, manipulator, &context_data->manipulator.transform);
        }
    }

    if(context_data->pickables.last_selected)
    {
        if(context_data->pickables.last_selected->type == ED_PICKABLE_TYPE_WIDGET)
        {
            ed_SetNextState(ed_l_TransformSelections);
        }
        else
        {
            context_data->pickables.last_selected = NULL;
        }
    }
    else
    {
        ed_SetNextState(ed_l_PickObject);
    }
}

void ed_l_PickObject(uint32_t just_changed)
{
    struct ed_level_state_t *context_data = &ed_level_state;
    uint32_t button_state = in_GetMouseButtonState(SDL_BUTTON_LEFT) | in_GetMouseButtonState(SDL_BUTTON_RIGHT);
    int32_t mouse_x;
    int32_t mouse_y;
//    uint32_t mode_change = context_data->pickables.edit_mode != context_data->pickables.next_edit_mode;

    in_GetMousePos(&mouse_x, &mouse_y);

    if(!(button_state & IN_KEY_STATE_PRESSED))
    {
        uint32_t ignore_types = context_data->pickables.ignore_types;
        struct ds_slist_t *pickables = &context_data->pickables.pickables;

        context_data->pickables.last_selected = ed_SelectPickable(mouse_x, mouse_y, pickables, NULL, ignore_types);

        if(context_data->pickables.last_selected)
        {
            struct ed_pickable_t *pickable = context_data->pickables.last_selected;

            if((1 << pickable->type) & ED_PICKABLE_BRUSH_PART_MASK)
            {
                struct ed_brush_t *brush = ed_GetBrush(pickable->primary_index);

                if(brush->pickable->selection_index != 0xffffffff)
                {
                    ed_w_DropSelection(brush->pickable, NULL);
                }
            }
            else if(pickable->type == ED_PICKABLE_TYPE_BRUSH)
            {
                struct ed_brush_t *brush = ed_GetBrush(pickable->primary_index);
                struct ed_face_t *face = brush->faces;

                while(face)
                {
                    if(face->pickable->selection_index != 0xffffffff)
                    {
                        ed_w_DropSelection(face->pickable, NULL);
                    }

                    face = face->next;
                }
            }

            uint32_t shift_state = in_GetKeyState(SDL_SCANCODE_LSHIFT);
            ed_w_AddSelection(context_data->pickables.last_selected, shift_state & IN_KEY_STATE_PRESSED, NULL);
        }

        context_data->pickables.last_selected = NULL;

        ed_SetNextState(ed_LevelEditorIdle);
    }
}

void ed_l_PlaceEntityAtCursor(uint32_t just_changed)
{
    struct ed_level_state_t *context_data = &ed_level_state;
    struct e_ent_def_t *ent_def = ed_level_state.pickables.ent_def;
    vec3_t position = ed_level_state.pickables.plane_point;
    mat3_t orientation = ed_level_state.pickables.plane_orientation;

    vec3_t_fmadd(&position, &position, &orientation.rows[2], 0.2);
    ed_CreateEntityPickable(ent_def, &position, &vec3_t_c(1.0, 1.0, 1.0), &orientation, NULL);
    ed_SetNextState(ed_LevelEditorIdle);
}

void ed_l_PlaceLightAtCursor(uint32_t just_changed)
{
    struct ed_level_state_t *context_data = &ed_level_state;
    uint32_t type = ed_level_state.pickables.light_type;
    vec3_t position;
    vec3_t_fmadd(&position, &context_data->pickables.plane_point, &context_data->pickables.plane_orientation.rows[1], 0.2);
    ed_CreateLightPickable(&position, &vec3_t_c(1.0, 1.0, 1.0), 6.0, 10.0, type, NULL);
    ed_SetNextState(ed_LevelEditorIdle);
}

void ed_l_PlaceEnemyAtCursor(uint32_t just_changed)
{
    struct ed_level_state_t *context_data = &ed_level_state;
    uint32_t type = ed_level_state.pickables.enemy_type;
    vec3_t position;
    vec3_t_fmadd(&position, &context_data->pickables.plane_point, &context_data->pickables.plane_orientation.rows[1], 0.2);
    ed_CreateEnemyPickable(type, &position, &mat3_t_c_id(), NULL);
    ed_SetNextState(ed_LevelEditorIdle);
}

void ed_l_TransformSelections(uint32_t just_changed)
{
    struct ed_level_state_t *context_data = &ed_level_state;
    uint32_t mouse_state = in_GetMouseButtonState(SDL_BUTTON_LEFT);

    if(mouse_state & IN_KEY_STATE_PRESSED)
    {
        struct ed_widget_t *manipulator = context_data->manipulator.widgets[context_data->manipulator.transform_type];

        uint32_t axis_index = context_data->pickables.last_selected->index;
        mat4_t *manipulator_transform = &ed_level_state.manipulator.transform;
        vec3_t axis_vec = manipulator_transform->rows[context_data->pickables.last_selected->index].xyz;
        vec3_t intersection;
        vec4_t axis_color[] =
        {
            vec4_t_c(1.0, 0.0, 0.0, 1.0),
            vec4_t_c(0.0, 1.0, 0.0, 1.0),
            vec4_t_c(0.0, 0.0, 1.0, 1.0),
        };
        int32_t mouse_x;
        int32_t mouse_y;

        in_GetMousePos(&mouse_x, &mouse_y);

        r_i_SetShader(NULL);
        r_i_SetViewProjectionMatrix(NULL);
        r_i_SetModelMatrix(NULL);

        switch(context_data->manipulator.transform_type)
        {
            case ED_L_TRANSFORM_TYPE_TRANSLATION:
            {
                vec3_t manipulator_cam_vec;
                vec3_t_sub(&manipulator_cam_vec, &r_camera_matrix.rows[3].xyz, &manipulator_transform->rows[3].xyz);
                float proj = vec3_t_dot(&manipulator_cam_vec, &axis_vec);

                vec3_t plane_normal;
                vec3_t_fmadd(&plane_normal, &manipulator_cam_vec, &axis_vec, -proj);
                vec3_t_normalize(&plane_normal, &plane_normal);

                ed_w_IntersectPlaneFromCamera(mouse_x, mouse_y, &manipulator_transform->rows[3].xyz, &plane_normal, &intersection);

                vec3_t cur_offset;
                vec3_t_sub(&cur_offset, &intersection, &manipulator_transform->rows[3].xyz);
                proj = vec3_t_dot(&cur_offset, &axis_vec);
                vec3_t_mul(&cur_offset, &axis_vec, proj);

                if(just_changed)
                {
                    context_data->manipulator.start_pos = manipulator_transform->rows[3].xyz;
                    context_data->manipulator.prev_offset = cur_offset;
                }

                vec3_t_sub(&cur_offset, &cur_offset, &context_data->manipulator.prev_offset);

                if(context_data->manipulator.linear_snap)
                {
                    for(uint32_t index = 0; index < 3; index++)
                    {
                        float f = floorf(cur_offset.comps[index] / context_data->manipulator.linear_snap);
                        cur_offset.comps[index] = context_data->manipulator.linear_snap * f;
                    }
                }

                int32_t window_x;
                int32_t window_y;
                vec3_t disp;
                vec3_t_sub(&disp, &manipulator_transform->rows[3].xyz, &context_data->manipulator.start_pos);

                ed_w_PointPixelCoords(&window_x, &window_y, &manipulator_transform->rows[3].xyz);
                igSetNextWindowPos((ImVec2){window_x, window_y}, 0, (ImVec2){0.0, 0.0});
                igSetNextWindowBgAlpha(0.25);
                if(igBegin("displacement", NULL, ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoTitleBar |
                                             ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDecoration))
                {
                    vec3_t pos = manipulator_transform->rows[3].xyz;
                    igText("disp: [%f m, %f m, %f m]", disp.x, disp.y, disp.z);
                    igText("pos: [%f, %f, %f]", pos.x, pos.y, pos.z);
                }
                igEnd();

                ed_LevelEditorTranslateSelected(&cur_offset, 0);
            }
            break;

            case ED_L_TRANSFORM_TYPE_ROTATION:
            {
                ed_w_IntersectPlaneFromCamera(mouse_x, mouse_y, &manipulator_transform->rows[3].xyz, &axis_vec, &intersection);

                vec3_t manipulator_mouse_vec;
                vec3_t_sub(&manipulator_mouse_vec, &intersection, &manipulator_transform->rows[3].xyz);
                vec3_t_normalize(&manipulator_mouse_vec, &manipulator_mouse_vec);

                if(just_changed)
                {
                    context_data->manipulator.prev_offset = manipulator_mouse_vec;
                }

                vec3_t angle_vec;
                vec3_t_cross(&angle_vec, &manipulator_mouse_vec, &context_data->manipulator.prev_offset);
                float angle = asin(vec3_t_dot(&angle_vec, &axis_vec)) / 3.14159265;
                mat3_t rotation;
                mat3_t_identity(&rotation);

                if(context_data->manipulator.angular_snap)
                {
                    if(angle > 0.0)
                    {
                        angle = floorf(angle / context_data->manipulator.angular_snap);
                    }
                    else
                    {
                        angle = ceilf(angle / context_data->manipulator.angular_snap);
                    }

                    angle *= context_data->manipulator.angular_snap;
                }

                if(angle)
                {
                    context_data->manipulator.prev_offset = manipulator_mouse_vec;
                }



                switch(axis_index)
                {
                    case 0:
                        mat3_t_rotate_x(&rotation, angle);
                    break;

                    case 1:
                        mat3_t_rotate_y(&rotation, angle);
                    break;

                    case 2:
                        mat3_t_rotate_z(&rotation, angle);
                    break;
                }

                ed_LevelEditorRotateSelected(&rotation, &context_data->manipulator.transform.rows[3].xyz, 0);
            }
            break;
        }
    }
    else
    {
        ed_SetNextState(ed_LevelEditorIdle);
        context_data->pickables.last_selected = NULL;
    }
}

void ed_SerializeLevel(void **level_buffer, size_t *buffer_size, uint32_t serialize_brushes)
{
    size_t out_buffer_size = sizeof(struct l_level_header_t);

    struct ds_list_t *light_list = &ed_level_state.pickables.game_pickables[ED_PICKABLE_TYPE_LIGHT];
    struct ds_list_t *entity_list = &ed_level_state.pickables.game_pickables[ED_PICKABLE_TYPE_ENTITY];
    struct ds_list_t *enemy_list = &ed_level_state.pickables.game_pickables[ED_PICKABLE_TYPE_ENEMY];

    size_t level_editor_section_size = sizeof(struct ed_l_section_t);

    if(serialize_brushes)
    {
        level_editor_section_size += sizeof(struct ed_brush_section_t);
        level_editor_section_size += sizeof(struct ed_brush_record_t) * ed_level_state.brush.brushes.used;
        level_editor_section_size += sizeof(struct ed_vert_record_t) * ed_level_state.brush.brush_vert_count;
        level_editor_section_size += sizeof(struct ed_edge_record_t) * ed_level_state.brush.brush_edges.used;
        level_editor_section_size += sizeof(struct ed_polygon_record_t) * ed_level_state.brush.brush_face_polygons.used;
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

    size_t game_section_size = 0;
    if(enemy_list->cursor)
    {
        game_section_size += sizeof(struct l_enemy_record_t) * enemy_list->cursor;
    }
    if(game_section_size)
    {
        game_section_size += sizeof(struct l_game_section_t);
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

    if(serialize_brushes)
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

                brush_record->vert_start = cur_out_buffer - start_out_buffer;
                cur_out_buffer += sizeof(struct ed_vert_record_t) * brush->vertices.used;
                brush_record->edge_start = cur_out_buffer - start_out_buffer;
                cur_out_buffer += sizeof(struct ed_edge_record_t) * brush->edge_count;
                brush_record->face_start = cur_out_buffer - start_out_buffer;
                cur_out_buffer += sizeof(struct ed_face_record_t) * brush->face_count;

                struct ed_vert_record_t *vert_records = (struct ed_vert_record_t *)(start_out_buffer + brush_record->vert_start);
                struct ed_edge_record_t *edge_records = (struct ed_edge_record_t *)(start_out_buffer + brush_record->edge_start);
                struct ed_face_record_t *face_records = (struct ed_face_record_t *)(start_out_buffer + brush_record->face_start);

                /* serialize vertices and edges */
                for(uint32_t vert_index = 0; vert_index < brush->vertices.cursor; vert_index++)
                {
                    struct ed_vert_t *vert = ed_GetVert(brush, vert_index);

                    if(vert)
                    {
                        struct ed_vert_record_t *vert_record = vert_records + brush_record->vert_count;
                        vert->s_index = brush_record->vert_count;
                        brush_record->vert_count++;
                        vert_record->vert = vert->vert;

                        /* go over the edges of this vertex, and serialize them if they haven't been already */
                        struct ed_vert_edge_t *vert_edge = vert->edges;

                        while(vert_edge)
                        {
                            struct ed_edge_t *edge = vert_edge->edge;
                            struct ed_edge_record_t *edge_record;

                            if(edge->s_index == 0xffffffff)
                            {
                                /* this edge hasn't been serialized yet, so create a record */
                                edge_record = edge_records + brush_record->edge_count;
                                /* store the serialization index, so the record for this edge can
                                be quickly found later */
                                edge->s_index = brush_record->edge_count;
                                brush_record->edge_count++;

                                edge_record->polygons[0] = 0xffffffff;
                                edge_record->polygons[1] = 0xffffffff;
                                /* This constant value is used by the deserializer to know whether an edge hasn't been
                                "seen" already. Could be done in the deserializer before processing polygons, but we're
                                already touching the record here, might as well fill this now. */
                                edge_record->d_index = 0xffffffff;
                            }
                            else
                            {
                                edge_record = edge_records + edge->s_index;
                            }

                            uint32_t vert_index = edge->verts[1].vert == vert;
                            edge_record->vertices[vert_index] = vert->s_index;
                            vert_edge = vert_edge->next;
                        }
                    }
                }

                /* for serialization purposes we generate sequential polygon ids here. Those ids are generated the same
                during deserialization, so everything is fine. Edges reference those sequential ids */
                uint32_t polygon_id = 0;
                struct ed_face_t *face = brush->faces;
                /* serialize faces and polygons */
                while(face)
                {
                    struct ed_face_record_t *face_record = face_records + brush_record->face_count;
                    brush_record->face_count++;

                    strcpy(face_record->material, face->material->name);
                    face_record->uv_rot = face->tex_coords_rot;
                    face_record->uv_scale = face->tex_coords_scale;
                    face_record->polygon_start = cur_out_buffer - start_out_buffer;

                    struct ed_face_polygon_t *polygon = face->polygons;

                    while(polygon)
                    {
                        struct ed_polygon_record_t *polygon_record = (struct ed_polygon_record_t *)cur_out_buffer;
                        cur_out_buffer += sizeof(struct ed_polygon_record_t);
                        cur_out_buffer += sizeof(size_t) * polygon->edge_count;

                        /* go over all the edges of this polygon and store connectivity data. The indices
                        stored are for the serialized records */
                        struct ed_edge_t *edge = polygon->edges;

                        while(edge)
                        {
                            uint32_t polygon_side = edge->polygons[1].polygon == polygon;
                            struct ed_edge_record_t *edge_record = edge_records + edge->s_index;

                            /* store the serialization index of this edge in the edge list of this polygon
                            record */
                            polygon_record->edges[polygon_record->edge_count] = edge->s_index;
                            /* store on which side of this edge the polygon is */
                            edge_record->polygons[polygon_side] = polygon_id;
                            polygon_record->edge_count++;

                            if(edge_record->polygons[0] != 0xffffffff && edge_record->polygons[1] != 0xffffffff)
                            {
                                /* this edge has been referenced twice, so we can clear its serialization index. This
                                is necessary for other serializations to properly happen in the future. */
                                edge->s_index = 0xffffffff;
                            }

                            edge = edge->polygons[polygon_side].next;
                        }

                        face_record->polygon_count++;
                        polygon = polygon->next;
                        polygon_id++;
                    }

                    face = face->next;
                }

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
        struct l_game_section_t *game_section = (struct l_game_section_t *)cur_out_buffer;
        cur_out_buffer += sizeof(struct l_game_section_t);

        if(enemy_list->cursor)
        {
            game_section->enemy_start = cur_out_buffer - start_out_buffer;
            game_section->enemy_count = enemy_list->cursor;

            struct l_enemy_record_t *enemy_records = (struct l_enemy_record_t *)cur_out_buffer;
            cur_out_buffer += sizeof(struct l_enemy_record_t ) * game_section->enemy_count;

            for(uint32_t enemy_index = 0; enemy_index < game_section->enemy_count; enemy_index++)
            {
                struct ed_pickable_t *pickable = *(struct ed_pickable_t **)ds_list_get_element(enemy_list, enemy_index);
                struct g_enemy_t *enemy = g_GetEnemy(pickable->secondary_index, pickable->primary_index);
                struct l_enemy_record_t *record = enemy_records + enemy_index;
                struct e_node_t *node = enemy->entity->node;

                record->position = node->position;
                record->orientation = node->orientation;
                record->type = enemy->type;
                record->s_index = enemy->index;

                switch(enemy->type)
                {
                    case G_ENEMY_TYPE_CAMERA:
                    {
                        struct g_camera_t *camera = (struct g_camera_t *)enemy;
                        record->camera_fields = camera->fields;
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
                entity_record->scale = transform->scale;
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

    for(uint32_t light_type = R_LIGHT_TYPE_POINT; light_type < R_LIGHT_TYPE_LAST; light_type++)
    {
        for(uint32_t light_index = 0; light_index < r_lights[light_type].cursor; light_index++)
        {
            struct r_light_t *light = r_GetLight(R_LIGHT_INDEX(light_type, light_index));

            if(light)
            {
                ed_CreateLightPickable(NULL, NULL, 0.0, 0.0, 0, light);
            }
        }
    }

    for(uint32_t enemy_type = G_ENEMY_TYPE_CAMERA; enemy_type < G_ENEMY_TYPE_LAST; enemy_type++)
    {
        for(uint32_t enemy_index = 0; enemy_index < g_enemies[enemy_type].cursor; enemy_index++)
        {
            struct g_enemy_t *enemy = g_GetEnemy(enemy_type, enemy_index);

            if(enemy)
            {
                ed_CreateEnemyPickable(0, NULL, NULL, enemy);
            }
        }
    }

    for(uint32_t entity_index = 0; entity_index < e_root_transforms.cursor; entity_index++)
    {
        struct e_node_t *node = *(struct e_node_t **)ds_list_get_element(&e_root_transforms, entity_index);

        if(node->entity->def)
        {
            ed_CreateEntityPickable(NULL, NULL, NULL, NULL, node->entity);
        }
    }

    char *start_in_buffer = level_buffer;
    char *cur_in_buffer = start_in_buffer;
    struct l_level_header_t *level_header = (struct l_level_header_t *)cur_in_buffer;

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
        brush->main_brush = brush;

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
        for(uint32_t face_index = 0; face_index < brush_record->face_count; face_index++)
        {
            struct ed_face_record_t *face_record = face_records + face_index;
            struct ed_face_t *face = ed_AllocFace(brush);

            face->material = r_FindMaterial(face_record->material);
            face->tex_coords_rot = face_record->uv_rot;
            face->tex_coords_scale = face_record->uv_scale;

            struct ed_face_polygon_t *last_polygon = NULL;

            struct ed_polygon_record_t *polygon_records = (struct ed_polygon_record_t *)(start_in_buffer + face_record->polygon_start);
            for(uint32_t polygon_index = 0; polygon_index < face_record->polygon_count; polygon_index++)
            {
                struct ed_polygon_record_t *polygon_record = polygon_records + polygon_index;
                struct ed_face_polygon_t *polygon = ed_AllocFacePolygon(brush, face);

                for(uint32_t edge_index = 0; edge_index < polygon_record->edge_count; edge_index++)
                {
                    struct ed_edge_record_t *edge_record = edge_records + polygon_record->edges[edge_index];
                    /* find in which side of the serialized edge the serialized polygon is */
                    uint32_t polygon_side = edge_record->polygons[1] == polygon_id;
                    struct ed_edge_t *edge;

                    if(edge_record->d_index == 0xffffffff)
                    {
                        edge = ed_AllocEdge(brush);
                        /* we store the allocated index for the edge in the serialized data, so
                        we can quickly map to the allocated edge from the serialized edge */
                        edge_record->d_index = edge->index;
                        struct ed_vert_record_t *vert0_record = vert_records + edge_record->vertices[0];
                        struct ed_vert_record_t *vert1_record = vert_records + edge_record->vertices[1];
                        struct ed_vert_t *vert0 = ed_GetVert(brush, vert0_record->d_index);
                        struct ed_vert_t *vert1 = ed_GetVert(brush, vert1_record->d_index);

                        edge->verts[0].vert = vert0;
                        edge->verts[1].vert = vert1;

                        ed_LinkVertEdge(vert0, edge);
                        ed_LinkVertEdge(vert1, edge);
                    }
                    else
                    {
                        edge = ed_GetEdge(edge_record->d_index);
                        /* clearing this here allows us to deserialize the contents of this
                        buffer as many times as we want. Not terribly useful, but ehh. */
                        edge_record->d_index = 0xffffffff;
                    }

                    edge->polygons[polygon_side].polygon = polygon;
                    ed_LinkFacePolygonEdge(polygon, edge);
                }

                polygon_id++;
            }
        }

        ed_UpdateBrush(brush);
        ed_CreateBrushPickable(NULL, NULL, NULL, brush);
    }

    ed_w_UpdatePickableObjects();
    ed_l_ClearBrushEntities();
    ed_level_state.world_data_stale = 0;
}

void ed_l_SurfaceUnderMouse(int32_t mouse_x, int32_t mouse_y, vec3_t *plane_point, mat3_t *plane_orientation)
{
    uint32_t ignore_types = ED_PICKABLE_OBJECT_MASK | ED_PICKABLE_TYPE_MASK_EDGE | ED_PICKABLE_TYPE_MASK_VERT;
    struct ds_slist_t *pickables = &ed_level_state.pickables.pickables;
    struct ed_pickable_t *surface = ed_SelectPickable(mouse_x, mouse_y, pickables, NULL, ignore_types);

    if(surface)
    {
        struct ed_brush_t *brush = ed_GetBrush(surface->primary_index);
        struct ed_face_t *face = ed_GetFace(surface->secondary_index);
        plane_orientation->rows[1] = face->polygons->normal;
        *plane_point = *(vec3_t *)ds_list_get_element(&face->clipped_polygons->vertices, 0);
        mat3_t_vec3_t_mul(&plane_orientation->rows[1], &plane_orientation->rows[1], &brush->orientation);
        mat3_t_vec3_t_mul(plane_point, plane_point, &brush->orientation);
        vec3_t_add(plane_point, plane_point, &brush->position);

        float max_axis_proj = -FLT_MAX;
        uint32_t j_axis_index = 0;
//        mat3_t *plane_orientation = &context_data->brush.plane_orientation;
        vec3_t axes[] =
        {
            vec3_t_c(1.0, 0.0, 0.0),
            vec3_t_c(0.0, 1.0, 0.0),
            vec3_t_c(0.0, 0.0, 1.0),
        };

        for(uint32_t comp_index = 0; comp_index < 3; comp_index++)
        {
            float axis_proj = fabsf(plane_orientation->rows[1].comps[comp_index]);

            if(axis_proj > max_axis_proj)
            {
                max_axis_proj = axis_proj;
                j_axis_index = comp_index;
            }
        }

        uint32_t k_axis_index = (j_axis_index + 1) % 3;
        vec3_t_cross(&plane_orientation->rows[0], &axes[k_axis_index], &plane_orientation->rows[1]);
        vec3_t_normalize(&plane_orientation->rows[0], &plane_orientation->rows[0]);

        vec3_t_cross(&plane_orientation->rows[2], &plane_orientation->rows[1], &plane_orientation->rows[0]);
        vec3_t_normalize(&plane_orientation->rows[2], &plane_orientation->rows[2]);
    }
    else
    {
        *plane_point = vec3_t_c(0.0, 0.0, 0.0);
        *plane_orientation = mat3_t_c_id();
    }
}

void ed_l_LinearSnapValueOnSurface(vec3_t *plane_point, mat3_t *plane_orientation, vec3_t *snapped_value)
{
    vec3_t plane_origin;
    /* compute where the world origin projects onto the plane */
    vec3_t_mul(&plane_origin, &plane_orientation->rows[1], vec3_t_dot(plane_point, &plane_orientation->rows[1]));

    /* transform intersection point from world space to plane space */
    vec3_t_sub(snapped_value, snapped_value, &plane_origin);
    vec3_t transformed_intersection;
    transformed_intersection.x = vec3_t_dot(snapped_value, &plane_orientation->rows[0]);
    transformed_intersection.y = vec3_t_dot(snapped_value, &plane_orientation->rows[1]);
    transformed_intersection.z = vec3_t_dot(snapped_value, &plane_orientation->rows[2]);
    *snapped_value = transformed_intersection;

    float linear_snap = ed_level_state.manipulator.linear_snap;
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

        g_SetBasePath(ed_level_state.project.base_folder);

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
        g_SetBasePath(ed_level_state.project.base_folder);

        read_file(fp, &buffer, &buffer_size);
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
    g_SetBasePath(ed_level_state.project.base_folder);
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
                    if(pickable->primary_index == record->s_index)
                    {
                        pickable->primary_index = record->d_index;

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
            struct l_game_section_t *game_section = (struct l_game_section_t *)(level_buffer + level_header->game_section_start);

            if(game_section->enemy_count)
            {
                struct ds_list_t *enemy_list = &ed_level_state.pickables.game_pickables[ED_PICKABLE_TYPE_ENEMY];
                struct l_enemy_record_t *enemy_records = (struct l_enemy_record_t *)(level_buffer + game_section->enemy_start);

                for(uint32_t enemy_index = 0; enemy_index < enemy_list->cursor; enemy_index++)
                {
                    struct ed_pickable_t *pickable = *(struct ed_pickable_t **)ds_list_get_element(enemy_list, enemy_index);

                    for(uint32_t record_index = 0; record_index < game_section->enemy_count; record_index++)
                    {
                        struct l_enemy_record_t *record = enemy_records + record_index;

                        if(record->s_index == pickable->primary_index && record->type == pickable->secondary_index)
                        {
                            pickable->primary_index = record->d_index;

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

    g_SetBasePath("");

    l_DestroyWorld();

    for(uint32_t pickable_index = 0; pickable_index < ed_level_state.pickables.pickables.cursor; pickable_index++)
    {
        struct ed_pickable_t *pickable = ed_GetPickable(pickable_index);

        if(pickable)
        {
            ed_DestroyPickable(pickable);
        }
    }

    ed_level_state.pickables.pickables.cursor = 0;
    ed_level_state.pickables.pickables.free_stack_top = 0xffffffff;
    ed_level_state.pickables.selections.cursor = 0;
}

void ed_l_BuildWorldData()
{
//    ed_l_ClearBrushEntities();

    if((ed_level_state.world_data_stale || !l_world_collider) && ed_level_state.brush.brushes.used)
    {
        printf("ed_l_BuildWorldData\n");
        ed_level_state.world_data_stale = 0;

        float start = g_GetDeltaTime();
        struct ed_bsp_polygon_t *clipped_polygons = NULL;

        for(uint32_t brush_index = 0; brush_index < ed_level_state.brush.brushes.cursor; brush_index++)
        {
            struct ed_brush_t *brush = ed_GetBrush(brush_index);

            if(brush)
            {
                struct ed_bsp_polygon_t *brush_polygons = ed_BspPolygonsFromBrush(brush);
                clipped_polygons = ed_ClipPolygonLists(clipped_polygons, brush_polygons);
            }
        }

        float end = g_GetDeltaTime();
        printf("bsp took %f seconds\n", end - start);

        struct ds_buffer_t *polygon_buffer = &ed_level_state.brush.polygon_buffer;
        struct ds_buffer_t *batch_buffer = &ed_level_state.brush.batch_buffer;
        struct ds_buffer_t *vertex_buffer = &ed_level_state.brush.vertex_buffer;
        struct ds_buffer_t *index_buffer = &ed_level_state.brush.index_buffer;

        struct ed_bsp_polygon_t *polygon = clipped_polygons;
        uint32_t polygon_count = 0;
        uint32_t index_count = 0;
        uint32_t vert_count = 0;

        while(polygon)
        {
            if(polygon_count >= polygon_buffer->buffer_size)
            {
                ds_buffer_resize(polygon_buffer, polygon_buffer->buffer_size + 16);
            }

            ((struct ed_bsp_polygon_t **)polygon_buffer->buffer)[polygon_count] = polygon;
            index_count += (polygon->vertices.cursor - 2) * 3;
            vert_count += polygon->vertices.cursor;
            polygon_count++;
            polygon = polygon->next;
        }

        qsort(polygon_buffer->buffer, polygon_count, polygon_buffer->elem_size, ed_CompareBspPolygons);

        if(index_count > index_buffer->buffer_size)
        {
            ds_buffer_resize(index_buffer, index_count);
        }

        if(vert_count > vertex_buffer->buffer_size)
        {
            ds_buffer_resize(vertex_buffer, vert_count);
        }

        uint32_t batch_count = 0;
        struct r_material_t *cur_material = NULL;
        struct ds_buffer_t col_vertex_buffer = ds_buffer_create(sizeof(vec3_t), vert_count);

        uint32_t *world_indices = index_buffer->buffer;
        struct r_vert_t *world_verts = vertex_buffer->buffer;
        vec3_t *world_col_verts = col_vertex_buffer.buffer;


        vert_count = 0;

        struct r_batch_t *batch = NULL;

        for(uint32_t polygon_index = 0; polygon_index < polygon_count; polygon_index++)
        {
            struct ed_bsp_polygon_t *polygon = ((struct ed_bsp_polygon_t **)polygon_buffer->buffer)[polygon_index];

            if(polygon->face_polygon->face->material != cur_material)
            {
                cur_material = polygon->face_polygon->face->material;

                if(batch_count >= batch_buffer->buffer_size)
                {
                    ds_buffer_resize(batch_buffer, batch_count + 1);
                }

                batch = ((struct r_batch_t *)batch_buffer->buffer) + batch_count;
                batch->material = cur_material;
                batch->count = 0;
                batch->start = 0;

                if(batch_count)
                {
                    struct r_batch_t *prev_batch = ((struct r_batch_t *)batch_buffer->buffer) + batch_count - 1;
                    batch->start = prev_batch->start + prev_batch->count;
                }

                batch_count++;
            }

            for(uint32_t vert_index = 1; vert_index < polygon->vertices.cursor - 1;)
            {
                world_indices[batch->start + batch->count] = vert_count;
                batch->count++;

                world_indices[batch->start + batch->count] = vert_count + vert_index;
                vert_index++;
                batch->count++;

                world_indices[batch->start + batch->count] = vert_count + vert_index;
                batch->count++;
            }

            for(uint32_t vert_index = 0; vert_index < polygon->vertices.cursor; vert_index++)
            {
                struct r_vert_t *vert = ds_list_get_element(&polygon->vertices, vert_index);
                world_verts[vert_index + vert_count] = *vert;
                world_col_verts[vert_index + vert_count] = vert->pos;
            }

            vert_count += polygon->vertices.cursor;
            polygon = polygon->next;
        }

        struct ds_buffer_t col_index_buffer = ds_buffer_copy(index_buffer);

        if(vert_count)
        {
            l_world_shape->itri_mesh.verts = world_col_verts;
            l_world_shape->itri_mesh.vert_count = vert_count;
            l_world_shape->itri_mesh.indices = col_index_buffer.buffer;
            l_world_shape->itri_mesh.index_count = index_count;
            l_world_collider = p_CreateCollider(&l_world_col_def, &vec3_t_c(0.0, 0.0, 0.0), &mat3_t_c_id());

            struct r_model_geometry_t model_geometry = {};

            model_geometry.batches = batch_buffer->buffer;
            model_geometry.batch_count = batch_count;
            model_geometry.verts = vertex_buffer->buffer;
            model_geometry.vert_count = vert_count;
            model_geometry.indices = index_buffer->buffer;
            model_geometry.index_count = index_count;
            l_world_model = r_CreateModel(&model_geometry, NULL, "world_model");
        }
    }
}

void ed_l_ClearWorldData()
{
    printf("ed_l_ClearWorldData\n");
    l_DestroyWorld();
}











