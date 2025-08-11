#include "Timer.h"
#include <SDL3/SDL_timer.h>

Uint64 Timer::last_tick = 0;
Uint64 Timer::performance_frequency = 0;
float Timer::delta_time = 0.0f;

void Timer::init() {
    performance_frequency = SDL_GetPerformanceFrequency();
    last_tick = SDL_GetPerformanceCounter();
}

void Timer::tick() {
    Uint64 current_tick = SDL_GetPerformanceCounter();
    delta_time = static_cast<float>(current_tick - last_tick) / performance_frequency;
    last_tick = current_tick;
}

float Timer::time_elapsed() {
    return delta_time;
}
