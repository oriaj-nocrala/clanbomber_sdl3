#include "AudioMixer.h"
#include <iostream>
#include <algorithm>
#include <vector>

// Static member initialization
SDL_AudioStream* AudioMixer::stream = nullptr;
SDL_AudioSpec AudioMixer::device_spec;
std::map<std::string, MixerAudio*> AudioMixer::sounds;
AudioPosition AudioMixer::listener_pos(400.0f, 300.0f, 0.0f);
Channel AudioMixer::channels[MAX_CHANNELS];

// Helper functions for 3D audio calculations
static float calculate_distance(const AudioPosition& sound_pos) {
    float dx = sound_pos.x - AudioMixer::get_listener_position().x;
    float dy = sound_pos.y - AudioMixer::get_listener_position().y;
    float dz = sound_pos.z - AudioMixer::get_listener_position().z;
    return std::sqrt(dx*dx + dy*dy + dz*dz);
}

static void calculate_stereo_pan(const AudioPosition& sound_pos, float& left_gain, float& right_gain) {
    float dx = sound_pos.x - AudioMixer::get_listener_position().x;
    float pan_range = 400.0f; // Stereo effect range in pixels
    
    float pan = std::max(-1.0f, std::min(1.0f, dx / pan_range));
    
    if (pan <= 0) {
        left_gain = 1.0f;
        right_gain = 1.0f + pan;
    } else {
        left_gain = 1.0f - pan;
        right_gain = 1.0f;
    }
    
    left_gain = std::max(0.0f, left_gain);
    right_gain = std::max(0.0f, right_gain);
}


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
    
    // Convert all loaded sounds to the device format on load
    // This simplifies the mixing callback significantly
    SDL_AudioSpec target_spec = device_spec;
    if (audio->spec.format != target_spec.format ||
        audio->spec.channels != target_spec.channels ||
        audio->spec.freq != target_spec.freq) {
        
        Uint8* converted_buffer = nullptr;
        int converted_size = 0;
        if (!SDL_ConvertAudioSamples(&audio->spec, audio->buffer, audio->length,
                                   &target_spec, &converted_buffer, &converted_size)) {
            SDL_Log("Failed to convert sound %s on load: %s", path.c_str(), SDL_GetError());
            SDL_free(audio->buffer);
            delete audio;
            return nullptr;
        }
        
        SDL_free(audio->buffer);
        audio->buffer = converted_buffer;
        audio->length = converted_size;
        audio->spec = target_spec;
    }

    return audio;
}

void AudioMixer::add_sound(const std::string& name, MixerAudio* audio) {
    if (audio) {
        sounds[name] = audio;
    }
}

bool AudioMixer::play_sound(const std::string& name) {
    return play_sound_3d(name, listener_pos, 0.0f);
}

bool AudioMixer::play_sound_3d(const std::string& name, const AudioPosition& pos, float max_distance) {
    if (!stream) return false;
    
    auto it = sounds.find(name);
    if (it == sounds.end() || !it->second->buffer) {
        return false;
    }
    
    MixerAudio* audio = it->second;
    
    float distance = calculate_distance(pos);
    float volume = 1.0f;
    
    if (max_distance > 0.0f) {
        if (distance > max_distance) return true; // Too far
        volume = std::max(0.0f, 1.0f - (distance / max_distance));
        if (volume < 0.01f) return true; // Too quiet
    }
    
    float left_gain, right_gain;
    calculate_stereo_pan(pos, left_gain, right_gain);
    
    // Find an available channel
    int channel_id = -1;
    for (int i = 0; i < MAX_CHANNELS; ++i) {
        if (!channels[i].active) {
            channel_id = i;
            break;
        }
    }
    
    if (channel_id == -1) {
        // Optional: find the oldest channel and replace it
        SDL_Log("No free audio channels to play sound: %s", name.c_str());
        return false; // No free channels
    }
    
    // Activate the channel
    channels[channel_id].audio = audio;
    channels[channel_id].position = 0;
    channels[channel_id].volume = volume;
    channels[channel_id].left_gain = left_gain;
    channels[channel_id].right_gain = right_gain;
    channels[channel_id].active = true;
    
    return true;
}

void AudioMixer::set_listener_position(const AudioPosition& pos) {
    listener_pos = pos;
}

void AudioMixer::audio_callback(void* userdata, SDL_AudioStream* stream, int additional_amount, int total_amount) {
    // Buffer for mixing, using 32-bit samples to prevent clipping during accumulation
    std::vector<Sint32> mix_buffer(additional_amount / sizeof(Sint16), 0);

    // Mix active channels
    for (int i = 0; i < MAX_CHANNELS; ++i) {
        if (channels[i].active) {
            Channel& chan = channels[i];
            MixerAudio* audio = chan.audio;
            
            Uint32 remaining_length = audio->length - chan.position;
            Uint32 amount_to_mix = std::min((Uint32)additional_amount, remaining_length);
            
            Sint16* src_samples = (Sint16*)(audio->buffer + chan.position);
            
            // Mix into the 32-bit buffer
            for (Uint32 j = 0; j < amount_to_mix / sizeof(Sint16); j += 2) {
                // Left channel
                mix_buffer[j] += (Sint32)(src_samples[j] * chan.volume * chan.left_gain);
                // Right channel
                mix_buffer[j + 1] += (Sint32)(src_samples[j + 1] * chan.volume * chan.right_gain);
            }
            
            chan.position += amount_to_mix;
            if (chan.position >= audio->length) {
                chan.active = false;
            }
        }
    }

    // Clamp and convert back to 16-bit
    std::vector<Sint16> final_buffer(additional_amount / sizeof(Sint16));
    for (size_t i = 0; i < mix_buffer.size(); ++i) {
        final_buffer[i] = (Sint16)std::clamp(mix_buffer[i], (Sint32)SDL_MIN_SINT16, (Sint32)SDL_MAX_SINT16);
    }

    // Put the mixed data into the stream
    if (SDL_PutAudioStreamData(stream, final_buffer.data(), additional_amount) < 0) {
        SDL_Log("Failed to put audio data in callback: %s", SDL_GetError());
    }
}