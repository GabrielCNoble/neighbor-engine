#include "dstuff/ds_stack_list.h"
#include "dstuff/ds_mem.h"
#include "dstuff/ds_file.h"
#include "dstuff/ds_vector.h"
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
struct stack_list_t g_triggers;
#define G_PLAYER_AREA_Z 7.0
float g_camera_z = G_PLAYER_AREA_Z;

#define G_SCREEN_Y_OFFSET 20.0
#define G_PLAYER_RIGHT_ANGLE 0.0
#define G_PLAYER_LEFT_ANGLE 1.0


struct r_model_t *g_player_model;
struct r_model_t *g_floor_tile_model;
struct r_model_t *g_wall_tile_model;
struct r_model_t *g_wiggle_model;

struct g_entity_t *wiggle_entity;
struct g_entity_t *g_player_entity;
struct a_animation_t *g_run_animation;
struct a_animation_t *g_idle_animation;
struct a_animation_t *g_jump_animation;
struct a_animation_t *g_fall_animation;
struct s_sound_t *g_jump_sound;
struct s_sound_t *g_land_sound;
struct s_sound_t *g_footstep_sounds[5];

uint32_t g_game_state = G_GAME_STATE_LOADING;

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
    r_SetViewPitch(-0.05);
    r_SetViewPos(&vec3_t_c(2.0, 3.5, g_camera_z));
    
    g_player_model = r_LoadModel("models/tall_box2.mof");
    g_floor_tile_model = r_LoadModel("models/floor_tile.mof");
    g_wall_tile_model = r_LoadModel("models/wall_tile.mof");
    g_wiggle_model = r_LoadModel("models/dude.mof");
    g_run_animation = a_LoadAnimation("models/dude_run.anf");
    g_idle_animation = a_LoadAnimation("models/idle.anf");
    g_jump_animation = a_LoadAnimation("models/jump.anf");
    g_fall_animation = a_LoadAnimation("models/fall.anf");
    
    
//    struct s_sound_t *sound = s_LoadSound("sounds/fall2.ogg");
    g_footstep_sounds[0] = s_LoadSound("sounds/step2.ogg");
    g_jump_sound = s_LoadSound("sounds/jump.ogg");
    g_land_sound = s_LoadSound("sounds/fall.ogg");
//    g_footstep_sounds[1] = s_LoadSound("sounds/pl_tile2.ogg");
//    g_footstep_sounds[2] = s_LoadSound("sounds/pl_tile3.ogg");
//    g_footstep_sounds[3] = s_LoadSound("sounds/pl_tile4.ogg");
//    g_footstep_sounds[4] = s_LoadSound("sounds/pl_tile5.ogg");
//    s_PlaySound(sound, &vec3_t_c(10.0, 0.0, 0.0), 0.5, 0);
    
    mat4_t transform;
    mat4_t_identity(&transform);
