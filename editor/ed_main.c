#include "ed_main.h"
#include "ed_bsp.h"
#include "ed_level.h"
#include "dstuff/ds_slist.h"
#include "dstuff/ds_list.h"
#include "dstuff/ds_mem.h"
#include "dstuff/ds_vector.h"
#include "dstuff/ds_matrix.h"
#include "dstuff/ds_path.h"
#include "dstuff/ds_dir.h"
#include "dstuff/ds_buffer.h"
#include "../engine/game.h"
#include "../engine/input.h"
#include "../engine/gui.h"
#include "../engine/r_draw.h"
#include <limits.h>
#include <string.h>
#include <float.h>

//struct ds_slist_t ed_brushes;
//uint32_t ed_global_brush_vert_count;
//uint32_t ed_global_brush_index_count;
//struct ds_list_t ed_global_brush_batches;

//struct ds_slist_t ed_objects;

struct ed_context_t *ed_active_context;
struct ed_context_t ed_contexts[ED_CONTEXT_LAST];
struct ed_editor_t ed_editors[ED_EDITOR_LAST];
struct ed_editor_t *ed_active_editor;
//uint32_t ed_grid_vert_count;
//struct r_vert_t *ed_grid;
//struct r_i_verts_t *ed_grid;
struct ds_slist_t ed_polygons;
struct ds_slist_t ed_bsp_nodes;

//struct r_shader_t *ed_center_grid_shader;
//struct r_shader_t *ed_picking_shader;
//uint32_t ed_picking_shader_type_uniform;
//uint32_t ed_picking_shader_index_uniform;

//struct r_shader_t *ed_outline_shader;
//uint32_t ed_outline_shader_color_uniform;



//float ed_camera_pitch;
//float ed_camera_yaw;
//vec3_t ed_camera_pos;
uint32_t ed_picking_framebuffer;
uint32_t ed_picking_depth_texture;
uint32_t ed_picking_object_texture;
uint32_t ed_show_renderer_info_window;

//struct r_model_t *ed_translation_widget_model;

extern uint32_t g_game_state;
extern mat4_t r_camera_matrix;
extern float r_z_near;
extern float r_fov;
extern uint32_t r_width;
extern uint32_t r_height;
extern mat4_t r_view_projection_matrix;
extern uint32_t r_vertex_buffer;
extern uint32_t r_index_buffer;
extern struct ds_slist_t r_lights[];
extern uint32_t r_prev_draw_call_count;

//extern struct r_renderer_stats_t r_renderer_stats;
extern struct r_renderer_state_t r_renderer_state;


//#define ED_GRID_DIVS 301
//#define ED_GRID_QUAD_SIZE 500.0

//struct ed_state_t ed_world_context_states[] =
//{
//    [ED_WORLD_CONTEXT_STATE_IDLE] = ed_WorldContextIdleState,
//    [ED_WORLD_CONTEXT_STATE_LEFT_CLICK] = ed_WorldContextLeftClickState,
//    [ED_WORLD_CONTEXT_STATE_BRUSH_BOX] = ed_WorldContextStateBrushBox,
//    [ED_WORLD_CONTEXT_STATE_CREATE_BRUSH] = ed_WorldContextCreateBrush,
//    [ED_WORLD_CONTEXT_STATE_PROCESS_SELECTION] = ed_WorldContextProcessSelection,
//};

//struct ed_world_context_data_t ed_world_context_data;

struct ed_explorer_ext_filter_t
{
    char extension[8];
};

struct ed_explorer_drive_t
{
    char drive[4];
};

enum ED_EDITOR_EXPLORER_MODE
{
    ED_EDITOR_EXPLORER_MODE_OPEN = 0,
    ED_EDITOR_EXPLORER_MODE_SAVE,
};

struct
{
    uint32_t open;
    uint32_t mode;
    char current_path[PATH_MAX];
    char current_file[PATH_MAX];
    char search_bar[PATH_MAX];
    struct ds_list_t dir_entries;
    struct ds_list_t matched_dir_entries;
    struct ds_list_t ext_filters;
    struct ds_list_t drives;

    void (*load_callback)(char *path, char *file);
    void (*save_callback)(char *path, char *file);
}ed_explorer_state;

//uint32_t ed_explorer_open = 0;

void test_load_callback(char *path, char *file)
{
    printf("load file %s/%s\n", path, file);
}

void test_save_callback(char *path, char *file)
{
    printf("save file %s/%s\n", path, file);
}

