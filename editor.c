#include "editor.h"
#include "ed_bsp.h"
#include "dstuff/ds_slist.h"
#include "dstuff/ds_list.h"
#include "dstuff/ds_mem.h"
#include "dstuff/ds_vector.h"
#include "dstuff/ds_matrix.h"
#include "dstuff/ds_path.h"
#include "dstuff/ds_dir.h"
#include "game.h"
#include "input.h"
#include "gui.h"
#include "r_draw.h"
#include <limits.h>
#include <string.h>
#include <float.h>

struct ds_slist_t ed_brushes;
struct ed_context_t *ed_active_context;
struct ed_context_t ed_contexts[ED_CONTEXT_LAST];
//uint32_t ed_grid_vert_count;
//struct r_vert_t *ed_grid;
struct r_i_verts_t *ed_grid;
struct ds_slist_t ed_polygons;
struct ds_slist_t ed_bsp_nodes;

struct r_shader_t *ed_center_grid_shader;
struct r_shader_t *ed_picking_shader;
uint32_t ed_picking_shader_type_uniform;
uint32_t ed_picking_shader_index_uniform;

uint32_t ed_cube_brush_indices[][4] =
{
    /* -Z */
    {0, 1, 2, 3},
    /* +Z */
    {4, 5, 6, 7},
    /* -X */
    {0, 3, 5, 4},
    /* +X */
    {7, 6, 2, 1},
    /* -Y */
    {5, 3, 2, 6},
    /* +Y */
    {0, 4, 7, 1}
};

vec3_t ed_cube_brush_vertices[] =
{
    vec3_t_c(-0.5, 0.5, -0.5),
    vec3_t_c(0.5, 0.5, -0.5),
    vec3_t_c(0.5, -0.5, -0.5),
    vec3_t_c(-0.5, -0.5, -0.5),

    vec3_t_c(-0.5, 0.5, 0.5),
    vec3_t_c(-0.5, -0.5, 0.5),
    vec3_t_c(0.5, -0.5, 0.5),
    vec3_t_c(0.5, 0.5, 0.5),
};

vec3_t ed_cube_brush_normals[] =
{
    vec3_t_c(0.0, 0.0, -1.0),
    vec3_t_c(0.0, 0.0, 1.0),
    vec3_t_c(-1.0, 0.0, 0.0),
    vec3_t_c(1.0, 0.0, 0.0),
    vec3_t_c(0.0, -1.0, 0.0),
    vec3_t_c(0.0, 1.0, 0.0)
};

vec3_t ed_cube_brush_tangents[] =
{
    vec3_t_c(1.0, 0.0, 0.0),
    vec3_t_c(-1.0, 0.0, 0.0),
    vec3_t_c(0.0, 0.0, 1.0),
    vec3_t_c(0.0, 0.0, -1.0),
    vec3_t_c(1.0, 0.0, 0.0),
    vec3_t_c(-1.0, 0.0, 0.0),
};

float ed_camera_pitch;
float ed_camera_yaw;
vec3_t ed_camera_pos;
uint32_t ed_picking_framebuffer;
uint32_t ed_picking_depth_texture;
uint32_t ed_picking_object_texture;
uint32_t ed_show_renderer_info_window ;

extern uint32_t g_game_state;
extern mat4_t r_camera_matrix;
extern float r_z_near;
extern float r_fov;
extern uint32_t r_width;
extern uint32_t r_height;
extern mat4_t r_view_projection_matrix;
extern uint32_t r_vertex_buffer;
extern uint32_t r_index_buffer;
extern struct ds_slist_t r_lights;
extern uint32_t r_prev_draw_call_count;

extern struct r_renderer_stats_t r_renderer_stats;
extern struct r_renderer_state_t r_renderer_state;


#define ED_GRID_DIVS 301
#define ED_GRID_QUAD_SIZE 500.0

