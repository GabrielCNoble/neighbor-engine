#ifndef ED_LIGHT_H
#define ED_LIGHT_H

#include "obj.h"

struct ed_light_args_t
{
    vec3_t      color;
    uint32_t    type;
    float       radius;
    float       energy;
};

void *ed_CreateLightObject(vec3_t *position, mat3_t *orientation, vec3_t *scale, void *args);

void ed_DestroyLightObject(void *base_obj);

void ed_RenderPickLightObject(struct ed_obj_t *object, struct r_i_cmd_buffer_t *cmd_buffer);

void ed_RenderOutlineLightObject(struct ed_obj_t *object, struct r_i_cmd_buffer_t *cmd_buffer);

void ed_UpdateLightObject(struct ed_obj_t *object);

#endif // ED_LIGHT_H