void ed_Init()
{
    SDL_DisplayMode desktop_display_mode;

//    ed_editors = ds_slist_create(sizeof(struct ed_editor_t), 8);

    SDL_GetDesktopDisplayMode(0, &desktop_display_mode);

    glGenFramebuffers(1, &ed_picking_framebuffer);
    glGenTextures(1, &ed_picking_object_texture);
    glBindTexture(GL_TEXTURE_2D, ed_picking_object_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32UI, r_width, r_height, 0, GL_RG_INTEGER, GL_INT, NULL);

    glGenTextures(1, &ed_picking_depth_texture);
    glBindTexture(GL_TEXTURE_2D, ed_picking_depth_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, r_width, r_height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, ed_picking_framebuffer);
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ed_picking_object_texture, 0);
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, ed_picking_depth_texture, 0);

    ed_w_Init();
    ed_active_context = ed_contexts + ED_CONTEXT_WORLD;

    ed_explorer_state.current_file[0] = '\0';
    ed_explorer_state.current_path[0] = '\0';
    ed_explorer_state.open = 0;
    ed_explorer_state.dir_entries = ds_list_create(sizeof(struct ds_dir_entry_t), 64);
    ed_explorer_state.matched_dir_entries = ds_list_create(sizeof(struct ds_dir_entry_t *), 64);
    ed_explorer_state.ext_filters = ds_list_create(sizeof(struct ed_explorer_ext_filter_t), 64);
    ed_explorer_state.drives = ds_list_create(sizeof(struct ed_explorer_drive_t), 8);

    strcpy(ed_explorer_state.current_path, "C:\\Users");

    ed_EnumerateExplorerDrives();
    ed_SetExplorerLoadCallback(test_load_callback);
    ed_SetExplorerSaveCallback(test_save_callback);

    in_SetMouseRelative(1);
}

void ed_Shutdown()
{

}

struct ed_editor_t *ed_RegisterEditor(struct ed_editor_t *editor)
{
//    uint32_t index;
//    struct ed_editor_t *new_editor;
//
//    index = ds_slist_add_element(&ed_editors, editor);
//    new_editpr = ds_slist_get_element(&ed_editors, index);
//    new_editor->index = index;
//    new_editor->init();
//    return new_editor;
}

void ed_SwitchToEditor(struct ed_editor_t *editor)
{
    if(ed_active_editor)
    {
        ed_active_editor->suspend();
    }

    ed_active_editor = editor;
    ed_active_editor->resume();
}

void ed_UpdateEditor()
{
    for(uint32_t context_index = 0; context_index < ED_CONTEXT_LAST; context_index++)
    {
        struct ed_context_t *context = ed_contexts + context_index;

        if(context == ed_active_context)
        {
            uint32_t just_changed = context->current_state != context->next_state;
            context->current_state = context->next_state;
            context->current_state(context, just_changed);
        }

        context->update();
    }

    if(igBeginMainMenuBar())
    {
        if(igBeginMenu("File", 1))
        {
            if(igMenuItem_Bool("New", NULL, 0, 1))
            {
                ed_ResetLevelEditor();
            }

            if(igMenuItem_Bool("Save", NULL, 0, 1))
            {

            }

            if(igMenuItem_Bool("Save as...", NULL, 0, 1))
            {
                ed_SetExplorerSaveCallback(ed_SaveLevel);
                ed_OpenExplorer(ed_explorer_state.current_path, ED_EDITOR_EXPLORER_MODE_SAVE);
            }

            if(igMenuItem_Bool("Load", NULL, 0, 1))
            {
                ed_SetExplorerLoadCallback(ed_LoadLevel);
                ed_OpenExplorer(ed_explorer_state.current_path, ED_EDITOR_EXPLORER_MODE_OPEN);
            }

            if(igBeginMenu("Recent", 1))
            {
                igEndMenu();
            }

            if(igMenuItem_Bool("Exit", NULL, 0, 1))
            {
                g_SetGameState(G_GAME_STATE_QUIT);
            }

            igEndMenu();
        }

        if(igBeginMenu("Misc", 1))
        {
            if(igMenuItem_Bool("Renderer info", NULL, 0, 1))
            {
                ed_show_renderer_info_window ^= 1;
            }
            if(igMenuItem_Bool("Gen world collider", NULL, 0, 1))
            {
                ed_BuildWorldGeometry();
            }
            igEndMenu();
        }
        igEndMainMenuBar();

        if(ed_show_renderer_info_window)
        {
            if(igBegin("Renderer info", NULL, 0))
            {
                igText("Draw calls: %d", r_renderer_state.draw_call_count);
                igText("Shader swaps: %d", r_renderer_state.shader_swaps);
                igText("Material swaps: %d", r_renderer_state.material_swaps);
                igCheckbox("Z prepass", &r_renderer_state.use_z_prepass);
                igCheckbox("Draw lights", &r_renderer_state.draw_lights);
            }
            igEnd();
        }
    }

    ed_UpdateExplorer();
}

