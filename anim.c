#include <stdio.h>
#include "anim.h"
#include "r_draw.h"
#include "dstuff/ds_file.h"
#include "dstuff/ds_stack_list.h"
#include "dstuff/ds_mem.h"

struct stack_list_t a_skeletons;
struct stack_list_t a_animations;
struct stack_list_t a_players;
struct stack_list_t a_mixers;

void a_Init()
{
    a_skeletons = create_stack_list(sizeof(struct a_skeleton_t), 512);
    a_animations = create_stack_list(sizeof(struct a_animation_t), 128);
    a_players = create_stack_list(sizeof(struct a_player_t), 64);
    a_mixers = create_stack_list(sizeof(struct a_mixer_t), 16);
}

void a_Shutdown()
{
    
}

struct a_animation_t *a_LoadAnimation(char *file_name)
{
    FILE *file;
    struct a_animation_t *animation = NULL;
    
    if(file_exists(file_name))
    {
        void *buffer;
        file = fopen(file_name, "rb");
        read_file(file, &buffer, NULL);
        fclose(file);
        
        struct a_anim_sec_header_t *header;
        ds_get_section_data(buffer, "[animation]", (void **)&header, NULL);
        struct a_anim_sec_data_t(header->transform_count, header->pair_count, header->range_count) *data = (void *)(header + 1);
        
        uint32_t index = add_stack_list_element(&a_animations, NULL);
        animation = get_stack_list_element(&a_animations, index);
        animation->index = index;
        animation->bone_count = header->bone_count;
        animation->duration = header->duration;
        animation->framerate = header->framerate;
        animation->transform_count = header->transform_count;
        animation->range_count = header->range_count;
        animation->pair_count = header->pair_count;
        
        animation->transforms = mem_Calloc(animation->transform_count, sizeof(struct a_transform_t));
        memcpy(animation->transforms, data->transforms, sizeof(struct a_transform_t) * animation->transform_count);
        animation->pairs = mem_Calloc(header->pair_count, sizeof(struct a_bone_transform_pair_t));
        memcpy(animation->pairs, data->pairs, sizeof(struct a_bone_transform_pair_t) * animation->pair_count);
        animation->ranges = mem_Calloc(animation->range_count, sizeof(struct a_transform_range_t));
        memcpy(animation->ranges, data->ranges, sizeof(struct a_transform_range_t) * animation->range_count);
        mem_Free(buffer);
    }
    else
    {
        printf("Couldn't find animation %s\n", file_name);
    }
    
    return animation;
}

struct a_skeleton_t *a_CreateSkeleton(uint32_t bone_count, struct a_bone_t *bones)
{
    struct a_skeleton_t *skeleton;
    uint32_t index = add_stack_list_element(&a_skeletons, NULL);
    skeleton = get_stack_list_element(&a_skeletons, index);
    
    skeleton->index = index;
    skeleton->bone_count = bone_count;
    skeleton->bones = mem_Calloc(bone_count, sizeof(struct a_bone_t));
    memcpy(skeleton->bones, bones, sizeof(struct a_bone_t) * bone_count);
    
    return skeleton;
}

struct a_player_t *a_CreatePlayer(struct a_animation_t *animation)
{
    struct a_player_t *player;
    uint32_t index;
    
    index = add_stack_list_element(&a_players, NULL);
    player = get_stack_list_element(&a_players, index);
    
    player->animation = animation;
//    player->transfor = 0;
    player->pairs = mem_Calloc(animation->bone_count, sizeof(struct a_transform_pair_t));
    player->transforms = mem_Calloc(animation->bone_count, sizeof(struct a_transform_t));
    player->weight = 1.0;
    player->time = 0.0;
    player->scale = 1.0;
    
    a_ResetAnimation(player);
    
    return player;
}

void a_ResetAnimation(struct a_player_t *player)
{
    struct a_animation_t *animation = player->animation;
    struct a_transform_range_t *range = animation->ranges;
    struct a_bone_transform_pair_t *pairs = animation->pairs + range->start;
    
    for(uint32_t pair_index = 0; pair_index < range->count; pair_index++)
    {
        struct a_bone_transform_pair_t *pair = pairs + pair_index;
        player->pairs[pair->bone_index] = pair->pair;
    }

    player->time = 0.0;
}

struct a_mixer_t *a_PlayAnimation(struct a_animation_t *animation, struct r_model_t *model)
{
    struct a_mixer_t *mixer;
    uint32_t index = add_stack_list_element(&a_mixers, NULL);
    mixer = get_stack_list_element(&a_mixers, index);
    
