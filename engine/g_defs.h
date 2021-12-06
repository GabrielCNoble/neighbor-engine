#ifndef G_DEFS_H
#define G_DEFS_H

#include <stdint.h>
#include "e_defs.h"
#include "r_defs.h"
#include "p_defs.h"
#include "../lib/dstuff/ds_vector.h"
#include "../lib/dstuff/ds_slist.h"


/*
========================================================

    base "entity"

========================================================
*/
enum G_ENTITY_TYPES
{
    G_ENTITY_TYPE_CAMERA = 0,
    G_ENTITY_TYPE_TURRET,
    G_ENTITY_TYPE_INTEL,
    G_ENTITY_TYPE_DOOR,
    G_ENTITY_TYPE_LAST
};

#define G_ENTITY_FIELDS                         \
    uint32_t index;                             \
    struct e_entity_t *entity

struct g_entity_t
{
    G_ENTITY_FIELDS;
};

/*
========================================================

    enemies

========================================================
*/

enum G_ENEMY_TYPES
{
    G_ENEMY_TYPE_CAMERA,
    G_ENEMY_TYPE_TURRET,
    G_ENEMY_TYPE_MOTION_SENSOR,
    G_ENEMY_TYPE_LAST
};


#define G_ENEMY_FIELDS              \
    G_ENTITY_FIELDS;                \
    uint32_t type

struct g_enemy_t
{
    G_ENEMY_FIELDS;
};

enum G_CAMERA_STATES
{
    G_CAMERA_STATE_IDLE = 0,
    G_CAMERA_STATE_STARTLED,
    G_CAMERA_STATE_ALERT,
};

#define G_CAMERA_MIN_YAW -1.0
#define G_CAMERA_MAX_YAW 1.0
#define G_CAMERA_ALERT_COOLDOWN 5.0
#define G_CAMERA_ALERT_SEEK_TIME 5.0
#define G_CAMERA_IDLE_SWEEP_TIMER 5.0
#define G_CAMERA_STARTLED_MIN_SWEEP_TIMER 0.5
#define G_CAMERA_STARTLED_MAX_YAW_DELTA 0.05
#define G_CAMERA_STARTLED_COOLDOWN 15.0

enum G_CAMERA_FLAGS
{
    G_CAMERA_FLAG_SWEEP_LEFT = 1,
    G_CAMERA_FLAG_SEEK_LOST = 1 << 1,
    G_CAMERA_FLAG_GAVE_UP = 1 << 2,
    G_CAMERA_FLAG_PLAYER_HID = 1 << 3,
    G_CAMERA_FLAG_RELOAD_SWEEP_TIMER = 1 << 4,
};

struct g_camera_t
{
    G_ENEMY_FIELDS;

    uint32_t flags;

    uint32_t state;
    float state_timer;

    float max_pitch;
    float min_pitch;

    float max_yaw;
    float min_yaw;

    float idle_pitch;
    float startled_timer_delta;
    float startled_yaw_delta;
    float startled_yaw;

    float cur_pitch;
    float cur_yaw;

    vec3_t last_player_vec;
    float last_player_dist;

    float sweep_timer;
    float startled_timer;
    float alert_timer;

    float range;
    struct r_spot_light_t *light;
};

struct g_turret_t
{
    G_ENEMY_FIELDS;
    uint32_t state;
};

/*
========================================================

    other objects

========================================================
*/

struct g_intel_t
{
    G_ENTITY_FIELDS;
};

struct g_spawn_point_t
{
    uint32_t index;
    vec3_t position;
};

struct g_player_t
{
    uint32_t flags;
    float yaw;
    float pitch;
    struct e_entity_t *entity;
    float grab_time;
    vec3_t relative_grab_offset;
    struct e_entity_t *grabbed_entity;
    struct p_character_collider_t *collider;
};


#endif // G_EN_DEFS_H





