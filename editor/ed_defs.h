#ifndef ED_COM_H
#define ED_COM_H

#include <stdint.h>
#include "../lib/dstuff/ds_buffer.h"
#include "../lib/dstuff/ds_vector.h"
#include "../lib/dstuff/ds_matrix.h"
#include "../engine/r_defs.h"

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

enum ED_EXPLORER_FLAGS
{
    ED_EXPLORER_FLAG_LOAD = 1,
    ED_EXPLORER_FLAG_SAVE = 1 << 1,
    ED_EXPLORER_FLAG_SELECT_FOLDER = 1 << 2,
};

struct ed_explorer_state_t
{
    uint32_t open;
    uint32_t mode;
    char path_buffer[PATH_MAX];
    char current_path[PATH_MAX];
    char current_file[PATH_MAX];
    char search_bar[PATH_MAX];
    struct ds_dir_entry_t *selected_entry;
    struct ds_list_t dir_entries;
    struct ds_list_t matched_dir_entries;
    struct ds_list_t ext_filters;
    struct ds_list_t drives;

    uint32_t (*load_callback)(char *path, char *file);
    uint32_t (*save_callback)(char *path, char *file);
};

struct ed_context_t;

struct ed_state_t
{
    void (*update)(struct ed_context_t *context, uint32_t just_changed);
};

enum ED_CONTEXTS
{
    ED_CONTEXT_WORLD = 0,
    ED_CONTEXT_LAST,
};

enum ED_EDITORS
{
    ED_EDITOR_LEVEL,
    ED_EDITOR_ENTITY,
    ED_EDITOR_PROJ,
    ED_EDITOR_LAST
};

struct ed_editor_t
{
    uint32_t index;
    void (*init)(struct ed_editor_t *editor);
    void (*shutdown)();
    void (*suspend)();
    void (*resume)();
    void (*update)();
    void (*open_explorer_save)(struct ed_explorer_state_t *explorer_state);
    void (*open_explorer_load)(struct ed_explorer_state_t *explorer_state);
    uint32_t (*explorer_load)(char *path, char *file);
    uint32_t (*explorer_save)(char *path, char *file);
    void (*explorer_new)();

    void (*current_state)(uint32_t just_changed);
    void (*next_state)(uint32_t just_changed);
};




#endif // ED_COM_H
