#ifndef ANIM_H
#define ANIM_H

#include "dstuff/ds_vector.h"
#include "dstuff/ds_matrix.h"
#include "dstuff/ds_list.h"
#include "dstuff/ds_stack_list.h"
#include <stdint.h>

struct a_weight_t
{
    uint32_t bone_index;
    float weight;
};

struct a_bone_t  
{
    mat4_t transform;
    mat4_t inv_bind_matrix;
    uint32_t child_count;
};

struct a_skeleton_t
{
    uint32_t index;
    uint32_t bone_count;
    struct a_bone_t *bones;
};

struct a_transform_t
{
    vec4_t rot;
    vec3_t pos;
};

//struct a_keyframe_t 
//{
//    float time;
//    uint32_t bone_index;
//    struct a_transform_t transform;
//};

//struct a_keyframe_pair_t
//{
//    struct a_keyframe_t *start;
//    struct a_keyframe_t *end;
//};

struct a_transform_pair_t
{
    uint16_t start;
    uint16_t end;
    uint16_t end_time;
};

struct a_bone_transform_pair_t
{
    uint16_t bone_index;
    struct a_transform_pair_t pair;
};

struct a_transform_range_t
{
    uint16_t start;
    uint16_t count;
};

//struct a_channel_t
//{
//    uint32_t start;
//    uint32_t count;
//};

//struct a_animation_t
//{
//    char *name;
//    uint32_t index;
//    uint32_t duration;
//    float framerate;
//    uint32_t bone_count;
//    
//    /* channels, one per bone. Each channel is a slice into the
//    keyframe_indices array */
//    struct a_channel_t *channels;
//    uint32_t keyframe_count;
//    /* indices into the keyframes array, grouped by bone, required
//    to quickly find the previous/next keyframe for a specific bone,
//    starting from a specific keyframe */
//    uint32_t *keyframe_indices;
//    
//    /* keyframes, sorted by time */
//    struct a_keyframe_t *keyframes;
//};

struct a_animation_t
{
    char *name;
    uint32_t index;
    uint32_t duration;
    float framerate;
    uint32_t bone_count;
    uint32_t transform_count;
    struct a_transform_t *transforms;
    uint32_t pair_count;
    struct a_bone_transform_pair_t *pairs;
    uint32_t range_count;
    struct a_transform_range_t *ranges;
};

struct a_player_t
{
    uint32_t index;
    float time;
    float weight;
    float scale;
    uint32_t keyframe_cursor;
    struct a_animation_t *animation;
    struct a_transform_pair_t *pairs;
    struct a_transform_t *transforms;
};

struct a_mixer_t
{
    uint32_t index;
    uint32_t weight_count;
    struct a_weight_t *weights;
    struct a_skeleton_t *skeleton;
    struct r_model_t *model;
    struct list_t players;
    /* final transforms ready to be used for skinning. Those are computed
    after all tracks have been mixed */
    mat4_t *transforms;
};





/* serialization structs */
struct a_weight_record_t 
{
    uint32_t vert_index;
    struct a_weight_t weight;
};

struct a_weight_section_t
{
    uint32_t weight_count;
    struct a_weight_record_t weights[];
};

struct a_skeleton_section_t
{
    uint32_t bone_count;
    struct a_bone_t bones[];
};

struct a_anim_sec_header_t
{
    char name[64];
    uint32_t duration;
    float framerate;
    uint32_t bone_count;
    uint32_t transform_count;
    uint32_t pair_count;
    uint32_t range_count;
};

/*#define a_anim_sec_data_t(kfs, bns)             \
{                                               \
    uint32_t keyframe_indices[kfs];             \
    struct a_keyframe_t keyframes[kfs];         \
    struct a_channel_t channels[bns];           \
}*/

#define a_anim_sec_data_t(keyframe_count, pair_count, range_count)               \
{                                                                                \
    struct a_bone_transform_pair_t pairs[pair_count];                                 \
    struct a_transform_range_t ranges[range_count];                              \
    struct a_transform_t transforms[keyframe_count];                              \
}

//struct a_animation_section_t  
//{
//    char name[64];
//    float duration;
//    float framerate;
//    uint32_t bone_count;
//    uint32_t keyframe_count;
//    struct a_keyframe_t keyframes[];
//};

void a_Init();

void a_Shutdown();

struct a_animation_t *a_LoadAnimation(char *file_name);

struct a_skeleton_t *a_CreateSkeleton(uint32_t bone_count, struct a_bone_t *bones);

struct a_player_t *a_CreatePlayer(struct a_animation_t *animation);

void a_ResetAnimation(struct a_player_t *player);

struct a_mixer_t *a_PlayAnimation(struct a_animation_t *animation, struct r_model_t *model);

void a_UpdateAnimations(float delta_time);

void a_UpdateMixer(struct a_mixer_t *mixer, float delta_time);

void a_DrawSkeleton(struct a_mixer_t *mixer);


#endif // ANIM_H




