#include "dstuff/ds_slist.h"
#include "dstuff/ds_mem.h"
#include "dstuff/ds_file.h"
#include "dstuff/ds_vector.h"
#include "dstuff/ds_path.h"
#include "stb/stb_image.h"
#include "g_main.h"
#include "input.h"
#include "anim.h"
#include "phys.h"
#include "../editor/ed_main.h"
#include "ent.h"
#include "g_player.h"
#include "g_game.h"
#include "level.h"
#include "sound.h"
#include "gui.h"
#include <string.h>
#include <stdio.h>
#include <float.h>


extern mat4_t r_view_matrix;
extern mat4_t r_inv_view_matrix;
extern mat4_t r_view_projection_matrix;
uint32_t g_editor = 0;

//struct ds_slist_t g_entities;
//struct ds_list_t g_projectiles;
//struct ds_slist_t g_triggers;

#define G_CAMERA_Z 8.0
#define G_SCREEN_Y_OFFSET 20.0
#define G_CAMERA_PITCH (-0.05)
#define G_PLAYER_RIGHT_ANGLE 0.0
#define G_PLAYER_LEFT_ANGLE 1.0

vec3_t g_camera_pos = {.z = G_CAMERA_Z};
float g_camera_pitch = G_CAMERA_PITCH;
float g_camera_yaw = 0.0;

char g_base_path[PATH_MAX] = "";


//struct r_model_t *g_player_model;
//struct r_model_t *g_floor_tile_model;
//struct r_model_t *g_wall_tile_model;
//struct r_model_t *g_cube_model;
//struct r_model_t *g_wiggle_model;
//struct r_model_t *g_gun_model;
//struct r_model_t *g_boy_model;
//struct r_model_t *g_sponza_model;
//
//struct g_entity_t *wiggle_entity;
//struct g_entity_t *g_player_entity;
//struct g_entity_t *g_gun_entity;
//struct a_animation_t *g_run_animation;
//struct a_animation_t *g_idle_animation;
//struct a_animation_t *g_jump_animation;
//struct a_animation_t *g_fall_animation;
//struct a_animation_t *g_shoot_animation;
//struct a_animation_t *g_dance_animation;
//struct s_sound_t *g_jump_sound;
//struct s_sound_t *g_land_sound;
//struct s_sound_t *g_footstep_sounds[5];
//struct s_sound_t *g_ric_sounds[10];
//struct s_sound_t *g_shot_sound;
//struct r_light_t *g_player_light;
//struct r_light_t *g_lights[3];
//struct r_light_t *g_lights[8];
//uint32_t g_hook_index;
//mat4_t *g_hook_transform;

uint32_t g_game_state = G_GAME_STATE_LOADING;
//char *upper_body_bones[] =
//{
//    "head",
//    "neck",
//    "KTF.R",
//    "KTF.L",
//    "chest",
//    "hip",
//
//    "upperarm.R",
//    "upperarm.L",
//    "lowerarm.R",
//    "lowerarm.L",
//    "hand.R",
//    "hand.L",
//
//    "tooseup.R",
//    "toosemid.R",
//    "lowertoose.R",
//    "upfinger4.R",
//    "midfinger4.R",
//    "lowerfinger4.R",
//    "upfinger3.R",
//    "midfinger3.R",
//    "lowerfinger3.R",
//    "upfinger2.R",
//    "midfinger2.R",
//    "lowerfinger2.R",
//    "upfinger.R",
//    "midfinger.R",
//    "lowerfinger.R",
//
//    "tooseup.L",
//    "toosemid.L",
//    "lowertoose.L",
//    "upfinger4.L",
//    "midfinger4.L",
//    "lowerfinger4.L",
//    "upfinger3.L",
//    "midfinger3.L",
//    "lowerfinger3.L",
//    "upfinger2.L",
//    "midfinger2.L",
//    "lowerfinger2.L",
//    "upfinger.L",
//    "midfinger.L",
//    "lowerfinger.L",
//};
//uint32_t upper_body_bone_count = sizeof(upper_body_bones) / sizeof(upper_body_bones[0]);
//char *upper_body_players[] = {"shoot_player", "run_player", "jump_player", "idle_player", "fall_player"};
//
//char *lower_body_bones[] =
//{
//    "wiest",
//    "upperleg.R",
//    "upperleg.L",
//    "lowerleg.R",
//    "lowerleg.L",
//    "foot.R",
//    "foot.L",
//    "toose.R",
//    "toose.L",
//    "hellikk.R",
//    "hellikk.L",
//    "Bone",
//};
//uint32_t lower_body_bone_count = sizeof(lower_body_bones) / sizeof(lower_body_bones[0]);
//char *lower_body_players[] = {"run_player", "jump_player", "idle_player", "fall_player"};

