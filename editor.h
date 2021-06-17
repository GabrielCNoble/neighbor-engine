#ifndef EDITOR_H
#define EDITOR_H

#include "dstuff/ds_vector.h"
#include "dstuff/ds_matrix.h"
#include "dstuff/ds_alloc.h"
#include "dstuff/ds_buffer.h"
#include "ed_com.h"

//struct ed_polygon_t
//{
//    struct ds_buffer_t verts;
//};

//struct ed_face_t
//{
//    struct r_material_t *material;
//    struct ds_buffer_t indices;
//    vec3_t normal;
//};
//
//struct ed_brush_t
//{
//    mat3_t orientation;
//    vec3_t position;
//    uint32_t index;
//    struct list_t faces;
//    struct ds_buffer_t vertices;
//    struct r_model_t *model;
//};

struct ed_context_t;

struct ed_state_t
{
    void (*update)(struct ed_context_t *context, uint32_t just_changed);
};

struct ed_context_t
{
    void (*update)();
    uint32_t current_state;
    uint32_t next_state;
    struct ed_state_t *states;
    void *context_data;
};

struct ed_world_context_data_t
{
    vec3_t box_start;
    vec3_t box_end;
    struct list_t selections;
};

enum ED_SELECTION_TYPE
{
    ED_SELECTION_TYPE_BRUSH = 0,
    ED_SELECTION_TYPE_ENTITY,
    ED_SELECTION_TYPE_LIGHT,
    ED_SELECTION_TYPE_FACE,
    ED_SELECTION_TYPE_EDGE
};

struct ed_selection_t
{
    uint32_t type;
    union
    {
        uint32_t index;
        void *pointer;
    }selection;
};


enum ED_CONTEXTS
{
    ED_CONTEXT_WORLD = 0,
    ED_CONTEXT_LAST,
};


enum ED_WORLD_CONTEXT_STATES
{
    ED_WORLD_CONTEXT_STATE_IDLE = 0,
    ED_WORLD_CONTEXT_STATE_LEFT_CLICK,
    ED_WORLD_CONTEXT_STATE_RIGHT_CLICK,
    ED_WORLD_CONTEXT_STATE_BRUSH_BOX,
    ED_WORLD_CONTEXT_STATE_CREATE_BRUSH,
    ED_WORLD_CONTEXT_STATE_LAST
};



void ed_Init();

void ed_Shutdown();

void ed_UpdateEditor();

void ed_FlyCamera();

void ed_DrawGrid();

void ed_DrawBrushes();

void ed_DrawLights();

void ed_SetContextState(struct ed_context_t *context, uint32_t state);

void ed_WorldContextUpdate();

void ed_WorldContextIdleState(struct ed_context_t *context, uint32_t just_changed);

void ed_WorldContextLeftClickState(struct ed_context_t *context, uint32_t just_changed);

void ed_WorldContextStateBrushBox(struct ed_context_t *context, uint32_t just_changed);

void ed_WorldContextCreateBrush(struct ed_context_t *context, uint32_t just_changed);

uint32_t ed_PickObject(int32_t mouse_x, int32_t mouse_y, struct ed_selection_t *selection);

struct ed_brush_t *ed_CreateBrush(vec3_t *position, mat3_t *orientation, vec3_t *size);

struct ed_brush_t *ed_GetBrush(uint32_t index);

void ed_UpdateBrush(struct ed_brush_t *brush);

#endif // ED_H