    mixer->index = index;
    mixer->skeleton = model->skeleton;
    mixer->weight_count = model->weight_count;
    mixer->weights = model->weights;
    mixer->model = model;
    mixer->transforms = mem_Calloc(mixer->skeleton->bone_count, sizeof(mat4_t));
    
    if(!mixer->players.buffers)
    {
        mixer->players = create_list(sizeof(struct a_player_t *), 8);
    }
    
    if(animation)
    {
        struct a_player_t *player = a_CreatePlayer(animation);
        add_list_element(&mixer->players, &player);
    }
    
    for(uint32_t transform_index = 0; transform_index < mixer->skeleton->bone_count; transform_index++)
    {
        mat4_t_identity(mixer->transforms + transform_index);
    }

    return mixer;
}

void a_UpdateAnimations(float delta_time)
{
    for(uint32_t mixer_index = 0; mixer_index < a_mixers.cursor; mixer_index++)
    {
        struct a_mixer_t *mixer = get_stack_list_element(&a_mixers, mixer_index);
        
        if(mixer->index != 0xffffffff)
        {
            a_UpdateMixer(mixer, delta_time);
        }
    }
}

uint32_t a_UpdateMixerTransforms(struct a_mixer_t *mixer, uint32_t bone_index, mat4_t *parent_transform)
{
    struct a_bone_t *bone = mixer->skeleton->bones + bone_index;
    mat4_t *transform = mixer->transforms + bone_index;
    mat4_t_mul(transform, transform, parent_transform);
    
    bone_index++;
    
    for(uint32_t child_index = 0; child_index < bone->child_count; child_index++)
    {
        bone_index = a_UpdateMixerTransforms(mixer, bone_index, transform);
    }
    
    return bone_index;
}