//struct p_character_collider_t *character_collider;

extern struct r_renderer_state_t r_renderer_state;

//struct e_ent_def_t *g_ent_def;

//void g_TestCallback(void *data, float delta_time)
//{
//    struct g_entity_t *entity = (struct g_entity_t *)data;
//    struct p_movable_collider_t *collider = (struct p_movable_collider_t *)entity->collider;
//    if(collider->flags & P_COLLIDER_FLAG_ON_GROUND)
//    {
////        uint32_t index = rand() % 5;
//        s_PlaySound(g_footstep_sounds[0], &vec3_t_c(0.0, 0.0, 0.0), 1.0, 0);
//    }
//}

void g_Init(uint32_t editor_active)
{
    g_editor = editor_active;
//    g_entities = ds_slist_create(sizeof(struct g_entity_t), 512);
//    g_projectiles = ds_list_create(sizeof(struct g_projectile_t), 512);
//    r_SetViewPitchYaw(g_camera_pitch, g_camera_yaw);
//    r_SetViewPos(&g_camera_pos);
    g_GameInit();
    g_game_state = G_GAME_STATE_MAIN_MENU;

    if(g_editor)
    {
        ed_Init();
    }

//    g_cube_model = r_LoadModel("models/Cube.mof");
}

void g_Shutdown()
{

}

void g_SetGameState(uint32_t game_state)
{
    g_game_state = game_state;
}

void g_BeginGame()
{
    g_SetGameState(G_GAME_STATE_PLAYING);
}

void g_ResumeGame()
{
    g_SetGameState(G_GAME_STATE_PLAYING);
}

void g_PauseGame()
{
    in_SetMouseRelative(0);
    in_SetMouseWarp(0);

    if(g_editor)
    {
        g_StopGame();
    }
    else
    {
        g_SetGameState(G_GAME_STATE_PAUSED);
    }
}

void g_StopGame()
{
    g_SetGameState(G_GAME_STATE_MAIN_MENU);

    l_ClearLevel();

    if(g_editor)
    {
        ed_l_StopGame();
    }
    else
    {

    }
}

void g_GameMain(float delta_time)
{
    if(in_GetKeyState(SDL_SCANCODE_ESCAPE) & IN_KEY_STATE_PRESSED)
    {
        g_PauseGame();
    }
    else
    {
        g_StepGame(delta_time);
    }
}

void g_GamePaused()
{
    if(in_GetKeyState(SDL_SCANCODE_ESCAPE) & IN_KEY_STATE_PRESSED)
    {
        g_ResumeGame();
    }
}

void g_MainMenu()
{
    if(g_editor)
    {
        ed_UpdateEditor();
    }
    else
    {

    }
}

void g_SetBasePath(char *path)
{
    strcpy(g_base_path, path);
    printf("base path set to %s\n", g_base_path);
}

void g_ResourcePath(char *path, char *out_path, uint32_t out_size)
{
    ds_path_format_path(path, out_path, out_size);

    if(!ds_path_is_absolute(out_path))
    {
        ds_path_append_end(g_base_path, out_path, out_path, out_size);
    }
}

//struct g_projectile_t *g_SpawnProjectile(vec3_t *position, vec3_t *velocity, vec3_t *color, float radius, uint32_t life)
//{
//    uint32_t index;
//    struct g_projectile_t *projectile;
//
//    index = ds_list_add_element(&g_projectiles, NULL);
//    projectile = ds_list_get_element(&g_projectiles, index);
//
//    projectile->position = *position;
//    projectile->velocity = *velocity;
//    projectile->life = life;
//    projectile->light = r_CreateLight(R_LIGHT_TYPE_POINT, position, color, radius, 20.0);
//    projectile->bouces = 0;
//
//    return projectile;
//}
//
//void g_DestroyProjectile(struct g_projectile_t *projectile)
//{
//
//}

