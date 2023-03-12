#ifndef ED_OBJ_H
#define ED_OBJ_H

#include "../../lib/dstuff/ds_slist.h"
#include "../../lib/dstuff/ds_matrix.h"
#include "../../lib/dstuff/ds_vector.h"
#include "../../engine/r_defs.h"
#include "../tool.h"

enum ED_OBJ_TYPES
{
    ED_OBJ_TYPE_BRUSH,
    ED_OBJ_TYPE_ENTITY,
    ED_OBJ_TYPE_LIGHT,
    ED_OBJ_TYPE_LAST,
};

enum ED_WIDGET_TYPES
{
    ED_WIDGET_TYPE_MANIPULATOR,
    ED_WIDGET_TYPE_LAST
};

enum ED_OBJ_DISPLAY
{
    ED_OBJ_DISPLAY_WORLD_SPACE,
    ED_OBJ_DISPLAY_CAMERA_SPACE,
    ED_OBJ_DISPLAY_BILBOARD,
};

#define ED_INVALID_OBJ_SELECTION_INDEX 0xffffffff

struct ed_obj_t
{
    uint32_t                        index;
    uint32_t                        type;
    uint32_t                        selection_index;
    uint32_t                        sub_obj_count;
    struct ed_obj_context_t *       context;
    mat4_t                          transform;
    void *                          base_obj;
};

//#define ED_PICK_MAX_DRAW_LISTS 3
//struct ed_pick_data_t
//{
//    struct r_i_draw_list_t *    draw_lists[ED_PICK_MAX_DRAW_LISTS];
//    uint32_t                    draw_list_count;
//};

struct ed_obj_result_t
{
    struct ed_obj_t *   object;
    uint32_t            type;
    uint32_t            index;
    uint32_t            extra0;
    uint32_t            extra1;
};


//enum ED_OBJ_EVENTS
//{
//    ED_OBJ_EVENT_PICK = 0,
//    ED_OBJ_EVENT_TRANSLATE,
//    ED_OBJ_EVENT_ROTATE,
//    ED_OBJ_EVENT_SCALE,
//    ED_OBJ_EVENT_LAST,
//};
//
//struct ed_obj_pick_event_data_t
//{
//    uint32_t mouse_button;
//};
//
//struct ed_obj_translate_event_data_t
//{
//    vec3_t      translation;
//};
//
//struct ed_obj_rotate_event_data_t
//{
//    vec3_t      axis;
//    float       angle;
//};
//
//struct ed_obj_scale_event_data_t
//{
//    vec3_t      axis;
//    float       factor;
//};
//
//struct ed_obj_event_t
//{
//    uint32_t        type;
//    void *          data;
//};

struct ed_obj_event_t;

struct ed_obj_funcs_t
{
    void *                    (*create)                  (vec3_t *position, mat3_t *orientation, vec3_t *scale, void *args);
    void                      (*destroy)                 (struct ed_obj_t *object);
    void                      (*update)                  (struct ed_obj_t *object, struct ed_obj_event_t *event);
    struct r_i_draw_list_t *  (*pick)                    (struct ed_obj_t *object, struct r_i_cmd_buffer_t *command_buffer, void *args);
    struct r_i_draw_list_t *  (*draw)                    (struct ed_obj_t *object, struct r_i_cmd_buffer_t *command_buffer);
    struct r_i_draw_list_t *  (*draw_selected)           (struct ed_obj_t *object, struct r_i_cmd_buffer_t *command_buffer);
};

struct ed_obj_h
{
    uint32_t index;
    uint32_t type;
};

#define ED_INVALID_OBJ_INDEX 0xffffffff
#define ED_INVALID_OBJ (struct ed_obj_h){.type = ED_OBJ_TYPE_LAST, .index = ED_INVALID_OBJ_INDEX}
#define ED_PICK_SHADER_UNIFORM_MODEL_VIEW_PROJECTION_MATRIX 0
#define ED_PICK_SHADER_UNIFORM_OBJ_TYPE                     1
#define ED_PICK_SHADER_UNIFORM_OBJ_INDEX                    2
#define ED_PICK_SHADER_UNIFORM_OBJ_EXTRA0                   3
#define ED_PICK_SHADER_UNIFORM_OBJ_EXTRA1                   4


#define ED_OUTLINE_SHADER_UNIFORM_MODEL_VIEW_PROJECTION_MATRIX  0
#define ED_OUTLINE_SHADER_UNIFORM_COLOR                         1

enum ED_OPERATORS
{
    ED_OPERATOR_TRANSFORM = 0,
    ED_OPERATOR_LAST
};

enum ED_TRANSFORM_OPERATOR_MODES
{
    ED_TRANSFORM_OPERATOR_MODE_TRANSLATE = 0,
    ED_TRANSFORM_OPERATOR_MODE_ROTATE,
    ED_TRANSFORM_OPERATOR_MODE_SCALE,
    ED_TRANSFORM_OPERATOR_MODE_LAST,
};

struct ed_operator_t
{
    uint32_t            type;
    struct r_model_t *  model;
    struct ds_list_t    objects;
    uint32_t            visible;
    void *              data;
};

struct ed_obj_context_t
{
    struct ed_operator_t        operators[ED_OPERATOR_LAST];
    struct ds_slist_t           objects[ED_OBJ_TYPE_LAST];
    struct ds_list_t            selections;
    struct ed_obj_result_t      last_picked;
};

