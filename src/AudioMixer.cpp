#include "AudioMixer.h"
#include <iostream>
#include <algorithm>

SDL_AudioStream* AudioMixer::stream = nullptr;
SDL_AudioSpec AudioMixer::device_spec;
std::map<std::string, MixerAudio*> AudioMixer::sounds;
AudioPosition AudioMixer::listener_pos(400.0f, 300.0f, 0.0f); // Default listener at screen center

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
    // Play at listener position (no 3D effect)
    return play_sound_3d(name, listener_pos, 0.0f);
}

bool AudioMixer::play_sound_3d(const std::string& name, const AudioPosition& pos, float max_distance) {
    if (!stream) return false;
    
    auto it = sounds.find(name);
    if (it == sounds.end() || !it->second->buffer) {
        return false;
    }
    
    MixerAudio* audio = it->second;
    
    // Calculate 3D audio effects
    float distance = calculate_distance(pos);
    float volume = 1.0f;
    
    // Apply distance attenuation if max_distance > 0
    if (max_distance > 0.0f && distance > max_distance) {
        return true; // Too far away, don't play
    }
    if (max_distance > 0.0f) {
        volume = std::max(0.0f, 1.0f - (distance / max_distance));
        if (volume < 0.1f) return true; // Too quiet, don't play
    }
    
    // Calculate stereo panning
    float left_gain, right_gain;
    calculate_stereo_pan(pos, left_gain, right_gain);
    
    return play_sound_with_effects(audio, volume, left_gain, right_gain);
}

bool AudioMixer::play_sound_with_effects(MixerAudio* audio, float volume, float left_gain, float right_gain) {
    if (!stream || !audio->buffer) return false;
    
    // Convert audio if needed
    Uint8* converted_buffer = nullptr;
    int converted_size = 0;
    bool needs_conversion = (audio->spec.format != device_spec.format ||
                           audio->spec.channels != device_spec.channels ||
                           audio->spec.freq != device_spec.freq);
    
    if (needs_conversion) {
        if (!SDL_ConvertAudioSamples(&audio->spec, audio->buffer, audio->length, 
                                   &device_spec, &converted_buffer, &converted_size)) {
            std::cerr << "Failed to convert audio: " << SDL_GetError() << std::endl;
            return false;
        }
    } else {
        converted_buffer = audio->buffer;
        converted_size = audio->length;
    }
    
    // Apply volume and stereo effects for 16-bit stereo audio
    if (device_spec.format == SDL_AUDIO_S16LE && device_spec.channels == 2) {
        Sint16* samples = (Sint16*)converted_buffer;
        int num_frames = converted_size / (sizeof(Sint16) * 2);
        
        for (int i = 0; i < num_frames; i++) {
            samples[i * 2] = (Sint16)(samples[i * 2] * volume * left_gain);     // Left channel
            samples[i * 2 + 1] = (Sint16)(samples[i * 2 + 1] * volume * right_gain); // Right channel
        }
    }
    
    SDL_PutAudioStreamData(stream, converted_buffer, converted_size);
    
    if (needs_conversion) {
        SDL_free(converted_buffer);
    }
    
    return true;
}

float AudioMixer::calculate_distance(const AudioPosition& sound_pos) {
    float dx = sound_pos.x - listener_pos.x;
    float dy = sound_pos.y - listener_pos.y;
    float dz = sound_pos.z - listener_pos.z;
    return std::sqrt(dx*dx + dy*dy + dz*dz);
}

void AudioMixer::calculate_stereo_pan(const AudioPosition& sound_pos, float& left_gain, float& right_gain) {
    // Calculate horizontal panning based on X position
    float dx = sound_pos.x - listener_pos.x;
    float pan_range = 400.0f; // Stereo effect range in pixels
    
    float pan = std::max(-1.0f, std::min(1.0f, dx / pan_range));
    
    // Convert pan (-1 to 1) to stereo gains
    if (pan <= 0) {
        left_gain = 1.0f;
        right_gain = 1.0f + pan; // pan is negative, so this reduces right
    } else {
        left_gain = 1.0f - pan;
        right_gain = 1.0f;
    }
    
    // Ensure gains are non-negative
    left_gain = std::max(0.0f, left_gain);
    right_gain = std::max(0.0f, right_gain);
}

void AudioMixer::set_listener_position(const AudioPosition& pos) {
    listener_pos = pos;
}

void AudioMixer::audio_callback(void* userdata, SDL_AudioStream* stream, int additional_amount, int total_amount) {
    // This callback is called when SDL3 needs more audio data
    // For our simple mixer, we don't need to do anything here since we're using SDL_PutAudioStreamData
}