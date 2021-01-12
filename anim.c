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
        struct a_anim_sec_data_t(header->transform_count, (header->bone_count * header->duration)) *data = (void *)(header + 1);
        
        uint32_t index = add_stack_list_element(&a_animations, NULL);
        animation = get_stack_list_element(&a_animations, index);
        animation->index = index;
        animation->bone_count = header->bone_count;
        animation->duration = header->duration;
        animation->framerate = header->framerate;
        animation->transform_count = header->transform_count;
//        animation->range_count = header->range_count;
//        animation->pair_count = header->pair_count;
        
        animation->transforms = mem_Calloc(animation->transform_count, sizeof(struct a_transform_t));
        memcpy(animation->transforms, data->transforms, sizeof(struct a_transform_t) * animation->transform_count);
        animation->pairs = mem_Calloc(header->duration * header->bone_count, sizeof(struct a_transform_pair_t));
        memcpy(animation->pairs, data->pairs, sizeof(struct a_transform_pair_t) * header->bone_count * header->duration);
//        animation->ranges = mem_Calloc(animation->range_count, sizeof(struct a_transform_range_t));
//        memcpy(animation->ranges, data->ranges, sizeof(struct a_transform_range_t) * animation->range_count);
        
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

struct a_player_t *a_CreatePlayer()
{
    struct a_player_t *player;
    uint32_t index;
    
    index = add_stack_list_element(&a_players, NULL);
    player = get_stack_list_element(&a_players, index);
    
    if(!player->callbacks.buffers)
    {
        player->callbacks = create_list(sizeof(struct a_callback_t), 4);
    }
    
    player->weight = 1.0;
    player->time = 0.0;
    player->scale = 1.0;    
    
    return player;
}

struct a_player_t *a_PlayAnimation(struct a_animation_t *animation)
{
    struct a_player_t *player = a_CreatePlayer();
    player->animation = animation;
//    player->pairs = mem_Calloc(animation->bone_count, sizeof(struct a_transform_pair_t));
    player->transforms = mem_Calloc(animation->bone_count, sizeof(struct a_transform_t));
    player->current_frame = 0xffff;
    a_SeekAnimationAbsolute(player, 0.0);
    return player;
}

void a_SeekAnimationAbsolute(struct a_player_t *player, float time)
{
    struct a_animation_t *animation = player->animation;
    struct a_mixer_t *mixer = player->mixer;
    float total_time = (float)animation->duration / animation->framerate;
    player->time = fmodf(time, total_time);
    uint32_t cur_frame = (uint32_t)(player->time * animation->framerate);
    uint32_t prev_frame = player->current_frame;
    
    if(player->current_frame != cur_frame)
    {
//        struct a_transform_pair_t *pairs = animation->pairs + cur_frame * animation->bone_count;
//        memcpy(player->pairs, pairs, sizeof(struct a_transform_pair_t) * animation->bone_count);
        player->pairs = animation->pairs + cur_frame * animation->bone_count;
        float cur_frame_time = (float)cur_frame / (float)animation->duration;
        float delta_time = player->time - cur_frame_time;
        
        for(uint32_t callback_index = 0; callback_index < player->callbacks.cursor; callback_index++)
        {
            struct a_callback_t *callback = get_list_element(&player->callbacks, callback_index);
            if(callback->frame >= player->current_frame && callback->frame < cur_frame)
            {
                /* call back the callback */
                callback->callback(callback->data, delta_time);
            }
        }
        
        player->current_frame = cur_frame;
    }
    
    for(uint32_t bone_index = 0; bone_index < animation->bone_count; bone_index++)
    {
        struct a_transform_pair_t *pair = player->pairs + bone_index;
        
        if(pair->start != pair->end)
        {
            struct a_transform_t *transform = player->transforms + bone_index;
        
            struct a_transform_t *start = animation->transforms + pair->start;
            struct a_transform_t *end = animation->transforms + pair->end;
            
            float start_time = (float)player->current_frame / animation->framerate;
            float end_time = (float)pair->end_frame / animation->framerate;

            float fraction = fabs(player->time - start_time) / fabs(end_time - start_time);

            quat_slerp(&transform->rot, &start->rot, &end->rot, fraction);
            vec3_t_lerp(&transform->pos, &start->pos, &end->pos, fraction);
        }
    }
    
    if(mixer && (mixer->flags & A_MIXER_FLAG_COMPUTE_ROOT_DISPLACEMENT))
    {
        struct a_transform_pair_t *pair = player->pairs;
        struct a_transform_t *current = player->transforms;
        
        if(pair->start != pair->end)
        {
            struct a_transform_t *start = animation->transforms + pair->start;
        
            if(!player->current_frame && (player->current_frame != prev_frame))
            {
                vec3_t start_prev_vec;
                vec3_t cur_prev_vec;
                vec3_t prev_offset;
                vec3_t_sub(&start_prev_vec, &player->prev_pos, &start->pos);
                vec3_t_sub(&cur_prev_vec, &player->prev_pos, &current->pos);
                vec3_t_sub(&prev_offset, &start_prev_vec, &cur_prev_vec);
                vec3_t_sub(&player->prev_pos, &start->pos, &prev_offset);
            }
            
            vec3_t disp;
            vec3_t_sub(&disp, &current->pos, &player->prev_pos);
            vec3_t_fmadd(&mixer->root_disp, &mixer->root_disp, &disp, 1.0);
            player->prev_pos = current->pos;
        }
    }
}