//void g_PlayAnimation(struct g_entity_t *entity, struct a_animation_t *animation, char *player_name)
//{
////    if(!entity->mixer)
////    {
////        entity->model = r_ShallowCopyModel(entity->model);
////        entity->mixer = a_CreateMixer(entity->model);
//////        entity->item->model = entity->model;
////    }
////
////    a_MixAnimation(entity->mixer, animation, player_name);
//}

//struct g_trigger_t *g_CreateTrigger(vec3_t *position, vec3_t *size, thinker_t *thinker)
//{
//    struct g_trigger_t *trigger;
//    uint32_t trigger_index = add_stack_list_element(&g_triggers, NULL);
//    trigger = get_stack_list_element(&g_triggers, trigger_index);
//    trigger->index = trigger_index;
//    trigger->thinker = thinker;
//    trigger->collider = p_CreateCollider(P_COLLIDER_TYPE_TRIGGER, position, size);
//
//    return trigger;
//}

//void *g_GetProp(struct g_entity_t *entity, char *prop_name)
//{
////    for(uint32_t prop_index = 0; prop_index < entity->props.cursor; prop_index++)
////    {
////        struct g_prop_t *prop = ds_list_get_element(&entity->props, prop_index);
////        if(!strcmp(prop->name, prop_name))
////        {
////            return prop->data;
////        }
////    }
////
////    return NULL;
//}

//void *g_SetProp(struct g_entity_t *entity, char *prop_name, uint32_t size, void *data)
//{
////    struct g_prop_t *prop = g_GetProp(entity, prop_name);
////
////    if(!entity->props.buffers)
////    {
////        entity->props = ds_list_create(sizeof(struct g_prop_t), 8);
////    }
////
////    if(!prop)
////    {
////        prop = ds_list_get_element(&entity->props, ds_list_add_element(&entity->props, NULL));
////        prop->name = strdup(prop_name);
////    }
////
////    if(prop->size < size)
////    {
////        prop->data = mem_Realloc(prop->data, size);
////        prop->size = size;
////    }
////
////    memcpy(prop->data, data, size);
////
////    return prop->data;
//}

//void g_RemoveProp(struct g_entity_t *entity, char *prop_name)
//{
////    for(uint32_t prop_index = 0; prop_index < entity->props.cursor; prop_index++)
////    {
////        struct g_prop_t *prop = ds_list_get_element(&entity->props, prop_index);
////        if(!strcmp(prop->name, prop_name))
////        {
////            mem_Free(prop->name);
////            mem_Free(prop->data);
////            ds_list_remove_element(&entity->props, prop_index);
////            return;
////        }
////    }
//}