void a_UpdateMixer(struct a_mixer_t *mixer, float delta_time)
{
    struct r_model_t *model = mixer->model->base;
    struct r_model_t *copy = mixer->model;
    struct a_skeleton_t *skeleton = mixer->skeleton; 

    for(uint32_t player_index = 0; player_index < mixer->players.cursor; player_index++)
    {
        struct a_player_t *player = *(struct a_player_t **)get_list_element(&mixer->players, player_index);
        struct a_animation_t *animation = player->animation;        
        uint32_t prev_frame = (uint32_t)(player->time * animation->framerate);
        player->time += delta_time * player->scale;
        uint32_t current_frame = (uint32_t)(player->time * animation->framerate);
//        printf("%d\n", current_frame);
        if(current_frame < animation->duration)
        {
//            uint32_t next_frame = (current_frame + 1) % animation->duration;
        
            if(prev_frame != current_frame)
            {
                struct a_transform_range_t *range = animation->ranges + current_frame;
                struct a_bone_transform_pair_t *pairs = animation->pairs + range->start;
                for(uint32_t pair_index = 0; pair_index < range->count; pair_index++)
                {
                    struct a_bone_transform_pair_t *pair = pairs + pair_index;
                    player->pairs[pair->bone_index] = pair->pair;
                }
            }
        }
        else
        {
            a_ResetAnimation(player);
            current_frame = 0;
        }
        
        for(uint32_t bone_index = 0; bone_index < animation->bone_count; bone_index++)
        {
            struct a_transform_pair_t *pair = player->pairs + bone_index;
            struct a_transform_t *transform = player->transforms + bone_index;
            
            struct a_transform_t *start = animation->transforms + pair->start;
            struct a_transform_t *end = animation->transforms + pair->end;
            
            float start_time = (float)current_frame / animation->framerate;
            float end_time = (float)pair->end_time / animation->framerate;

            float fraction = fabs(player->time - start_time) / fabs(end_time - start_time);
            
//            vec4_t_lerp(&transform->rot, &start->rot, &end->rot, fraction);
//            vec4_t_normalize(&transform->rot, &transform->rot);
//            vec3_t_lerp(&transform->pos, &start->pos, &end->pos, fraction);

            transform->rot = start->rot;
            transform->pos = start->pos;
        }
    }
    
    for(uint32_t player_index = 0; player_index < mixer->players.cursor; player_index++)
    {
        struct a_player_t *player = *(struct a_player_t **)get_list_element(&mixer->players, player_index);
        struct a_animation_t *animation = player->animation;
        for(uint32_t bone_index = 0; bone_index < animation->bone_count; bone_index++)
        {
//            struct a_channel_t *channel = animation->channels + bone_index;
//            if(channel->count)
//            {
            struct a_transform_t *transform = player->transforms + bone_index;
            mat4_t *mat_transform = mixer->transforms + bone_index;
            
            mat_transform->rows[0].x = 1.0 - 2.0 * (transform->rot.y * transform->rot.y + transform->rot.z * transform->rot.z);
            mat_transform->rows[0].y = 2.0 * (transform->rot.x * transform->rot.y + transform->rot.z * transform->rot.w);
            mat_transform->rows[0].z = 2.0 * (transform->rot.x * transform->rot.z - transform->rot.y * transform->rot.w);
            
            mat_transform->rows[1].x = 2.0 * (transform->rot.x * transform->rot.y - transform->rot.z * transform->rot.w);
            mat_transform->rows[1].y = 1.0 - 2.0 * (transform->rot.x * transform->rot.x + transform->rot.z * transform->rot.z);
            mat_transform->rows[1].z = 2.0 * (transform->rot.y * transform->rot.z + transform->rot.x * transform->rot.w);
            
            mat_transform->rows[2].x = 2.0 * (transform->rot.x * transform->rot.z + transform->rot.y * transform->rot.w);
            mat_transform->rows[2].y = 2.0 * (transform->rot.y * transform->rot.z - transform->rot.x * transform->rot.w);
            mat_transform->rows[2].z = 1.0 - 2.0 * (transform->rot.x * transform->rot.x + transform->rot.y * transform->rot.y);
            
            mat_transform->rows[3].x = transform->pos.x;
            mat_transform->rows[3].y = transform->pos.y;
            mat_transform->rows[3].z = transform->pos.z;
            
            mat_transform->rows[0].w = 0.0;
            mat_transform->rows[1].w = 0.0;
            mat_transform->rows[2].w = 0.0;
            mat_transform->rows[3].w = 1.0;
                
                
//            }
        } 
    }
    
    mat4_t parent_transform;
    mat4_t_identity(&parent_transform);
//    mat4_t_rotate_x(&parent_transform, -0.5);
    a_UpdateMixerTransforms(mixer, 0, &parent_transform);
//    a_DrawSkeleton(mixer);

    for(uint32_t bone_index = 0; bone_index < skeleton->bone_count; bone_index++)
    {
        mat4_t *transform = mixer->transforms + bone_index;
        mat4_t_mul(transform, &skeleton->bones[bone_index].inv_bind_matrix, transform);
    }

    for(uint32_t vert_index = 0; vert_index < model->vert_count; vert_index++)
    {
        struct r_vert_t *vert = model->verts + vert_index;
        
        vec4_t position = {0.0, 0.0, 0.0, 1.0};
        float total_weight = 0.0;
        for(uint32_t weight_index = 0; weight_index < R_MAX_VERTEX_WEIGHTS; weight_index++)
        {
            struct a_weight_t *weight = model->weights + vert_index * R_MAX_VERTEX_WEIGHTS + weight_index;
            if(weight->weight > 0.0)
            {
                vec4_t temp_pos = vec4_t_c(vert->pos.x, vert->pos.y, vert->pos.z, 1.0);
                mat4_t_vec4_t_mul(&temp_pos, mixer->transforms + weight->bone_index, &temp_pos);        
                vec4_t_fmadd(&position, &position, &temp_pos, weight->weight);
                total_weight += weight->weight;
            }
        }
//        printf("%f - %d\n", total_weight, vert_index);
        vert = copy->verts + vert_index;
        vert->pos = vec3_t_c(position.x, position.y, position.z);
    }
    
    r_FillVertices(copy->vert_chunk, copy->verts, copy->vert_count);
}

uint32_t a_DrawBones(struct a_mixer_t *mixer, uint32_t bone_index)
{
    struct a_bone_t *bone = mixer->skeleton->bones + bone_index;
    mat4_t *bone_transform = mixer->transforms + bone_index;
//    mat4_t transform;
    struct r_vert_t verts[2];
    
    verts[0].pos.x = bone_transform->rows[3].x;
    verts[0].pos.y = bone_transform->rows[3].y;
    verts[0].pos.z = bone_transform->rows[3].z;
    verts[0].color = vec3_t_c(1.0, 0.0, 0.0);
    
    r_DrawPoint(&verts[0].pos, &vec3_t_c(1.0, 1.0, 1.0), 4.0);
    
    bone_index++;
    
    for(uint32_t child_index = 0; child_index < bone->child_count; child_index++)
    {
        mat4_t *child_transform = mixer->transforms + bone_index;
        verts[1].pos.x = child_transform->rows[3].x;
        verts[1].pos.y = child_transform->rows[3].y;
        verts[1].pos.z = child_transform->rows[3].z;
        verts[1].color = vec3_t_c(1.0, 0.0, 0.0);
        r_DrawLines(verts, 2);
        
        bone_index = a_DrawBones(mixer, bone_index);
    }
    
    return bone_index;
}

void a_DrawSkeleton(struct a_mixer_t *mixer)
{
    a_DrawBones(mixer, 0);
}