void a_SeekAnimationToFrame(struct a_player_t *player, uint32_t frame)
{
    struct a_animation_t *animation = player->animation;
    frame = frame % animation->duration;
    a_SeekAnimationAbsolute(player, (float)frame / (float)player->animation->duration);
}

void a_SeekAnimationRelative(struct a_player_t *player, float fraction)
{
    struct a_animation_t *animation = player->animation;
    float total_time = (float)animation->duration / animation->framerate;
    a_SeekAnimationAbsolute(player, total_time * fraction);
}

void a_SetCallbackAbsolute(struct a_player_t *player, a_callback_function *function, void *data, float time)
{
    uint32_t frame = (uint32_t)(time * player->animation->framerate);
    a_SetCallbackFrame(player, function, data, frame);
}

void a_SetCallbackFrame(struct a_player_t *player, a_callback_function *function, void *data, uint32_t frame)
{
    struct a_callback_t *callback;
    uint32_t index;
    
    index = add_list_element(&player->callbacks, NULL);
    callback = get_list_element(&player->callbacks, index);
    
    callback->callback = function;
    callback->data = data;
    callback->frame = frame % player->animation->duration;
}

struct a_mixer_t *a_MixAnimation(struct a_mixer_t *mixer, struct a_animation_t *animation, struct r_model_t *model)
{
    if(!mixer)
    {
        uint32_t index = add_stack_list_element(&a_mixers, NULL);
        mixer = get_stack_list_element(&a_mixers, index);
        mixer->index = index;
        mixer->skeleton = model->skeleton;
        mixer->weight_count = model->weight_count;
        mixer->weights = model->weights;
        mixer->model = model;
        mixer->skinning_matrices = mem_Calloc(mixer->skeleton->bone_count, sizeof(mat4_t));
        mixer->transforms = mem_Calloc(mixer->skeleton->bone_count, sizeof(struct a_transform_t));
        mixer->touched_bones = mem_Calloc(mixer->skeleton->bone_count, sizeof(uint16_t));
        
        if(!mixer->players.buffers)
        {
            mixer->players = create_list(sizeof(struct a_player_t *), 8);
            mixer->mix_players = create_list(sizeof(struct a_player_t *), 8);
            
            for(uint32_t transform_index = 0; transform_index < mixer->skeleton->bone_count; transform_index++)
            {
//                mat4_t_identity(mixer->skinning_matrices + transform_index);
                mixer->skinning_matrices[transform_index] = mixer->skeleton->bones[transform_index].transform;
                mixer->transforms[transform_index].rot = vec4_t_c(0.0, 0.0, 0.0, 1.0);
                mixer->transforms[transform_index].pos = vec3_t_c(0.0, 0.0, 0.0);
            }
        }
    }
    
    if(animation)
    {
        struct a_player_t *player = a_PlayAnimation(animation);
        player->mixer = mixer;
        add_list_element(&mixer->players, &player);
    }
    
    return mixer;
}

struct a_player_t *a_GetPlayer(struct a_mixer_t *mixer, uint32_t index)
{
    struct a_player_t **player;
    
    player = get_list_element(&mixer->players, index);
    
    if(player)
    {
        return *player;
    }
    
    return NULL;
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
    mat4_t *transform = mixer->skinning_matrices + bone_index;
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
    
    mixer->root_disp = vec3_t_c(0.0, 0.0, 0.0);
    mixer->mix_players.cursor = 0;
    float total_weight = 0.0;
    for(uint32_t player_index = 0; player_index < mixer->players.cursor; player_index++)
    {
        struct a_player_t *player = *(struct a_player_t **)get_list_element(&mixer->players, player_index);
        a_SeekAnimationAbsolute(player, player->time + delta_time * player->scale);
        if(player->weight)
        {
            add_list_element(&mixer->mix_players, &player);
            total_weight += player->weight;
        }
    }
    
    uint32_t first_bone = 0;
    
