#include "dstuff/ds_stack_list.h"
#include "dstuff/ds_mem.h"
#include "dstuff/ds_file.h"
#include "dstuff/ds_vector.h"
#include "stb/stb_image.h"
#include "game.h"
#include "input.h"
#include "anim.h"
#include "physics.h"
#include "editor.h"
#include "world.h"
#include "sound.h"
#include <string.h>
#include <stdio.h>


extern mat4_t r_view_matrix;
extern mat4_t r_inv_view_matrix;
extern mat4_t r_view_projection_matrix;

struct stack_list_t g_entities;
struct list_t g_projectiles;
struct stack_list_t g_triggers;

//float g_camera_z = G_PLAYER_AREA_Z;

#define G_CAMERA_Z 8.0
#define G_SCREEN_Y_OFFSET 20.0
#define G_CAMERA_PITCH (-0.05)
#define G_PLAYER_RIGHT_ANGLE 0.0
#define G_PLAYER_LEFT_ANGLE 1.0

vec3_t g_camera_pos = {.z = G_CAMERA_Z};
float g_camera_pitch = G_CAMERA_PITCH;
float g_camera_yaw = 0.0;


struct r_model_t *g_player_model;
struct r_model_t *g_floor_tile_model;
struct r_model_t *g_wall_tile_model;
struct r_model_t *g_cube_model;
struct r_model_t *g_wiggle_model;
struct r_model_t *g_gun_model;

struct g_entity_t *wiggle_entity;
struct g_entity_t *g_player_entity;
struct g_entity_t *g_gun_entity;
struct a_animation_t *g_run_animation;
struct a_animation_t *g_idle_animation;
struct a_animation_t *g_jump_animation;
struct a_animation_t *g_fall_animation;
struct a_animation_t *g_shoot_animation;
struct s_sound_t *g_jump_sound;
struct s_sound_t *g_land_sound;
struct s_sound_t *g_footstep_sounds[5];
struct s_sound_t *g_ric_sounds[10];
struct s_sound_t *g_shot_sound;
struct r_light_t *g_player_light;
uint32_t g_hook_index;
mat4_t *g_hook_transform;

uint32_t g_game_state = G_GAME_STATE_LOADING;
char *upper_body_bones[] = 
{
    "head",
    "neck",
    "KTF.R",
    "KTF.L",
    "chest",
    "hip",
    
    "upperarm.R",
    "upperarm.L",
    "lowerarm.R",
    "lowerarm.L",
    "hand.R",
    "hand.L",
    
    "tooseup.R",
    "toosemid.R",
    "lowertoose.R",
    "upfinger4.R",
    "midfinger4.R",
    "lowerfinger4.R",
    "upfinger3.R",
    "midfinger3.R",
    "lowerfinger3.R",
    "upfinger2.R",
    "midfinger2.R",
    "lowerfinger2.R",
    "upfinger.R",
    "midfinger.R",
    "lowerfinger.R",
    
    "tooseup.L",
    "toosemid.L",
    "lowertoose.L",
    "upfinger4.L",
    "midfinger4.L",
    "lowerfinger4.L",
    "upfinger3.L",
    "midfinger3.L",
    "lowerfinger3.L",
    "upfinger2.L",
    "midfinger2.L",
    "lowerfinger2.L",
    "upfinger.L",
    "midfinger.L",
    "lowerfinger.L",
};
uint32_t upper_body_bone_count = sizeof(upper_body_bones) / sizeof(upper_body_bones[0]);
char *upper_body_players[] = {"shoot_player", "run_player", "jump_player", "idle_player", "fall_player"};

char *lower_body_bones[] = 
{
    "wiest",
    "upperleg.R",
    "upperleg.L",
    "lowerleg.R",
    "lowerleg.L",
    "foot.R",
    "foot.L",
    "toose.R",
    "toose.L",
    "hellikk.R",
    "hellikk.L",
    "Bone",
};
uint32_t lower_body_bone_count = sizeof(lower_body_bones) / sizeof(lower_body_bones[0]);
char *lower_body_players[] = {"run_player", "jump_player", "idle_player", "fall_player"};

extern uint32_t r_use_z_prepass;

void g_TestCallback(void *data, float delta_time)
{
    struct g_entity_t *entity = (struct g_entity_t *)data;
    struct p_movable_collider_t *collider = (struct p_movable_collider_t *)entity->collider;
    if(collider->flags & P_COLLIDER_FLAG_ON_GROUND)
    {
//        uint32_t index = rand() % 5;
        s_PlaySound(g_footstep_sounds[0], &vec3_t_c(0.0, 0.0, 0.0), 1.0, 0);
    }
}

