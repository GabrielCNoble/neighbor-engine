#include <float.h>
#include "ed_level.h"
#include "ed_pick.h"
#include "ed_brush.h"
#include "ed_main.h"
#include "../engine/r_main.h"
#include "dstuff/ds_buffer.h"
#include "../engine/gui.h"
#include "../engine/game.h"
#include "../engine/input.h"
#include "../engine/l_defs.h"
#include "../engine/ent.h"
#include "../engine/level.h"

extern struct ed_context_t ed_contexts[];
struct ed_level_state_t ed_level_state;
struct ed_context_t *ed_world_context;
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
extern struct ds_slist_t e_entities;
extern struct ds_slist_t e_ent_defs[];
extern struct ds_list_t e_components[];
extern struct ds_list_t e_root_transforms;

extern mat4_t r_projection_matrix;
extern mat4_t r_camera_matrix;
extern uint32_t r_width;
extern uint32_t r_height;
extern float r_fov;
extern float r_z_near;

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

void ed_w_Init()
{
    ed_world_context = ed_contexts + ED_CONTEXT_WORLD;
    ed_world_context->update = ed_w_Update;
    ed_world_context->next_state = ed_w_Idle;
    ed_world_context->context_data = &ed_level_state;

    ed_level_state.pickables.last_selected = NULL;
    ed_level_state.pickables.selections = ds_list_create(sizeof(struct ed_pickable_t *), 512);
    ed_level_state.pickables.pickables = ds_slist_create(sizeof(struct ed_pickable_t), 512);
    ed_level_state.pickables.modified_brushes = ds_list_create(sizeof(struct ed_brush_t *), 512);
    ed_level_state.pickables.modified_pickables = ds_list_create(sizeof(struct ed_pickable_t *), 512);
    ed_level_state.pickables.secondary_click_function = ED_LEVEL_SECONDARY_CLICK_FUNC_BRUSH;

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
    ed_level_state.manipulator.mode = ED_LEVEL_MANIP_MODE_TRANSLATION;
    ed_level_state.manipulator.widgets[ED_LEVEL_MANIP_MODE_TRANSLATION] = ed_CreateWidget(NULL);
    struct ed_widget_t *widget = ed_level_state.manipulator.widgets[ED_LEVEL_MANIP_MODE_TRANSLATION];
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
    translation_axis->draw_transform = translation_axis->transform;

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
    translation_axis->draw_transform = translation_axis->transform;

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
    translation_axis->draw_transform = translation_axis->transform;








    ed_level_state.manipulator.widgets[ED_LEVEL_MANIP_MODE_ROTATION] = ed_CreateWidget(NULL);
    widget = ed_level_state.manipulator.widgets[ED_LEVEL_MANIP_MODE_ROTATION];
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
    rotation_axis->draw_transform = rotation_axis->transform;

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
    rotation_axis->draw_transform = rotation_axis->transform;

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
    rotation_axis->draw_transform = rotation_axis->transform;



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

void ed_w_Shutdown()
{

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

void ed_w_TranslateSelected(vec3_t *translation, uint32_t transform_mode)
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

void ed_w_RotateSelected(mat3_t *rotation, vec3_t *pivot, uint32_t transform_mode)
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
                            igText("Material: %s", face->material->name);
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

    igSetNextWindowBgAlpha(0.5);
    igPushStyleVar_Float(ImGuiStyleVar_WindowBorderSize, 0.0);
    igSetNextWindowSize((ImVec2){r_width, 0}, 0);
    igSetNextWindowPos((ImVec2){0.0, r_height}, 0, (ImVec2){0, 1});
    if(igBegin("Footer window", NULL, window_flags))
    {
        char *transform_mode_text = NULL;

        switch(ed_level_state.manipulator.mode)
        {
            case ED_LEVEL_MANIP_MODE_TRANSLATION:
                transform_mode_text = "Translation";
            break;

            case ED_LEVEL_MANIP_MODE_ROTATION:
                transform_mode_text = "Rotation";
            break;
        }


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

            igTableNextColumn();
            igText("Manipulator: %s  ", transform_mode_text);
            igTableNextColumn();

            char snap_label[32];
            char snap_preview[32];
            igText("Snap: ");
            igSameLine(0.0, -1.0);
            if(ed_level_state.manipulator.mode == ED_LEVEL_MANIP_MODE_ROTATION)
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
            if(igButton("Brush", (ImVec2){0.0, 20.0}))
            {
                ed_level_state.pickables.secondary_click_function = ED_LEVEL_SECONDARY_CLICK_FUNC_BRUSH;
            }

            igSameLine(0.0, -1.0);

            if(igButton("Light", (ImVec2){0.0, 20.0}))
            {
                ed_level_state.pickables.secondary_click_function = ED_LEVEL_SECONDARY_CLICK_FUNC_LIGHT;
            }

            igEndTable();
        }

        igSameLine(0.0, -1.0);

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

        struct ds_list_t *selections = &ed_level_state.pickables.selections;

//        if(selections->cursor)
//        {
//            char node_label[32];
//
//            igSetNextWindowSizeConstraints((ImVec2){-1.0, 20.0}, (ImVec2){-1.0, 250.0}, NULL, NULL);
//            if(igBeginChild_Str("Selections", (ImVec2){0.0, 0.0}, 0, ImGuiWindowFlags_))
//            {
//                for(uint32_t selection_index = 0; selection_index < selections->cursor; selection_index++)
//                {
//                    struct ed_pickable_t *pickable = *(struct ed_pickable_t **)ds_list_get_element(selections, selection_index);
//                    char *pickable_label = "Object";
//
//                    switch(pickable->type)
//                    {
//                        case ED_PICKABLE_TYPE_BRUSH:
//                            pickable_label = "Brush";
//                        break;
//
//                        case ED_PICKABLE_TYPE_LIGHT:
//                            pickable_label = "Light";
//                        break;
//
//                        case ED_PICKABLE_TYPE_ENTITY:
//                            pickable_label = "Entity";
//                        break;
//
//                        case ED_PICKABLE_TYPE_FACE:
//                            pickable_label = "Face";
//                        break;
//
//                        case ED_PICKABLE_TYPE_EDGE:
//                            pickable_label = "Edge";
//                        break;
//
//                        case ED_PICKABLE_TYPE_VERT:
//                            pickable_label = "Vertex";
//                        break;
//                    }
//
//                    sprintf(node_label, "%s##%d", pickable_label, selection_index);
//
//                    if(igTreeNode_Str(node_label))
//                    {
//                        igText("blah");
//                        igTreePop();
//                    }
//                }
//            }
//
//            igEndChild();
//        }

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

    if(ed_level_state.manipulator.visible)
    {
        mat4_t_identity(&ed_level_state.manipulator.transform);
        vec3_t *translation = &ed_level_state.manipulator.transform.rows[3].xyz;
        for(uint32_t selection_index = 0; selection_index < selections->cursor; selection_index++)
        {
            struct ed_pickable_t *pickable = *(struct ed_pickable_t **)ds_list_get_element(selections, selection_index);
            vec3_t_add(translation, translation, &pickable->transform.rows[3].xyz);
        }

        vec3_t_div(translation, translation, (float)selections->cursor);

//        printf("manip pos: [%f %f %f]\n", translation->x, translation->y, translation->z);


//        mat4_t view_projection_matrix;
//        struct ed_widget_t *manipulator = ed_level_state.manipulator.widgets[ed_level_state.manipulator.mode];
//
//        manipulator->mvp_mat_fn(&view_projection_matrix, &ed_level_state.manipulator.transform);
//        vec3_t manipulator_min = vec3_t_c(FLT_MAX, FLT_MAX, FLT_MAX);
//        vec3_t manipulator_max = vec3_t_c(-FLT_MAX, -FLT_MAX, -FLT_MAX);
//
//        for(uint32_t pickable_index = 0; pickable_index < manipulator->pickables.cursor; pickable_index++)
//        {
//            struct ed_pickable_t *pickable = ed_GetPickableOnList(pickable_index, &manipulator->pickables);
//
//            if(pickable)
//            {
////                vec3_t min = pickable->mo
//                vec3_t corners[8];
//            }
//        }
    }
}

void ed_w_UpdatePickableObjects()
{
    struct ds_list_t *pickables = &ed_level_state.pickables.modified_pickables;

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

                        /* update brush to update its entity transform */
                        ed_w_MarkBrushModified(brush);
                    }

                    struct ed_face_t *face = brush->faces;

                    while(face)
                    {
                        /* face/edge/vertice pickables also depend on the up to date brush transform,
                        so mark those as modified so they can have their transforms updated as well */
                        ed_w_MarkPickableModified(face->pickable);
                        face = face->next;
                    }

                    mat4_t_comp(&pickable->transform, &brush->orientation, &brush->position);
                    pickable->draw_transform = pickable->transform;
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
                        mat4_t_mul(&pickable->draw_transform, &draw_offset, &pickable->transform);
                    }
                }
                break;

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
    //                    if(!pickable->transform_flags)
    //                    {
    //                        mat4_t_comp(&pickable->transform, &brush->orientation, &brush->position);
    //                    }
    //                }
    //                break;

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
                    pickable->draw_transform = pickable->transform;
                }
                break;

                case ED_PICKABLE_TYPE_ENTITY:

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

