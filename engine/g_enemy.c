#include "g_enemy.h"
#include "ent.h"
#include "g_game.h"
#include "r_main.h"
#include "phys.h"
#include "../lib/dstuff/ds_slist.h"

struct ds_slist_t g_enemies[G_ENEMY_TYPE_LAST];
struct e_ent_def_t *g_enemy_ent_defs[G_ENEMY_TYPE_LAST];
void (*g_enemy_step[G_ENEMY_TYPE_LAST])(struct g_enemy_t *enemy);
extern float r_spot_light_cos_lut[];
extern struct g_player_t g_player;
char *g_enemy_names[G_ENEMY_TYPE_LAST] =
{
    [G_ENEMY_TYPE_CAMERA] = "Camera",
    [G_ENEMY_TYPE_TURRET] = "Turret"
};

vec3_t g_camera_state_colors[] =
{
    [G_CAMERA_STATE_IDLE] = vec3_t_c(0.0, 1.0, 0.0),
    [G_CAMERA_STATE_STARTLED] = vec3_t_c(1.0, 1.0, 0.0),
    [G_CAMERA_STATE_ALERT] = vec3_t_c(1.0, 0.0, 0.0),
};

void g_EnemyInit()
{
    g_enemies[G_ENEMY_TYPE_CAMERA] = ds_slist_create(sizeof(struct g_camera_t), 32);
    g_enemies[G_ENEMY_TYPE_TURRET] = ds_slist_create(sizeof(struct g_turret_t), 32);

    g_enemy_ent_defs[G_ENEMY_TYPE_CAMERA] = e_LoadEntDef("camera.ent");
    g_enemy_ent_defs[G_ENEMY_TYPE_TURRET] = e_LoadEntDef("turret.ent");
}

struct g_enemy_t *g_CreateEnemy(uint32_t type, vec3_t *position, mat3_t *orientation)
{
    struct g_enemy_t *enemy = (struct g_enemy_t *)g_CreateEntity(&g_enemies[type], g_enemy_ent_defs[type], position, &vec3_t_c(1.0, 1.0, 1.0), orientation);
    enemy->type = type;
    return enemy;
}

void g_DestroyEnemy(struct g_enemy_t *enemy)
{
    if(enemy && enemy->index != 0xffffffff)
    {
        switch(enemy->type)
        {
            case G_ENEMY_TYPE_CAMERA:
            {
                struct g_camera_t *camera = (struct g_camera_t *)enemy;
                r_DestroyLight(camera->light);
            }
            break;
        }

        g_DestroyEntity((struct g_entity_t *)enemy);
    }
}

void g_DestroyAllEnemies()
{
    for(uint32_t type = G_ENEMY_TYPE_CAMERA; type < G_ENEMY_TYPE_LAST; type++)
    {
        for(uint32_t index = 0; index < g_enemies[type].cursor; index++)
        {
            struct g_enemy_t *enemy = g_GetEnemy(type, index);

            if(enemy)
            {
                g_DestroyEnemy(enemy);
            }
        }
    }
}

struct g_enemy_t *g_GetEnemy(uint32_t type, uint32_t index)
{
    struct g_enemy_t *enemy = ds_slist_get_element(&g_enemies[type], index);

    if(enemy && enemy->index == 0xffffffff)
    {
        enemy = NULL;
    }

    return enemy;
}

struct g_camera_t *g_CreateCamera(vec3_t *position, float min_pitch, float max_pitch, float min_yaw, float max_yaw, float idle_pitch, float range)
{
    mat3_t camera_orientation = mat3_t_c_id();
    mat3_t_rotate_x(&camera_orientation, -0.5);

    struct g_camera_t *camera = (struct g_camera_t *)g_CreateEnemy(G_ENEMY_TYPE_CAMERA, position, &camera_orientation);
    camera->min_pitch = min_pitch;
    camera->max_pitch = max_pitch;
    camera->min_yaw = min_yaw;
    camera->max_yaw = max_yaw;
    camera->idle_pitch = idle_pitch;
    camera->range = range;
    camera->cur_pitch = idle_pitch;
    camera->cur_yaw = (max_yaw + min_yaw) * 0.5;
    camera->state = G_CAMERA_STATE_IDLE;
    camera->flags = 0;
    camera->sweep_timer = 0;

    mat3_t pitch = mat3_t_c_id();
    mat3_t yaw = mat3_t_c_id();
    mat3_t_rotate_x(&pitch, camera->cur_pitch);
    mat3_t_rotate_y(&yaw, camera->cur_yaw);
    mat3_t_mul(&yaw, &pitch, &yaw);
    mat3_t_mul(&camera_orientation, &yaw, &camera_orientation);