void g_Init(uint32_t editor_active)
{
    r_Init();
    p_Init();
    a_Init();
    in_Input();
    w_Init();
    s_Init();
    
    if(editor_active)
    {
        ed_Init();
    }
    
    g_entities = create_stack_list(sizeof(struct g_entity_t), 512);
    g_projectiles = create_list(sizeof(struct g_projectile_t), 512);
    r_SetViewPitchYaw(g_camera_pitch, g_camera_yaw);
    r_SetViewPos(&g_camera_pos);
    
//    g_player_model = r_LoadModel("models/tall_box2.mof");
    g_gun_model = r_LoadModel("models/shocksplinter.mof");
//    g_floor_tile_model = r_LoadModel("models/floor_tile.mof");
//    g_wall_tile_model = r_LoadModel("models/wall_tile.mof");
    g_wiggle_model = r_LoadModel("models/dude.mof");
    g_cube_model = r_LoadModel("models/cube.mof");
    g_run_animation = a_LoadAnimation("models/run.anf");
    g_idle_animation = a_LoadAnimation("models/idle.anf");
    g_jump_animation = a_LoadAnimation("models/jump.anf");
    g_fall_animation = a_LoadAnimation("models/fall.anf");
    g_shoot_animation = a_LoadAnimation("models/shoot.anf");
    
    
//    struct s_sound_t *sound = s_LoadSound("sounds/fall2.ogg");
    g_footstep_sounds[0] = s_LoadSound("sounds/step2.ogg");
    g_jump_sound = s_LoadSound("sounds/jump.ogg");
    g_land_sound = s_LoadSound("sounds/fall.ogg");
    g_shot_sound = s_LoadSound("sounds/shot.ogg");
    
    g_ric_sounds[0] = s_LoadSound("sounds/ric0.ogg");
    g_ric_sounds[1] = s_LoadSound("sounds/ric1.ogg");
    g_ric_sounds[2] = s_LoadSound("sounds/ric2.ogg");
    g_ric_sounds[3] = s_LoadSound("sounds/ric3.ogg");
    g_ric_sounds[4] = s_LoadSound("sounds/ric4.ogg");
    g_ric_sounds[5] = s_LoadSound("sounds/ric5.ogg");
    g_ric_sounds[6] = s_LoadSound("sounds/ric6.ogg");
    g_ric_sounds[7] = s_LoadSound("sounds/ric7.ogg");
    g_ric_sounds[8] = s_LoadSound("sounds/ric8.ogg");
    g_ric_sounds[9] = s_LoadSound("sounds/ric9.ogg");
    
//    g_footstep_sounds[1] = s_LoadSound("sounds/pl_tile2.ogg");
//    g_footstep_sounds[2] = s_LoadSound("sounds/pl_tile3.ogg");
//    g_footstep_sounds[3] = s_LoadSound("sounds/pl_tile4.ogg");
//    g_footstep_sounds[4] = s_LoadSound("sounds/pl_tile5.ogg");
//    s_PlaySound(sound, &vec3_t_c(10.0, 0.0, 0.0), 0.5, 0);
    
    mat4_t transform;
    mat4_t_identity(&transform);
    mat4_t_rotate_x(&transform, -0.5);
    mat4_t_rotate_y(&transform, 0.5);
    #define SCALE 0.45
    transform.rows[0].x *= SCALE;
    transform.rows[0].y *= SCALE;
    transform.rows[0].z *= SCALE;
    
    transform.rows[1].x *= SCALE;
    transform.rows[1].y *= SCALE;
    transform.rows[1].z *= SCALE;
    
    transform.rows[2].x *= SCALE;
    transform.rows[2].y *= SCALE;
    transform.rows[2].z *= SCALE;
    transform.rows[3] = vec4_t_c(8.0, -3.0, 0.0, 1.0);
    
    g_player_entity = g_CreateEntity(&transform, g_PlayerThinker, g_wiggle_model);
    g_SetEntityCollider(g_player_entity, P_COLLIDER_TYPE_MOVABLE, &vec3_t_c(1.0, 2.0, 1.0));
    g_PlayAnimation(g_player_entity, g_run_animation, "run_player");
    g_PlayAnimation(g_player_entity, g_idle_animation, "idle_player");
    g_PlayAnimation(g_player_entity, g_jump_animation, "jump_player");
    g_PlayAnimation(g_player_entity, g_fall_animation, "fall_player");
    g_PlayAnimation(g_player_entity, g_shoot_animation, "shoot_player");
    g_player_entity->mixer->flags |= A_MIXER_FLAG_COMPUTE_ROOT_DISPLACEMENT;
//    struct a_player_t *player = a_GetMixerPlayer(g_player_entity->mixer, "run_player");
    a_CreateAnimationMask(g_player_entity->mixer, "lower_body", 4, lower_body_players, lower_body_bone_count, lower_body_bones);
    struct a_mask_t *mask = a_CreateAnimationMask(g_player_entity->mixer, "upper_body", 5, upper_body_players, upper_body_bone_count, upper_body_bones);
    struct a_mask_player_t *player = a_GetMaskPlayer(mask, "run_player");
    player->weight = 0.0;
    player = a_GetMaskPlayer(mask, "idle_player");
    player->weight = 0.0;
    player = a_GetMaskPlayer(mask, "jump_player");
    player->weight = 0.0;
    player = a_GetMaskPlayer(mask, "fall_player");
    player->weight = 0.0;
    
    mask = a_GetAnimationMask(g_player_entity->mixer, "lower_body");
    player = a_GetMaskPlayer(mask, "run_player");
    a_SetCallbackFrame(player->player, g_TestCallback, g_player_entity, 8);
    a_SetCallbackFrame(player->player, g_TestCallback, g_player_entity, 20);
    player->weight = 0.0;
    player = a_GetMaskPlayer(mask, "idle_player");
    player->weight = 0.0;
    player = a_GetMaskPlayer(mask, "jump_player");
    player->weight = 0.0;
    player = a_GetMaskPlayer(mask, "fall_player");
    player->weight = 0.0;
    
//    struct a_mask_player_t *player = a_GetMaskPlayer(mask, "shoot_player");
//    player->player->scale = 0.79;
//    player->weight = 0.2;
    
    
    mat4_t_identity(&transform);
    transform.rows[0].x *= 0.5;
    transform.rows[1].y *= 0.5;
    transform.rows[2].z *= 0.5;
    mat4_t_rotate_x(&transform, -0.5);
    mat4_t_rotate_y(&transform, 0.5);
    g_gun_entity = g_CreateEntity(&transform, NULL, g_gun_model);
    
    mat4_t_identity(&transform);
    transform.rows[0].x *= 2.0;
    transform.rows[1].y *= 2.0;
    transform.rows[2].z *= 2.0;
    mat4_t_rotate_z(&transform, 0.2);
    transform.rows[3].y = 0.0;
    transform.rows[3].x = 5.8;
    
    struct g_entity_t *rotated_entity = g_CreateEntity(&transform, NULL, g_cube_model);
    g_SetEntityCollider(rotated_entity, P_COLLIDER_TYPE_STATIC, &vec3_t_c(4.0, 4.0, 4.0));
    mat3_t_rotate_z(&rotated_entity->collider->orientation, 0.2);
    p_UpdateColliderNode(rotated_entity->collider);
    
//    player = a_GetPlayer(g_player_entity->mixer, 2);
//    player->scale = 0.0;
//    player = a_GetPlayer(g_player_entity->mixer, 3);
//    player->scale = 0.0;


//    player = a_GetPlayer(player_entity->mixer, 0);
//    player->scale = 0.0;
    
    
//    mat4_t_identity(&transform);
//    mat4_t_rotate_y(&transform, 0.5);
//    transform.rows[3] = vec4_t_c(4.0, 1.0, 0.0, 1.0);
//    wiggle_entity = g_CreateEntity(&transform, NULL, g_wiggle_model);
//    g_PlayAnimation(wiggle_entity, wiggle_animation);
//    struct a_player_t *player = *(struct a_player_t **)get_list_element(&wiggle_entity->mixer->players, 0);
//    wiggle_entity->mixer->flags |= A_MIXER_FLAG_COMPUTE_ROOT_DISPLACEMENT;
//    player->scale = 0.0;
    
    
    g_LoadMap("map8.png");
    
    g_game_state = G_GAME_STATE_EDITING;
    
//    for(uint32_t y = 0; y < 11; y++)
//    {
//        for(uint32_t x = 0; x < 75; x++)
//        {
//            r_CreateLight(R_LIGHT_TYPE_POINT, &vec3_t_c(x * 8.0, -8.0, -0.6), &vec3_t_c(1.0, 0.3, 0.3), 8.0, 5.0);
//        }
//    }
    
//    r_CreateLight(R_LIGHT_TYPE_POINT, &vec3_t_c(4.0, -0.3, -2.0), &vec3_t_c(1.0, 1.0, 1.0), 6.0, 5.0);
    
//    
//    r_CreateLight(R_LIGHT_TYPE_POINT, &vec3_t_c(0.0,  2.5, 0.5), &vec3_t_c(1.0, 1.0, 1.0), 4.0, 5.0);
//    r_CreateLight(R_LIGHT_TYPE_POINT, &vec3_t_c(4.0,  2.5, 0.5), &vec3_t_c(1.0, 0.0, 0.0), 4.0, 5.0);
//    r_CreateLight(R_LIGHT_TYPE_POINT, &vec3_t_c(8.0,  2.5, 0.5), &vec3_t_c(0.0, 1.0, 0.0), 4.0, 5.0);
//    r_CreateLight(R_LIGHT_TYPE_POINT, &vec3_t_c(12.0, 2.5, 0.5), &vec3_t_c(0.0, 0.3, 1.0), 4.0, 5.0);
////    
//    r_CreateLight(R_LIGHT_TYPE_POINT, &vec3_t_c(16.0, 2.5, 0.5), &vec3_t_c(1.0, 1.0, 0.3), 4.0, 5.0);
//    r_CreateLight(R_LIGHT_TYPE_POINT, &vec3_t_c(20.0, 2.5, 0.5), &vec3_t_c(1.0, 0.3, 1.0), 4.0, 5.0);
//    r_CreateLight(R_LIGHT_TYPE_POINT, &vec3_t_c(24.0, 2.5, 0.5), &vec3_t_c(0.3, 1.0, 1.0), 4.0, 5.0);
//    
//    r_CreateLight(R_LIGHT_TYPE_POINT, &vec3_t_c(28.0, 2.5, 0.5), &vec3_t_c(1.0, 0.5, 0.3), 4.0, 5.0);
//    r_CreateLight(R_LIGHT_TYPE_POINT, &vec3_t_c(32.0, 2.5, 0.5), &vec3_t_c(1.0, 0.3, 0.5), 4.0, 5.0);
//    r_CreateLight(R_LIGHT_TYPE_POINT, &vec3_t_c(36.0, 2.5, 0.5), &vec3_t_c(0.3, 1.0, 0.5), 4.0, 5.0);
    
    
//    r_CreateLight(R_LIGHT_TYPE_POINT, &vec3_t_c(24.0, -7.3, 0.5), &vec3_t_c(0.3, 1.0, 0.3), 5.0, 5.0);
//    r_CreateLight(R_LIGHT_TYPE_POINT, &vec3_t_c(20.0, -7.3, 0.5), &vec3_t_c(0.3, 0.3, 1.0), 5.0, 5.0);
//    r_CreateLight(R_LIGHT_TYPE_POINT, &vec3_t_c(16.0, -7.3, 0.5), &vec3_t_c(1.0, 0.3, 0.3), 5.0, 5.0);
//    
//    r_CreateLight(R_LIGHT_TYPE_POINT, &vec3_t_c(12.0, -7.3, 0.5), &vec3_t_c(1.0, 0.2, 0.0), 5.0, 5.0);
//    r_CreateLight(R_LIGHT_TYPE_POINT, &vec3_t_c(8.0, -7.3, 0.5), &vec3_t_c(0.3, 0.5, 0.5), 5.0, 5.0);
//    r_CreateLight(R_LIGHT_TYPE_POINT, &vec3_t_c(4.0, -7.3, 0.5), &vec3_t_c(0.3, 0.0, 1.0), 5.0, 5.0);
//    
    g_player_light = r_CreateLight(R_LIGHT_TYPE_POINT, &vec3_t_c(8.0, -3.0, 3.0), &vec3_t_c(1.0, 1.0, 1.0), 8.0, 5.0);
    g_hook_index = a_GetBoneIndex(g_player_entity->mixer, "hand.R");
    g_hook_transform = a_GetBoneTransform(g_player_entity->mixer, g_hook_index);
}