void ed_w_Update()
{
    r_SetViewPos(&ed_level_state.camera_pos);
    r_SetViewPitchYaw(ed_level_state.camera_pitch, ed_level_state.camera_yaw);

    ed_w_UpdateUI();
    ed_w_UpdatePickableObjects();
    ed_w_UpdateManipulator();

    ed_w_DrawGrid();
    ed_w_DrawSelections();
    ed_w_DrawBrushes();
    ed_w_DrawLights();
    ed_w_DrawWidgets();

    if(in_GetKeyState(SDL_SCANCODE_P) & IN_KEY_STATE_JUST_PRESSED)
    {
        ed_SaveGameLevelSnapshot();
        g_BeginGame();
    }
}

/*
=============================================================
=============================================================
=============================================================
*/

void ed_w_DrawManipulator()
{
    if(ed_level_state.manipulator.visible)
    {
        struct ed_widget_t *manipulator = ed_level_state.manipulator.widgets[ed_level_state.manipulator.mode];
        ed_DrawWidget(manipulator, &ed_level_state.manipulator.transform);
    }
}

void ed_w_DrawWidgets()
{
    r_i_SetShader(ed_outline_shader);
    ed_w_DrawManipulator();
}

void ed_w_DrawGrid()
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

void ed_w_DrawBrushes()
{
    return;

    for(uint32_t brush_index = 0; brush_index < ed_level_state.brush.brushes.cursor; brush_index++)
    {
        struct ed_brush_t *brush = ed_GetBrush(brush_index);

        if(brush)
        {
            mat4_t transform;
            mat4_t_identity(&transform);
            mat4_t_comp(&transform, &brush->orientation, &brush->position);
            r_DrawEntity(&transform, brush->model);
        }
    }
}

void ed_w_DrawLights()
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

