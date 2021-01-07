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

struct a_weight_range_t
{
    uint32_t start;
    uint32_t count;
};

struct a_transform_t
{
    vec4_t rot;
    vec3_t pos;
};

struct a_bone_t  
{
    mat4_t transform;
    mat4_t inv_bind_matrix;
//    struct a_transform_t inv_bind_transform;
    uint32_t child_count;
};

struct a_skeleton_t
{
    uint32_t index;
    uint32_t bone_count;
    struct a_bone_t *bones;
};

struct a_transform_pair_t
{
    uint16_t start;
    uint16_t end;
    uint16_t end_frame;
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

enum A_ANIMATION_FLAGS
{
    A_ANIMATION_FLAG_TRANSFORMS_ROOT = 1,
};

struct a_animation_t
{
    char *name;
    uint32_t index;
    uint32_t duration;
    float framerate;
    uint32_t flags;
    uint32_t bone_count;
    uint32_t transform_count;
    struct a_transform_t *transforms;
    struct a_transform_pair_t *pairs;
};

enum A_MIXER_FLAGS
{
    A_MIXER_FLAG_COMPUTE_ROOT_DISPLACEMENT = 1,
};

struct a_mixer_t
{
    uint32_t index;
    uint32_t weight_count;
    uint32_t flags;
    struct a_weight_t *weights;
    struct a_skeleton_t *skeleton;
    struct r_model_t *model;
    struct list_t players;
    /* final transforms ready to be used for skinning. Those are computed
    after all tracks have been mixed */
    struct a_transform_t *mixed_transforms;
    mat4_t *transforms;
    vec3_t root_disp;
};

typedef void (a_callback_function)(void *data, float delta_time);

struct a_callback_t
{
    uint32_t frame;
    float time;
    a_callback_function *callback;
    void *data;
};

struct a_player_t
{
    uint32_t index;
    float time;
    float weight;
    float scale;
    vec3_t prev_pos;
    vec3_t root_disp;
    uint16_t current_frame;
    struct a_mixer_t *mixer;
    struct a_animation_t *animation;
    struct a_transform_pair_t *pairs;
    struct a_transform_t *transforms;
    struct list_t callbacks;
};


/* serialization structs */
struct a_weight_record_t 
{
    uint32_t vert_index;
    struct a_weight_t weight;
};

#define a_weight_section_t(w_count, r_count)                  \
{                                                             \
    uint32_t weight_count;                                    \
    uint32_t range_count;                                     \
    struct a_weight_t weights[w_count];                       \
    struct a_weight_range_t ranges[r_count];                  \
}
    
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
};

#define a_anim_sec_data_t(keyframe_count, pair_count)      \
{                                                          \
    struct a_transform_pair_t pairs[pair_count];           \
    struct a_transform_t transforms[keyframe_count];       \
}

void a_Init();

void a_Shutdown();

struct a_animation_t *a_LoadAnimation(char *file_name);

struct a_skeleton_t *a_CreateSkeleton(uint32_t bone_count, struct a_bone_t *bones);

struct a_player_t *a_CreatePlayer();

struct a_player_t *a_PlayAnimation(struct a_animation_t *animation);

void a_SeekAnimationAbsolute(struct a_player_t *player, float time);

void a_SeekAnimationToFrame(struct a_player_t *player, uint32_t frame);

void a_SeekAnimationRelative(struct a_player_t *player, float fraction);

void a_SetCallbackAbsolute(struct a_player_t *player, a_callback_function *function, void *data, float time);

void a_SetCallbackFrame(struct a_player_t *player, a_callback_function *function, void *data, uint32_t frame);

struct a_mixer_t *a_MixAnimation(struct a_mixer_t *mixer, struct a_animation_t *animation, struct r_model_t *model);

struct a_player_t *a_GetPlayer(struct a_mixer_t *mixer, uint32_t index);

void a_UpdateAnimations(float delta_time);

void a_UpdateMixer(struct a_mixer_t *mixer, float delta_time);

void a_DrawSkeleton(struct a_mixer_t *mixer);


#endif // ANIM_H