void g_Shutdown()
{
    
}

void g_SetGameState(uint32_t game_state)
{
    g_game_state = game_state;
}

void g_MainLoop(uint32_t editor_active)
{
    while(g_game_state != G_GAME_STATE_QUIT)
    {
        in_Input();
        
        switch(g_game_state)
        {
            case G_GAME_STATE_PLAYING:
                p_UpdateColliders();
                g_UpdateEntities();
                a_UpdateAnimations(0.01666);
            break;
            
            case G_GAME_STATE_EDITING:
                ed_UpdateEditor();
            break;
        }
        
        r_VisibleLights();
        r_VisibleVisItems();
        r_BeginFrame();
        r_DrawSortedBatches();
        r_DrawImmediateBatches();
        r_EndFrame();
    }
}

void g_LoadMap(char *file_name)
{
    FILE *file;
    
    if(file_exists(file_name))
    {
        int32_t width;
        int32_t height;
        int32_t components;
        mat4_t transform;
        
        mat4_t_identity(&transform);
        
        uint32_t *pixels = (uint32_t *)stbi_load(file_name, &width, &height, &components, STBI_rgb_alpha);
        
        transform.rows[3].x = (float)width / 2;
        transform.rows[3].y = -(float)height / 2;
        transform.rows[3].z = -2.0;
        transform.rows[0].x = (float)width;
        transform.rows[1].y = (float)width;
        g_CreateEntity(&transform, NULL, g_cube_model);
        
        
        transform.rows[0].x = 0.5;
        transform.rows[1].y = 0.5;
        transform.rows[2].z = 0.5;
        transform.rows[3].z = 0.0;
        
        for(int32_t y = 0; y < height; y++)
        {
            for(int32_t x = 0; x < width; x++)
            {
                if(pixels[x + y * width] != 0xffffffff)
                {
                    transform.rows[3].x = x;
                    transform.rows[3].y = -y;
                    struct g_entity_t *cube = g_CreateEntity(&transform, NULL, g_cube_model);
                    g_SetEntityCollider(cube, P_COLLIDER_TYPE_STATIC, &vec3_t_c(1.0, 1.0, 1.0));
                }
            }
        }
        
        free(pixels);
//        file = fopen(file_name, "r");
//        char *contents;
//        uint32_t length;
//        read_file(file, (void **)&contents, &length);
//        fclose(file);
//
//        mat4_t cur_transform;
//        mat4_t_identity(&cur_transform);
//        cur_transform.rows[2].z = 16.0;
//        
//        for(uint32_t index = 0; index < length; index++)
//        {
//            struct r_model_t *model = NULL;
//            mat4_t transform = cur_transform;
//            vec3_t size;
//            switch(contents[index])
//            {
//                case '|':
//                    transform.rows[3].y += 2.0;
//                    model = g_wall_tile_model;
//                    cur_transform.rows[3].x += 1.0;
//                    size = vec3_t_c(1.0, 2.0, 2.0);
//                break;
//                
//                case ' ':
//                    cur_transform.rows[3].x += 1.0;
//                break;
//                
//                case '_':
//                    model = g_floor_tile_model;
//                    cur_transform.rows[3].x += 2.0;
//                    size = vec3_t_c(2.0, 1.0, 2.0);
//                break;
//                
//                case '\n':
//                    cur_transform.rows[3].y -= 2.0;
//                    cur_transform.rows[3].x = 0.0;
//                break;
//            }
//            
//            if(model)
//            {
//                struct g_entity_t *entity = g_CreateEntity(&transform, NULL, model);
//                g_SetEntityCollider(entity, P_COLLIDER_TYPE_STATIC, &size);
//            }
//        }
        
//        mem_Free(contents);
    }
}