void ed_SetNextContextState(struct ed_context_t *context, void (*state_fn)(struct ed_context_t *context, uint32_t just_changed))
{
    context->next_state = state_fn;
}

/*
=============================================================
=============================================================
=============================================================
*/

void ed_UpdateExplorer()
{
    char temp_path[PATH_MAX];
    if(ed_explorer_state.open)
    {
        float frame_height = igGetFrameHeight();
        igSetNextWindowPos((ImVec2){0, frame_height}, ImGuiCond_Always, (ImVec2){0, 0});
        igSetNextWindowSize((ImVec2){r_width, r_height - frame_height}, ImGuiCond_Always);

        if(igBegin("Explorer", NULL, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse))
        {
            if(igBeginTable("explorer_table", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchSame, (ImVec2){0, 0}, 0.0))
            {
                igTableSetupColumn("dir_tree", 0, 0.25, 0);
                igTableNextColumn();
                if(igBeginChild_Str("left_side", (ImVec2){0, 0}, 1, 0))
                {
                    igText("Drives");
                    igSameLine(0.0, -1.0);
                    if(igButton("Refresh", (ImVec2){0, 0}))
                    {
                        ed_EnumerateExplorerDrives();
                    }
                    if(igBeginListBox("", (ImVec2){-FLT_MIN, 120.0}))
                    {
                        for(uint32_t drive_index = 0; drive_index < ed_explorer_state.drives.cursor; drive_index++)
                        {
                            struct ed_explorer_drive_t *drive = ds_list_get_element(&ed_explorer_state.drives, drive_index);
                            if(igMenuItem_Bool(drive->drive, NULL, 0, 1))
                            {
                                ed_ChangeExplorerPath(drive->drive);
                            }
                        }
                        igEndListBox();
                    }

//                    igSeparator();
//
//                    igText("Recent");
//                    if(igBeginListBox("", (ImVec2){-FLT_MIN, 250.0}))
//                    {
//                        igEndListBox();
//                    }
                }
                igEndChild();


                igTableNextColumn();
                if(igBeginChild_Str("right_side_top", (ImVec2){0.0, 35.0}, 1, 0))
                {
                    igButton("Back", (ImVec2){0.0, 0.0});
                    igSameLine(0.0, -1.0);
                    igButton("Forward", (ImVec2){0.0, 0.0});
                    igSameLine(0.0, -1.0);


                    igSetNextItemWidth(600.0);
                    igInputText("##path", ed_explorer_state.current_path, PATH_MAX, 0, NULL, NULL);
                    if(igIsItemDeactivatedAfterEdit())
                    {
                        ed_ChangeExplorerPath(ed_explorer_state.current_path);
                    }

                    igSameLine(0.0, -1.0);
                    igSetNextItemWidth(160.0);
                    if(igInputText("##search_box", ed_explorer_state.search_bar, PATH_MAX, 0, NULL, NULL))
                    {
                        if(igIsItemDeactivated())
                        {
                            ed_explorer_state.search_bar[0] = '\0';
                        }
                        ed_MatchExplorerEntries(ed_explorer_state.search_bar);
                    }
                }
                igEndChild();


                if(igBeginChild_Str("right_side_middle", (ImVec2){0, -40}, 1, 0))
                {
                    if(igBeginTable("##entry_list", 1, ImGuiTableFlags_ScrollY, (ImVec2){0, 0.0}, 0.0))
                    {
                        igTableSetupScrollFreeze(0, 1);
                        igTableSetupColumn("Name", 0, 0.0, 0);
                        igTableHeadersRow();

                        for(uint32_t entry_index = 0; entry_index < ed_explorer_state.matched_dir_entries.cursor; entry_index++)
                        {
                            struct ds_dir_entry_t *entry = *(struct ds_dir_entry_t **)ds_list_get_element(&ed_explorer_state.matched_dir_entries, entry_index);

                            igTableNextRow(0, 25.0);
                            igTableSetColumnIndex(0);
                            igMenuItem_Bool(entry->name, NULL, 0, 1);
                            if(igIsItemClicked(ImGuiMouseButton_Left))
                            {
                                if(entry->type == DS_DIR_ENTRY_TYPE_DIR)
                                {
                                    ds_path_append_end(ed_explorer_state.current_path, entry->name, ed_explorer_state.current_path, PATH_MAX);
//                                    strcpy(ed_explorer_state.current_path, ds_path_AppendPath(ed_explorer_state.current_path, entry->path));
                                    ed_ChangeExplorerPath(ed_explorer_state.current_path);
                                }
                                else
                                {
                                    if(igIsMouseDoubleClicked(ImGuiMouseButton_Left))
                                    {

                                    }
                                    else
                                    {
                                        strcpy(ed_explorer_state.current_file, entry->name);
                                    }
                                }
                            }
                        }
                        igEndTable();
                    }
                }
                igEndChild();

                if(igBeginChild_Str("right_side_bottom", (ImVec2){0, 0}, 1, 0))
                {
                    igSetNextItemWidth(600.0);
                    if(igInputText("##file", ed_explorer_state.current_file, PATH_MAX, 0, NULL, NULL))
                    {

                    }

                    igSameLine(0.0, -1.0);

                    switch(ed_explorer_state.mode)
                    {
                        case ED_EDITOR_EXPLORER_MODE_OPEN:
                            if(igButton("Open", (ImVec2){100.0, 0.0}) && ed_explorer_state.load_callback)
                            {
                                ed_explorer_state.load_callback(ed_explorer_state.current_path, ed_explorer_state.current_file);
                                ed_CloseExplorer();
                            }
                        break;

                        case ED_EDITOR_EXPLORER_MODE_SAVE:
                            if(igButton("Save", (ImVec2){100.0, 0.0}) && ed_explorer_state.save_callback)
                            {
                                ed_explorer_state.save_callback(ed_explorer_state.current_path, ed_explorer_state.current_file);
                                ed_CloseExplorer();
                            }
                        break;
                    }

                    igSameLine(0.0, -1.0);
                    if(igButton("Cancel", (ImVec2){100.0, 0.0}))
                    {
                        ed_CloseExplorer();
                    }
                }
                igEndChild();


                igEndTable();
            }
        }

        igEnd();
    }
}

