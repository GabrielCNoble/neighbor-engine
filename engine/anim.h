#ifndef ANIM_H
#define ANIM_H

#include "../lib/dstuff/ds_vector.h"
#include "../lib/dstuff/ds_matrix.h"
#include "../lib/dstuff/ds_list.h"
#include "../lib/dstuff/ds_slist.h"
#include <stdint.h>

/* influence of a single bone. No vertex id is necessary here, because
those weights are sorted by vertex id */
struct a_weight_t
{
    uint32_t bone_index;
    float weight;
};

/* range of bone weights per vertex. There's also no need to store a vertex id here
because those are also sorted by vertex id when exported. So, the first in the list
belongs to vertex 0, the second to vertex 1, and so on */
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
    uint32_t child_count;
};

struct a_bone_name_t
{
    char *name;
};

struct a_skeleton_t
{
    uint32_t index;
    uint32_t bone_count;
    struct a_bone_t *bones;
    struct a_bone_name_t *names;
};

/* pair of transforms to be used for a specific frame */
struct a_transform_pair_t
{
    /* start and end transforms */
    uint16_t start;
    uint16_t end;

    /* this is necessary for interpolation */
    uint16_t end_frame;
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
    /* each bone has one of those for each animation frame. If a bone doesn't get
    modified at all by this animation, every pair will reference invalid transforms */
    struct a_transform_pair_t *pairs;
};

enum A_MIXER_FLAGS
{
    A_MIXER_FLAG_COMPUTE_ROOT_DISPLACEMENT = 1,
};

struct a_mask_player_t
{
    struct a_player_t *player;
    float weight;
};

struct a_mask_t
{
    char *name;
    uint32_t index;
    uint32_t player_count;
    struct a_mask_player_t *players;
    uint32_t bone_count;
    uint32_t *bones;
};

struct a_mixer_t
{
    uint32_t index;
    uint32_t weight_count;
    uint32_t flags;
    struct a_weight_t *weights;
    struct a_skeleton_t *skeleton;
    struct r_model_t *model;
    struct ds_list_t players;
    struct ds_list_t mix_players;
    struct ds_list_t masks;
    uint8_t *touched_bones;
    /* result of all mixed players */
    struct a_transform_t *mixed_transforms;
    /* mixed transforms, with the bone_to_world transforms concatenated */
    mat4_t *bone_transforms;
    /* skinning matrices, which are just the inverse bind
    pose with the bone_to_world transforms concatenated */
    mat4_t *skinning_matrices;
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
    char *name;
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
    struct ds_list_t callbacks;
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

//struct a_skeleton_section_t
//{
//    uint32_t bone_count;
//    struct a_bone_t bones[];
//};

#define a_skeleton_section_t(b_count, bn_length)                       \
{                                                                      \
    uint32_t bone_names_length;                                        \
    uint32_t bone_count;                                               \
    struct a_bone_t bones[b_count];                                    \
    char bone_names[bn_length];                                        \
}

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

struct a_skeleton_t *a_CreateSkeleton(uint32_t bone_count, struct a_bone_t *bones, uint32_t bone_names_length, char *bone_names);

void a_DestroySkeleton(struct a_skeleton_t *skeleton);

struct a_player_t *a_CreatePlayer(char *player_name);

struct a_player_t *a_PlayAnimation(struct a_animation_t *animation, char *player_name);

void a_SeekAnimationAbsolute(struct a_player_t *player, float time);

void a_SeekAnimationToFrame(struct a_player_t *player, uint32_t frame);

void a_SeekAnimationRelative(struct a_player_t *player, float fraction);

void a_SetCallbackAbsolute(struct a_player_t *player, a_callback_function *function, void *data, float time);

void a_SetCallbackFrame(struct a_player_t *player, a_callback_function *function, void *data, uint32_t frame);

struct a_mixer_t *a_CreateMixer(struct r_model_t *model);

void a_MixAnimation(struct a_mixer_t *mixer, struct a_animation_t *animation, char *player_name);

uint32_t a_GetBoneIndex(struct a_mixer_t *mixer, char *bone_name);

mat4_t *a_GetBoneTransform(struct a_mixer_t *mixer, uint32_t bone_index);

struct a_mask_t *a_CreateAnimationMask(struct a_mixer_t *mixer, char *name, uint32_t player_count, char **player_names, uint32_t bone_count, char **bone_names);

struct a_mask_t *a_GetAnimationMask(struct a_mixer_t *mixer, char *name);

void a_DestroyAnimationMask(struct a_mixer_t *mixer, char *name);

struct a_player_t *a_GetMixerPlayer(struct a_mixer_t *mixer, char *name);

struct a_mask_player_t *a_GetMaskPlayer(struct a_mask_t *mask, char *name);

void a_UpdateAnimations(float delta_time);

void a_UpdateMixer(struct a_mixer_t *mixer, float delta_time);

void a_DrawSkeleton(struct a_mixer_t *mixer);


#endif // ANIM_H