void g_UpdateEntities()
{
    r_i_SetTransform(NULL);
    r_i_SetPrimitiveType(GL_LINES);
    r_i_SetPolygonMode(GL_FILL);
    r_i_SetSize(4.0);
    
    for(uint32_t entity_index = 0; entity_index < g_entities.cursor; entity_index++)
    {
        struct g_entity_t *entity = get_stack_list_element(&g_entities, entity_index);
        
        if(entity->index != 0xffffffff)
        {
            if(entity->collider)
            {
                entity->local_transform.rows[3].x = entity->collider->position.x;
                entity->local_transform.rows[3].y = entity->collider->position.y;
                entity->local_transform.rows[3].z = entity->collider->position.z;
            }
            
            if(entity->thinker)
            {
                entity->thinker(entity);
            }
            
            if(entity->parent_transform)
            {
                mat4_t_mul(&entity->transform, &entity->local_transform, entity->parent_transform);
            }
            else
            {
                entity->transform = entity->local_transform;
            }
        }
    }
    
    for(uint32_t projectile_index = 0; projectile_index < g_projectiles.cursor; projectile_index++)
    {
        struct g_projectile_t *projectile = get_list_element(&g_projectiles, projectile_index);
        
        if(!projectile->life)
        {
            r_DestroyLight(projectile->light);
            remove_list_element(&g_projectiles, projectile_index);
            projectile_index--;
            continue;
        }
        
        vec3_t start = projectile->position;
        vec3_t end;
        struct p_trace_t trace = {.time = 1.0};
        vec3_t_add(&end, &start, &projectile->velocity);
        if(p_Raycast(&start, &end, &trace))
        {
            vec3_t_fmadd(&projectile->position, &projectile->position, &projectile->velocity, trace.time * 0.999);
            
            trace.normal.x += (((float)(rand() % 513) / 512.0) * 2.0 - 1.0) * 0.09;
            trace.normal.y += (((float)(rand() % 513) / 512.0) * 2.0 - 1.0) * 0.09;
//            trace.normal.z += ((512.0 / (float)(rand() % 511)) * 2.0 - 1.0) * 0.01;
            
            vec3_t_normalize(&trace.normal, &trace.normal);
            
            float proj = vec3_t_dot(&trace.normal, &projectile->velocity);
            vec3_t_fmadd(&projectile->velocity, &projectile->velocity, &trace.normal, -proj * 2);
            
            uint32_t sound = rand()% 10;
            s_PlaySound(g_ric_sounds[sound], &vec3_t_c(0.0, 0.0, 0.0), 0.2, 0);
            
            projectile->bouces++;
        }
        else
        {
            vec3_t_add(&projectile->position, &projectile->position, &projectile->velocity);
        }

        struct r_light_t *light = projectile->light;
        light->data.pos_rad.x = projectile->position.x;
        light->data.pos_rad.y = projectile->position.y;
        light->data.pos_rad.z = projectile->position.z;
        
        r_i_DrawLine(&start, &end, &light->data.color, 4.0);
        
        projectile->life--;
        
        if(projectile->bouces > 5)
        {
            projectile->life = 0;
        }
    }
    
//    for(uint32_t trigger_index = 0; trigger_index < g_triggers.cursor; trigger_index++)
//    {
//        struct g_trigger_t *trigger = get_stack_list_element(&g_triggers, trigger_index);
//        if(trigger->index != 0xffffffff)
//        {
//            if(trigger->thinker)
//            {
//                trigger->thinker(trigger);
//            }
//        }
//    }
}

