#ifndef ED_ENT_DEFS_H
#define ED_ENT_DEFS_H

#include "ed_defs.h"
#include "../engine/e_defs.h"

struct ed_entity_state_t
{
    float camera_pitch;
    float camera_yaw;
    float camera_zoom;

    vec3_t camera_offset;

    struct e_ent_def_t *cur_ent_def;
};


#endif // ED_ENT_DEFS_H
