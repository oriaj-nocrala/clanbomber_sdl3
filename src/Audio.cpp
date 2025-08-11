#include "Audio.h"
#include "Resources.h"
#include <iostream>

SDL_AudioStream* Audio::stream = nullptr;
SDL_AudioSpec Audio::device_spec;

void Audio::init() {
    SDL_zero(device_spec);
    device_spec.freq = 44100;
    device_spec.format = SDL_AUDIO_S16LE;
    device_spec.channels = 2;

    stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &device_spec, NULL, NULL);
    if (!stream) {
        std::cerr << "Failed to open audio stream: " << SDL_GetError() << std::endl;
    } else {
        SDL_ResumeAudioDevice(SDL_GetAudioStreamDevice(stream));
    }
}

void Audio::shutdown() {
    if (stream) {
        SDL_DestroyAudioStream(stream);
    }
}

void Audio::play(Sound* sound) {
    if (!stream || !sound || !sound->buffer) {
        return;
    }

    // Check if conversion is needed
    if (sound->spec.format != device_spec.format ||
        sound->spec.channels != device_spec.channels ||
        sound->spec.freq != device_spec.freq)
    {
        Uint8* converted_buffer = nullptr;
        int converted_size = 0;

        if (!SDL_ConvertAudioSamples(&sound->spec, sound->buffer, sound->length, &device_spec, &converted_buffer, &converted_size)) {
            std::cerr << "Failed to convert audio: " << SDL_GetError() << std::endl;
            return;
        }

        SDL_PutAudioStreamData(stream, converted_buffer, converted_size);
        SDL_free(converted_buffer);

    } else {
        // No conversion needed, play directly
        SDL_PutAudioStreamData(stream, sound->buffer, sound->length);
    }
}