struct g_entity_t *g_CreateEntity(mat4_t *transform, thinker_t *thinker, struct r_model_t *model)
{
    uint32_t entity_index;
    struct g_entity_t *entity;
    
    entity_index = add_stack_list_element(&g_entities, NULL);
    entity = get_stack_list_element(&g_entities, entity_index);
    
    entity->index = entity_index;
    entity->local_transform = *transform;
    entity->transform = *transform;
    entity->model = model;
    entity->thinker = thinker;
    entity->item = r_AllocateVisItem(&entity->transform, entity->model);
    
    return entity;
}

struct g_entity_t *g_GetEntity(uint32_t index)
{
    struct g_entity_t *entity;
    entity = get_stack_list_element(&g_entities, index);
    if(entity && entity->index == 0xffffffff)
    {
        entity = NULL;
    }
    
    return entity;
}

void g_DestroyEntity(struct g_entity_t *entity)
{
    
}

void g_ParentEntity(struct g_entity_t *parent, struct g_entity_t *entity)
{
    if(parent && entity)
    {
        entity->parent_transform = &parent->transform;
    }
}

void g_SetEntityCollider(struct g_entity_t *entity, uint32_t type, vec3_t *size)
{
    vec3_t position = vec3_t_c_vec4_t(&entity->transform.rows[3]);
    entity->collider = p_CreateCollider(type, &position, NULL, size);
    entity->collider->user_data = entity;
}

struct g_projectile_t *g_SpawnProjectile(vec3_t *position, vec3_t *velocity, vec3_t *color, float radius, uint32_t life)
{
    uint32_t index;
    struct g_projectile_t *projectile;
    
    index = add_list_element(&g_projectiles, NULL);
    projectile = get_list_element(&g_projectiles, index);
    
    projectile->position = *position;
    projectile->velocity = *velocity;
    projectile->life = life;
    projectile->light = r_CreateLight(R_LIGHT_TYPE_POINT, position, color, radius, 20.0);
    projectile->bouces = 0;
    
    return projectile;
}

void g_DestroyProjectile(struct g_projectile_t *projectile)
{
    
}

void g_PlayAnimation(struct g_entity_t *entity, struct a_animation_t *animation, char *player_name)
{
    if(!entity->mixer)
    {
        entity->model = r_ShallowCopyModel(entity->model);
        entity->mixer = a_CreateMixer(entity->model);
        entity->item->model = entity->model;
    }
    
    a_MixAnimation(entity->mixer, animation, player_name);
}

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

void *g_GetProp(struct g_entity_t *entity, char *prop_name)
{
    for(uint32_t prop_index = 0; prop_index < entity->props.cursor; prop_index++)
    {
        struct g_prop_t *prop = get_list_element(&entity->props, prop_index);
        if(!strcmp(prop->name, prop_name))
        {
            return prop->data;
        }
    }
    
    return NULL;
}

void *g_SetProp(struct g_entity_t *entity, char *prop_name, uint32_t size, void *data)
{
    struct g_prop_t *prop = g_GetProp(entity, prop_name);
    
    if(!entity->props.buffers)
    {
        entity->props = create_list(sizeof(struct g_prop_t), 8);
    }
    
    if(!prop)
    {
        prop = get_list_element(&entity->props, add_list_element(&entity->props, NULL));
        prop->name = strdup(prop_name);
    }
    
    if(prop->size < size)
    {
        prop->data = mem_Realloc(prop->data, size);
        prop->size = size;
    }
    
    memcpy(prop->data, data, size);
    
    return prop->data;
}

