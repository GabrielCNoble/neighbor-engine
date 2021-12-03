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

uint64_t g_counter_frequency;
uint64_t g_cur_counter;
uint64_t g_prev_counter;
float g_frame_delta;

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

void g_UpdateDeltaTime()
{
    g_counter_frequency = SDL_GetPerformanceFrequency();
    g_prev_counter = g_cur_counter;
    g_cur_counter = SDL_GetPerformanceCounter();
    g_frame_delta = (float)(g_cur_counter - g_prev_counter) / (float)g_counter_frequency;
}

float g_GetDeltaTime()
{
    uint64_t counter_frequency = SDL_GetPerformanceFrequency();
    uint64_t cur_counter = SDL_GetPerformanceCounter();
    return (float)(cur_counter - g_cur_counter) / (float)counter_frequency;
}