//    mat4_t_rotate_x(&transform, -0.5);
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
    transform.rows[3] = vec4_t_c(1.0, 4.0, 0.0, 1.0);
    
    g_player_entity = g_CreateEntity(&transform, g_PlayerThinker, g_wiggle_model);
    g_SetEntityCollider(g_player_entity, P_COLLIDER_TYPE_MOVABLE, &vec3_t_c(1.0, 2.0, 1.0));
    g_PlayAnimation(g_player_entity, g_run_animation);
    g_PlayAnimation(g_player_entity, g_idle_animation);
    g_PlayAnimation(g_player_entity, g_jump_animation);
    g_PlayAnimation(g_player_entity, g_fall_animation);
    g_player_entity->mixer->flags |= A_MIXER_FLAG_COMPUTE_ROOT_DISPLACEMENT;
    struct a_player_t *player = a_GetPlayer(g_player_entity->mixer, 0);
    a_SetCallbackFrame(player, g_TestCallback, g_player_entity, 8);
    a_SetCallbackFrame(player, g_TestCallback, g_player_entity, 20);
    
    player = a_GetPlayer(g_player_entity->mixer, 2);
    player->scale = 0.0;
    player = a_GetPlayer(g_player_entity->mixer, 3);
    player->scale = 0.0;
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
    
    
    g_LoadMap("map6.map");
    
    g_game_state = G_GAME_STATE_PLAYING;
    
    r_CreateLight(R_LIGHT_TYPE_POINT, &vec3_t_c(4.0, -0.3, 0.2), &vec3_t_c(1.0, 0.0, 0.3), 10.0);
    r_CreateLight(R_LIGHT_TYPE_POINT, &vec3_t_c(8.0, -0.3, 0.2), &vec3_t_c(0.3, 1.0, 0.0), 10.0);
    r_CreateLight(R_LIGHT_TYPE_POINT, &vec3_t_c(12.0, -0.3, 0.2), &vec3_t_c(0.0, 0.3, 1.0), 10.0);
    
    r_CreateLight(R_LIGHT_TYPE_POINT, &vec3_t_c(16.0, -0.3, 0.2), &vec3_t_c(1.0, 1.0, 0.3), 10.0);
    r_CreateLight(R_LIGHT_TYPE_POINT, &vec3_t_c(20.0, -0.3, 0.2), &vec3_t_c(1.0, 0.3, 1.0), 10.0);
    r_CreateLight(R_LIGHT_TYPE_POINT, &vec3_t_c(24.0, -0.3, 0.2), &vec3_t_c(0.3, 1.0, 1.0), 10.0);
    
    r_CreateLight(R_LIGHT_TYPE_POINT, &vec3_t_c(24.0, -7.3, 0.2), &vec3_t_c(0.3, 1.0, 0.3), 10.0);
    r_CreateLight(R_LIGHT_TYPE_POINT, &vec3_t_c(20.0, -7.3, 0.2), &vec3_t_c(0.3, 0.3, 1.0), 10.0);
    r_CreateLight(R_LIGHT_TYPE_POINT, &vec3_t_c(16.0, -7.3, 0.2), &vec3_t_c(1.0, 0.3, 0.3), 10.0);
    
    r_CreateLight(R_LIGHT_TYPE_POINT, &vec3_t_c(12.0, -7.3, 0.2), &vec3_t_c(1.0, 0.2, 0.0), 10.0);
    r_CreateLight(R_LIGHT_TYPE_POINT, &vec3_t_c(8.0, -7.3, 0.2), &vec3_t_c(0.3, 0.5, 0.5), 10.0);
    r_CreateLight(R_LIGHT_TYPE_POINT, &vec3_t_c(4.0, -7.3, 0.2), &vec3_t_c(0.3, 0.0, 1.0), 10.0);
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
        
        g_DrawEntities();
        w_VisibleLights();
        r_BeginFrame();
        r_DrawBatches();
        r_DrawImmediateBatches();
        r_EndFrame();
    }
}

void g_LoadMap(char *file_name)
{
    FILE *file;
    
    if(file_exists(file_name))
    {
        file = fopen(file_name, "r");
        char *contents;
        uint32_t length;
        read_file(file, (void **)&contents, &length);
        fclose(file);

        mat4_t cur_transform;
        mat4_t_identity(&cur_transform);
        cur_transform.rows[2].z = 16.0;
        
        for(uint32_t index = 0; index < length; index++)
        {
            struct r_model_t *model = NULL;
            mat4_t transform = cur_transform;
            vec3_t size;
            switch(contents[index])
            {
                case '|':
                    transform.rows[3].y += 2.0;
                    model = g_wall_tile_model;
                    cur_transform.rows[3].x += 1.0;
                    size = vec3_t_c(1.0, 2.0, 2.0);
                break;
                
                case ' ':
                    cur_transform.rows[3].x += 1.0;
                break;
                
                case '_':
                    model = g_floor_tile_model;
                    cur_transform.rows[3].x += 2.0;
                    size = vec3_t_c(2.0, 1.0, 2.0);
                break;
                
                case '\n':
                    cur_transform.rows[3].y -= 2.0;
                    cur_transform.rows[3].x = 0.0;
                break;
            }
            
            if(model)
            {
                struct g_entity_t *entity = g_CreateEntity(&transform, NULL, model);
                g_SetEntityCollider(entity, P_COLLIDER_TYPE_STATIC, &size);
            }
        }
        
        mem_Free(contents);
    }
}