struct ed_state_t ed_world_context_states[] =
{
    [ED_WORLD_CONTEXT_STATE_IDLE] = ed_WorldContextIdleState,
    [ED_WORLD_CONTEXT_STATE_LEFT_CLICK] = ed_WorldContextLeftClickState,
    [ED_WORLD_CONTEXT_STATE_BRUSH_BOX] = ed_WorldContextStateBrushBox,
    [ED_WORLD_CONTEXT_STATE_CREATE_BRUSH] = ed_WorldContextCreateBrush
};

struct ed_world_context_data_t ed_world_context_data;

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
    ed_brushes = ds_slist_create(sizeof(struct ed_brush_t), 512);
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

    ed_center_grid_shader = r_LoadShader("shaders/ed_grid.vert", "shaders/ed_grid.frag");
    ed_picking_shader = r_LoadShader("shaders/ed_pick.vert", "shaders/ed_pick.frag");
    ed_picking_shader_type_uniform = r_GetUniformIndex(ed_picking_shader, "ed_type");
    ed_picking_shader_index_uniform = r_GetUniformIndex(ed_picking_shader, "ed_index");

    SDL_DisplayMode desktop_display_mode;

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
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, r_width, r_height, 0, GL_RG, GL_FLOAT, NULL);

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

    ed_contexts[ED_CONTEXT_WORLD].update = ed_WorldContextUpdate;
    ed_contexts[ED_CONTEXT_WORLD].states = ed_world_context_states;
    ed_contexts[ED_CONTEXT_WORLD].current_state = ED_WORLD_CONTEXT_STATE_IDLE;
    ed_contexts[ED_CONTEXT_WORLD].context_data = &ed_world_context_data;
    ed_world_context_data.selections = ds_list_create(sizeof(struct ed_selection_t), 512);
    ed_active_context = ed_contexts + ED_CONTEXT_WORLD;

    ed_polygons = ds_slist_create(sizeof(struct ed_polygon_t ), 1024);
    ed_bsp_nodes = ds_slist_create(sizeof(struct ed_bspn_t), 1024);

    ed_explorer_state.current_file[0] = '\0';
    ed_explorer_state.current_path[0] = '\0';
    ed_explorer_state.open = 0;
    ed_explorer_state.dir_entries = ds_list_create(sizeof(struct ds_dir_entry_t), 64);
    ed_explorer_state.matched_dir_entries = ds_list_create(sizeof(struct ds_dir_entry_t *), 64);
    ed_explorer_state.ext_filters = ds_list_create(sizeof(struct ed_explorer_ext_filter_t), 64);
    ed_explorer_state.drives = ds_list_create(sizeof(struct ed_explorer_drive_t), 8);

    ed_EnumerateExplorerDrives();
    ed_SetExplorerLoadCallback(test_load_callback);
    ed_SetExplorerSaveCallback(test_save_callback);
}

void ed_Shutdown()
{

}