//void g_PlayerThinker(struct g_entity_t *entity)
//{
//    vec3_t disp = {};
//    vec4_t collider_disp = {};
//    float rotation = 0.0;
//    int32_t move_dir = 0;
//    static float zoom = 0.0;
////    uint32_t moving = 0;
//
//
////    struct p_movable_collider_t *collider = (struct p_movable_collider_t *)entity->collider;
////    struct g_player_state_t *player_state;
////    struct a_mask_t *upper_body_mask = a_GetAnimationMask(entity->mixer, "upper_body");
////    struct a_mask_t *lower_body_mask = a_GetAnimationMask(entity->mixer, "lower_body");
////
////
////    struct a_mask_player_t *upper_shoot_player = a_GetMaskPlayer(upper_body_mask, "shoot_player");
////    struct a_mask_player_t *upper_run_player = a_GetMaskPlayer(upper_body_mask, "run_player");
////    struct a_mask_player_t *upper_idle_player = a_GetMaskPlayer(upper_body_mask, "idle_player");
////    struct a_mask_player_t *upper_jump_player = a_GetMaskPlayer(upper_body_mask, "jump_player");
////    struct a_mask_player_t *upper_fall_player = a_GetMaskPlayer(upper_body_mask, "fall_player");
////
////    struct a_mask_player_t *lower_run_player = a_GetMaskPlayer(lower_body_mask, "run_player");
////    struct a_mask_player_t *lower_idle_player = a_GetMaskPlayer(lower_body_mask, "idle_player");
////    struct a_mask_player_t *lower_jump_player = a_GetMaskPlayer(lower_body_mask, "jump_player");
////    struct a_mask_player_t *lower_fall_player = a_GetMaskPlayer(lower_body_mask, "fall_player");
////    struct a_player_t *lower_shoot_player = a_GetMixerPlayer(entity->mixer, "shoot_player");
//
////    struct a_player_t *run_player = a_GetMixerPlayer(entity->mixer, "run_player");
////    struct a_player_t *idle_player = a_GetMixerPlayer(entity->mixer, "idle_player");
////    struct a_player_t *jump_player = a_GetMixerPlayer(entity->mixer, "jump_player");
////    struct a_player_t *fall_player = a_GetMixerPlayer(entity->mixer, "fall_player");
////    struct a_player_t *shoot_player = a_GetMixerPlayer(entity->mixer, "shoot_player");
////
////    if(in_GetKeyState(SDL_SCANCODE_ESCAPE) & IN_KEY_STATE_PRESSED)
////    {
////        g_SetGameState(G_GAME_STATE_EDITING);
////    }
////
////    player_state = g_GetProp(entity, "player_state");
////    if(!player_state)
////    {
////        player_state = g_SetProp(entity, "player_state", sizeof(struct g_player_state_t), &(struct g_player_state_t){});
////        player_state->run_scale = 1.0;
////        player_state->shoot_frac = 0.0;
////    }
//
////    if(in_GetKeyState(SDL_SCANCODE_A) & IN_KEY_STATE_PRESSED)
////    {
////        move_dir--;
////    }
////    if(in_GetKeyState(SDL_SCANCODE_D) & IN_KEY_STATE_PRESSED)
////    {
////        move_dir++;
////    }
////
////    if(move_dir > 0)
////    {
////        if(player_state->direction == G_PLAYER_LEFT_ANGLE)
////        {
////            player_state->flags |= G_PLAYER_FLAG_TURNING;
////            player_state->run_frac = 0.0;
////        }
////    }
////    else if(move_dir < 0)
////    {
////        if(player_state->direction == G_PLAYER_RIGHT_ANGLE)
////        {
////            player_state->flags |= G_PLAYER_FLAG_TURNING | G_PLAYER_FLAG_TURNING_LEFT;
////            player_state->run_frac = 0.0;
////        }
////    }
////    else
////    {
////        collider->disp.x = 0.0;
////        player_state->run_frac = 0.0;
////    }
////
////    if(player_state->flags & G_PLAYER_FLAG_TURNING)
////    {
////        if(player_state->flags & G_PLAYER_FLAG_TURNING_LEFT)
////        {
////            rotation = 0.1;
////            player_state->direction += 0.1;
////            if(player_state->direction >= 1.0)
////            {
////                player_state->flags &= ~(G_PLAYER_FLAG_TURNING | G_PLAYER_FLAG_TURNING_LEFT);
////                player_state->direction = 1.0;
////            }
////        }
////        else
////        {
////            rotation = -0.1;
////            player_state->direction -= 0.1;
////            if(player_state->direction <= 0.0)
////            {
////                player_state->flags &= ~G_PLAYER_FLAG_TURNING;
////                player_state->direction = 0.0;
////            }
////        }
////    }
////    else if(move_dir != 0)
////    {
////        collider_disp.x = -entity->mixer->root_disp.y * SCALE;
////        player_state->run_frac += 0.1;
////    }
////
////    if(in_GetKeyState(SDL_SCANCODE_SPACE) & IN_KEY_STATE_PRESSED)
////    {
////        if(in_GetKeyState(SDL_SCANCODE_SPACE) & IN_KEY_STATE_JUST_PRESSED && collider->flags & P_COLLIDER_FLAG_ON_GROUND)
////        {
////            player_state->flags |= G_PlAYER_FLAG_JUMPING;
////            player_state->jump_y = collider->position.y;
////            player_state->jump_disp = 0.2;
////            player_state->jump_frac = 0.0;
////            s_PlaySound(g_jump_sound, &vec3_t_c(0.0, 0.0, 0.0), 0.6, 0);
////        }
////    }
////    else
////    {
////        player_state->flags &= ~G_PlAYER_FLAG_JUMPING;
////    }
////
////    if(player_state->jump_disp > 0.0)
////    {
////        if(collider->flags & P_COLLIDER_FLAG_TOP_COLLIDED)
////        {
////            player_state->jump_disp = 0.0;
////            player_state->flags &= ~G_PlAYER_FLAG_JUMPING;
////        }
////        else
////        {
////            if(player_state->flags & G_PlAYER_FLAG_JUMPING)
////            {
////                if(collider->position.y - player_state->jump_y > 3.5)
////                {
////                    player_state->jump_disp *= 0.9;
////                }
////            }
////            else
////            {
////                player_state->jump_disp *= 0.5;
////            }
////
////            if(player_state->jump_disp < 0.04)
////            {
////                player_state->flags &= ~G_PlAYER_FLAG_JUMPING;
////                player_state->jump_disp = 0.0;
////            }
////
////            collider->disp.y = player_state->jump_disp;
////        }
////    }
////    else
////    {
////        collider->disp.y -= 0.01;
////    }
////
////    if(player_state->run_frac > 1.0)
////    {
////        player_state->run_frac = 1.0;
////    }
////
////    if(player_state->flags & G_PlAYER_FLAG_JUMPING)
////    {
////        player_state->jump_frac += 0.12;
////        if(player_state->jump_frac > 0.99)
////        {
////            player_state->jump_frac = 0.99;
////        }
////
////        lower_run_player->weight = 0.0;
////        upper_run_player->weight = 0.0;
////
////        lower_idle_player->player->scale = 0.0;
////        lower_idle_player->weight = 0.0;
////        upper_idle_player->weight = 0.0;
////
////        upper_jump_player->weight = 1.0;
////        lower_jump_player->weight = 1.0;
////        lower_jump_player->player->scale = 0.0;
////
////        upper_fall_player->weight = 0.0;
////        lower_fall_player->weight = 0.0;
////        lower_fall_player->player->scale = 0.0;
////
////        a_SeekAnimationRelative(lower_jump_player->player, player_state->jump_frac);
////    }
////    else
////    {
////        if(collider->flags & P_COLLIDER_FLAG_ON_GROUND)
////        {
////            if(!(player_state->collider_flags & P_COLLIDER_FLAG_ON_GROUND))
////            {
////                s_PlaySound(g_footstep_sounds[0], &vec3_t_c(0.0, 0.0, 0.0), 1.0, 0);
////            }
////            lower_run_player->weight = player_state->run_frac;
////            upper_run_player->weight = player_state->run_frac;
////
////            lower_idle_player->player->scale = 0.6 * (1.0 - player_state->run_frac);
////            lower_idle_player->weight = 1.0 - player_state->run_frac;
////            upper_idle_player->weight = 1.0 - player_state->run_frac;
////
////            lower_jump_player->weight = 0.0;
////            upper_jump_player->weight = 0.0;
////
////            lower_fall_player->weight = 0.0;
////            upper_fall_player->weight = 0.0;
////
////            player_state->jump_frac = 1.0;
////        }
////        else
////        {
////            lower_jump_player->weight = 0.0;
////            upper_jump_player->weight = 0.0;
////
////            lower_fall_player->weight = 1.0;
////            upper_fall_player->weight = 1.0;
////
////            lower_run_player->weight = 0.0;
////            upper_run_player->weight = 0.0;
////
////            lower_idle_player->weight = 0.0;
////            upper_idle_player->weight = 0.0;
////
////            player_state->jump_frac -= 0.05;
////            if(player_state->jump_frac < 0.0)
////            {
////                player_state->jump_frac = 0.0;
////            }
////
////            a_SeekAnimationRelative(lower_fall_player->player, (1.0 - player_state->jump_frac) * 0.999);
////        }
////    }
////
////    if(in_GetKeyState(SDL_SCANCODE_KP_MINUS) & IN_KEY_STATE_PRESSED)
////    {
////        zoom += 0.003;
////        if(zoom > 0.3)
////        {
////            zoom = 0.3;
////        }
////    }
////    else if(in_GetKeyState(SDL_SCANCODE_KP_PLUS) & IN_KEY_STATE_PRESSED)
////    {
////        zoom -= 0.003;
////        if(zoom < -0.3)
////        {
////            zoom = -0.3;
////        }
////    }
////    else
////    {
////        zoom *= 0.9;
////    }
////
////
////    player_state->collider_flags = collider->flags;
////    lower_run_player->player->scale = 1.9 * player_state->run_frac * player_state->run_scale;
////    upper_run_player->player->scale = 1.9 * player_state->run_frac * player_state->run_scale;
////
////    mat4_t yaw_matrix;
////    mat4_t_identity(&yaw_matrix);
////    mat4_t_rotate_z(&yaw_matrix, player_state->direction);
////
////    mat4_t_vec4_t_mul(&collider_disp, &yaw_matrix, &collider_disp);
////    mat4_t_identity(&yaw_matrix);
////    mat4_t_rotate_z(&yaw_matrix, rotation);
////
////    mat4_t_mul(&entity->local_transform, &yaw_matrix, &entity->local_transform);
////    collider->disp.x = collider_disp.x;
////
////    vec4_t player_pos = entity->local_transform.rows[3];
////    mat4_t_identity(&g_gun_entity->local_transform);
////    g_gun_entity->local_transform.rows[3].z = 0.1;
////    g_gun_entity->local_transform.rows[3].x = -0.85;
////    mat4_t_rotate_x(&g_gun_entity->local_transform, -0.5);
////    mat4_t_rotate_z(&g_gun_entity->local_transform, -0.5);
////    mat4_t_rotate_y(&g_gun_entity->local_transform, -0.5);
////    mat4_t_mul(&g_gun_entity->local_transform, &g_gun_entity->local_transform, g_hook_transform);
////    mat4_t_mul(&g_gun_entity->local_transform, &g_gun_entity->local_transform, &entity->local_transform);
////
////    vec4_t spawn_point = vec4_t_c(0.0, -2.5, 0.5, 1.0);
////    mat4_t_vec4_t_mul(&spawn_point, &entity->local_transform, &spawn_point);
////
////    float rx = (((float)(rand() % 513) / 512.0) * 2.0 - 1.0) * 0.35;
////
////    vec4_t spawn_direction = vec4_t_c(0.0, -16.0, rx, 0.0);
////    mat4_t_vec4_t_mul(&spawn_direction, &entity->local_transform, &spawn_direction);
////
////    if((in_GetKeyState(SDL_SCANCODE_C) & IN_KEY_STATE_PRESSED) && !player_state->shoot_frac)
////    {
////        player_state->shoot_frac = 0.1;
////        vec3_t color;
////
////        color.x = (float)(rand() % 513) / 512.0;
////        color.y = (float)(rand() % 513) / 512.0;
////        color.z = (float)(rand() % 513) / 512.0;
////
////        vec3_t_normalize(&color, &color);
////
////        s_PlaySound(g_shot_sound, &vec3_t_c(0.0, 0.0, 0.0), 0.6, 0);
////        g_SpawnProjectile(&vec3_t_c(spawn_point.x, spawn_point.y, spawn_point.z),
////                          &vec3_t_c(spawn_direction.x, spawn_direction.y, spawn_direction.z), &color, 4.0, 480);
////
////        player_state->shot_light = r_CreateLight(R_LIGHT_TYPE_POINT, &vec3_t_c(spawn_point.x, spawn_point.y, spawn_point.z), &color, 8.0, 60.0);
////    }
////
////    if(player_state->shoot_frac)
////    {
////        if(player_state->shot_light && player_state->shoot_frac > 0.2)
////        {
////            r_DestroyLight(player_state->shot_light);
////            player_state->shot_light = NULL;
////        }
////
////        uint32_t stop = 0;
////        player_state->shoot_frac += 0.20;
////        if(player_state->shoot_frac > 0.9999)
////        {
////            player_state->shoot_frac = 0.9999;
////            stop = 1;
////        }
////
////        upper_shoot_player->weight = 1.0;
////        upper_idle_player->weight = 0.0;
////        upper_run_player->weight = 0.0;
////        upper_jump_player->weight = 0.0;
////        upper_fall_player->weight = 0.0;
////        a_SeekAnimationRelative(upper_shoot_player->player, player_state->shoot_frac);
////
////        if(stop)
////        {
////            player_state->shoot_frac = 0.0;
////        }
////    }
////    else
////    {
////        upper_shoot_player->weight = 0.0;
////    }
//
////    collider->disp.x = 0.0;
////    collider->disp.y = 0.0;
////    collider->disp.z = 0.0;
//
//    if(in_GetKeyState(SDL_SCANCODE_A) & IN_KEY_STATE_PRESSED)
//    {
////        collider->disp.x = -0.03;
//    }
//    else if(in_GetKeyState(SDL_SCANCODE_D) & IN_KEY_STATE_PRESSED)
//    {
////        collider->disp.x = 0.03;
//    }
//
//
//    if(in_GetKeyState(SDL_SCANCODE_W) & IN_KEY_STATE_PRESSED)
//    {
////        collider->disp.y = 0.03;
//    }
//    else if(in_GetKeyState(SDL_SCANCODE_S) & IN_KEY_STATE_PRESSED)
//    {
////        collider->disp.y = -0.03;
//    }
//
//    if(in_GetKeyState(SDL_SCANCODE_E) & IN_KEY_STATE_PRESSED)
//    {
////        collider->disp.z = 0.03;
//    }
//    else if(in_GetKeyState(SDL_SCANCODE_R) & IN_KEY_STATE_PRESSED)
//    {
////        collider->disp.z = -0.03;
//    }
//
////    vec4_t light_pos = entity->local_transform.rows[3];
////    g_player_light->data.pos_rad.x = light_pos.x;
////    g_player_light->data.pos_rad.y = light_pos.y;
////    g_player_light->data.pos_rad.z = light_pos.z + 1.5;
//
////    mat4_t_vec4_t_mul_fast(&player_pos, &r_inv_view_matrix, &player_pos);
////    player_pos.w = 0.0;
////    player_pos.z = 0.0;
////    player_pos.y += G_SCREEN_Y_OFFSET * 0.05;
////    mat4_t_vec4_t_mul_fast(&player_pos, &r_view_matrix, &player_pos);
////    vec3_t_add(&g_camera_pos, &g_camera_pos, &vec3_t_c(player_pos.x * 0.1, player_pos.y * 0.1, zoom));
////    r_SetViewPos(&g_camera_pos);
////    r_SetViewPitchYaw(g_camera_pitch, g_camera_yaw);
//}