void ed_w_DrawSelections()
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

                mat4_t model_view_projection_matrix;
                ed_PickableModelViewProjectionMatrix(pickable, NULL, &model_view_projection_matrix);
                r_i_SetViewProjectionMatrix(&model_view_projection_matrix);
                struct r_i_draw_list_t *draw_list = NULL;

                switch(pickable->type)
                {
                    case ED_PICKABLE_TYPE_FACE:
                        draw_list = r_i_AllocDrawList(pickable->range_count);
                        struct ed_pickable_range_t *range = pickable->ranges;

                        for(uint32_t range_index = 0; range_index < pickable->range_count; range_index++)
                        {
                            draw_list->commands[range_index].start = range->start;
                            draw_list->commands[range_index].count = range->count;
                            range = range->next;
                        }
                        draw_list->size = 6.0;
                        draw_list->indexed = 1;

                        r_i_SetDrawMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE, 0xff);
                        r_i_SetStencil(GL_TRUE, GL_KEEP, GL_KEEP, GL_REPLACE, GL_ALWAYS, 0xff, 0xff);
                        r_i_SetDepth(GL_TRUE, GL_ALWAYS);
                        r_i_SetRasterizer(GL_TRUE, GL_BACK, GL_FILL);
                        r_i_DrawImmediate(R_I_DRAW_CMD_TRIANGLE_LIST, draw_list);



                        r_i_SetUniform(color_uniform, 1, &vec4_t_c(0.2, 0.7, 0.4, 1.0));
                        draw_list = r_i_AllocDrawList(pickable->range_count);
                        range = pickable->ranges;

                        for(uint32_t range_index = 0; range_index < pickable->range_count; range_index++)
                        {
                            draw_list->commands[range_index].start = range->start;
                            draw_list->commands[range_index].count = range->count;
                            range = range->next;
                        }
                        draw_list->size = 6.0;
                        draw_list->indexed = 1;

                        r_i_SetDrawMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
                        r_i_SetStencil(GL_TRUE, GL_KEEP, GL_KEEP, GL_KEEP, GL_EQUAL, 0xff, 0x00);
                        r_i_SetRasterizer(GL_TRUE, GL_BACK, GL_LINE);
                        r_i_DrawImmediate(R_I_DRAW_CMD_TRIANGLE_LIST, draw_list);



                        r_i_SetUniform(color_uniform, 1, &vec4_t_c(0.3, 0.4, 1.0, 0.4));
                        draw_list = r_i_AllocDrawList(pickable->range_count);
                        range = pickable->ranges;

                        for(uint32_t range_index = 0; range_index < pickable->range_count; range_index++)
                        {
                            draw_list->commands[range_index].start = range->start;
                            draw_list->commands[range_index].count = range->count;
                            range = range->next;
                        }
                        draw_list->size = 4.0;
                        draw_list->indexed = 1;

                        r_i_SetBlending(GL_TRUE, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                        r_i_SetDrawMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE, 0xff);
                        r_i_SetStencil(GL_TRUE, GL_KEEP, GL_KEEP, GL_REPLACE, GL_ALWAYS, 0xff, 0x00);
                        r_i_SetDepth(GL_TRUE, GL_ALWAYS);
                        r_i_SetRasterizer(GL_TRUE, GL_BACK, GL_FILL);
                        r_i_DrawImmediate(R_I_DRAW_CMD_TRIANGLE_LIST, draw_list);
                    break;


                    case ED_PICKABLE_TYPE_EDGE:
                    {
                        draw_list = r_i_AllocDrawList(pickable->range_count);
                        struct ed_pickable_range_t *range = pickable->ranges;

                        r_i_SetUniform(color_uniform, 1, &vec4_t_c(1.0, 0.7, 0.4, 1.0));

                        for(uint32_t range_index = 0; range_index < pickable->range_count; range_index++)
                        {
                            draw_list->commands[range_index].start = range->start;
                            draw_list->commands[range_index].count = range->count;
                            range = range->next;
                        }
                        draw_list->size = 4.0;
                        draw_list->indexed = 1;

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
}

void ed_w_PingInfoWindow()
{
    ed_level_state.info_window_alpha = 1.0;
}

uint32_t ed_w_IntersectPlaneFromCamera(float mouse_x, float mouse_y, vec3_t *plane_point, vec3_t *plane_normal, vec3_t *result)
{
    vec3_t mouse_pos;
    vec3_t camera_pos;
    vec4_t mouse_vec = {.x = mouse_x, .y = mouse_y, .z = 0.0, .w = 0.0};

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

void ed_w_Idle(struct ed_context_t *context, uint32_t just_changed)
{
    struct ed_level_state_t *context_data = context->context_data;
    struct ds_list_t *selections = &ed_level_state.pickables.selections;

//    igText("R Mouse down: fly camera");
//    igText("L Mouse down: select object...");
//    igText("Tab: switch edit mode");
//    igText("Delete: delete selections");

    int32_t mouse_x;
    int32_t mouse_y;

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
        ed_SetNextContextState(context, ed_w_FlyCamera);
        in_SetMouseWarp(1);
    }
    else if(in_GetMouseButtonState(SDL_BUTTON_LEFT) & IN_KEY_STATE_JUST_PRESSED)
    {
        ed_SetNextContextState(context, ed_w_LeftClick);
    }
    else if(in_GetMouseButtonState(SDL_BUTTON_RIGHT) & IN_KEY_STATE_JUST_PRESSED)
    {
        ed_SetNextContextState(context, ed_w_RightClick);
    }
    else if(in_GetKeyState(SDL_SCANCODE_LCTRL) & IN_KEY_STATE_PRESSED)
    {
        ed_SetNextContextState(context, ed_w_BrushBox);
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
        ed_level_state.manipulator.mode = ED_LEVEL_MANIP_MODE_TRANSLATION;
    }

    else if(in_GetKeyState(SDL_SCANCODE_R) & IN_KEY_STATE_JUST_PRESSED)
    {
        ed_level_state.manipulator.mode = ED_LEVEL_MANIP_MODE_ROTATION;
    }
}

