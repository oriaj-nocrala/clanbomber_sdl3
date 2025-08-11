#ifndef AUDIOMIXER_H
#define AUDIOMIXER_H

#include <SDL3/SDL.h>
#include <string>
#include <map>

struct MixerAudio {
    SDL_AudioSpec spec;
    Uint8* buffer;
    Uint32 length;
    bool needs_free;
};

class AudioMixer {
public:
    static void init();
    static void shutdown();
    
    static bool play_sound(const std::string& name);
    static MixerAudio* load_sound(const std::string& path);
    static void add_sound(const std::string& name, MixerAudio* audio);
    
    static SDL_AudioStream* get_stream() { return stream; }

private:
    static SDL_AudioStream* stream;
    static SDL_AudioSpec device_spec;
    static std::map<std::string, MixerAudio*> sounds;
    
    static void audio_callback(void* userdata, SDL_AudioStream* stream, int additional_amount, int total_amount);
};

#endif