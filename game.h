#ifndef GAME_H
#define GAME_H

#include "dstuff/ds_vector.h"
#include "dstuff/ds_matrix.h"
#include "dstuff/ds_list.h"
#include "physics.h"
#include "anim.h"
#include "r_draw.h"
#include "r_vis.h"
#include <stdint.h>

struct g_prop_t
{
    char *name;
    uint32_t size;
    void *data;
};

struct g_entity_t;

typedef void (thinker_t)(struct g_entity_t *this);

struct g_entity_t
{
    uint32_t index;
    mat4_t transform;
    mat4_t local_transform;
    mat4_t *parent_transform; /* can be either other entity or skeleton attachment point */
    struct ds_list_t props;
    thinker_t *thinker;
    struct p_collider_t *collider;
    struct r_model_t *model;
    struct a_mixer_t *mixer;
    struct r_vis_item_t *item;
    vec3_t extents;
};

enum G_PLAYER_FLAGS
{
    G_PlAYER_FLAG_JUMPING = 1,
    G_PLAYER_FLAG_TURNING = 1 << 1,
    G_PLAYER_FLAG_TURNING_LEFT = 1 << 2,
};

struct g_player_state_t
{
    uint32_t flags;
    uint32_t collider_flags;
    float direction;
    float jump_y;
    float jump_disp;
    float run_frac;
    float jump_frac;
    float run_scale;
    float shoot_frac;
    struct r_light_t *shot_light;
};

enum G_GAME_STATE
{
    G_GAME_STATE_PLAYING = 0,
    G_GAME_STATE_PAUSED,
    G_GAME_STATE_EDITING,
    G_GAME_STATE_LOADING,
    G_GAME_STATE_QUIT,
};

struct g_projectile_t
{
    vec3_t position;
    vec3_t velocity;
    uint32_t index;
    struct r_light_t *light;
    uint32_t life;
    uint32_t bouces;
};

void g_Init(uint32_t editor_active);

void g_Shutdown();

void g_SetGameState(uint32_t game_state);

void g_MainLoop();

void g_LoadMap(char *file_name);

void g_UpdateEntities();

struct g_entity_t *g_CreateEntity(mat4_t *transform, thinker_t *thinker, struct r_model_t *model);

struct g_entity_t *g_GetEntity(uint32_t index);

void g_UpdateEntityExtents(struct g_entity_t *entity);

void g_DestroyEntity(struct g_entity_t *entity);

void g_ParentEntity(struct g_entity_t *parent, struct g_entity_t *entity);

void g_SetEntityCollider(struct g_entity_t *entity, uint32_t type, vec3_t *size);

struct g_projectile_t *g_SpawnProjectile(vec3_t *position, vec3_t *velocity, vec3_t *color, float radius, uint32_t life);

void g_DestroyProjectile(struct g_projectile_t *projectile);

void g_PlayAnimation(struct g_entity_t *entity, struct a_animation_t *animation, char *player_name);

void *g_GetProp(struct g_entity_t *entity, char *prop_name);

void *g_SetProp(struct g_entity_t *entity, char *prop_name, uint32_t size, void *data);

void g_RemoveProp(struct g_entity_t *entity, char *prop_name);

void g_PlayerThinker(struct g_entity_t *entity);

void g_ElevatorThinker(struct g_entity_t *entity);

void g_TriggerThinker(struct g_entity_t *trigger);


#endif // GAME_H





