#ifndef ED_OBJ_H
#define ED_OBJ_H

#include "../../lib/dstuff/ds_slist.h"
#include "../../lib/dstuff/ds_matrix.h"
#include "../../lib/dstuff/ds_vector.h"
#include "../../engine/r_defs.h"

enum ED_OBJ_TYPES
{
    ED_OBJ_TYPE_BRUSH,
    ED_OBJ_TYPE_ENTITY,
    ED_OBJ_TYPE_LIGHT,
    ED_OBJ_TYPE_FACE,
    ED_OBJ_TYPE_EDGE,
    ED_OBJ_TYPE_VERT,
    ED_OBJ_TYPE_LAST,
};

enum ED_OBJ_DISPLAY
{
    ED_OBJ_DISPLAY_WORLD_SPACE,
    ED_OBJ_DISPLAY_CAMERA_SPACE,
    ED_OBJ_DISPLAY_BILBOARD,
};

struct ed_obj_context_t
{
    struct ds_slist_t       objects[ED_OBJ_TYPE_LAST];
    struct ds_list_t        selections;
};

#define ED_INVALID_OBJ_SELECTION_INDEX 0xffffffff

struct ed_obj_t
{
    uint32_t                        index;
    uint32_t                        type;
    uint32_t                        selection_index;
    struct ed_obj_context_t *       context;
    struct ed_obj_t *               next_selected;
    struct ed_obj_t *               prev_selected;

    mat4_t                          transform;
    void *                          base_obj;
};

struct ed_obj_funcs_t
{
    void                (*render_pick)      (struct ed_obj_t *object, struct r_i_cmd_buffer_t *cmd_buffer);
    void                (*render_outline)   (struct ed_obj_t *object, struct r_i_cmd_buffer_t *cmd_buffer);
    void                (*update)           (struct ed_obj_t *object);
    void *              (*create)           (vec3_t *position, mat3_t *orientation, vec3_t *scale, void *args);
    void                (*destroy)          (void *base_obj);
    void                (*draw_transform)   (struct ed_obj_t *objectt, mat4_t *view_projection_matrix);
};

struct ed_obj_h
{
    uint32_t index;
    uint32_t type;
};

#define ED_INVALID_OBJ (struct ed_obj_h){.type = ED_OBJ_TYPE_LAST, .index = 0xffffffff}
#define ED_PICK_SHADER_UNIFORM_MODEL_VIEW_PROJECTION_MATRIX 0
#define ED_PICK_SHADER_UNIFORM_OBJ_TYPE                     1
#define ED_PICK_SHADER_UNIFORM_OBJ_INDEX                    2


#define ED_OUTLINE_SHADER_UNIFORM_MODEL_VIEW_PROJECTION_MATRIX  0
#define ED_OUTLINE_SHADER_UNIFORM_COLOR                         1

void ed_ObjInit();

void ed_ObjShutdown();

struct ed_obj_context_t ed_CreateObjContext();

void ed_DestroyObjContext(struct ed_obj_context_t *context);

struct ed_obj_h ed_CreateObj(struct ed_obj_context_t *context, uint32_t type, vec3_t *position, mat3_t *orientation, vec3_t *scale, void *args);

void ed_DestroyObj(struct ed_obj_context_t *context, struct ed_obj_t *object);

struct ed_obj_t *ed_GetObject(struct ed_obj_context_t *context, struct ed_obj_h handle);

struct ed_obj_t *ed_PickObject(struct ed_obj_context_t *context, int32_t mouse_x, int32_t mouse_y, uint32_t ignore_mask);

void ed_DrawSelections(struct ed_obj_context_t *context, struct r_i_cmd_buffer_t *cmd_buffer);

void ed_AddObjToSelections(struct ed_obj_context_t *context, uint32_t multiple, struct ed_obj_t *obj);

void ed_DropObjFromSelections(struct ed_obj_context_t *context, struct ed_obj_t *obj);

void ed_ClearSelections(struct ed_obj_context_t *context);


/*
============================================================================
============================================================================
============================================================================
*/

void ed_WorldSpaceDrawTransform(struct ed_obj_t *object, mat4_t *model_view_projection_matrix);

void ed_CameraSpaceDrawTransform(struct ed_obj_t *object, mat4_t *model_view_projection_matrix);




#endif // ED_PICKABLE_H
