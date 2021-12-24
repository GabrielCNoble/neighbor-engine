#ifndef G_DEFS_H
#define G_DEFS_H

#include <stdint.h>
#include "../engine/e_defs.h"
#include "../engine/r_defs.h"
#include "../engine/p_defs.h"
#include "../lib/dstuff/ds_vector.h"
#include "../lib/dstuff/ds_slist.h"


/*
========================================================

    base "entity"

========================================================
*/

enum G_THING_TYPES
{
    G_THING_TYPE_ENEMY = 0,
    G_THING_TYPE_BARRIER,
    G_THING_TYPE_ITEM,
    G_THING_TYPE_SWITCH,
    G_THING_TYPE_LAST
};

struct g_thing_t
{
    uint32_t index;
    uint32_t type;
    char name[32];
    struct e_entity_t *entity;
    struct ds_slist_t *list;
};

/*
========================================================

    enemies

========================================================
*/

/********** base enemy **********/
enum G_ENEMY_TYPES
{
    G_ENEMY_TYPE_CAMERA,
    G_ENEMY_TYPE_TURRET,
    G_ENEMY_TYPE_LAST
};

struct g_enemy_t
{
    struct g_thing_t thing;
    uint32_t type;
};

/********** camera **********/

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

#define G_CAMERA_DEF                         \
    float max_pitch;                            \
    float min_pitch;                            \
    float max_yaw;                              \
    float min_yaw;                              \
    float cur_pitch;                            \
    float cur_yaw;                              \
    float idle_pitch;                           \
    float range;                                \

struct g_camera_def_t
{
    G_CAMERA_DEF;
};

struct g_camera_t
{
    struct g_enemy_t enemy;

    union
    {
        struct { G_CAMERA_DEF; };
        struct g_camera_def_t def;
    };

    uint32_t flags;
    uint32_t state;

    float startled_timer_delta;
    float startled_yaw_delta;
    float startled_yaw;

    vec3_t last_player_pos;

    float sweep_timer;
    float startled_timer;
    float alert_timer;

    struct r_spot_light_t *light;
    struct p_collider_t *detected_collider;
};

/********** turret **********/

#define G_TURRET_DEF                             \


struct g_turret_def_t
{
    G_TURRET_DEF;
};

struct g_turret_t
{
    struct g_enemy_t enemy;

    union
    {
        struct {G_TURRET_DEF; };
        struct g_turret_def_t def;
    };

    uint32_t state;
    uint32_t flags;
};

struct g_enemy_def_t
{
    uint32_t type;

    union
    {
        struct g_camera_def_t camera;
        struct g_turret_def_t turret;
    };
};

/*
========================================================

    barriers

========================================================
*/


/********** base barrier **********/

#define G_MAX_BARRIER_SUBTYPES 3

enum G_BARRIER_TYPES
{
    G_BARRIER_TYPE_HINGE_DOOR,
    G_BARRIER_TYPE_SLIDE_DOOR,
    G_BARRIER_TYPE_LAST,
};

struct g_barrier_t
{
    struct g_thing_t thing;
    uint16_t type;
    uint16_t sub_type;
};

/********** hinge-like doors **********/

enum G_HINGE_DOOR_TYPES
{
    G_HINGE_DOOR_TYPE_NORMAL,
    G_HINGE_DOOR_TYPE_TRAP_DOOR,
};

#define G_HINGE_DOOR_FIELDS             \
    uint32_t locked;                    \

struct g_hinge_door_fields_t
{
    G_HINGE_DOOR_FIELDS;
};

struct g_hinge_door_t
{
    struct g_barrier_t barrier;

    union
    {
        struct { G_HINGE_DOOR_FIELDS; };
        struct g_hinge_door_fields_t fields;
    };
};

struct g_barrier_fields_t
{
    uint16_t type;
    uint16_t sub_type;

    union
    {
        struct g_hinge_door_fields_t hinge;
    };
};



/*
========================================================

    items

========================================================
*/

/********** base item **********/
enum G_ITEM_TYPES
{
    G_ITEM_TYPE_INTEL,
    G_ITEM_TYPE_KEY,
};

struct g_item_t
{
    struct g_thing_t thing;
    uint32_t type;
};

/********** keys/keycards for doors/gates **********/
#define G_KEY_FIELDS                            \
    char barrier_name[32];                      \

struct g_key_fields_t
{
    G_KEY_FIELDS;
};

struct g_key_t
{
    struct g_item_t item;
    struct g_barrier_t *barrier;
    union
    {
        struct {G_KEY_FIELDS; };
        struct g_key_fields_t fields;
    };
};

/********** intel **********/
#define G_INTEL_FIELDS

struct g_intel_fields_t
{
    G_INTEL_FIELDS;
};

struct g_item_fields_t
{
    uint32_t type;

    union
    {
        struct g_key_fields_t key;
        struct g_intel_fields_t intel;
    };
};

/*
========================================================

    serialization stuff

========================================================
*/

struct g_thing_record_t
{
    mat3_t orientation;
    vec3_t position;
    vec3_t scale;
    uint32_t type;
    uint32_t d_index;
    uint32_t s_index;
};

//struct g_enemy_section_t
//{
//    uint64_t record_start;
//    uint64_t record_count;
//};

union g_enemy_record_t
{
    struct
    {
        struct g_thing_record_t thing;
        struct g_enemy_def_t def;
    };

    uint64_t data[32];
};

union g_barrier_record_t
{
    struct
    {
        struct g_thing_record_t thing;
        struct g_barrier_fields_t fields;
    };

    uint64_t data[32];
};

union g_item_record_t
{
    struct
    {
        struct g_thing_record_t thing;
        struct g_item_fields_t fields;
    };

    uint64_t data[32];
};



struct g_game_section_t
{
    uint64_t player_start;
    uint64_t player_size;

    uint64_t enemy_start;
    uint64_t enemy_count;

    uint64_t barrier_start;
    uint64_t barrier_size;

    uint64_t item_start;
    uint64_t item_size;

    uint64_t switch_start;
    uint64_t switch_size;
};

/*
========================================================

    other objects

========================================================
*/

//struct g_intel_t
//{
//    G_ENTITY_FIELDS;
//};

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
    vec3_t prev_pos;
    struct e_entity_t *grabbed_entity;
    float grabbed_entity_mass;
    struct p_character_collider_t *collider;
};

/*
========================================================

    main menu stuff

========================================================
*/


#endif // G_EN_DEFS_H





