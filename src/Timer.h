#ifndef TIMER_H
#define TIMER_H

#include <SDL3/SDL_stdinc.h>

class Timer {
public:
    static void init();
    static void tick();
    static float time_elapsed();

private:
    static Uint64 last_tick;
    static Uint64 performance_frequency;
    static float delta_time;
};

#endif