//void g_ElevatorThinker(struct g_entity_t *entity)
//{
////    uint32_t *state = g_GetProp(entity, "state");
////
////    if(entity->collider->position.y >= 10.0)
////    {
////        *state = 1;
//////        p_DisplaceCollider(entity->collider, &vec3_t_c(0.0, -0.1, 0.0));
////    }
////    else if(entity->collider->position.y <= 1.0)
////    {
////        *state = 0;
//////        p_DisplaceCollider(entity->collider, &vec3_t_c(0.0, -0.1, 0.0));
////    }
////
////    if(*state)
////    {
////        p_DisplaceCollider(entity->collider, &vec3_t_c(0.0, -0.05, 0.0));
////    }
////    else
////    {
////        p_DisplaceCollider(entity->collider, &vec3_t_c(0.0, 0.05, 0.0));
////    }
//}

//void g_TriggerThinker(struct g_entity_t *trigger)
//{
////    struct p_trigger_collider_t *collider = (struct p_trigger_collider_t *)trigger->collider;
////    if(collider->collision_count)
////    {
////        for(uint32_t collision_index = 0; collision_index < collider->collision_count; collision_index++)
////        {
////            struct p_collider_t *collision = p_GetCollision((struct p_collider_t *)collider, collision_index);
////            if(collision->user_data)
////            {
////                struct g_entity_t *player = (struct g_entity_t *)collision->user_data;
////
////                if(g_GetProp(player, "the_player"))
////                {
////                    printf("touching player!\n");
////                }
////            }
////        }
////    }
////    else
////    {
////        printf("nah...\n");
////    }
//}




