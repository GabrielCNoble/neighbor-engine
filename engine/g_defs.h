#ifndef G_DEFS_H
#define G_DEFS_H

#include <stdint.h>
#include "e_defs.h"
#include "r_defs.h"
#include "../lib/dstuff/ds_vector.h"
#include "../lib/dstuff/ds_slist.h"


enum G_ENTITY_TYPES
{
    G_ENTITY_TYPE_CAMERA = 0,
    G_ENTITY_TYPE_TURRET,
    G_ENTITY_TYPE_INTEL,
    G_ENTITY_TYPE_LAST
};

#define G_ENTITY_FIELDS                     \
    uint32_t type;                          \
    struct e_entity_t *entity

struct g_entity_t
{
    G_ENTITY_FIELDS;
};


struct g_camera_t
{
    G_ENTITY_FIELDS;
    uint32_t state;
    float pitch;
    float yaw;
    struct r_spot_light_t *light;
};

struct g_turret_t
{
    G_ENTITY_FIELDS;
    uint32_t state;
};

struct g_intel_t
{
    G_ENTITY_FIELDS;
};

struct g_spawn_point_t
{
    vec3_t position;
};



#endif // G_EN_DEFS_H