void ed_OpenExplorer(char *path, uint32_t mode)
{
    ed_explorer_state.open = 1;
    ed_explorer_state.mode = mode;
    ed_explorer_state.search_bar[0] = '\0';

    if(path)
    {
        ed_ChangeExplorerPath(path);
    }
}

void ed_CloseExplorer()
{
    ed_explorer_state.open = 0;
    ed_explorer_state.load_callback = NULL;
    ed_explorer_state.save_callback = NULL;
}

void ed_EnumerateExplorerDrives()
{
    struct ds_dir_t dir = {};
    ed_explorer_state.drives.cursor = 0;

    for(uint32_t drive_letter = 'A'; drive_letter <= 'Z'; drive_letter++)
    {
        struct ed_explorer_drive_t drive = {};
        strcpy(drive.drive, " :/");
        drive.drive[0] = drive_letter;

        if(ds_dir_open_dir(drive.drive, &dir))
        {
            ds_list_add_element(&ed_explorer_state.drives, &drive);
            ds_dir_close_dir(&dir);
        }
    }
}

void ed_ChangeExplorerPath(char *path)
{
    struct ds_dir_entry_t entry;
    struct ds_dir_t dir;
    char ext[PATH_MAX];

    if(ds_dir_open_dir(path, &dir))
    {
        ed_explorer_state.dir_entries.cursor = 0;

        while(ds_dir_next_entry(&dir, &entry))
        {
            if(entry.type == DS_DIR_ENTRY_TYPE_FILE && ed_explorer_state.ext_filters.cursor)
            {
//                char *ext = ds_path_GetExt(entry.path);
                ds_path_get_ext(entry.name, ext, PATH_MAX);

                for(uint32_t ext_filter_index = 0; ext_filter_index < ed_explorer_state.ext_filters.cursor; ext_filter_index++)
                {
                    struct ed_explorer_ext_filter_t *ext_filter = ds_list_get_element(&ed_explorer_state.ext_filters, ext_filter_index);

                    if(!strcmp(ext_filter->extension, ext))
                    {
                        ds_list_add_element(&ed_explorer_state.dir_entries, &entry);
                        break;
                    }
                }
            }
            else
            {
                ds_list_add_element(&ed_explorer_state.dir_entries, &entry);
            }
        }

        strcpy(ed_explorer_state.current_path, dir.name);

        ds_dir_close_dir(&dir);

        ed_MatchExplorerEntries(ed_explorer_state.search_bar);

        if(!ed_explorer_state.matched_dir_entries.cursor)
        {
            /* we just changed dirs, and there's no matching entry in the new dir, so we'll clear the
            search bar to at least show something */
            ed_MatchExplorerEntries("");
        }
    }
}

void ed_AddExplorerExtFilter(char *ext_filter)
{
    struct ed_explorer_ext_filter_t filter = {};
    strcpy(filter.extension, ext_filter);
    ds_list_add_element(&ed_explorer_state.ext_filters, &filter);
}

