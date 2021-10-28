#ifndef SOUND_H
#define SOUND_H

#include "dstuff/ds_vector.h"
#include <stdint.h>

struct s_sound_t
{
    uint32_t buffer;
    uint32_t index;
    float duration;
};

enum S_SOURCE_FLAGS
{
    S_SOURCE_FLAG_PERSISTENT = 1,
};

struct s_source_t
{
    vec3_t position;
    float volume;
    uint32_t source;
    uint32_t flags;
    uint32_t index;
    struct s_sound_t *sound;
};

enum S_COMMAND_TYPE
{
    S_COMMAND_PLAY_SOUND = 0,
    S_COMMAND_PAUSE_SOURCE,
    S_COMMAND_STOP_SOURCE,
    S_COMMAND_RESUME_SOURCE,
};

struct s_command_t 
{
    uint32_t type;
    struct s_source_t *source;
    struct s_sound_t *sound;
    vec3_t position;
    float volume;
};


void s_Init();

void s_Shutdown();

struct s_sound_t *s_LoadSound(char *file_name);

struct s_source_t *s_AllocateSource();

void s_FreeSource(struct s_source_t *source);

struct s_source_t *s_PlaySound(struct s_sound_t *sound, vec3_t *position, float volume, uint32_t persist);

void s_ResumeSource(struct s_source_t *source);

void s_PauseSource(struct s_source_t *source);

void s_StopSource(struct s_source_t *source);

void s_QueueCommand(struct s_command_t *command);

int s_SoundThread(void *arg);




#endif // SOUND_H
