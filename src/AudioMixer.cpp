#include "AudioMixer.h"
#include <iostream>

SDL_AudioStream* AudioMixer::stream = nullptr;
SDL_AudioSpec AudioMixer::device_spec;
std::map<std::string, MixerAudio*> AudioMixer::sounds;

void AudioMixer::init() {
    SDL_zero(device_spec);
    device_spec.freq = 44100;
    device_spec.format = SDL_AUDIO_S16LE;
    device_spec.channels = 2;

    stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &device_spec, 
                                       audio_callback, nullptr);
    if (!stream) {
        std::cerr << "Failed to open audio mixer stream: " << SDL_GetError() << std::endl;
    } else {
        SDL_ResumeAudioDevice(SDL_GetAudioStreamDevice(stream));
    }
}

void AudioMixer::shutdown() {
    if (stream) {
        SDL_DestroyAudioStream(stream);
        stream = nullptr;
    }
    
    // Clean up all loaded sounds
    for (auto& [name, audio] : sounds) {
        if (audio->needs_free && audio->buffer) {
            SDL_free(audio->buffer);
        }
        delete audio;
    }
    sounds.clear();
}

MixerAudio* AudioMixer::load_sound(const std::string& path) {
    MixerAudio* audio = new MixerAudio();
    audio->needs_free = true;
    
    if (!SDL_LoadWAV(path.c_str(), &audio->spec, &audio->buffer, &audio->length)) {
        std::cerr << "Failed to load sound: " << path << " - " << SDL_GetError() << std::endl;
        delete audio;
        return nullptr;
    }
    
    return audio;
}

void AudioMixer::add_sound(const std::string& name, MixerAudio* audio) {
    if (audio) {
        sounds[name] = audio;
    }
}

bool AudioMixer::play_sound(const std::string& name) {
    if (!stream) return false;
    
    auto it = sounds.find(name);
    if (it == sounds.end() || !it->second->buffer) {
        return false;
    }
    
    MixerAudio* audio = it->second;
    
    // Convert audio if needed
    if (audio->spec.format != device_spec.format ||
        audio->spec.channels != device_spec.channels ||
        audio->spec.freq != device_spec.freq)
    {
        Uint8* converted_buffer = nullptr;
        int converted_size = 0;

        if (!SDL_ConvertAudioSamples(&audio->spec, audio->buffer, audio->length, 
                                   &device_spec, &converted_buffer, &converted_size)) {
            std::cerr << "Failed to convert audio for " << name << ": " << SDL_GetError() << std::endl;
            return false;
        }

        SDL_PutAudioStreamData(stream, converted_buffer, converted_size);
        SDL_free(converted_buffer);
    } else {
        // No conversion needed
        SDL_PutAudioStreamData(stream, audio->buffer, audio->length);
    }
    
    return true;
}

void AudioMixer::audio_callback(void* userdata, SDL_AudioStream* stream, int additional_amount, int total_amount) {
    // This callback is called when SDL3 needs more audio data
    // For our simple mixer, we don't need to do anything here since we're using SDL_PutAudioStreamData
}