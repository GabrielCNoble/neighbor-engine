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
#include <string.h>
#include <stdio.h>


extern mat4_t r_view_matrix;
extern mat4_t r_inv_view_matrix;
extern mat4_t r_view_projection_matrix;

struct stack_list_t g_entities;
struct stack_list_t g_triggers;
#define G_PLAYER_AREA_Z 5.0
float g_camera_z = G_PLAYER_AREA_Z;

#define G_SCREEN_Y_OFFSET 20.0


struct r_model_t *g_player_model;
struct r_model_t *g_floor_tile_model;
struct r_model_t *g_wall_tile_model;
struct r_model_t *g_wiggle_model;

struct g_entity_t *wiggle_entity;
struct a_animation_t *wiggle_animation;

uint32_t g_game_state = G_GAME_STATE_LOADING;

void g_Init(uint32_t editor_active)
{
    r_Init();
    p_Init();
    a_Init();
    in_Input();
    w_Init();
    
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
    
    mat4_t transform;
    mat4_t_identity(&transform);
    transform.rows[3] = vec4_t_c(1.0, 4.0, 0.0, 1.0);
    
    struct g_entity_t *player_entity = g_CreateEntity(&transform, g_PlayerThinker, g_player_model);
    g_SetEntityCollider(player_entity, P_COLLIDER_TYPE_MOVABLE, &vec3_t_c(1.0, 2.0, 1.0));
    
    wiggle_animation = a_LoadAnimation("models/run.anf");
//    struct r_model_t *copy = r_ShallowCopyModel(g_wiggle_model);
//    a_PlayAnimation(wiggle_animation, copy);
    mat4_t_identity(&transform);
    mat4_t_rotate_x(&transform, -0.5);
    mat4_t_rotate_y(&transform, 0.5);
    transform.rows[3] = vec4_t_c(4.0, 1.0, 0.0, 1.0);
//    g_CreateEntity(&transform, NULL, g_wiggle_model);
//    transform.rows[3] = vec4_t_c(4.0, 0.0, 0.0, 1.0);
    wiggle_entity = g_CreateEntity(&transform, NULL, g_wiggle_model);
    g_PlayAnimation(wiggle_entity, wiggle_animation);
    struct a_player_t *player = *(struct a_player_t **)get_list_element(&wiggle_entity->mixer->players, 0);
    player->scale = 1.0;
    g_LoadMap("map6.map");
    
    g_game_state = G_GAME_STATE_PLAYING;
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
    entity->model = r_ShallowCopyModel(entity->model);
    entity->mixer = a_PlayAnimation(animation, entity->model);
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
//    uint32_t moving = 0;
    struct p_movable_collider_t *collider = (struct p_movable_collider_t *)entity->collider;
    struct g_player_state_t *player_state;
    
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
        disp.x -= 0.2;
    }
    if(in_GetKeyState(SDL_SCANCODE_D) & IN_KEY_STATE_PRESSED)
    {
        disp.x += 0.2;
    }
    
    if(!disp.x)
    {
        collider->disp.x *= 0.8;
    }
    else
    {
        collider->disp.x = disp.x;
    }
    
    if(in_GetKeyState(SDL_SCANCODE_SPACE) & IN_KEY_STATE_PRESSED)
    {
        if(in_GetKeyState(SDL_SCANCODE_SPACE) & IN_KEY_STATE_JUST_PRESSED && collider->flags & P_COLLIDER_FLAG_ON_GROUND)
        {
            player_state->flags |= G_PlAYER_FLAG_JUMPING;
            player_state->jump_y = collider->position.y;
            player_state->jump_disp = 0.3;
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
                if(collider->position.y - player_state->jump_y > 2.0)
                {
                    player_state->jump_disp *= 0.9;
                }
            }
            else
            {
                player_state->jump_disp *= 0.9;
            }
            
            if(player_state->jump_disp < 0.04)
            {
                player_state->flags &= ~G_PlAYER_FLAG_JUMPING;
                player_state->jump_disp = 0.0;
            }
            
            collider->disp.y = player_state->jump_disp;
        }
    }
    struct a_player_t *player = *(struct a_player_t **)get_list_element(&wiggle_entity->mixer->players, 0);
    
//    if(in_GetKeyState(SDL_SCANCODE_C) & IN_KEY_STATE_PRESSED)
//    {
//        player->scale = 1.0;
//    }
//    else
//    {
//        player->scale = 0.0;
//    }
    
    float translate_z = 0.0;
    
    if(in_GetKeyState(SDL_SCANCODE_M) & IN_KEY_STATE_JUST_PRESSED)
    {
        translate_z -= 0.5;
    }
    else if(in_GetKeyState(SDL_SCANCODE_N) & IN_KEY_STATE_JUST_PRESSED)
    {
        translate_z += 0.5;
    }
    
    r_TranslateView(&vec3_t_c(0.0, 0.0, translate_z));
    
    
    vec4_t player_pos = entity->local_transform.rows[3];
    mat4_t_vec4_t_mul(&player_pos, &r_inv_view_matrix, &player_pos);
    disp = vec3_t_c(player_pos.x * 0.1, player_pos.y * 0.1, 0.0);
    r_TranslateView(&disp);
    
    static float t = 0.0;
    
//    mat4_t_identity(&wiggle_entity->animation->transforms[0]);
//    mat4_t_identity(&wiggle_entity->animation->transforms[1]);
//    mat4_t_rotate_x(&wiggle_entity->animation->transforms[0], sin(t) * 0.5);
//    mat4_t_rotate_x(&wiggle_entity->animation->transforms[1], sin(t - 0.5) * 0.5);
//    wiggle_entity->animation->transforms[0].rows[3].y = -1.0;
//    wiggle_entity->animation->transforms[1].rows[3].y = 1.0;
//    
//    t += 0.04;
    
//    mat4_t parent_transform;
//    mat4_t_identity(&parent_transform);
    
//    r_DrawPoint(&vec3_t_c(0.0, 3.0, 0.0), &vec3_t_c(0.0, 1.0, 0.0), 8.0);
    
//    a_DrawSkeleton(g_wiggle_model->skeleton);
    
//    r_DrawLine(&vec3_t_c(0.0, -20.0, 0.0), &vec3_t_c(0.0, 20.0, 0.0), &vec3_t_c(0.0, 1.0, 0.0));
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




