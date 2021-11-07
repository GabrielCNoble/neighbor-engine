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

struct ed_context_t
{
    void (*update)();
    void (*current_state)(struct ed_context_t *context, uint32_t just_changed);
    void (*next_state)(struct ed_context_t *context, uint32_t just_changed);
//    struct ed_state_t *states;
    void *context_data;
};





#endif // ED_COM_H
