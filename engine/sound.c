#include <stdio.h>
#include "sound.h"
#include "al.h"
#include "alc.h"
#include "SDL2/SDL_thread.h"
#include "dstuff/ds_slist.h"
#include "dstuff/ds_list.h"
#include "dstuff/ds_file.h"
#include "dstuff/ds_mem.h"
#include "codec.h"
#include "vorbisfile.h"
#include "tinycthread.h"
#include "log.h"

ALCdevice *s_device;
ALCcontext *s_context;
SDL_Thread *s_sound_thread;
spnl_t s_source_spinlock;
spnl_t s_command_spinlock;
struct ds_slist_t s_sources;
struct ds_slist_t s_sounds;
struct ds_list_t s_command_queue;
struct ds_list_t s_active_sources;


void s_Init()
{
    log_ScopedLogMessage(LOG_TYPE_NOTICE, "Initializing sound...");
    s_device = alcOpenDevice(NULL);

    s_context = alcCreateContext(s_device, (ALCint []){ALC_STEREO_SOURCES, 350, 0});
    alcMakeContextCurrent(s_context);
    s_sound_thread = SDL_CreateThread(s_SoundThread, "sound thread", NULL);

    s_sources = ds_slist_create(sizeof(struct s_source_t), 350);
    s_active_sources = ds_list_create(sizeof(struct s_source_t *), 350);
    s_command_queue = ds_list_create(sizeof(struct s_command_t), 350);
    s_sounds = ds_slist_create(sizeof(struct s_sound_t), 350);

    float listener_orientation[] =
    {
        0.0, 0.0, -1.0,
        0.0, 1.0, 0.0,
    };
    alListener3f(AL_POSITION, 0.0, 0.0, 0.0);
    alListenerfv(AL_ORIENTATION, listener_orientation);

    log_ScopedLogMessage(LOG_TYPE_NOTICE, "Vendor: %s", alGetString(AL_VENDOR));
    log_ScopedLogMessage(LOG_TYPE_NOTICE, "Version: %s", alGetString(AL_VERSION));
    log_ScopedLogMessage(LOG_TYPE_NOTICE, "Renderer: %s", alGetString(AL_RENDERER));

    log_ScopedLogMessage(LOG_TYPE_NOTICE, "Sound initialized!");
}

void s_Shutdown()
{
    log_ScopedLogMessage(LOG_TYPE_NOTICE, "Shutting down sound...");
    log_ScopedLogMessage(LOG_TYPE_NOTICE, "Sound shut down!");
}

struct s_sound_t *s_LoadSound(char *file_name)
{
    struct s_sound_t *sound = NULL;
    uint32_t index;

    if(ds_file_exists(file_name))
    {
        OggVorbis_File file;
        vorbis_info *info;
        ov_fopen(file_name, &file);
        info = ov_info(&file, -1);
        uint32_t read_bytes = 0;
        uint32_t total_samples = ov_pcm_total(&file, -1);
        uint32_t buffer_size = info->channels * total_samples * sizeof(uint16_t);
        void *buffer;
        buffer = mem_Calloc(buffer_size, 1);
        buffer_size = 0;

        while((read_bytes = ov_read(&file, (char *)buffer + buffer_size, 8192, 0, 2, 1, &(int){0})))
        {
            buffer_size += read_bytes;
        }

        index = ds_slist_add_element(&s_sounds, NULL);
        sound = ds_slist_get_element(&s_sounds, index);
        sound->index = index;

        uint32_t format;
        switch(info->channels)
        {
            case 1:
                format = AL_FORMAT_MONO16;
            break;

            case 2:
                format = AL_FORMAT_STEREO16;
            break;
        }

        alGenBuffers(1, &sound->buffer);
        alBufferData(sound->buffer, format, buffer, buffer_size, info->rate);
        ov_clear(&file);
    }

    return sound;
}

struct s_source_t *s_AllocateSource()
{
    struct s_source_t *source;
    uint32_t index;

    spnl_lock(&s_source_spinlock);
    index = ds_slist_add_element(&s_sources, NULL);
    spnl_unlock(&s_source_spinlock);