void ed_UpdateEditor()
{
    if(in_GetKeyState(SDL_SCANCODE_P) & IN_KEY_STATE_JUST_PRESSED)
    {
        g_SetGameState(G_GAME_STATE_PLAYING);
    }
    else
    {
        for(uint32_t context_index = 0; context_index < ED_CONTEXT_LAST; context_index++)
        {
            struct ed_context_t *context = ed_contexts + context_index;
            context->update();

            if(context == ed_active_context)
            {
                uint32_t just_changed = context->current_state != context->next_state;
                context->current_state = context->next_state;
                context->states[context->current_state].update(context, just_changed);
            }
        }
    }

    if(igBeginMainMenuBar())
    {
        if(igBeginMenu("File", 1))
        {
            if(igMenuItem_Bool("New", NULL, 0, 1))
            {

            }

            if(igMenuItem_Bool("Save", NULL, 0, 1))
            {

            }

            if(igMenuItem_Bool("Save as...", NULL, 0, 1))
            {
                ed_OpenExplorer("C:/Users/gabri/Documents/Compiler/metroid_thingy", ED_EDITOR_EXPLORER_MODE_SAVE);
            }

            if(igMenuItem_Bool("Load", NULL, 0, 1))
            {
                ed_OpenExplorer("C:/Users/gabri/Documents/Compiler/metroid_thingy", ED_EDITOR_EXPLORER_MODE_OPEN);
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
            igEndMenu();
        }
        igEndMainMenuBar();

        if(ed_show_renderer_info_window)
        {
            if(igBegin("Renderer info", NULL, 0))
            {
                igText("Draw calls: %d", r_renderer_stats.draw_call_count);
                igText("Shader swaps: %d", r_renderer_stats.shader_swaps);
                igText("Material swaps: %d", r_renderer_stats.material_swaps);
                igText("Z prepass: %s", r_renderer_state.use_z_prepass ? "Enabled" : "Disabled");
            }
            igEnd();
        }

//        igSameLine(0, -1);
//        igText("Draw calls: %d", r_renderer_stats.draw_call_count);
//        igText("Shader swaps: %d", r_renderer_stats.shader_swaps);
//        igText("Material swaps: %d", r_renderer_stats.material_swaps);
    }

    ed_UpdateExplorer();
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

void ed_FlyCamera()
{
    float dx;
    float dy;

    in_GetMouseDelta(&dx, &dy);

    ed_camera_pitch += dy;
    ed_camera_yaw -= dx;

    if(ed_camera_pitch > 0.5)
    {
        ed_camera_pitch = 0.5;
    }
    else if(ed_camera_pitch < -0.5)
    {
        ed_camera_pitch = -0.5;
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
    vec3_t_add(&ed_camera_pos, &ed_camera_pos, &vec3_t_c(translation.x, translation.y, translation.z));
}


void ed_SetContextState(struct ed_context_t *context, uint32_t state)
{
    context->next_state = state;
}

void ed_WorldContextUpdate()
{
    r_SetViewPos(&ed_camera_pos);
    r_SetViewPitchYaw(ed_camera_pitch, ed_camera_yaw);
    ed_DrawGrid();
    ed_DrawBrushes();
    ed_DrawLights();

    int32_t mouse_x;
    int32_t mouse_y;
    struct ed_selection_t selection = {};

    in_GetMousePos(&mouse_x, &mouse_y);
    if(ed_PickObject(mouse_x, mouse_y, &selection))
    {
        struct ed_brush_t *brush = ed_GetBrush(selection.selection.index);
        mat4_t model_matrix;
        mat4_t_comp(&model_matrix, &brush->orientation, &brush->position);
        r_i_SetModelMatrix(&model_matrix);
        r_i_SetViewProjectionMatrix(NULL);
        uint32_t verts_size = sizeof(struct r_i_verts_t) + sizeof(struct r_vert_t) * brush->model->verts.buffer_size;
        uint32_t indices_size = sizeof(struct r_i_indices_t) + sizeof(uint32_t) * brush->model->indices.buffer_size;

        struct r_i_verts_t *verts = r_i_AllocImmediateData(verts_size);
        struct r_i_indices_t *indices = r_i_AllocImmediateData(indices_size);

        verts->count = brush->model->verts.buffer_size;
        indices->count = brush->model->indices.buffer_size;

        for(uint32_t vert_index = 0; vert_index < verts->count; vert_index++)
        {
            struct r_vert_t *out_vert = verts->verts + vert_index;
            struct r_vert_t *in_vert = (struct r_vert_t *)brush->model->verts.buffer + vert_index;
            *out_vert = *in_vert;
            out_vert->normal = vec4_t_c(1.0, 0.0, 0.0, 0.2);
        }

        memcpy(indices->indices, brush->model->indices.buffer, indices->count * sizeof(uint32_t));
        r_i_SetBlending(GL_TRUE, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        r_i_SetDepth(GL_TRUE, GL_LEQUAL);
        r_i_DrawVertsIndexed(R_I_DRAW_CMD_TRIANGLE_LIST, verts, indices);
    }
}

void ed_WorldContextIdleState(struct ed_context_t *context, uint32_t just_changed)
{
    if(in_GetMouseButtonState(SDL_BUTTON_RIGHT) & IN_KEY_STATE_PRESSED)
    {
        ed_FlyCamera();
    }
    else
    {
        if(in_GetMouseButtonState(SDL_BUTTON_LEFT) & IN_KEY_STATE_JUST_PRESSED)
        {
            ed_SetContextState(context, ED_WORLD_CONTEXT_STATE_LEFT_CLICK);
        }
    }
}

void ed_WorldContextLeftClickState(struct ed_context_t *context, uint32_t just_changed)
{
    uint32_t left_button_state = in_GetMouseButtonState(SDL_BUTTON_LEFT);
    float dx = 0;
    float dy = 0;
//    int32_t mouse_x;
//    int32_t mouse_y;

    in_GetMouseDelta(&dx, &dy);
//    in_GetMousePos(&mouse_x, &mouse_y);
//    printf("%d %d\n", mouse_x, mouse_y);

    if(left_button_state & IN_KEY_STATE_PRESSED)
    {
//        if(just_changed)
//        {
//            ed_PickObject(mouse_x, mouse_y);
//        }

        if(dx || dy)
        {
            ed_SetContextState(context, ED_WORLD_CONTEXT_STATE_BRUSH_BOX);
        }
    }
    else
    {
        ed_SetContextState(context, ED_WORLD_CONTEXT_STATE_IDLE);
    }
}

void ed_WorldContextStateBrushBox(struct ed_context_t *context, uint32_t just_changed)
{
    struct ed_world_context_data_t *context_data = (struct ed_world_context_data_t *)context->context_data;

    if(in_GetMouseButtonState(SDL_BUTTON_LEFT) & IN_KEY_STATE_PRESSED)
    {
        if(in_GetKeyState(SDL_SCANCODE_ESCAPE) & IN_KEY_STATE_PRESSED)
        {
            ed_SetContextState(context, ED_WORLD_CONTEXT_STATE_IDLE);
        }
        else
        {
            vec3_t mouse_pos;
            vec3_t camera_pos;
            vec4_t mouse_vec = {};

            float aspect = (float)r_width / (float)r_height;
            float top = tan(r_fov) * r_z_near;
            float right = top * aspect;

            in_GetNormalizedMousePos(&mouse_vec.x, &mouse_vec.y);
            mouse_vec.x *= right;
            mouse_vec.y *= top;
            mouse_vec.z = -r_z_near;
            vec4_t_normalize(&mouse_vec, &mouse_vec);
            mat4_t_vec4_t_mul_fast(&mouse_vec, &r_camera_matrix, &mouse_vec);

            camera_pos.x = r_camera_matrix.rows[3].x;
            camera_pos.y = r_camera_matrix.rows[3].y;
            camera_pos.z = r_camera_matrix.rows[3].z;

            mouse_pos.x = camera_pos.x + mouse_vec.x;
            mouse_pos.y = camera_pos.y + mouse_vec.y;
            mouse_pos.z = camera_pos.z + mouse_vec.z;

            float dist_a = camera_pos.y;
            float dist_b = mouse_pos.y;
            float denom = (dist_a - dist_b);

            r_i_SetModelMatrix(NULL);
            r_i_SetViewProjectionMatrix(NULL);

            if(denom)
            {
                float frac = dist_a / denom;
                vec3_t intersection = {};
                vec3_t_fmadd(&intersection, &camera_pos, &vec3_t_c(mouse_vec.x, mouse_vec.y, mouse_vec.z), frac);

                if(just_changed)
                {
                    context_data->box_start = intersection;
                }

                context_data->box_end = intersection;
                vec3_t start = context_data->box_start;
                vec3_t end = context_data->box_end;

                r_i_DrawLine(&start, &vec3_t_c(start.x, start.y, end.z), &vec4_t_c(0.0, 1.0, 0.0, 1.0), 2.0);
                r_i_DrawLine(&vec3_t_c(start.x, start.y, end.z), &end, &vec4_t_c(0.0, 1.0, 0.0, 1.0), 2.0);
                r_i_DrawLine(&end, &vec3_t_c(end.x, start.y, start.z), &vec4_t_c(0.0, 1.0, 0.0, 1.0), 2.0);
                r_i_DrawLine(&vec3_t_c(end.x, start.y, start.z), &start, &vec4_t_c(0.0, 1.0, 0.0, 1.0), 2.0);
            }
        }
    }
    else
    {
        if(just_changed)
        {
            ed_SetContextState(context, ED_WORLD_CONTEXT_STATE_IDLE);
        }

        ed_SetContextState(context, ED_WORLD_CONTEXT_STATE_CREATE_BRUSH);
    }
}

void ed_WorldContextCreateBrush(struct ed_context_t *context, uint32_t just_changed)
{
    struct ed_world_context_data_t *context_data = (struct ed_world_context_data_t *)context->context_data;
    vec3_t position;
    vec3_t size;
    mat3_t orientation;

    mat3_t_identity(&orientation);
    vec3_t_sub(&size, &context_data->box_end, &context_data->box_start);
    vec3_t_add(&position, &context_data->box_start, &context_data->box_end);
    vec3_t_mul(&position, &position, 0.5);

    size.y = 1.0;

    ed_CreateBrush(&position, &orientation, &size);

    ed_SetContextState(context, ED_WORLD_CONTEXT_STATE_IDLE);
}

uint32_t ed_PickObject(int32_t mouse_x, int32_t mouse_y, struct ed_selection_t *selection)
{
//    mouse_y = r_height - (mouse_y + 1);
//    mat4_t model_view_projection_matrix;
//    glBindBuffer(GL_ARRAY_BUFFER, r_vertex_buffer);
//    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, r_index_buffer);
//
//    r_BindShader(ed_picking_shader);
//    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, ed_picking_framebuffer);
//    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//    glUniform1i(ed_picking_shader_type_uniform, ED_SELECTION_TYPE_BRUSH + 1);
//
//    for(uint32_t brush_index = 0; brush_index < ed_brushes.cursor; brush_index++)
//    {
//        struct ed_brush_t *brush = ed_GetBrush(brush_index);
//
//        if(brush)
//        {
//            mat4_t model_matrix;
//            mat4_t_comp(&model_matrix, &brush->orientation, &brush->position);
//            mat4_t_mul(&model_view_projection_matrix, &model_matrix, &r_view_projection_matrix);
//            r_SetUniformMatrix4(R_UNIFORM_MODEL_VIEW_PROJECTION_MATRIX, &model_view_projection_matrix);
//            glUniform1i(ed_picking_shader_index_uniform, brush_index);
//            struct r_model_t *model = brush->model;
//            for(uint32_t batch_index = 0; batch_index < model->batches.buffer_size; batch_index++)
//            {
//                struct r_batch_t *batch = (struct r_batch_t *)model->batches.buffer + batch_index;
//                glDrawElements(GL_TRIANGLES, batch->count, GL_UNSIGNED_INT, (void *)(batch->start * sizeof(uint32_t)));
//            }
//        }
//    }
//
//    glBindFramebuffer(GL_READ_FRAMEBUFFER, ed_picking_framebuffer);
//    int32_t pick_values[2];
//    glReadPixels(mouse_x, mouse_y, 1, 1, GL_RG, GL_FLOAT, pick_values);
//
//    if(pick_values[0])
//    {
//        selection->type = pick_values[0] - 1;
//        selection->selection.index = pick_values[1];
//        return 1;
//    }

    return 0;
}

void ed_DrawGrid()
{
    r_i_SetModelMatrix(NULL);
    r_i_SetViewProjectionMatrix(NULL);
    r_i_SetBlending(GL_TRUE, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    r_i_SetCullFace(GL_FALSE, 0);
    r_i_SetShader(ed_center_grid_shader);
    r_i_DrawVerts(R_I_DRAW_CMD_TRIANGLE_LIST, ed_grid);
    r_i_SetShader(NULL);
    r_i_SetBlending(GL_TRUE, GL_ONE, GL_ZERO);
    r_i_DrawLine(&vec3_t_c(-10000.0, 0.0, 0.0), &vec3_t_c(10000.0, 0.0, 0.0), &vec4_t_c(1.0, 0.0, 0.0, 1.0), 3.0);
    r_i_DrawLine(&vec3_t_c(0.0, 0.0, -10000.0), &vec3_t_c(0.0, 0.0, 10000.0), &vec4_t_c(0.0, 0.0, 1.0, 1.0), 3.0);
}

void ed_DrawBrushes()
{
    for(uint32_t brush_index = 0; brush_index < ed_brushes.cursor; brush_index++)
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

void ed_DrawLights()
{
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
}

struct ed_brush_t *ed_CreateBrush(vec3_t *position, mat3_t *orientation, vec3_t *size)
{
    uint32_t index;
    struct ed_brush_t *brush;
    vec3_t dims;
    vec3_t_fabs(&dims, size);

    index = ds_slist_add_element(&ed_brushes, NULL);
    brush = ds_slist_get_element(&ed_brushes, index);
    brush->index = index;

    brush->vertices = ds_create_buffer(sizeof(vec3_t), 8);
    vec3_t *vertices = (vec3_t *)brush->vertices.buffer;

    for(uint32_t vert_index = 0; vert_index < brush->vertices.buffer_size; vert_index++)
    {
        vertices[vert_index].x = dims.x * ed_cube_brush_vertices[vert_index].x;
        vertices[vert_index].y = dims.y * ed_cube_brush_vertices[vert_index].y;
        vertices[vert_index].z = dims.z * ed_cube_brush_vertices[vert_index].z;
    }

    brush->faces = ds_list_create(sizeof(struct ed_face_t), 6);

    brush->orientation = *orientation;
    brush->position = *position;

    for(uint32_t face_index = 0; face_index < brush->faces.size; face_index++)
    {
        ds_list_add_element(&brush->faces, NULL);
        struct ed_face_t *face = ds_list_get_element(&brush->faces, face_index);

        face->material = r_GetDefaultMaterial();
        face->normal = ed_cube_brush_normals[face_index];
        face->tangent = ed_cube_brush_tangents[face_index];
        face->indices = ds_create_buffer(sizeof(uint32_t), 4);
        ds_fill_buffer(&face->indices, 0, ed_cube_brush_indices[face_index], 4);
    }

    brush->model = NULL;

    ed_UpdateBrush(brush);

    return brush;
}

struct ed_brush_t *ed_GetBrush(uint32_t index)
{
    struct ed_brush_t *brush = NULL;

    if(index != 0xffffffff)
    {
        brush = ds_slist_get_element(&ed_brushes, index);

        if(brush && brush->index == 0xffffffff)
        {
            brush = NULL;
        }
    }

    return brush;
}

void ed_UpdateBrush(struct ed_brush_t *brush)
{
    struct ed_polygon_t *polygons;
    struct r_model_geometry_t geometry = {};

    polygons = ed_PolygonsFromBrush(brush);
    brush->bsp = ed_BspFromPolygons(polygons);
    ed_GeometryFromBsp(&geometry, brush->bsp);

    if(!brush->model)
    {
        brush->model = r_CreateModel(&geometry, NULL);
    }
    else
    {
        r_UpdateModelGeometry(brush->model, &geometry);
    }
}

















