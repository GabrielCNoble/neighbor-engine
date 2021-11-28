#ifndef ED_COM_H
#define ED_COM_H

#include <stdint.h>
#include "../lib/dstuff/ds_buffer.h"
#include "../lib/dstuff/ds_vector.h"
#include "../lib/dstuff/ds_matrix.h"
#include "../engine/r_defs.h"

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
    uint32_t (*explorer_load)(char *path, char *file);
    uint32_t (*explorer_save)(char *path, char *file);
    void (*explorer_new)();

    void (*current_state)(uint32_t just_changed);
    void (*next_state)(uint32_t just_changed);
};





#endif // ED_COM_H
