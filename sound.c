#include <stdlib.h>
#include <stdio.h>
#include "tinycthread.h"
#include <string.h>
#include <stdatomic.h>
#include "sound.h"
#include "AL/alc.h" 
#include "SDL2/SDL.h"
#include "stb_vorbis.h"

ALCcontext *        s_audio_context;
ALCdevice *         s_audio_device;
thrd_t              s_audio_thread;
uint32_t            s_run_thread = 1;

// ALuint              s_sound_sources[MAX_SOUND_SOURCES];
struct source_t     s_sound_sources[MAX_SOUND_SOURCES];

mtx_t               s_free_source_mutex;
uint32_t            s_free_source_count = 0;
struct source_t *   s_free_sources[MAX_SOUND_SOURCES];

uint32_t            s_active_source_count = 0;
struct source_t *   s_active_sources[MAX_SOUND_SOURCES];

uint32_t            s_start_source_in = 0;
uint32_t            s_start_source_out = 0;
struct source_t *   s_start_sources[MAX_SOUND_SOURCES];

int audio_thread_func(void *arg)
{
    ALuint processed_buffers[2];

    while(s_run_thread)
    {
        for(uint32_t index = 0; index < s_active_source_count; index++)
        {
            struct source_t *source = s_active_sources[index];

            ALint processed_buffer_count;
            alGetSourcei(source->source, AL_BUFFERS_PROCESSED, &processed_buffer_count);
            
            while(processed_buffer_count > 0)
            {
                ALint unqueue_count = 2;
                source->current_buffer += unqueue_count;

                if(unqueue_count > processed_buffer_count)
                {
                    unqueue_count = processed_buffer_count;
                }

                alSourceUnqueueBuffers(source->source, unqueue_count, processed_buffers);

                processed_buffer_count -= unqueue_count;
            }

            ALint status;
            alGetSourcei(source->source, AL_SOURCE_STATE, &status);

            if(status == AL_STOPPED)
            {
                if(index < s_active_source_count - 1)
                {
                    s_active_sources[index] = s_active_sources[s_active_source_count - 1];
                }
                s_active_source_count--;
                index--;

                free_source(source);
            }
        }

        uint32_t source_in = s_start_source_in;

        while(s_start_source_out != source_in)
        {
            struct source_t *source = s_start_sources[s_start_source_out];
            alSourcePlay(source->source);

            s_active_sources[s_active_source_count] = source;
            s_active_source_count++;

            s_start_source_out = (s_start_source_out + 1) % MAX_SOUND_SOURCES;
        }

        SDL_Delay(10);
    }

    return 0;
}

uint32_t sound_init()
{
    s_audio_device = alcOpenDevice(NULL);

    if(s_audio_device == NULL)
    {
        printf("well, cock...\n");
        return 0;
    }

    ALCint attribs[] = {ALC_STEREO_SOURCES, MAX_SOUND_SOURCES, 0, 0};
    
    s_audio_context = alcCreateContext(s_audio_device, attribs);

    if(s_audio_context == NULL)
    {
        printf("well, piss...\n");
        return 0;
    }

    alcMakeContextCurrent(s_audio_context);
    for(uint32_t index = 0; index < MAX_SOUND_SOURCES; index++)
    {
        struct source_t *source = s_sound_sources + index;
        alGenSources(1, &source->source);
        source->alloc_index = 0xffffffff;
        s_free_sources[index] = source;
    }

    s_free_source_count = MAX_SOUND_SOURCES;

    thrd_create(&s_audio_thread, audio_thread_func, NULL);
    thrd_detach(s_audio_thread);

    mtx_init(&s_free_source_mutex, mtx_plain);

    return 1;
}

void sound_shutdown()
{
    s_run_thread = 0;
}

struct sound_t load_sound(const char *file_name)
{
    struct sound_t sound = {};
    int channels;
    int sample_rate;
    short *samples;
    int sample_count = stb_vorbis_decode_filename(file_name, &channels, &sample_rate, &samples);

    if(sample_count > 0)
    {
        uint32_t format;

        if(channels == 1)
        {
            format = AL_FORMAT_MONO16;
        }
        else
        {
            format = AL_FORMAT_STEREO16;
        }

        alGenBuffers(1, &sound.buffer);
        alBufferData(sound.buffer, format, samples, sizeof(short) * sample_count * channels, sample_rate);
        free(samples);
    }
    else
    {
        printf("couldn't load sound %s\n", file_name);
    }

    return sound;
}

void free_sound(struct sound_t sound)
{
 
}

struct source_t *play_sound(struct sound_t *sound, uint32_t loop, float volume)
{
    struct source_t *source = alloc_source();

    if(source != NULL && sound != NULL)
    {
        alSourcef(source->source, AL_GAIN, volume);
        alSourcei(source->source, AL_LOOPING, loop);
        queue_sound(source, sound);
        s_start_sources[s_start_source_in] = source;
        s_start_source_in = (s_start_source_in + 1) % MAX_SOUND_SOURCES;
    }

    return source;
}

void stop_sound(struct source_t *source)
{
    if(source != NULL)
    {
        alSourceStop(source->source);
    }
}

void queue_sound(struct source_t *source, struct sound_t *sound)
{
    if(source != NULL)
    {
        alSourceQueueBuffers(source->source, 1, &sound->buffer);
    }
}

void loop_source(struct source_t *source, uint32_t loop)
{
    if(source != NULL)
    {
        alSourcei(source->source, AL_LOOPING, loop);
    }
}

struct source_t *alloc_source()
{
    struct source_t *source = NULL;

    if(s_free_source_count > 0)
    {
        mtx_lock(&s_free_source_mutex);
        s_free_source_count--;
        source = s_free_sources[s_free_source_count];
        source->alloc_index = s_free_source_count;
        mtx_unlock(&s_free_source_mutex);
    }

    return source;
}

void free_source(struct source_t *source)
{
    if(source != NULL && source->alloc_index != 0xffffffff)
    {
        mtx_lock(&s_free_source_mutex);
        s_free_sources[s_free_source_count] = source;
        s_free_source_count++;
        source->alloc_index = 0xffffffff;
        mtx_unlock(&s_free_source_mutex);
    }
}