    source = ds_slist_get_element(&s_sources, index);

    source->index = index;
    source->sound = NULL;
    source->volume = 1.0;

    if(!source->source)
    {
        alGenSources(1, &source->source);
    }

    return source;
}

void s_FreeSource(struct s_source_t *source)
{
    if(source && source->index != 0xffffffff)
    {
        spnl_lock(&s_source_spinlock);
        ds_slist_remove_element(&s_sources, source->index);
        spnl_unlock(&s_source_spinlock);
        source->index = 0xffffffff;
    }
}

struct s_source_t *s_PlaySound(struct s_sound_t *sound, vec3_t *position, float volume, uint32_t persist)
{
    struct s_source_t *source = NULL;
    struct s_command_t command = {};
    if(persist)
    {
        source = s_AllocateSource();
    }

    command.type = S_COMMAND_PLAY_SOUND;
    command.sound = sound;
    command.source = source;
    command.position = *position;
    command.volume = volume;

    s_QueueCommand(&command);

    return source;
}

void s_ResumeSource(struct s_source_t *source)
{
    struct s_command_t command = {};
    command.type = S_COMMAND_RESUME_SOURCE;
    command.source = source;
    s_QueueCommand(&command);
}

void s_PauseSource(struct s_source_t *source)
{
    struct s_command_t command = {};
    command.type = S_COMMAND_PAUSE_SOURCE;
    command.source = source;
    s_QueueCommand(&command);
}

void s_StopSource(struct s_source_t *source)
{
    struct s_command_t command = {};
    command.type = S_COMMAND_STOP_SOURCE;
    command.source = source;
    s_QueueCommand(&command);
}

void s_QueueCommand(struct s_command_t *command)
{
    spnl_lock(&s_command_spinlock);
    ds_list_add_element(&s_command_queue, command);
    spnl_unlock(&s_command_spinlock);
}

int s_SoundThread(void *arg)
{
    while(1)
    {
        spnl_lock(&s_command_spinlock);
        for(uint32_t command_index = 0; command_index < s_command_queue.cursor; command_index++)
        {
            struct s_command_t *command = ds_list_get_element(&s_command_queue, command_index);
            uint32_t source_state;
            switch(command->type)
            {
                case S_COMMAND_PLAY_SOUND:
                    if(!command->source)
                    {
                        command->source = s_AllocateSource();
                    }

                    command->source->sound = command->sound;
                    command->source->position = command->position;
                    command->source->volume = command->volume;

                    alSourcei(command->source->source, AL_BUFFER, command->sound->buffer);
                    alSourcef(command->source->source, AL_GAIN, command->volume);
                    alSourcefv(command->source->source, AL_POSITION, command->position.comps);
                    alSourcei(command->source->source, AL_SOURCE_RELATIVE, AL_TRUE);

                    ds_list_add_element(&s_active_sources, &command->source);

                    /* fallthrough */

                case S_COMMAND_RESUME_SOURCE:
                    alSourcePlay(command->source->source);
                break;

                case S_COMMAND_PAUSE_SOURCE:
                    alSourcePause(command->source->source);
                break;

                case S_COMMAND_STOP_SOURCE:
                    alSourceStop(command->source->source);
                break;
            }
        }
        s_command_queue.cursor = 0;
        spnl_unlock(&s_command_spinlock);

        for(uint32_t source_index = 0; source_index < s_active_sources.cursor; source_index++)
        {
            struct s_source_t *source = *(struct s_source_t **)ds_list_get_element(&s_active_sources, source_index);
            int32_t state;
            alGetSourcei(source->source, AL_SOURCE_STATE, &state);
            if(state == AL_STOPPED)
            {
                if(!(source->flags & S_SOURCE_FLAG_PERSISTENT))
                {
                    ds_list_remove_element(&s_active_sources, source_index);
                    source_index--;
                    s_FreeSource(source);
                }

                continue;
            }

            alSourcefv(source->source, AL_POSITION, source->position.comps);
            alSourcef(source->source, AL_GAIN, source->volume);
        }
        SDL_Delay(16);
    }
}