void ed_MatchExplorerEntries(char *match)
{
    if(match != ed_explorer_state.search_bar)
    {
        /* the value of match might be the buffer used for the search bar,
        and strcpy expects pointers that don't alias */
        strcpy(ed_explorer_state.search_bar, match);
    }

    ed_explorer_state.matched_dir_entries.cursor = 0;

    for(uint32_t entry_index = 0; entry_index < ed_explorer_state.dir_entries.cursor; entry_index++)
    {
        struct ds_dir_entry_t *entry = ds_list_get_element(&ed_explorer_state.dir_entries, entry_index);

        if(match[0] == '\0')
        {
            ds_list_add_element(&ed_explorer_state.matched_dir_entries, &entry);
        }
        else
        {
            char *match_address = strstr(entry->name, match);

            if(match_address == entry->name)
            {
                /* we'll be matching the beginning of the strings */
                ds_list_add_element(&ed_explorer_state.matched_dir_entries, &entry);
            }
        }
    }
}

void ed_ClearExplorerExtFilters()
{
    ed_explorer_state.ext_filters.cursor = 0;
}

void ed_SetExplorerLoadCallback(void (*load_callback)(char *path, char *file))
{
    ed_explorer_state.load_callback = load_callback;
}

void ed_SetExplorerSaveCallback(void (*save_callback)(char *path, char *file))
{
    ed_explorer_state.save_callback = save_callback;
}

/*
=============================================================
=============================================================
=============================================================
*/

//void ed_FlyCamera()
//{
//    float dx;
//    float dy;
//
//    in_GetMouseDelta(&dx, &dy);
//
//    ed_camera_pitch += dy;
//    ed_camera_yaw -= dx;
//
//    if(ed_camera_pitch > 0.5)
//    {
//        ed_camera_pitch = 0.5;
//    }
//    else if(ed_camera_pitch < -0.5)
//    {
//        ed_camera_pitch = -0.5;
//    }
//
//    vec4_t translation = {};
//
//    if(in_GetKeyState(SDL_SCANCODE_W) & IN_KEY_STATE_PRESSED)
//    {
//        translation.z -= 0.05;
//    }
//    if(in_GetKeyState(SDL_SCANCODE_S) & IN_KEY_STATE_PRESSED)
//    {
//        translation.z += 0.05;
//    }
//
//    if(in_GetKeyState(SDL_SCANCODE_A) & IN_KEY_STATE_PRESSED)
//    {
//        translation.x -= 0.05;
//    }
//    if(in_GetKeyState(SDL_SCANCODE_D) & IN_KEY_STATE_PRESSED)
//    {
//        translation.x += 0.05;
//    }
//
//    mat4_t_vec4_t_mul_fast(&translation, &r_camera_matrix, &translation);
//    vec3_t_add(&ed_camera_pos, &ed_camera_pos, &vec3_t_c(translation.x, translation.y, translation.z));
//}
//
//