    if(mixer->flags & A_MIXER_FLAG_COMPUTE_ROOT_DISPLACEMENT)
    {
        mixer->skinning_matrices[0] = mixer->skeleton->bones->transform;
        first_bone = 1;
    }
    
    for(uint32_t bone_index = 0; bone_index < mixer->skeleton->bone_count; bone_index++)
    {
        mixer->touched_bones[bone_index] = 0;
    }
    
    if(mixer->mix_players.cursor)
    {
        struct a_player_t *player = *(struct a_player_t **)get_list_element(&mixer->mix_players, 0);
        
        /* first player has to have weight == 1.0, so whatever transform data present from a previous 
        update gets completely stomped */
        float denom = player->weight;
        
        for(uint32_t player_index = 0; player_index < mixer->mix_players.cursor; player_index++)
        {
            player = *(struct a_player_t **)get_list_element(&mixer->mix_players, player_index);
            struct a_animation_t *animation = player->animation;
            float weight = player->weight / total_weight;
            denom = total_weight;
            
            for(uint32_t bone_index = first_bone; bone_index < animation->bone_count; bone_index++)
            {
                struct a_transform_pair_t *pair = player->pairs + bone_index;
                
                if(pair->start != pair->end)
                {
                    struct a_transform_t *transform = player->transforms + bone_index;
                    struct a_transform_t *mixed_transform = mixer->transforms + bone_index;
                    quat_slerp(&mixed_transform->rot, &mixed_transform->rot, &transform->rot, weight);
                    vec3_t_lerp(&mixed_transform->pos, &mixed_transform->pos, &transform->pos, weight);
                    mixer->touched_bones[bone_index] = 1;
                }
            }
        }
    }
    
    for(uint32_t bone_index = first_bone; bone_index < mixer->skeleton->bone_count; bone_index++)
    {
        struct a_transform_t *transform = mixer->transforms + bone_index;
        mat4_t *mat_transform = mixer->skinning_matrices + bone_index;
        
        if(mixer->touched_bones[bone_index])
        {
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
        }
        else
        {
            *mat_transform = mixer->skeleton->bones[bone_index].transform;
        }
        
//        transform->rot = vec4_t_c(0.0, 0.0, 0.0, 1.0);
//        transform->pos = vec3_t_c(0.0, 0.0, 0.0);
    }
    
    mat4_t parent_transform;
    mat4_t_identity(&parent_transform);
    mat4_t_rotate_x(&parent_transform, -0.5);
    a_UpdateMixerTransforms(mixer, 0, &parent_transform);

    for(uint32_t bone_index = 0; bone_index < skeleton->bone_count; bone_index++)
    {
        mat4_t *transform = mixer->skinning_matrices + bone_index;
        mat4_t_mul(transform, &skeleton->bones[bone_index].inv_bind_matrix, transform);
    }

    for(uint32_t vert_index = 0; vert_index < model->vert_count; vert_index++)
    {
        struct r_vert_t *vert = model->verts + vert_index;
        struct a_weight_range_t *range = model->weight_ranges + vert_index;
        struct a_weight_t *weights = model->weights + range->start;
        vec4_t position = {0.0, 0.0, 0.0, 1.0};
        vec4_t normal = {};
        for(uint32_t weight_index = 0; weight_index < range->count; weight_index++)
        {
            struct a_weight_t *weight = weights + weight_index;
            vec4_t temp_pos = vec4_t_c(vert->pos.x, vert->pos.y, vert->pos.z, 1.0);
            vec4_t temp_normal = vec4_t_c(vert->normal.x, vert->normal.y, vert->normal.z, 0.0);
            mat4_t_vec4_t_mul(&temp_pos, mixer->skinning_matrices + weight->bone_index, &temp_pos);        
            mat4_t_vec4_t_mul(&temp_normal, mixer->skinning_matrices + weight->bone_index, &temp_normal);        
            vec4_t_mul(&temp_pos, &temp_pos, weight->weight);
            vec4_t_add(&position, &position, &temp_pos);
            vec4_t_mul(&temp_normal, &temp_normal, weight->weight);
            vec4_t_add(&normal, &normal, &temp_normal);
        }
        
        vert = copy->verts + vert_index;
        vert->pos = vec3_t_c(position.x, position.y, position.z);
        vert->normal = vec3_t_c(normal.x, normal.y, normal.z);
    }
    
    r_FillVertices(copy->vert_chunk, copy->verts, copy->vert_count);
}

uint32_t a_DrawBones(struct a_mixer_t *mixer, uint32_t bone_index)
{
    struct a_bone_t *bone = mixer->skeleton->bones + bone_index;
    mat4_t *bone_transform = mixer->skinning_matrices + bone_index;
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
        mat4_t *child_transform = mixer->skinning_matrices + bone_index;
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