    camera->light = r_CreateSpotLight(position, &g_camera_state_colors[camera->state], &camera_orientation, camera->range, 5.0, 25, 0.1);
    return camera;
}

void g_StepCameras(float delta_time)
{
    for(uint32_t camera_index = 0; camera_index < g_enemies[G_ENEMY_TYPE_CAMERA].cursor; camera_index++)
    {
        struct g_camera_t *camera = ds_slist_get_element(&g_enemies[G_ENEMY_TYPE_CAMERA], camera_index);

        if(camera && camera->index != 0xffffffff)
        {
            struct e_node_t *camera_node = camera->entity->node;
            struct r_spot_light_t *camera_light = camera->light;

            float time;
            vec3_t player_vec;
            struct p_character_collider_t *player_collider = NULL;
            vec3_t_sub(&player_vec, &g_player.collider->position, &camera_light->position);

            float player_dist = vec3_t_length(&player_vec);
            float player_vec_proj = -vec3_t_dot(&player_vec, &camera_light->orientation.rows[2]) / player_dist;
            float spot_cosine = r_spot_light_cos_lut[camera_light->angle - R_SPOT_LIGHT_MIN_ANGLE];

            switch(camera->state)
            {
                case G_CAMERA_STATE_IDLE:
                {
                    if(camera->sweep_timer <= 0.0)
                    {
                        if(camera->flags & G_CAMERA_FLAG_SWEEP_LEFT)
                        {
                            if(camera->cur_yaw < camera->max_yaw)
                            {
                                camera->cur_yaw += 0.003;
                            }
                            else
                            {
                                camera->cur_yaw = camera->max_yaw;
                                camera->flags &= ~G_CAMERA_FLAG_SWEEP_LEFT;
                                camera->flags |= G_CAMERA_FLAG_RELOAD_SWEEP_TIMER;
                            }
                        }
                        else
                        {
                            if(camera->cur_yaw > camera->min_yaw)
                            {
                                camera->cur_yaw -= 0.003;
                            }
                            else
                            {
                                camera->cur_yaw = camera->min_yaw;
                                camera->flags |= G_CAMERA_FLAG_SWEEP_LEFT | G_CAMERA_FLAG_RELOAD_SWEEP_TIMER;
                            }
                        }

                        if(camera->flags & G_CAMERA_FLAG_RELOAD_SWEEP_TIMER)
                        {
                            camera->flags &= ~G_CAMERA_FLAG_RELOAD_SWEEP_TIMER;
                            camera->sweep_timer = G_CAMERA_IDLE_SWEEP_TIMER;
                        }
                    }
                    else
                    {
                        camera->sweep_timer -= delta_time;
                    }

                    if(player_vec_proj > spot_cosine)
                    {
                        player_collider = p_Raycast(&camera_light->position, &g_player.collider->position, &time);

                        if(player_collider == g_player.collider)
                        {
                            camera->state = G_CAMERA_STATE_ALERT;
                        }
                    }
                }
                break;

                case G_CAMERA_STATE_STARTLED:
                {
                    if(player_vec_proj > spot_cosine)
                    {
                        player_collider = p_Raycast(&camera_light->position, &g_player.collider->position, &time);
                    }

                    uint32_t player_found = player_vec_proj > spot_cosine && player_collider == g_player.collider;

                    if(player_found)
                    {
                        camera->state = G_CAMERA_STATE_ALERT;
                    }
                    else
                    {
                        camera->startled_timer -= delta_time;

                        if(camera->startled_timer <= 0.0)
                        {
                            camera->state = G_CAMERA_STATE_IDLE;
                        }
                        else
                        {
                            if(camera->sweep_timer <= 0.0)
                            {
                                if(camera->flags & G_CAMERA_FLAG_SWEEP_LEFT)
                                {
                                    if(camera->cur_yaw < camera->startled_yaw + camera->startled_yaw_delta)
                                    {
                                        camera->cur_yaw += 0.005;
                                    }
                                    else
                                    {
                                        camera->cur_yaw = camera->startled_yaw + camera->startled_yaw_delta;
                                        camera->flags &= ~G_CAMERA_FLAG_SWEEP_LEFT;
                                        camera->flags |= G_CAMERA_FLAG_RELOAD_SWEEP_TIMER;
                                    }
                                }
                                else
                                {
                                    if(camera->cur_yaw > camera->startled_yaw - camera->startled_yaw_delta)
                                    {
                                        camera->cur_yaw -= 0.005;
                                    }
                                    else
                                    {
                                        camera->cur_yaw = camera->startled_yaw - camera->startled_yaw_delta;
                                        camera->flags |= G_CAMERA_FLAG_SWEEP_LEFT | G_CAMERA_FLAG_RELOAD_SWEEP_TIMER;
                                    }
                                }

                                if(camera->flags & G_CAMERA_FLAG_RELOAD_SWEEP_TIMER)
                                {
                                    camera->flags &= ~G_CAMERA_FLAG_RELOAD_SWEEP_TIMER;
                                    camera->startled_yaw_delta = (float)(rand() % 45) / 720.0;
                                    camera->startled_timer_delta = (float)(rand() % 50) / 100.0;
                                    camera->sweep_timer = G_CAMERA_STARTLED_MIN_SWEEP_TIMER + camera->startled_timer_delta;

                                    if(camera->startled_yaw_delta > G_CAMERA_STARTLED_MAX_YAW_DELTA)
                                    {
                                        camera->startled_yaw_delta = G_CAMERA_STARTLED_MAX_YAW_DELTA;
                                    }
                                }
                            }
                            else
                            {
                                camera->sweep_timer -= delta_time;
                            }
                        }
                    }
                }
                break;

                case G_CAMERA_STATE_ALERT:
                {
                    player_collider = p_Raycast(&camera_light->position, &g_player.collider->position, &time);
                    uint32_t player_lost = player_vec_proj < spot_cosine || player_collider != g_player.collider;

                    if(player_collider == g_player.collider && !(camera->flags & G_CAMERA_FLAG_PLAYER_HID))
                    {
                        camera->last_player_vec = player_vec;
                        camera->last_player_dist = player_dist;
                    }
                    else
                    {
                        camera->flags |= G_CAMERA_FLAG_PLAYER_HID;
                    }

                    if(player_lost && !(camera->flags & G_CAMERA_FLAG_SEEK_LOST))
                    {
                        if(!(camera->flags & G_CAMERA_FLAG_GAVE_UP))
                        {
                            camera->flags |= G_CAMERA_FLAG_SEEK_LOST;
                            camera->alert_timer = G_CAMERA_ALERT_SEEK_TIME;
                        }
                        else
                        {
                            camera->alert_timer -= delta_time;

                            if(camera->alert_timer <= 0.0)
                            {
                                camera->state = G_CAMERA_STATE_STARTLED;
                                camera->startled_yaw = camera->cur_yaw;
                                camera->startled_timer = G_CAMERA_STARTLED_COOLDOWN;
                            }
                        }
                    }
                    else
                    {
                        float pitch_increment = vec3_t_dot(&camera_light->orientation.rows[1], &camera->last_player_vec) / camera->last_player_dist;
                        float yaw_increment = -vec3_t_dot(&camera_light->orientation.rows[0], &camera->last_player_vec) / camera->last_player_dist;

                        camera->cur_pitch += pitch_increment * 0.02;
                        camera->cur_yaw += yaw_increment * 0.02;

                        if(!player_lost)
                        {
                            camera->flags &= ~(G_CAMERA_FLAG_SEEK_LOST | G_CAMERA_FLAG_GAVE_UP | G_CAMERA_FLAG_PLAYER_HID);
                        }
                        else if(camera->flags & G_CAMERA_FLAG_SEEK_LOST)
                        {
                            camera->alert_timer -= delta_time;

                            if(camera->alert_timer <= 0.0)
                            {
                                camera->flags |= G_CAMERA_FLAG_GAVE_UP;
                                camera->flags &= ~G_CAMERA_FLAG_SEEK_LOST;
                                camera->alert_timer = G_CAMERA_ALERT_COOLDOWN;
                            }
                        }
                    }
                }
                break;
            }

            if(camera->cur_yaw > 1.0)
            {
                camera->cur_yaw = camera->cur_yaw - 2.0;
            }
            else if(camera->cur_yaw < -1.0)
            {
                camera->cur_yaw = 2.0 - camera->cur_yaw;
            }

            mat3_t spot_pitch = mat3_t_c_id();
            mat3_t spot_yaw = mat3_t_c_id();
            mat3_t_rotate_x(&spot_pitch, camera->cur_pitch);
            mat3_t_rotate_y(&spot_yaw, camera->cur_yaw);
            mat3_t_mul(&spot_yaw, &spot_pitch, &spot_yaw);
            mat3_t_mul(&camera_light->orientation, &camera_node->orientation, &spot_yaw);
            camera_light->position = camera_node->position;
            camera_light->color = g_camera_state_colors[camera->state];
        }
    }
}

void g_StepEnemies(float delta_time)
{
    g_StepCameras(delta_time);
}






