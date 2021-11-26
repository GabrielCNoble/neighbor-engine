#include "ed_main.h"
#include "ed_bsp.h"
#include "ed_level.h"
#include "ed_ent.h"
#include "ed_proj.h"
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
//struct ed_context_t ed_contexts[ED_CONTEXT_LAST];
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
//uint32_t ed_picking_framebuffer;
//uint32_t ed_picking_depth_texture;
//uint32_t ed_picking_object_texture;
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
    char path_buffer[PATH_MAX];
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
//    SDL_DisplayMode desktop_display_mode;

//    ed_editors = ds_slist_create(sizeof(struct ed_editor_t), 8);

//    SDL_GetDesktopDisplayMode(0, &desktop_display_mode);

    ed_PickingInit();

    ed_editors[ED_EDITOR_LEVEL] = (struct ed_editor_t ){
        .init = ed_l_Init,
        .shutdown = ed_l_Shutdown,
        .update = ed_l_Update,
        .suspend = ed_l_Suspend,
        .resume = ed_l_Resume,
        .explorer_load = ed_l_LoadFile,
        .explorer_save = ed_l_SaveLevel,
        .explorer_new = ed_l_ResetEditor
    };

    ed_editors[ED_EDITOR_ENTITY] = (struct ed_editor_t){
        .init = ed_e_Init,
        .shutdown = ed_e_Shutdown,
        .update = ed_e_Update,
        .suspend = ed_e_Suspend,
        .resume = ed_e_Resume,
        .explorer_new = ed_e_ResetEditor
    };

    ed_editors[ED_EDITOR_PROJ] = (struct ed_editor_t){
        .init = ed_ProjEditorInit,
        .shutdown = ed_ProjEditorShutdown,
        .update = ed_ProjEditorUpdate,
        .suspend = ed_ProjEditorSuspend,
        .resume = ed_ProjEditorResume,
    };

//    ed_active_editor = ed_editors + ED_EDITOR_LEVEL;

    for(uint32_t editor_index = ED_EDITOR_LEVEL; editor_index < ED_EDITOR_LAST; editor_index++)
    {
        struct ed_editor_t *editor = ed_editors + editor_index;
        editor->init(editor);
    }

    ed_explorer_state.current_file[0] = '\0';
    ed_explorer_state.current_path[0] = '\0';
    ed_explorer_state.open = 0;
    ed_explorer_state.dir_entries = ds_list_create(sizeof(struct ds_dir_entry_t), 64);
    ed_explorer_state.matched_dir_entries = ds_list_create(sizeof(struct ds_dir_entry_t *), 64);
    ed_explorer_state.ext_filters = ds_list_create(sizeof(struct ed_explorer_ext_filter_t), 64);
    ed_explorer_state.drives = ds_list_create(sizeof(struct ed_explorer_drive_t), 8);

//    strcpy(ed_explorer_state.current_path, "C:\\Users\\gabri\\Documents");

    ed_ChangeExplorerPath("C:/Users/gabri/Documents");
    ed_EnumerateExplorerDrives();
    ed_SetExplorerLoadCallback(test_load_callback);
    ed_SetExplorerSaveCallback(test_save_callback);

    in_SetMouseRelative(0);

    ed_SwitchToEditor(ed_editors + ED_EDITOR_LEVEL);
}

void ed_Shutdown()
{
    for(uint32_t editor_index = ED_EDITOR_LEVEL; editor_index < ED_EDITOR_LAST; editor_index++)
    {
        ed_editors[editor_index].shutdown();
    }
}

void ed_Quit()
{
    g_SetGameState(G_GAME_STATE_QUIT);
}

void ed_SwitchToEditor(struct ed_editor_t *editor)
{
    if(editor != ed_active_editor)
    {
        if(ed_active_editor)
        {
            ed_active_editor->suspend();
        }

        ed_active_editor = editor;
        ed_active_editor->resume();
    }
}

