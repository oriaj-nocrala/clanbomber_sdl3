#ifndef AUDIO_H
#define AUDIO_H

#include <SDL3/SDL.h>

struct Sound;

class Audio {
public:
    static void init();
    static void shutdown();
    static void play(Sound* sound);

private:
    static SDL_AudioStream* stream;
    static SDL_AudioSpec device_spec;
};

#endif