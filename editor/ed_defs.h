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
    ED_EDITOR_LAST
};

struct ed_context_t
{
    void (*update)();
    void (*current_state)(struct ed_context_t *context, uint32_t just_changed);
    void (*next_state)(struct ed_context_t *context, uint32_t just_changed);
    void *context_data;
};

struct ed_editor_t
{
    uint32_t index;
    void (*init)();
    void (*suspend)();
    void (*resume)();
    void (*update)();

    void (*current_state)(uint32_t just_changed);
    void (*next_state)(uint32_t just_changed);
};





#endif // ED_COM_H
