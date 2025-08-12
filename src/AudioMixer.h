#ifndef AUDIOMIXER_H
#define AUDIOMIXER_H

#include <SDL3/SDL.h>
#include <string>
#include <vector>
#include <map>
#include <cmath>

struct AudioPosition {
    float x, y, z;
    AudioPosition(float _x = 0.0f, float _y = 0.0f, float _z = 0.0f) : x(_x), y(_y), z(_z) {}
};

struct MixerAudio {
    SDL_AudioSpec spec;
    Uint8* buffer;
    Uint32 length;
    bool needs_free;
};

const int MAX_CHANNELS = 16;

struct Channel {
    MixerAudio* audio = nullptr;
    Uint32 position = 0;
    float volume = 1.0f;
    float left_gain = 1.0f;
    float right_gain = 1.0f;
    bool active = false;
};

class AudioMixer {
public:
    static void init();
    static void shutdown();
    
    static bool play_sound(const std::string& name);
    static bool play_sound_3d(const std::string& name, const AudioPosition& pos, float max_distance = 800.0f);
    static MixerAudio* load_sound(const std::string& path);
    static void add_sound(const std::string& name, MixerAudio* audio);
    
    static void set_listener_position(const AudioPosition& pos);
    static AudioPosition get_listener_position() { return listener_pos; }
    
    static SDL_AudioStream* get_stream() { return stream; }

private:
    static void audio_callback(void* userdata, SDL_AudioStream* stream, int additional_amount, int total_amount);

    static SDL_AudioStream* stream;
    static SDL_AudioSpec device_spec;
    static std::map<std::string, MixerAudio*> sounds;
    static AudioPosition listener_pos;
    
    static Channel channels[MAX_CHANNELS];
};

#endif