struct ed_operator_funcs_t
{
    void                    (*update)(struct ed_obj_context_t *context, struct ed_operator_t *operator);
    uint32_t                (*apply)(struct ed_obj_context_t *context, struct ed_operator_t *operator);
    struct ed_obj_funcs_t     obj_funcs;
};

struct ed_pick_args_t
{
    struct ed_obj_context_t *   context;
    void *                      args[ED_OBJ_TYPE_LAST];
};

//struct ed_translation_event_t
//{
//    vec3_t      translation;
//    mat3_t      axes;
//};
//
//struct ed_rotation_event_t
//{
//    vec3_t      axis;
//    vec3_t      offset;
//    float       amount;
//};
//
//struct ed_scale_event_t
//{
//    vec3_t      axis;
//    float       factor;
//};

struct ed_transform_event_t
{
    uint32_t                    type;

    union
    {
        struct
        {
            vec3_t              translation;
        } translation;

        struct
        {
            mat3_t              rotation;
            vec3_t              offset;
        } rotation;

        struct
        {
            vec3_t              axis;
            float               factor;
        } scale;
    };
};

struct ed_operator_event_t
{
    uint32_t                        type;

    union
    {
        struct ed_transform_event_t transform;
    };
};

struct ed_pick_event_t
{
    struct ed_obj_result_t  result;
    uint32_t                multiple;
};

enum ED_OBJ_EVENT_TYPES
{
    ED_OBJ_EVENT_TYPE_PICK = 0,
    ED_OBJ_EVENT_TYPE_OPERATOR,
};

struct ed_obj_event_t
{
    uint32_t                        type;

    union
    {
        struct ed_operator_event_t  operator;
        struct ed_pick_event_t      pick;
    };
};


//struct ed_operator_event_t
//{
//    struct ed_operator_t *  operator;
//
//    union
//    {
//        struct ed_transform_event_t transform_event;
//    };
//};

struct ed_transform_operator_data_t
{
    uint32_t                    mode;
    float                       linear_snap;
    float                       angular_snap;
    vec3_t                      start_pos;
    vec3_t                      prev_offset;
};

void ed_ObjInit();

void ed_ObjShutdown();

struct ed_obj_context_t ed_CreateObjContext(void **operator_data);

void ed_DestroyObjContext(struct ed_obj_context_t *context);

struct ed_obj_t *ed_CreateObj(struct ed_obj_context_t *context, uint32_t type, vec3_t *position, mat3_t *orientation, vec3_t *scale, void *args);

void ed_DestroyObj(struct ed_obj_context_t *context, struct ed_obj_t *object);

struct ed_obj_t *ed_GetObject(struct ed_obj_context_t *context, struct ed_obj_h handle);

void ed_BeginPick(struct ed_obj_context_t *context);

struct ed_obj_result_t ed_EndPick(struct ed_obj_context_t *context, int32_t mouse_x, int32_t mouse_y);

void ed_PickObjectWithFuncs(struct ed_obj_t *object, struct ed_obj_funcs_t *funcs, void *args);

void ed_PickObjectFromListWithFuncs(struct ds_slist_t *objects, struct ed_obj_funcs_t *funcs, void *args);

//struct ed_obj_result_t ed_PickObjectWithFilter(struct ed_obj_context_t *context, int32_t mouse_x, int32_t mouse_y, uint32_t *types, uint32_t type_count);

struct ed_obj_result_t ed_PickObject(struct ed_pick_args_t *pick_args, int32_t mouse_x, int32_t mouse_y);

struct ed_obj_result_t ed_PickOperator(struct ed_obj_context_t *context, int32_t mouse_x, int32_t mouse_y);

void ed_UpdateOperators(struct ed_obj_context_t *context);

void ed_ApplyOperator(struct ed_obj_context_t *context, struct ed_operator_t *operator, struct ed_obj_event_t *event);

void ed_UpdateObjectContext(struct ed_obj_context_t *context, struct r_i_cmd_buffer_t *command_buffer);

//void ed_DrawSelections(struct ed_obj_context_t *context, struct r_i_cmd_buffer_t *cmd_buffer);

void ed_AddObjToSelections(struct ed_obj_context_t *context, uint32_t multiple, struct ed_obj_result_t *obj);

void ed_DropObjFromSelections(struct ed_obj_context_t *context, struct ed_obj_result_t *obj);

void ed_ClearSelections(struct ed_obj_context_t *context);


/*
============================================================================
============================================================================
============================================================================
*/

uint32_t ed_ClickPickState(struct ed_tool_context_t *context, void *state_data, uint32_t just_changed);

uint32_t ed_ApplyOperatorState(struct ed_tool_context_t *context, void *state_data, uint32_t just_changed);

uint32_t ed_CameraRay(int32_t mouse_x, int32_t mouse_y, vec3_t *plane_point, vec3_t *plane_normal, vec3_t *result);

void ed_PointPixelCoords(int32_t *x, int32_t *y, vec3_t *point);

void ed_CameraSpaceDrawTransform(mat4_t *in_transform, mat4_t *out_transform, float camera_distance);


/*
============================================================================
============================================================================
============================================================================
*/

void ed_TranslateSelections(struct ed_obj_context_t *context, vec3_t *translation);

void ed_RotateSelections(struct ed_obj_context_t *context, mat3_t *rotation);

/*
============================================================================
============================================================================
============================================================================
*/

void ed_DrawModelOutline(mat4_t *transform, struct r_model_t *model, struct r_i_cmd_buffer_t *command_buffer, struct r_i_draw_list_t *draw_list, vec4_t *outline_color);


#endif // ED_PICKABLE_H