void g_UpdateEntities()
{
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

void g_DrawEntities()
{
    r_UpdateViewProjectionMatrix();
    for(uint32_t entity_index = 0; entity_index < g_entities.cursor; entity_index++)
    {
        struct g_entity_t *entity = g_GetEntity(entity_index);
        
        if(entity && entity->model)
        {
            r_DrawModel(&entity->transform, entity->model);
        }
    }
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
    entity->collider = p_CreateCollider(type, &position, size);
    entity->collider->user_data = entity;
}

void g_PlayAnimation(struct g_entity_t *entity, struct a_animation_t *animation)
{
    if(!entity->mixer)
    {
        entity->model = r_ShallowCopyModel(entity->model);
    }
    entity->mixer = a_MixAnimation(entity->mixer, animation, entity->model);
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
//    uint32_t moving = 0;
    struct p_movable_collider_t *collider = (struct p_movable_collider_t *)entity->collider;
    struct g_player_state_t *player_state;
    struct a_player_t *run_player = a_GetPlayer(entity->mixer, 0);    
    struct a_player_t *idle_player = a_GetPlayer(entity->mixer, 1);
    struct a_player_t *jump_player = a_GetPlayer(entity->mixer, 2);
    struct a_player_t *fall_player = a_GetPlayer(entity->mixer, 3);
    
    if(in_GetKeyState(SDL_SCANCODE_ESCAPE) & IN_KEY_STATE_PRESSED)
    {
        g_SetGameState(G_GAME_STATE_QUIT);
    }
    
    player_state = g_GetProp(entity, "player_state");
    if(!player_state)
    {
        player_state = g_SetProp(entity, "player_state", sizeof(struct g_player_state_t), &(struct g_player_state_t){});
    }
    
    if(in_GetKeyState(SDL_SCANCODE_A) & IN_KEY_STATE_PRESSED)
    {
//        disp.x -= 0.2;
        move_dir--;
    }
    if(in_GetKeyState(SDL_SCANCODE_D) & IN_KEY_STATE_PRESSED)
    {
//        disp.x += 0.2;
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
                if(collider->position.y - player_state->jump_y > 3.0)
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
            
        run_player->weight = 0.0;
        idle_player->scale = 0.0;
        idle_player->weight = 0.0;
        jump_player->weight = 1.0;
        jump_player->scale = 0.0;
        fall_player->weight = 0.0;
        fall_player->scale = 0.0;
        
        a_SeekAnimationRelative(jump_player, player_state->jump_frac);
    }
    else
    {
        if(collider->flags & P_COLLIDER_FLAG_ON_GROUND)
        {
            if(!(player_state->collider_flags & P_COLLIDER_FLAG_ON_GROUND))
            {
                s_PlaySound(g_footstep_sounds[0], &vec3_t_c(0.0, 0.0, 0.0), 1.0, 0);
            }
            run_player->weight = player_state->run_frac;
            idle_player->scale = 0.6 * (1.0 - player_state->run_frac);
            idle_player->weight = 1.0 - player_state->run_frac;
            jump_player->weight = 0.0;
            fall_player->weight = 0.0;
            player_state->jump_frac = 1.0;
        }
        else
        {
            jump_player->weight = 0.0;
            fall_player->weight = 1.0;
            run_player->weight = 0.0;
            idle_player->weight = 0.0;
            player_state->jump_frac -= 0.05;
            if(player_state->jump_frac < 0.0)
            {
                player_state->jump_frac = 0.0;
            }
            
            a_SeekAnimationRelative(fall_player, (1.0 - player_state->jump_frac) * 0.999);
        }
    }
    
    player_state->collider_flags = collider->flags;
    
    run_player->scale = 1.9 * player_state->run_frac;
    
    mat4_t yaw_matrix;
    mat4_t_identity(&yaw_matrix);
    mat4_t_rotate_y(&yaw_matrix, player_state->direction);
    mat4_t_vec4_t_mul(&collider_disp, &yaw_matrix, &collider_disp);
    mat4_t_identity(&yaw_matrix);
    mat4_t_rotate_y(&yaw_matrix, rotation);
    
    mat4_t_mul(&entity->local_transform, &yaw_matrix, &entity->local_transform);
    collider->disp.x = collider_disp.x;
    
    vec4_t player_pos = entity->local_transform.rows[3];
    mat4_t_vec4_t_mul(&player_pos, &r_inv_view_matrix, &player_pos);
    disp = vec3_t_c(player_pos.x * 0.1, player_pos.y * 0.1, 0.0);
    r_TranslateView(&disp);
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




