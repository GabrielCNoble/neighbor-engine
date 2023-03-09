#ifndef ED_LIGHT_H
#define ED_LIGHT_H

#include "obj.h"

struct ed_light_args_t
{
    vec3_t      color;
    uint32_t    type;
    float       range;
    float       energy;
    float       inner_angle;
    float       outer_angle;
};

void ed_InitLightObjectFuncs();

//void *ed_CreateLightObject(vec3_t *position, mat3_t *orientation, vec3_t *scale, void *args);

//void ed_DestroyLightObject(void *base_obj);

//struct r_i_draw_list_t *ed_RenderPickLightObject(struct ed_obj_t *object, struct r_i_cmd_buffer_t *cmd_buffer);

//struct r_i_draw_list_t *ed_RenderOutlineLightObject(struct ed_obj_result_t *object, struct r_i_cmd_buffer_t *cmd_buffer);

//void ed_UpdateLightHandleObject(struct ed_obj_t *object);

//void ed_UpdateLightBaseObject(struct ed_obj_result_t *object);

#endif // ED_LIGHT_H
