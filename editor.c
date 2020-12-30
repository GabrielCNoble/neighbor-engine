#include "editor.h"
#include "dstuff/ds_stack_list.h"
#include "dstuff/ds_list.h"

struct stack_list_t ed_brushes;

void ed_Init()
{
    ed_brushes = create_stack_list(sizeof(struct ed_brush_t), 512);
}

void ed_Shutdown()
{
    
}

void ed_UpdateEditor()
{
    
}

void ed_DrawBrushes()
{
    
}

void ed_CreateBrush(vec3_t *position, mat3_t *orientation, vec3_t *size)
{
    
}
