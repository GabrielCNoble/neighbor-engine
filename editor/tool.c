#include "tool.h"


/*
    TODO: refactor the way a tool switches to an entry state
*/

void ed_UpdateTools(struct ed_tool_context_t *context)
{
    if(context->current_state == NULL)
    {
        for(uint32_t tool_index = 0; tool_index < context->tool_count; tool_index++)
        {
            struct ed_tool_t *tool = context->tools + tool_index;

            if(tool->entry_state(context, tool, 1))
            {
                context->current_tool = tool;
                ed_NextToolState(context, tool->entry_state);
                context->prev_state = tool->entry_state;
                break;
            }
        }
    }

    if(context->current_state != NULL)
    {
        uint32_t just_changed = 0;
        do
        {
            context->current_state(context, context->current_tool, just_changed);
            just_changed = context->current_state != context->prev_state;
            context->prev_state = context->current_state;
        }
        while(just_changed);
    }
}

void ed_NextToolState(struct ed_tool_context_t *context, ed_tool_state_func_t *next_state)
{
    context->prev_state = NULL;
    context->current_state = next_state;

    if(next_state == NULL)
    {
        context->current_tool = NULL;
    }
}