void g_RemoveProp(struct g_entity_t *entity, char *prop_name)
{
    for(uint32_t prop_index = 0; prop_index < entity->props.cursor; prop_index++)
    {
        struct g_prop_t *prop = get_list_element(&entity->props, prop_index);
        if(!strcmp(prop->name, prop_name))
        {
            mem_Free(prop->name);
            mem_Free(prop->data);
            remove_list_element(&entity->props, prop_index);
            return;
        }
    }
}

void g_PlayerThinker(struct g_entity_t *entity)
{
    vec3_t disp = {};
    vec4_t collider_disp = {};
    float rotation = 0.0;
    int32_t move_dir = 0;
    static float zoom = 0.0;
//    uint32_t moving = 0;

 
    struct p_movable_collider_t *collider = (struct p_movable_collider_t *)entity->collider;
    struct g_player_state_t *player_state;
    struct a_mask_t *upper_body_mask = a_GetAnimationMask(entity->mixer, "upper_body");
    struct a_mask_t *lower_body_mask = a_GetAnimationMask(entity->mixer, "lower_body");
    
    
    struct a_mask_player_t *upper_shoot_player = a_GetMaskPlayer(upper_body_mask, "shoot_player");
    struct a_mask_player_t *upper_run_player = a_GetMaskPlayer(upper_body_mask, "run_player");    
    struct a_mask_player_t *upper_idle_player = a_GetMaskPlayer(upper_body_mask, "idle_player");
    struct a_mask_player_t *upper_jump_player = a_GetMaskPlayer(upper_body_mask, "jump_player");
    struct a_mask_player_t *upper_fall_player = a_GetMaskPlayer(upper_body_mask, "fall_player");
    
    struct a_mask_player_t *lower_run_player = a_GetMaskPlayer(lower_body_mask, "run_player");    
    struct a_mask_player_t *lower_idle_player = a_GetMaskPlayer(lower_body_mask, "idle_player");
    struct a_mask_player_t *lower_jump_player = a_GetMaskPlayer(lower_body_mask, "jump_player");
    struct a_mask_player_t *lower_fall_player = a_GetMaskPlayer(lower_body_mask, "fall_player");
//    struct a_player_t *lower_shoot_player = a_GetMixerPlayer(entity->mixer, "shoot_player");
    
//    struct a_player_t *run_player = a_GetMixerPlayer(entity->mixer, "run_player");    
//    struct a_player_t *idle_player = a_GetMixerPlayer(entity->mixer, "idle_player");
//    struct a_player_t *jump_player = a_GetMixerPlayer(entity->mixer, "jump_player");
//    struct a_player_t *fall_player = a_GetMixerPlayer(entity->mixer, "fall_player");
//    struct a_player_t *shoot_player = a_GetMixerPlayer(entity->mixer, "shoot_player");
//    
    if(in_GetKeyState(SDL_SCANCODE_ESCAPE) & IN_KEY_STATE_PRESSED)
    {
        g_SetGameState(G_GAME_STATE_EDITING);
    }
//    
    player_state = g_GetProp(entity, "player_state");
    if(!player_state)
    {
        player_state = g_SetProp(entity, "player_state", sizeof(struct g_player_state_t), &(struct g_player_state_t){});
        player_state->run_scale = 1.0;
        player_state->shoot_frac = 0.0;
    }
    
    if(in_GetKeyState(SDL_SCANCODE_A) & IN_KEY_STATE_PRESSED)
    {
        move_dir--;
    }
    if(in_GetKeyState(SDL_SCANCODE_D) & IN_KEY_STATE_PRESSED)
    {
        move_dir++;
    }
    
    if(move_dir > 0)
    {
        if(player_state->direction == G_PLAYER_LEFT_ANGLE)
        {
            player_state->flags |= G_PLAYER_FLAG_TURNING;
            player_state->run_frac = 0.0;
        }
    }
    else if(move_dir < 0)
    {
        if(player_state->direction == G_PLAYER_RIGHT_ANGLE)
        {
            player_state->flags |= G_PLAYER_FLAG_TURNING | G_PLAYER_FLAG_TURNING_LEFT;
            player_state->run_frac = 0.0;
        }
    }
    else
    {
        collider->disp.x = 0.0;
        player_state->run_frac = 0.0;
    }
     
    if(player_state->flags & G_PLAYER_FLAG_TURNING)
    {
        if(player_state->flags & G_PLAYER_FLAG_TURNING_LEFT)
        {
            rotation = 0.1;
            player_state->direction += 0.1;
            if(player_state->direction >= 1.0)
            {
                player_state->flags &= ~(G_PLAYER_FLAG_TURNING | G_PLAYER_FLAG_TURNING_LEFT); 
                player_state->direction = 1.0;
            }
        }
        else
        {
            rotation = -0.1;
            player_state->direction -= 0.1;
            if(player_state->direction <= 0.0)
            {
                player_state->flags &= ~G_PLAYER_FLAG_TURNING; 
                player_state->direction = 0.0;
            }
        }
    }
    else if(move_dir != 0)
    {
        collider_disp.x = -entity->mixer->root_disp.y * SCALE;
        player_state->run_frac += 0.1;
    }
    
    if(in_GetKeyState(SDL_SCANCODE_SPACE) & IN_KEY_STATE_PRESSED)
    {
        if(in_GetKeyState(SDL_SCANCODE_SPACE) & IN_KEY_STATE_JUST_PRESSED && collider->flags & P_COLLIDER_FLAG_ON_GROUND)
        {
            player_state->flags |= G_PlAYER_FLAG_JUMPING;
            player_state->jump_y = collider->position.y;
            player_state->jump_disp = 0.2;
            player_state->jump_frac = 0.0;
            s_PlaySound(g_jump_sound, &vec3_t_c(0.0, 0.0, 0.0), 0.6, 0);
        }
    }
    else
    {
        player_state->flags &= ~G_PlAYER_FLAG_JUMPING;
    }
    
    if(player_state->jump_disp > 0.0)
    {
        if(collider->flags & P_COLLIDER_FLAG_TOP_COLLIDED)
        {
            player_state->jump_disp = 0.0;
            player_state->flags &= ~G_PlAYER_FLAG_JUMPING;
        }
        else
        {
            if(player_state->flags & G_PlAYER_FLAG_JUMPING)
            {
                if(collider->position.y - player_state->jump_y > 3.5)
                {
                    player_state->jump_disp *= 0.9;
                }
            }
            else
            {
                player_state->jump_disp *= 0.5;
            }
            
            if(player_state->jump_disp < 0.04)
            {
                player_state->flags &= ~G_PlAYER_FLAG_JUMPING;
                player_state->jump_disp = 0.0;
            }
            
            collider->disp.y = player_state->jump_disp;
        }
    }
    else
    {
        collider->disp.y -= 0.01;
    }
    
    if(player_state->run_frac > 1.0)
    {
        player_state->run_frac = 1.0;
    }
    
    if(player_state->flags & G_PlAYER_FLAG_JUMPING)
    {
        player_state->jump_frac += 0.12;
        if(player_state->jump_frac > 0.99)
        {
            player_state->jump_frac = 0.99;
        }
            
        lower_run_player->weight = 0.0;
        upper_run_player->weight = 0.0;
        
        lower_idle_player->player->scale = 0.0;
        lower_idle_player->weight = 0.0;
        upper_idle_player->weight = 0.0;
        
        upper_jump_player->weight = 1.0;
        lower_jump_player->weight = 1.0;
        lower_jump_player->player->scale = 0.0;
        
        upper_fall_player->weight = 0.0;
        lower_fall_player->weight = 0.0;
        lower_fall_player->player->scale = 0.0;
        
        a_SeekAnimationRelative(lower_jump_player->player, player_state->jump_frac);
    }
    else
    {
        if(collider->flags & P_COLLIDER_FLAG_ON_GROUND)
        {
            if(!(player_state->collider_flags & P_COLLIDER_FLAG_ON_GROUND))
            {
                s_PlaySound(g_footstep_sounds[0], &vec3_t_c(0.0, 0.0, 0.0), 1.0, 0);
            }
            lower_run_player->weight = player_state->run_frac;
            upper_run_player->weight = player_state->run_frac;
            
            lower_idle_player->player->scale = 0.6 * (1.0 - player_state->run_frac);
            lower_idle_player->weight = 1.0 - player_state->run_frac;
            upper_idle_player->weight = 1.0 - player_state->run_frac;
            
            lower_jump_player->weight = 0.0;
            upper_jump_player->weight = 0.0;
            
            lower_fall_player->weight = 0.0;
            upper_fall_player->weight = 0.0;
            
            player_state->jump_frac = 1.0;
        }
        else
        {
            lower_jump_player->weight = 0.0;
            upper_jump_player->weight = 0.0;
            
            lower_fall_player->weight = 1.0;
            upper_fall_player->weight = 1.0;
            
            lower_run_player->weight = 0.0;
            upper_run_player->weight = 0.0;
            
            lower_idle_player->weight = 0.0;
            upper_idle_player->weight = 0.0;
            
            player_state->jump_frac -= 0.05;
            if(player_state->jump_frac < 0.0)
            {
                player_state->jump_frac = 0.0;
            }
            
            a_SeekAnimationRelative(lower_fall_player->player, (1.0 - player_state->jump_frac) * 0.999);
        }
    }
    
    if(in_GetKeyState(SDL_SCANCODE_KP_MINUS) & IN_KEY_STATE_PRESSED)
    {
        zoom += 0.003;
        if(zoom > 0.3)
        {
            zoom = 0.3;
        }
    }
    else if(in_GetKeyState(SDL_SCANCODE_KP_PLUS) & IN_KEY_STATE_PRESSED)
    {
        zoom -= 0.003;
        if(zoom < -0.3)
        {
            zoom = -0.3;
        }
    }
    else
    {
        zoom *= 0.9;
    }

    
    player_state->collider_flags = collider->flags;
    lower_run_player->player->scale = 1.9 * player_state->run_frac * player_state->run_scale;
    upper_run_player->player->scale = 1.9 * player_state->run_frac * player_state->run_scale;
    
    mat4_t yaw_matrix;
    mat4_t_identity(&yaw_matrix);
    mat4_t_rotate_z(&yaw_matrix, player_state->direction);
    
    mat4_t_vec4_t_mul(&collider_disp, &yaw_matrix, &collider_disp);
    mat4_t_identity(&yaw_matrix);
    mat4_t_rotate_z(&yaw_matrix, rotation);
    
    mat4_t_mul(&entity->local_transform, &yaw_matrix, &entity->local_transform);
    collider->disp.x = collider_disp.x;
    
    vec4_t player_pos = entity->local_transform.rows[3];
    mat4_t_identity(&g_gun_entity->local_transform);
//    g_gun_entity->local_transform.rows[0].x *= 1;
//    g_gun_entity->local_transform.rows[1].y *= 0.7;
//    g_gun_entity->local_transform.rows[2].z *= 0.7;
    g_gun_entity->local_transform.rows[3].z = 0.1;
    g_gun_entity->local_transform.rows[3].x = -0.85;
    mat4_t_rotate_x(&g_gun_entity->local_transform, -0.5);
    mat4_t_rotate_z(&g_gun_entity->local_transform, -0.5);
    mat4_t_rotate_y(&g_gun_entity->local_transform, -0.5);
    mat4_t_mul(&g_gun_entity->local_transform, &g_gun_entity->local_transform, g_hook_transform);
    mat4_t_mul(&g_gun_entity->local_transform, &g_gun_entity->local_transform, &entity->local_transform);    
    
    vec4_t spawn_point = vec4_t_c(0.0, -2.5, 0.5, 1.0);
//    mat4_t_vec4_t_mul(&spawn_point, g_hook_transform, &spawn_point);
    mat4_t_vec4_t_mul(&spawn_point, &entity->local_transform, &spawn_point);
    
    float rx = (((float)(rand() % 513) / 512.0) * 2.0 - 1.0) * 0.35;
//    float ry = (float)(rand() % 129) / 128.0;
    
    vec4_t spawn_direction = vec4_t_c(0.0, -16.0, rx, 0.0);
    mat4_t_vec4_t_mul(&spawn_direction, &entity->local_transform, &spawn_direction);
    
    if((in_GetKeyState(SDL_SCANCODE_C) & IN_KEY_STATE_PRESSED) && !player_state->shoot_frac)
    {
        player_state->shoot_frac = 0.1;
        vec3_t color;
        
        color.x = (float)(rand() % 513) / 512.0;
        color.y = (float)(rand() % 513) / 512.0;
        color.z = (float)(rand() % 513) / 512.0;
        
        vec3_t_normalize(&color, &color);
        
        s_PlaySound(g_shot_sound, &vec3_t_c(0.0, 0.0, 0.0), 0.6, 0);
        g_SpawnProjectile(&vec3_t_c(spawn_point.x, spawn_point.y, spawn_point.z), 
                          &vec3_t_c(spawn_direction.x, spawn_direction.y, spawn_direction.z), &color, 4.0, 480);
                          
        player_state->shot_light = r_CreateLight(R_LIGHT_TYPE_POINT, &vec3_t_c(spawn_point.x, spawn_point.y, spawn_point.z), &color, 8.0, 60.0);
    }
    
    if(player_state->shoot_frac)
    {
        if(player_state->shot_light && player_state->shoot_frac > 0.2)
        {
            r_DestroyLight(player_state->shot_light);
            player_state->shot_light = NULL;
        }
        
        uint32_t stop = 0;
        player_state->shoot_frac += 0.20;
        if(player_state->shoot_frac > 0.9999)
        {
            player_state->shoot_frac = 0.9999;
            stop = 1;
        }
        
        upper_shoot_player->weight = 1.0;
        upper_idle_player->weight = 0.0;
        upper_run_player->weight = 0.0;
        upper_jump_player->weight = 0.0;
        upper_fall_player->weight = 0.0;
        a_SeekAnimationRelative(upper_shoot_player->player, player_state->shoot_frac);
        
        if(stop)
        {
            player_state->shoot_frac = 0.0;
        }
    }
    else
    {
        upper_shoot_player->weight = 0.0;
    }
    
    vec4_t light_pos = entity->local_transform.rows[3];    
    g_player_light->data.pos_rad.x = light_pos.x;
    g_player_light->data.pos_rad.y = light_pos.y;
    g_player_light->data.pos_rad.z = light_pos.z + 1.5;
//    
    mat4_t_vec4_t_mul_fast(&player_pos, &r_inv_view_matrix, &player_pos);
    player_pos.w = 0.0;
    player_pos.z = 0.0;
    player_pos.y += G_SCREEN_Y_OFFSET * 0.05;
    mat4_t_vec4_t_mul_fast(&player_pos, &r_view_matrix, &player_pos);
    vec3_t_add(&g_camera_pos, &g_camera_pos, &vec3_t_c(player_pos.x * 0.1, player_pos.y * 0.1, zoom));
    r_SetViewPos(&g_camera_pos);
    r_SetViewPitchYaw(g_camera_pitch, g_camera_yaw);
}