void ed_w_FlyCamera(struct ed_context_t *context, uint32_t just_changed)
{
    float dx;
    float dy;

    if(!(in_GetKeyState(SDL_SCANCODE_LALT) & IN_KEY_STATE_PRESSED))
    {
        ed_SetNextContextState(context, ed_w_Idle);
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

void ed_w_RightClick(struct ed_context_t *context, uint32_t just_changed)
{
    struct ed_level_state_t *context_data = (struct ed_level_state_t *)context->context_data;

    switch(context_data->pickables.secondary_click_function)
    {
        case ED_LEVEL_SECONDARY_CLICK_FUNC_BRUSH:
            context_data->pickables.ignore_types = ED_PICKABLE_OBJECT_MASK;
            ed_w_PickObjectOrWidget(context, just_changed);
        break;

        case ED_LEVEL_SECONDARY_CLICK_FUNC_LIGHT:
            ed_SetNextContextState(context, ed_w_PlaceLightAtCursor);
        break;
    }
}

void ed_w_LeftClick(struct ed_context_t *context, uint32_t just_changed)
{
    struct ed_level_state_t *context_data = (struct ed_level_state_t *)context->context_data;
    context_data->pickables.ignore_types = ED_PICKABLE_BRUSH_PART_MASK;
    ed_w_PickObjectOrWidget(context, just_changed);
}

void ed_w_BrushBox(struct ed_context_t *context, uint32_t just_changed)
{
    struct ed_level_state_t *context_data = (struct ed_level_state_t *)context->context_data;
    uint32_t right_button_down = in_GetMouseButtonState(SDL_BUTTON_RIGHT) & IN_KEY_STATE_PRESSED;
    uint32_t ctrl_down = in_GetKeyState(SDL_SCANCODE_LCTRL) & IN_KEY_STATE_PRESSED;

    if(just_changed)
    {
        context_data->brush.drawing = 0;
    }

    if((ctrl_down && (!context_data->brush.drawing)) || right_button_down)
    {
        if(in_GetKeyState(SDL_SCANCODE_ESCAPE) & IN_KEY_STATE_PRESSED)
        {
            ed_SetNextContextState(context, ed_w_Idle);
        }
        else
        {
            float normalized_mouse_x;
            float normalized_mouse_y;
            int32_t mouse_x;
            int32_t mouse_y;

            in_GetNormalizedMousePos(&normalized_mouse_x, &normalized_mouse_y);
            in_GetMousePos(&mouse_x, &mouse_y);

            vec3_t intersection = {};

            if(!context_data->brush.drawing)
            {
                uint32_t ignore_types = ED_PICKABLE_OBJECT_MASK | ED_PICKABLE_TYPE_MASK_EDGE | ED_PICKABLE_TYPE_MASK_VERT;
                struct ds_slist_t *pickables = &context_data->pickables.pickables;
                struct ed_pickable_t *surface = ed_SelectPickable(mouse_x, mouse_y, pickables, NULL, ignore_types);

                if(surface)
                {
                    struct ed_brush_t *brush = ed_GetBrush(surface->primary_index);
                    struct ed_face_t *face = ed_GetFace(surface->secondary_index);
                    context_data->brush.plane_orientation.rows[1] = face->polygons->normal;
                    context_data->brush.plane_point = *(vec3_t *)ds_list_get_element(&face->clipped_polygons->vertices, 0);
                    mat3_t_vec3_t_mul(&context_data->brush.plane_orientation.rows[1], &context_data->brush.plane_orientation.rows[1], &brush->orientation);
                    mat3_t_vec3_t_mul(&context_data->brush.plane_point, &context_data->brush.plane_point, &brush->orientation);
                    vec3_t_add(&context_data->brush.plane_point, &context_data->brush.plane_point, &brush->position);

                    float max_axis_proj = -FLT_MAX;
                    uint32_t j_axis_index = 0;
                    mat3_t *plane_orientation = &context_data->brush.plane_orientation;
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
                    context_data->brush.plane_point = vec3_t_c(0.0, 0.0, 0.0);
                    mat3_t_identity(&context_data->brush.plane_orientation);
                }
            }

            vec3_t plane_point = context_data->brush.plane_point;
            mat3_t plane_orientation = context_data->brush.plane_orientation;

            if(ed_w_IntersectPlaneFromCamera(normalized_mouse_x, normalized_mouse_y, &plane_point, &plane_orientation.rows[1], &intersection))
            {
                r_i_SetModelMatrix(NULL);
                r_i_SetViewProjectionMatrix(NULL);
                r_i_SetShader(NULL);

                vec3_t plane_origin;
                /* compute where the world origin projects onto the plane */
                vec3_t_mul(&plane_origin, &plane_orientation.rows[1], vec3_t_dot(&plane_point, &plane_orientation.rows[1]));

                /* transform intersection point from world space to plane space */
                vec3_t_sub(&intersection, &intersection, &plane_origin);
                vec3_t transformed_intersection;
                transformed_intersection.x = vec3_t_dot(&intersection, &plane_orientation.rows[0]);
                transformed_intersection.y = vec3_t_dot(&intersection, &plane_orientation.rows[1]);
                transformed_intersection.z = vec3_t_dot(&intersection, &plane_orientation.rows[2]);
                intersection = transformed_intersection;

                if(context_data->manipulator.linear_snap)
                {
                    float linear_snap = context_data->manipulator.linear_snap;
                    float x_a = ceilf(intersection.x / linear_snap) * linear_snap;
                    float x_b = floorf(intersection.x / linear_snap) * linear_snap;

                    float z_a = ceilf(intersection.z / linear_snap) * linear_snap;
                    float z_b = floorf(intersection.z / linear_snap) * linear_snap;

                    if(intersection.x > 0.0)
                    {
                        float t = x_a;
                        x_a = x_b;
                        x_b = t;
                    }

                    if(intersection.z > 0.0)
                    {
                        float t = z_a;
                        z_a = z_b;
                        z_b = t;
                    }

                    float snapped_x;
                    float snapped_z;

                    if(fabsf(fabsf(intersection.x) - fabsf(x_a)) < fabsf(fabsf(x_b) - fabsf(intersection.x)))
                    {
                        snapped_x = x_a;
                    }
                    else
                    {
                        snapped_x = x_b;
                    }

                    if(fabsf(fabsf(intersection.z) - fabsf(z_a)) < fabsf(fabsf(z_b) - fabsf(intersection.z)))
                    {
                        snapped_z = z_a;
                    }
                    else
                    {
                        snapped_z = z_b;
                    }

                    intersection.x = snapped_x;
                    intersection.z = snapped_z;
                }

                mat3_t_vec3_t_mul(&intersection, &intersection, &plane_orientation);
                vec3_t_add(&intersection, &intersection, &plane_origin);

                if(right_button_down)
                {
                    if(!context_data->brush.drawing)
                    {
                        context_data->brush.box_start = intersection;
                        context_data->brush.drawing |= right_button_down;
                    }

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
                else
                {
                    vec3_t start = intersection;
                    vec3_t end = intersection;
                    vec3_t normal = plane_orientation.rows[1];

                    vec3_t u_axis;
                    vec3_t v_axis;

                    vec3_t_mul(&u_axis, &plane_orientation.rows[0], ED_W_BRUSH_BOX_CROSSHAIR_DIM);
                    vec3_t_mul(&v_axis, &plane_orientation.rows[2], ED_W_BRUSH_BOX_CROSSHAIR_DIM);

                    int32_t window_x;
                    int32_t window_y;

                    ed_w_PointPixelCoords(&window_x, &window_y, &intersection);

                    igSetNextWindowPos((ImVec2){window_x, window_y}, 0, (ImVec2){0.0, 0.0});
                    igSetNextWindowBgAlpha(0.25);
                    if(igBegin("crosshair", NULL, ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoTitleBar |
                                             ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDecoration))
                    {
                        igText("pos: [%f, %f, %f]", intersection.x, intersection.y, intersection.z);
                        igText("norm: [%f, %f, %f]", normal.x, normal.y, normal.z);
                    }
                    igEnd();

                    r_i_DrawLine(&vec3_t_c(start.x + u_axis.x, start.y + u_axis.y, start.z + u_axis.z),
                                 &vec3_t_c(end.x - u_axis.x, end.y - u_axis.y, end.z - u_axis.z),
                                 &vec4_t_c(1.0, 1.0, 1.0, 1.0), 4.0);

                    r_i_DrawLine(&vec3_t_c(start.x + v_axis.x, start.y + v_axis.y, start.z + v_axis.z),
                                 &vec3_t_c(end.x - v_axis.x, end.y - v_axis.y, end.z - v_axis.z),
                                 &vec4_t_c(1.0, 1.0, 1.0, 1.0), 4.0);
                }
            }
        }
    }
    else
    {
//        if(!context_data->pickables.active_list->selections.cursor)
//        {
//            context_data->pickables.next_edit_mode = ED_W_CTX_EDIT_MODE_OBJECT;
//            ed_w_SetEditMode(context, just_changed);
//        }

        if(!context_data->brush.drawing)
        {
            ed_SetNextContextState(context, ed_w_Idle);
        }
        else
        {
            vec3_t position;
            vec3_t size;

            size.x = context_data->brush.box_size.x;
            size.y = 1.0;
            size.z = context_data->brush.box_size.y;

            vec3_t_add(&position, &context_data->brush.box_start, &context_data->brush.box_end);
            vec3_t_mul(&position, &position, 0.5);
            vec3_t_fmadd(&position, &position, &context_data->brush.plane_orientation.rows[1], size.y * 0.5);

            ed_CreateBrushPickable(&position, &context_data->brush.plane_orientation, &size, NULL);
            ed_SetNextContextState(context, ed_w_Idle);
        }
    }
}

void ed_w_PickObjectOrWidget(struct ed_context_t *context, uint32_t just_changed)
{
    struct ed_level_state_t *context_data = (struct ed_level_state_t *)context->context_data;
    int32_t mouse_x;
    int32_t mouse_y;

    in_GetMousePos(&mouse_x, &mouse_y);

    if(just_changed)
    {
        context_data->pickables.last_selected = NULL;

        if(context_data->pickables.selections.cursor)
        {
            struct ed_widget_t *manipulator = context_data->manipulator.widgets[context_data->manipulator.mode];
            context_data->pickables.last_selected = ed_SelectWidget(mouse_x, mouse_y, manipulator, &context_data->manipulator.transform);
        }
    }

    if(context_data->pickables.last_selected)
    {
        if(context_data->pickables.last_selected->type == ED_PICKABLE_TYPE_WIDGET)
        {
            ed_SetNextContextState(context, ed_w_TransformSelections);
        }
        else
        {
            context_data->pickables.last_selected = NULL;
        }
    }
    else
    {
        ed_SetNextContextState(context, ed_w_PickObject);
    }
}

void ed_w_PickObject(struct ed_context_t *context, uint32_t just_changed)
{
    struct ed_level_state_t *context_data = (struct ed_level_state_t *)context->context_data;
    uint32_t button_state = in_GetMouseButtonState(SDL_BUTTON_LEFT) | in_GetMouseButtonState(SDL_BUTTON_RIGHT);
    int32_t mouse_x;
    int32_t mouse_y;
//    uint32_t mode_change = context_data->pickables.edit_mode != context_data->pickables.next_edit_mode;

    in_GetMousePos(&mouse_x, &mouse_y);

    if(!(button_state & IN_KEY_STATE_PRESSED))
    {
        uint32_t ignore_types = context_data->pickables.ignore_types;;
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

        ed_SetNextContextState(context, ed_w_Idle);
    }
}

void ed_w_PlaceLightAtCursor(struct ed_context_t *context, uint32_t just_changed)
{
    struct ed_level_state_t *context_data = (struct ed_level_state_t *)context->context_data;

    if(!(in_GetMouseButtonState(SDL_BUTTON_RIGHT) & IN_KEY_STATE_PRESSED))
    {
        ed_CreateLightPickable(&vec3_t_c(0.0, 0.0, 0.0), &vec3_t_c(1.0, 1.0, 1.0), 6.0, 10.0, NULL);
        ed_SetNextContextState(context, ed_w_Idle);
    }
}

void ed_w_TransformSelections(struct ed_context_t *context, uint32_t just_changed)
{
    struct ed_level_state_t *context_data = (struct ed_level_state_t *)context->context_data;
    uint32_t mouse_state = in_GetMouseButtonState(SDL_BUTTON_LEFT);

    if(mouse_state & IN_KEY_STATE_PRESSED)
    {
        struct ed_widget_t *manipulator = context_data->manipulator.widgets[context_data->manipulator.mode];

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
        float mouse_x;
        float mouse_y;

        in_GetNormalizedMousePos(&mouse_x, &mouse_y);

        r_i_SetShader(NULL);
        r_i_SetViewProjectionMatrix(NULL);
        r_i_SetModelMatrix(NULL);

        switch(context_data->manipulator.mode)
        {
            case ED_LEVEL_MANIP_MODE_TRANSLATION:
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


//                r_i_DrawLine(&context_data->manipulator.start_pos, &manipulator_transform->rows[3].xyz, &axis_color[axis_index], 2.0);
//
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

                ed_w_TranslateSelected(&cur_offset, 0);
            }
            break;

            case ED_LEVEL_MANIP_MODE_ROTATION:
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

                ed_w_RotateSelected(&rotation, &context_data->manipulator.transform.rows[3].xyz, 0);
            }
            break;
        }
    }
    else
    {
        ed_SetNextContextState(context, ed_w_Idle);
        context_data->pickables.last_selected = NULL;
    }
}

void ed_SerializeLevel(void **level_buffer, size_t *buffer_size, uint32_t serialize_brushes)
{
    size_t out_buffer_size = sizeof(struct ed_level_section_t);
    size_t brush_section_size = 0;

    if(serialize_brushes)
    {
        brush_section_size = sizeof(struct ed_brush_section_t);
        brush_section_size += sizeof(struct ed_brush_record_t) * ed_level_state.brush.brushes.used;
        brush_section_size += sizeof(struct ed_vert_record_t) * ed_level_state.brush.brush_vert_count;
        brush_section_size += sizeof(struct ed_edge_record_t) * ed_level_state.brush.brush_edges.used;
        brush_section_size += sizeof(struct ed_polygon_record_t) * ed_level_state.brush.brush_face_polygons.used;
        /* each edge is referenced by two polygons, so its index will be serialized twice */
        brush_section_size += sizeof(size_t) * ed_level_state.brush.brush_edges.used * 2;
        brush_section_size += sizeof(struct ed_face_t) * ed_level_state.brush.brush_faces.used;
    }

    size_t light_section_size = sizeof(struct l_light_section_t);
    light_section_size += sizeof(struct l_light_record_t) * r_lights[R_LIGHT_TYPE_POINT].used;
    light_section_size += sizeof(struct l_light_record_t) * r_lights[R_LIGHT_TYPE_SPOT].used;

    size_t entity_section_size = sizeof(struct l_entity_section_t);
    /* the reason we use the amount of root transforms here instead of the total amount of entities
    is that an entity gets spawned from an entity def, which defines the hierarchical structure of
    an entity. Child entities shouldn't have an entity record because they'll get spawned from the
    ent def. Only one record is necessary for the root entity. */
    entity_section_size += sizeof(struct l_entity_record_t) * e_root_transforms.cursor;

    size_t ent_def_section_size = sizeof(struct l_ent_def_section_t);
    ent_def_section_size += sizeof(struct l_ent_def_record_t) * e_ent_defs[E_ENT_DEF_TYPE_ROOT].used;


    size_t world_section_size = 0;

    if(l_world_collider)
    {
        world_section_size = sizeof(struct l_world_section_t);
        world_section_size += sizeof(struct r_material_record_t) * l_world_model->batches.buffer_size;
//        world_section_size += sizeof(struct r_)
    }



    out_buffer_size += brush_section_size + light_section_size + entity_section_size + ent_def_section_size + world_section_size;

    char *start_out_buffer = mem_Calloc(1, out_buffer_size);
    char *cur_out_buffer = start_out_buffer;
    *level_buffer = start_out_buffer;
    *buffer_size = out_buffer_size;

    struct ed_level_section_t *level_section = (struct ed_level_section_t *)cur_out_buffer;
    cur_out_buffer += sizeof(struct ed_level_section_t);

    /* level editor stuff */
    level_section->camera_pos = ed_level_state.camera_pos;
    level_section->camera_pitch = ed_level_state.camera_pitch;
    level_section->camera_yaw = ed_level_state.camera_yaw;
    level_section->magic0 = ED_LEVEL_SECTION_MAGIC0;
    level_section->magic1 = ED_LEVEL_SECTION_MAGIC1;

    if(serialize_brushes)
    {
        /* brush stuff */
        level_section->brush_section_start = cur_out_buffer - start_out_buffer;
        level_section->brush_section_size = brush_section_size;

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
                brush_record->record_size = cur_out_buffer;
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
    }

    /* light stuff */
    level_section->light_section_size = light_section_size;
    level_section->light_section_start = cur_out_buffer - start_out_buffer;

    struct l_light_section_t *light_section = (struct l_light_section_t *)cur_out_buffer;
    cur_out_buffer += sizeof(struct l_light_section_t);
    light_section->record_start = cur_out_buffer - start_out_buffer;
    struct l_light_record_t *light_records = (struct l_light_record_t *)cur_out_buffer;
    cur_out_buffer += sizeof(struct l_light_record_t) * r_lights[R_LIGHT_TYPE_POINT].used;
    cur_out_buffer += sizeof(struct l_light_record_t) * r_lights[R_LIGHT_TYPE_SPOT].used;

    for(uint32_t light_index = 0; light_index < r_lights[R_LIGHT_TYPE_POINT].cursor; light_index++)
    {
        struct r_light_t *light = r_GetLight(light_index);

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
        }
    }

    level_section->ent_def_section_size = ent_def_section_size;
    level_section->ent_def_section_start = cur_out_buffer - start_out_buffer;

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


    level_section->entity_section_size = entity_section_size;
    level_section->entity_section_start = cur_out_buffer - start_out_buffer;

    struct l_entity_section_t *entity_section = (struct l_entity_section_t *)cur_out_buffer;
    cur_out_buffer += sizeof(struct l_entity_section_t);
    entity_section->record_start = cur_out_buffer - start_out_buffer;
    struct l_entity_record_t *entity_records = (struct l_entity_record_t *)cur_out_buffer;
    cur_out_buffer += sizeof(struct l_entity_record_t) * e_root_transforms.cursor;

    for(uint32_t entity_index = 0; entity_index < e_root_transforms.cursor; entity_index++)
    {
        struct e_local_transform_component_t *transform;
        transform = *(struct e_local_transform_component_t **)ds_list_get_element(&e_root_transforms, entity_index);

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

void ed_DeserializeLevel(void *level_buffer, size_t buffer_size)
{
    char *start_in_buffer = level_buffer;
    char *cur_in_buffer = start_in_buffer;
    struct ed_level_section_t *level_section = (struct ed_level_section_t *)cur_in_buffer;

    if(level_section->magic0 != ED_LEVEL_SECTION_MAGIC0 || level_section->magic1 != ED_LEVEL_SECTION_MAGIC1)
    {
        return;
    }

    /* level editor stuff */
    ed_level_state.camera_pitch = level_section->camera_pitch;
    ed_level_state.camera_yaw = level_section->camera_yaw;
    ed_level_state.camera_pos = level_section->camera_pos;

    /* brush stuff */
    struct ed_brush_section_t *brush_section = (struct ed_brush_section_t *)(start_in_buffer + level_section->brush_section_start);
    cur_in_buffer = start_in_buffer + brush_section->brush_record_start;

    for(uint32_t record_index = 0; record_index < brush_section->brush_record_count; record_index++)
    {
        struct ed_brush_record_t *brush_record = (struct ed_brush_record_t *)cur_in_buffer;
        cur_in_buffer += brush_record->record_size;

        struct ed_vert_record_t *vert_records = (struct ed_vert_record_t *)(start_in_buffer + brush_record->vert_start);
        struct ed_edge_record_t *edge_records = (struct ed_edge_record_t *)(start_in_buffer + brush_record->edge_start);
        struct ed_face_record_t *face_records = (struct ed_face_record_t *)(start_in_buffer + brush_record->face_start);

        struct ed_brush_t *brush = ed_AllocBrush();
        brush->flags |= ED_BRUSH_FLAG_GEOMETRY_MODIFIED;
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

            face->material = r_GetMaterial(face_record->material);
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

    l_DeserializeLevel(level_buffer, buffer_size, L_LEVEL_DATA_ALL & (~L_LEVEL_DATA_WORLD));

    for(uint32_t light_index = 0; light_index < r_lights[R_LIGHT_TYPE_POINT].cursor; light_index++)
    {
        struct r_light_t *light = r_GetLight(light_index);

        if(light)
        {
            ed_CreateLightPickable(NULL, NULL, 0.0, 0.0, light);
        }
    }
}

void ed_SaveLevel(char *path, char *file)
{
    void *buffer;
    size_t buffer_size;
    char file_name[PATH_MAX];

    ed_SerializeLevel(&buffer, &buffer_size, 1);
    ds_path_append_end(path, file, file_name, PATH_MAX);
    ds_path_set_ext(file_name, "nlv", file_name, PATH_MAX);

    FILE *fp = fopen(file_name, "wb");
    fwrite(buffer, buffer_size, 1, fp);
    fclose(fp);
}

void ed_LoadLevel(char *path, char *file)
{
    void *buffer;
    size_t buffer_size;
    char file_name[PATH_MAX];

    ds_path_append_end(path, file, file_name, PATH_MAX);
    FILE *fp = fopen(file_name, "rb");

    if(!fp)
    {
        printf("couldn't find level file %s\n", file_name);
        return;
    }

    ed_ResetLevelEditor();

    read_file(fp, &buffer, &buffer_size);
    ed_DeserializeLevel(buffer, buffer_size);
    mem_Free(buffer);
}

void ed_SaveGameLevelSnapshot()
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

    ed_BuildWorldData();
    ed_SerializeLevel(&ed_level_state.game_level_buffer, &ed_level_state.game_level_buffer_size, 0);
}

void ed_LoadGameLevelSnapshot()
{
    l_DeserializeLevel(ed_level_state.game_level_buffer, ed_level_state.game_level_buffer_size, L_LEVEL_DATA_ALL & (~L_LEVEL_DATA_WORLD));

    for(uint32_t brush_index = 0; brush_index < ed_level_state.brush.brushes.cursor; brush_index++)
    {
        struct ed_brush_t *brush = ed_GetBrush(brush_index);

        if(brush)
        {
            ed_UpdateBrushEntity(brush);
        }
    }

    char *level_buffer = ed_level_state.game_level_buffer;
    struct ed_level_section_t *level_section = (struct ed_level_section_t *)level_buffer;
    struct l_light_section_t *light_section = (struct l_light_section_t *)(level_buffer + level_section->light_section_start);
    struct l_light_record_t *light_records = (struct l_light_record_t *)(level_buffer + light_section->record_start);
    struct l_entity_section_t *entity_section = (struct l_entity_section_t *)(level_buffer + level_section->entity_section_start);
    struct l_entity_record_t *entity_records = (struct l_entity_record_t *)(level_buffer + entity_section->record_start);

    for(uint32_t pickable_index = 0; pickable_index < ed_level_state.pickables.pickables.cursor; pickable_index++)
    {
        struct ed_pickable_t *pickable = ed_GetPickable(pickable_index);

        if(pickable)
        {
            switch(pickable->type)
            {
                case ED_PICKABLE_TYPE_LIGHT:
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
                break;

                case ED_PICKABLE_TYPE_ENTITY:
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
                break;
            }
        }
    }

    mem_Free(ed_level_state.game_level_buffer);
}

void ed_ResetLevelEditor()
{
    ed_level_state.camera_pitch = ED_LEVEL_CAMERA_PITCH;
    ed_level_state.camera_yaw = ED_LEVEL_CAMERA_YAW;
    ed_level_state.camera_pos = ED_LEVEL_CAMERA_POS;

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

void ed_BuildWorldData()
{
    if(!l_world_collider && ed_level_state.brush.brushes.used)
    {
        struct ds_buffer_t world_col_verts_buffer = ds_buffer_create(sizeof(vec3_t), ed_level_state.brush.brush_model_vert_count);
        struct ds_buffer_t world_draw_verts_buffer = ds_buffer_create(sizeof(struct r_vert_t), ed_level_state.brush.brush_model_vert_count);
        struct ds_buffer_t world_indices_buffer = ds_buffer_create(sizeof(uint32_t), ed_level_state.brush.brush_model_index_count);

        uint32_t vert_offset = 0;
        uint32_t index_offset = 0;

        vec3_t *world_col_verts = world_col_verts_buffer.buffer;
        struct r_vert_t *world_draw_verts = world_draw_verts_buffer.buffer;
        uint32_t *world_indices = world_indices_buffer.buffer;

        struct ds_buffer_t world_batches_buffer = ds_buffer_create(sizeof(struct r_batch_t), ed_level_state.brush.brush_batches.cursor);
        struct r_batch_t *world_batches = world_batches_buffer.buffer;
        struct ds_list_t *global_batches = &ed_level_state.brush.brush_batches;

        for(uint32_t global_batch_index = 0; global_batch_index < global_batches->cursor; global_batch_index++)
        {
            struct r_batch_t *world_batch = world_batches + global_batch_index;
            struct ed_brush_batch_t *global_batch = ds_list_get_element(global_batches, global_batch_index);

            world_batch->start = 0;
            world_batch->count = global_batch->batch.count;
            world_batch->material = global_batch->batch.material;

            if(global_batch_index)
            {
                struct r_batch_t *prev_batch = world_batches + (global_batch_index - 1);
                world_batch->start = prev_batch->start + prev_batch->count;
                prev_batch->count = 0;
            }
        }

        world_batches[global_batches->cursor - 1].count = 0;

        for(uint32_t brush_index = 0; brush_index < ed_level_state.brush.brushes.cursor; brush_index++)
        {
            struct ed_brush_t *brush = ed_GetBrush(brush_index);

            if(brush)
            {
                struct r_batch_t *brush_batches = brush->model->batches.buffer;
                struct r_vert_t *brush_verts = brush->model->verts.buffer;
                uint32_t *brush_indices = brush->model->indices.buffer;

                for(uint32_t vert_index = 0; vert_index < brush->model->verts.buffer_size; vert_index++)
                {
                    struct r_vert_t *brush_vert = brush_verts + vert_index;
                    struct r_vert_t *draw_vert = world_draw_verts + vert_offset + vert_index;
                    *draw_vert = *brush_vert;

                    mat3_t_vec3_t_mul(&draw_vert->pos, &draw_vert->pos, &brush->orientation);
                    mat3_t_vec3_t_mul(&draw_vert->normal.xyz, &draw_vert->normal.xyz, &brush->orientation);
                    mat3_t_vec3_t_mul(&draw_vert->tangent, &draw_vert->tangent, &brush->orientation);
                    vec3_t_add(&draw_vert->pos, &draw_vert->pos, &brush->position);

                    world_col_verts[vert_offset + vert_index] = draw_vert->pos;
                }

                for(uint32_t brush_batch_index = 0; brush_batch_index < brush->model->batches.buffer_size; brush_batch_index++)
                {
                    struct r_batch_t *brush_batch = brush_batches + brush_batch_index;

                    for(uint32_t world_batch_index = 0; world_batch_index < world_batches_buffer.buffer_size; world_batch_index++)
                    {
                        struct r_batch_t *world_batch = world_batches + world_batch_index;

                        if(brush_batch->material == world_batch->material)
                        {
                            for(uint32_t index = 0; index < brush_batch->count; index++)
                            {
                                uint32_t world_indice_index = world_batch->start + world_batch->count;
                                uint32_t brush_indice_index = brush_batch->start - brush->model->model_start + index;
                                world_indices[world_indice_index] = brush_indices[brush_indice_index];
                                world_indices[world_indice_index] += vert_offset;
                                world_batch->count++;
                            }
                        }
                    }
                }

                vert_offset += brush->model->verts.buffer_size;
            }
        }

//        for(uint32_t brush_index = 0; brush_index < ed_level_state.brush.brushes.cursor; brush_index++)
//        {
//            struct ed_brush_t *brush = ed_GetBrush(brush_index);
//
//            if(brush)
//            {
//                struct r_vert_t *model_verts = brush->model->verts.buffer;
//                uint32_t *model_indices = brush->model->indices.buffer;
//
//                for(uint32_t index = 0; index < brush->model->indices.buffer_size; index++)
//                {
//                    indices[index_offset] = model_indices[index] + vert_offset;
//                    index_offset++;
//                }
//
//                for(uint32_t vert_index = 0; vert_index < brush->model->verts.buffer_size; vert_index++)
//                {
//                    col_verts[vert_offset] = model_verts[vert_index].pos;
//                    mat3_t_vec3_t_mul(&col_verts[vert_offset], &col_verts[vert_offset], &brush->orientation);
//                    vec3_t_add(&col_verts[vert_offset], &col_verts[vert_offset], &brush->position);
//                    vert_offset++;
//                }
//            }
//        }

        if(vert_offset)
        {
            l_world_shape->itri_mesh.verts = world_col_verts;
            l_world_shape->itri_mesh.vert_count = vert_offset;
            l_world_shape->itri_mesh.indices = world_indices;
            l_world_shape->itri_mesh.index_count = world_indices_buffer.buffer_size;
            l_world_collider = p_CreateCollider(&l_world_col_def, &vec3_t_c(0.0, 0.0, 0.0), &mat3_t_c_id());

            struct r_model_geometry_t model_geometry = {};

            model_geometry.batches = world_batches;
            model_geometry.batch_count = world_batches_buffer.buffer_size;
            model_geometry.verts = world_draw_verts;
            model_geometry.vert_count = world_draw_verts_buffer.buffer_size;
            model_geometry.indices = world_indices;
            model_geometry.index_count = world_indices_buffer.buffer_size;
            l_world_model = r_CreateModel(&model_geometry, NULL, "world_model");
        }
    }

//    struct ds_buffer_t batches = ds_buffer_create(sizeof(struct r_batch_t), ed_level_state.brush.brush_batches.cursor);
//

}









