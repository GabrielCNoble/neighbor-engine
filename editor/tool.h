#ifndef ED_TOOL_H
#define ED_TOOL_H

#include <stdint.h>

struct ed_tool_context_t;
struct ed_tool_t;

typedef uint32_t (ed_tool_state_func_t)(struct ed_tool_context_t *context, void *data, uint32_t just_changed);

struct ed_tool_t
{
    ed_tool_state_func_t *  entry_state;
    void *                  data;
//    ed_tool_state_func_t *test_input;
};

struct ed_tool_context_t
{
//    uint32_t                tool_count;
//    struct ed_tool_t *      tools;

    ed_tool_state_func_t *  idle_state;

    ed_tool_state_func_t *  current_state;
    void *                  current_data;

    ed_tool_state_func_t *  prev_state;
    void *                  context_data;
};

void ed_UpdateTools(struct ed_tool_context_t *context);

void ed_NextToolState(struct ed_tool_context_t *context, ed_tool_state_func_t *next_state, void *next_data);

#endif // ED_TOOL_H