void g_ElevatorThinker(struct g_entity_t *entity)
{
//    uint32_t *state = g_GetProp(entity, "state");
//    
//    if(entity->collider->position.y >= 10.0)
//    {
//        *state = 1;
////        p_DisplaceCollider(entity->collider, &vec3_t_c(0.0, -0.1, 0.0));
//    }
//    else if(entity->collider->position.y <= 1.0)
//    {
//        *state = 0;
////        p_DisplaceCollider(entity->collider, &vec3_t_c(0.0, -0.1, 0.0));
//    }
//    
//    if(*state)
//    {
//        p_DisplaceCollider(entity->collider, &vec3_t_c(0.0, -0.05, 0.0));
//    }
//    else
//    {
//        p_DisplaceCollider(entity->collider, &vec3_t_c(0.0, 0.05, 0.0));
//    }
}

void g_TriggerThinker(struct g_entity_t *trigger)
{
//    struct p_trigger_collider_t *collider = (struct p_trigger_collider_t *)trigger->collider;
//    if(collider->collision_count)
//    {
//        for(uint32_t collision_index = 0; collision_index < collider->collision_count; collision_index++)
//        {
//            struct p_collider_t *collision = p_GetCollision((struct p_collider_t *)collider, collision_index);
//            if(collision->user_data)
//            {
//                struct g_entity_t *player = (struct g_entity_t *)collision->user_data;
//                
//                if(g_GetProp(player, "the_player"))
//                {
//                    printf("touching player!\n");
//                }
//            }
//        }
//    }
//    else
//    {
//        printf("nah...\n");
//    }
}




