#include "light.h"
#include "../engine/r_main.h"

void *ed_CreateLightObject(vec3_t *position, mat3_t *orientation, vec3_t *scale, void *args)
{
    struct ed_light_args_t *light_args = (struct ed_light_args_t *)args;
    return r_CreateLight(light_args->type, position, &light_args->color, light_args->radius, light_args->energy);
}

void ed_DestroyLightObject(void *base_obj)
{

}

struct r_i_draw_list_t *ed_RenderPickLightObject(struct ed_obj_t *object, struct r_i_cmd_buffer_t *cmd_buffer)
{
    return NULL;
}

struct r_i_draw_list_t *ed_RenderOutlineLightObject(struct ed_obj_result_t *object, struct r_i_cmd_buffer_t *cmd_buffer)
{
    return NULL;
}

void ed_UpdateLightHandleObject(struct ed_obj_t *object)
{

}

void ed_UpdateLightBaseObject(struct ed_obj_result_t *object)
{

}