//
//void ed_WorldContextUpdate()
//{
//    r_SetViewPos(&ed_camera_pos);
//    r_SetViewPitchYaw(ed_camera_pitch, ed_camera_yaw);
//    ed_UpdatePickables();
//    ed_DrawSelections();
//    ed_DrawGrid();
//    ed_DrawBrushes();
//    ed_DrawLights();
//
////    int32_t mouse_x;
////    int32_t mouse_y;
////    struct ed_pickable_t selection = {};
//
////    in_GetMousePos(&mouse_x, &mouse_y);
////    if(ed_PickObject(mouse_x, mouse_y, &selection))
////    {
////        if(selection.type == ED_PICKABLE_TYPE_BRUSH)
////        {
////            struct ed_brush_t *brush = ed_GetBrush(selection.pick_index);
////            mat4_t model_matrix;
////            mat4_t_comp(&model_matrix, &brush->orientation, &brush->position);
////            r_i_SetModelMatrix(&model_matrix);
////            r_i_SetViewProjectionMatrix(NULL);
////            uint32_t verts_size = sizeof(struct r_i_verts_t) + sizeof(struct r_vert_t) * brush->model->verts.buffer_size;
////            uint32_t indices_size = sizeof(struct r_i_indices_t) + sizeof(uint32_t) * brush->model->indices.buffer_size;
////
////            struct r_i_verts_t *verts = r_i_AllocImmediateData(verts_size);
////            struct r_i_indices_t *indices = r_i_AllocImmediateData(indices_size);
////
////            verts->count = brush->model->verts.buffer_size;
////            indices->count = brush->model->indices.buffer_size;
////
////            for(uint32_t vert_index = 0; vert_index < verts->count; vert_index++)
////            {
////                struct r_vert_t *out_vert = verts->verts + vert_index;
////                struct r_vert_t *in_vert = (struct r_vert_t *)brush->model->verts.buffer + vert_index;
////                *out_vert = *in_vert;
////                out_vert->normal = vec4_t_c(1.0, 0.0, 0.0, 0.2);
////            }
////
////            memcpy(indices->indices, brush->model->indices.buffer, indices->count * sizeof(uint32_t));
////            r_i_SetBlending(GL_TRUE, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
////            r_i_SetDepth(GL_TRUE, GL_LEQUAL);
////            r_i_DrawVertsIndexed(R_I_DRAW_CMD_TRIANGLE_LIST, verts, indices);
////        }
////    }
//}
//
//void ed_WorldContextIdleState(struct ed_context_t *context, uint32_t just_changed)
//{
//    if(in_GetMouseButtonState(SDL_BUTTON_RIGHT) & IN_KEY_STATE_PRESSED)
//    {
//        ed_FlyCamera();
//    }
//    else
//    {
//        if(in_GetMouseButtonState(SDL_BUTTON_LEFT) & IN_KEY_STATE_JUST_PRESSED)
//        {
//            ed_SetContextState(context, ED_WORLD_CONTEXT_STATE_LEFT_CLICK);
//        }
//    }
//}
//
//void ed_WorldContextLeftClickState(struct ed_context_t *context, uint32_t just_changed)
//{
//    struct ed_world_context_data_t *context_data = (struct ed_world_context_data_t *)context->context_data;
//    uint32_t left_button_state = in_GetMouseButtonState(SDL_BUTTON_LEFT);
//    float dx = 0;
//    float dy = 0;
//    int32_t mouse_x;
//    int32_t mouse_y;
//
//    in_GetMouseDelta(&dx, &dy);
//    in_GetMousePos(&mouse_x, &mouse_y);
//
//    if(left_button_state & IN_KEY_STATE_JUST_PRESSED)
//    {
//        context_data->last_selected = ed_SelectPickable(mouse_x, mouse_y);
//    }
//
//    if(left_button_state & IN_KEY_STATE_PRESSED)
//    {
//        if(dx || dy)
//        {
//            ed_SetContextState(context, ED_WORLD_CONTEXT_STATE_BRUSH_BOX);
//        }
//    }
//    else
//    {
//        if(context_data->last_selected)
//        {
//            ed_SetContextState(context, ED_WORLD_CONTEXT_STATE_PROCESS_SELECTION);
//        }
//        else
//        {
//            ed_SetContextState(context, ED_WORLD_CONTEXT_STATE_IDLE);
//        }
//    }
//}
//
//void ed_WorldContextStateBrushBox(struct ed_context_t *context, uint32_t just_changed)
//{
//    struct ed_world_context_data_t *context_data = (struct ed_world_context_data_t *)context->context_data;
//
//    if(in_GetMouseButtonState(SDL_BUTTON_LEFT) & IN_KEY_STATE_PRESSED)
//    {
//        if(in_GetKeyState(SDL_SCANCODE_ESCAPE) & IN_KEY_STATE_PRESSED)
//        {
//            ed_SetContextState(context, ED_WORLD_CONTEXT_STATE_IDLE);
//        }
//        else
//        {
//            vec3_t mouse_pos;
//            vec3_t camera_pos;
//            vec4_t mouse_vec = {};
//
//            float aspect = (float)r_width / (float)r_height;
//            float top = tan(r_fov) * r_z_near;
//            float right = top * aspect;
//
//            in_GetNormalizedMousePos(&mouse_vec.x, &mouse_vec.y);
//            mouse_vec.x *= right;
//            mouse_vec.y *= top;
//            mouse_vec.z = -r_z_near;
//            vec4_t_normalize(&mouse_vec, &mouse_vec);
//            mat4_t_vec4_t_mul_fast(&mouse_vec, &r_camera_matrix, &mouse_vec);
//
//            camera_pos.x = r_camera_matrix.rows[3].x;
//            camera_pos.y = r_camera_matrix.rows[3].y;
//            camera_pos.z = r_camera_matrix.rows[3].z;
//
//            mouse_pos.x = camera_pos.x + mouse_vec.x;
//            mouse_pos.y = camera_pos.y + mouse_vec.y;
//            mouse_pos.z = camera_pos.z + mouse_vec.z;
//
//            float dist_a = camera_pos.y;
//            float dist_b = mouse_pos.y;
//            float denom = (dist_a - dist_b);
//
//            r_i_SetModelMatrix(NULL);
//            r_i_SetViewProjectionMatrix(NULL);
//
//            if(denom)
//            {
//                float frac = dist_a / denom;
//                vec3_t intersection = {};
//                vec3_t_fmadd(&intersection, &camera_pos, &vec3_t_c(mouse_vec.x, mouse_vec.y, mouse_vec.z), frac);
//
//                if(just_changed)
//                {
//                    context_data->box_start = intersection;
//                }
//
//                context_data->box_end = intersection;
//                vec3_t start = context_data->box_start;
//                vec3_t end = context_data->box_end;
//
//                r_i_DrawLine(&start, &vec3_t_c(start.x, start.y, end.z), &vec4_t_c(0.0, 1.0, 0.0, 1.0), 2.0);
//                r_i_DrawLine(&vec3_t_c(start.x, start.y, end.z), &end, &vec4_t_c(0.0, 1.0, 0.0, 1.0), 2.0);
//                r_i_DrawLine(&end, &vec3_t_c(end.x, start.y, start.z), &vec4_t_c(0.0, 1.0, 0.0, 1.0), 2.0);
//                r_i_DrawLine(&vec3_t_c(end.x, start.y, start.z), &start, &vec4_t_c(0.0, 1.0, 0.0, 1.0), 2.0);
//            }
//        }
//    }
//    else
//    {
//        if(just_changed)
//        {
//            ed_SetContextState(context, ED_WORLD_CONTEXT_STATE_IDLE);
//        }
//
//        ed_SetContextState(context, ED_WORLD_CONTEXT_STATE_CREATE_BRUSH);
//    }
//}
//
//void ed_WorldContextCreateBrush(struct ed_context_t *context, uint32_t just_changed)
//{
//    struct ed_world_context_data_t *context_data = (struct ed_world_context_data_t *)context->context_data;
//    vec3_t position;
//    vec3_t size;
//    mat3_t orientation;
//
//    mat3_t_identity(&orientation);
//    vec3_t_sub(&size, &context_data->box_end, &context_data->box_start);
//    vec3_t_add(&position, &context_data->box_start, &context_data->box_end);
//    vec3_t_mul(&position, &position, 0.5);
//
//    size.y = 1.0;
//
//    ed_CreateBrushPickable(&position, &orientation, &size);
//    ed_SetContextState(context, ED_WORLD_CONTEXT_STATE_IDLE);
//}
//
//void ed_WorldContextProcessSelection(struct ed_context_t *context, uint32_t just_changed)
//{
//    struct ed_world_context_data_t *context_data = (struct ed_world_context_data_t *)context->context_data;
//    uint32_t mouse_state = in_GetMouseButtonState(SDL_BUTTON_LEFT);
//    uint32_t shift_state = in_GetKeyState(SDL_SCANCODE_LSHIFT);
//
//    if(context_data->last_selected->type == ED_PICKABLE_TYPE_MANIPULATOR)
//    {
//
//    }
//    else
//    {
//        if(mouse_state & IN_KEY_STATE_JUST_RELEASED)
//        {
//            int32_t mouse_x;
//            int32_t mouse_y;
//            in_GetMousePos(&mouse_x, &mouse_y);
//            struct ed_pickable_t *selection = ed_SelectPickable(mouse_x, mouse_y);
//
//            if(selection == context_data->last_selected)
//            {
//                ed_AddSelection(selection, shift_state & IN_KEY_STATE_PRESSED);
//            }
//
//            context_data->last_selected = NULL;
//            ed_SetContextState(context, ED_WORLD_CONTEXT_STATE_IDLE);
//        }
//    }
//}
//
//void ed_DrawGrid()
//{
//    r_i_SetModelMatrix(NULL);
//    r_i_SetViewProjectionMatrix(NULL);
//    r_i_SetBlending(GL_TRUE, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//    r_i_SetStencil(GL_FALSE, GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE);
//    r_i_SetRasterizer(GL_FALSE, GL_BACK, GL_FILL);
//    r_i_SetShader(ed_center_grid_shader);
//    r_i_DrawVerts(R_I_DRAW_CMD_TRIANGLE_LIST, ed_grid, 1.0);
//    r_i_SetShader(NULL);
//    r_i_SetBlending(GL_TRUE, GL_ONE, GL_ZERO);
//    r_i_DrawLine(&vec3_t_c(-10000.0, 0.0, 0.0), &vec3_t_c(10000.0, 0.0, 0.0), &vec4_t_c(1.0, 0.0, 0.0, 1.0), 3.0);
//    r_i_DrawLine(&vec3_t_c(0.0, 0.0, -10000.0), &vec3_t_c(0.0, 0.0, 10000.0), &vec4_t_c(0.0, 0.0, 1.0, 1.0), 3.0);
//}
//
//void ed_DrawBrushes()
//{
//    for(uint32_t brush_index = 0; brush_index < ed_world_context_data.brushes.cursor; brush_index++)
//    {
//        struct ed_brush_t *brush = ed_GetBrush(brush_index);
//
//        if(brush)
//        {
//            mat4_t transform;
//            mat4_t_identity(&transform);
//            mat4_t_comp(&transform, &brush->orientation, &brush->position);
//            r_DrawEntity(&transform, brush->model);
//        }
//    }
//}
//
//void ed_DrawLights()
//{
//    r_i_SetModelMatrix(NULL);
//    r_i_SetViewProjectionMatrix(NULL);
//
//
//    for(uint32_t light_index = 0; light_index < r_lights.cursor; light_index++)
//    {
//        struct r_light_t *light = r_GetLight(light_index);
//
//        if(light)
//        {
//            vec3_t position = vec3_t_c(light->data.pos_rad.x, light->data.pos_rad.y, light->data.pos_rad.z);
//            vec4_t color = vec4_t_c(light->data.color_res.x, light->data.color_res.y, light->data.color_res.z, 1.0);
//            r_i_DrawPoint(&position, &color, 8.0);
//        }
//    }
//}
//
//void ed_DrawSelections()
//{
//    if(ed_world_context_data.selections.cursor)
//    {
//        r_i_SetViewProjectionMatrix(NULL);
//        r_i_SetShader(ed_outline_shader);
//        r_i_SetBuffers(NULL, NULL);
//        r_i_SetRasterizer(GL_TRUE, GL_FRONT, GL_LINE);
//
//        uint32_t selection_count = ed_world_context_data.selections.cursor - 1;
//        uint32_t selection_index = 0;
//
//        for(uint32_t index = 0; index < 2; index++)
//        {
//            if(!index && selection_count)
//            {
//                r_i_SetUniform(r_GetNamedUniform(ed_outline_shader, "ed_color"), 1, &vec4_t_c(1.0, 0.2, 0.0, 1.0));
//            }
//            else
//            {
//                r_i_SetUniform(r_GetNamedUniform(ed_outline_shader, "ed_color"), 1, &vec4_t_c(1.0, 0.4, 0.0, 1.0));
//            }
//
//            for(; selection_index < selection_count; selection_index++)
//            {
//                uint32_t pickable_index = *(uint32_t *)ds_list_get_element(&ed_world_context_data.selections, selection_index);
//                struct ed_pickable_t *pickable = ed_GetPickable(pickable_index);
//
//                struct r_i_draw_list_t *draw_list = r_i_AllocDrawList(1);
//                draw_list->commands[0].start = pickable->start;
//                draw_list->commands[0].count = pickable->count;
//                draw_list->size = 4.0;
//                draw_list->indexed = 1;
//
//                r_i_SetModelMatrix(&pickable->transform);
//
////                r_i_SetDrawMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE, 0xff);
////                r_i_SetDepth(GL_TRUE, GL_ALWAYS);
////                r_i_SetStencil(GL_TRUE, GL_KEEP, GL_KEEP, GL_REPLACE, GL_ALWAYS, 0xff, 0xff);
////                r_i_SetRasterizer(GL_TRUE, GL_FRONT, GL_FILL);
////                r_i_DrawImmediate(R_I_DRAW_CMD_TRIANGLE_LIST, draw_list);
////
////
////                draw_list = r_i_AllocDrawList(1);
////                draw_list->commands[0].start = pickable->start;
////                draw_list->commands[0].count = pickable->count;
////                draw_list->size = 4.0;
////                draw_list->indexed = 1;
//
////                r_i_SetDrawMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE, 0xff);
////                r_i_SetDepth(GL_TRUE, GL_LESS);
////                r_i_SetStencil(GL_TRUE, GL_KEEP, GL_KEEP, GL_REPLACE, GL_EQUAL, 0xff, 0x00);
////                r_i_SetRasterizer(GL_TRUE, GL_FRONT, GL_LINE);
//                r_i_DrawImmediate(R_I_DRAW_CMD_TRIANGLE_LIST, draw_list);
//
//
////                draw_list = r_i_AllocDrawList(1);
////                draw_list->commands[0].start = pickable->start;
////                draw_list->commands[0].count = pickable->count;
////                draw_list->size = 4.0;
////                draw_list->indexed = 1;
////
////                r_i_SetDrawMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE, 0xff);
////                r_i_SetDepth(GL_TRUE, GL_LESS);
////                r_i_SetStencil(GL_TRUE, GL_KEEP, GL_KEEP, GL_REPLACE, GL_EQUAL, 0xff, 0x00);
////                r_i_SetRasterizer(GL_TRUE, GL_FRONT, GL_LINE);
////                r_i_DrawImmediate(R_I_DRAW_CMD_TRIANGLE_LIST, draw_list);
//
//            }
//
//            selection_count++;
//        }
//
//        r_i_SetRasterizer(GL_TRUE, GL_BACK, GL_FILL);
//        r_i_SetDepth(GL_TRUE, GL_LESS);
//        r_i_SetStencil(GL_FALSE, GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE);
//    }
//}













