#ifndef SOUND_H
#define SOUND_H

#include "AL/al.h"
#include <stdint.h>

#define MAX_SOUND_SOURCES 128

struct sound_t
{
    ALuint buffer;
};

struct source_t
{
    ALuint      source;
    uint32_t    current_buffer;
    uint32_t    alloc_index;
    uint32_t    loop;
};

uint32_t sound_init(); 

void sound_shutdown();

struct sound_t load_sound(const char *file_name);

void free_sound(struct sound_t sound);

struct source_t *play_sound(struct sound_t *sound, uint32_t loop, float volume);

void stop_sound(struct source_t *source);

void queue_sound(struct source_t *source, struct sound_t *sound);

void loop_source(struct source_t *source, uint32_t loop);

struct source_t *alloc_source();

void free_source(struct source_t *source);



#endif