void ed_UpdateEditor()
{
    uint32_t just_changed = ed_active_editor->current_state != ed_active_editor->next_state;
    ed_active_editor->current_state = ed_active_editor->next_state;
    if(ed_active_editor->current_state)
    {
        ed_active_editor->current_state(just_changed);
    }
    ed_active_editor->update();

//    if(ed_active_editor == &ed_editors[ED_EDITOR_START])
//    {
//        return;
//    }

    if(igBeginMainMenuBar())
    {
        if(igBeginMenu("File", 1))
        {
            if(igMenuItem_Bool("New", NULL, 0, 1))
            {
                if(ed_active_editor->explorer_new)
                {
                    ed_active_editor->explorer_new();
                }
            }

            if(igMenuItem_Bool("Save", NULL, 0, 1))
            {
                if(ed_active_editor->explorer_save)
                {
                    ed_SetExplorerSaveCallback(ed_active_editor->explorer_save);
                    ed_OpenExplorer(ed_explorer_state.current_path, ED_EDITOR_EXPLORER_MODE_SAVE);
                }
            }

//            if(igMenuItem_Bool("Save as...", NULL, 0, 1))
//            {
//                if(ed_active_editor->explorer_save)
//                {
//                    ed_SetExplorerSaveCallback(ed_active_editor->explorer_save);
//                    ed_OpenExplorer(ed_explorer_state.current_path, ED_EDITOR_EXPLORER_MODE_SAVE);
//                }
//            }

            if(igMenuItem_Bool("Load", NULL, 0, 1))
            {
                if(ed_active_editor->explorer_load)
                {
                    ed_SetExplorerLoadCallback(ed_active_editor->explorer_load);
                    ed_OpenExplorer(ed_explorer_state.current_path, ED_EDITOR_EXPLORER_MODE_OPEN);
                }
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

        if(igBeginMenu("Editors", 1))
        {
            if(igMenuItem_Bool("Level", NULL, 0, 1))
            {
                ed_SwitchToEditor(ed_editors + ED_EDITOR_LEVEL);
            }

            if(igMenuItem_Bool("Entity", NULL, 0, 1))
            {
                ed_SwitchToEditor(ed_editors + ED_EDITOR_ENTITY);
            }

            igEndMenu();
        }

        if(igBeginMenu("Misc", 1))
        {
            if(igMenuItem_Bool("Renderer info", NULL, 0, 1))
            {
                ed_show_renderer_info_window ^= 1;
            }
//            if(igMenuItem_Bool("Gen world collider", NULL, 0, 1))
//            {
//                ed_BuildWorldGeometry();
//            }
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
                igCheckbox("Draw entity AABBs", &r_renderer_state.draw_entities);
                igCheckbox("Draw physics", &r_renderer_state.draw_colliders);
            }
            igEnd();
        }
    }

    ed_UpdateExplorer();
}

//void ed_SetNextContextState(struct ed_context_t *context, void (*state_fn)(struct ed_context_t *context, uint32_t just_changed))
//{
//    context->next_state = state_fn;
//}

void ed_SetNextState(void (*state_fn)(uint32_t just_changed))
{
    ed_active_editor->next_state = state_fn;
}

/*
=============================================================
=============================================================
=============================================================
*/

//void ed_InitProjectFolder(char *path, char *folder_name)
//{
//    char full_path[PATH_MAX];
//    ds_path_append_end(path, folder_name, full_path, PATH_MAX);
//}

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
                    igInputText("##path", ed_explorer_state.path_buffer, PATH_MAX, 0, NULL, NULL);
                    if(igIsItemDeactivatedAfterEdit())
                    {
                        ed_ChangeExplorerPath(ed_explorer_state.path_buffer);
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
                                strcpy(ed_explorer_state.current_file, entry->name);

                                if(igIsMouseDoubleClicked(ImGuiMouseButton_Left))
                                {
                                    if(entry->type == DS_DIR_ENTRY_TYPE_DIR)
                                    {
                                        ds_path_append_end(ed_explorer_state.current_path, entry->name, ed_explorer_state.current_path, PATH_MAX);
                                        ed_ChangeExplorerPath(ed_explorer_state.current_path);
                                    }
                                    else
                                    {
                                        ed_explorer_state.load_callback(ed_explorer_state.current_path, ed_explorer_state.current_file);
                                        ed_CloseExplorer();
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
        strcpy(ed_explorer_state.path_buffer, dir.name);

        ds_dir_close_dir(&dir);

        ed_MatchExplorerEntries(ed_explorer_state.search_bar);

        if(!ed_explorer_state.matched_dir_entries.cursor)
        {
            /* we just changed dirs, and there's no matching entry in the new dir, so we'll clear the
            search bar to at least show something */
            ed_MatchExplorerEntries("");
        }

        return 1;
    }
    else
    {
        strcpy(ed_explorer_state.path_buffer, ed_explorer_state.current_path);
    }

    return 0;
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

void ed_ExplorerSave()
{

}

void ed_ExplorerLoad()
{

}












