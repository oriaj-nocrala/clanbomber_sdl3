#ifndef SCREEN_H
#define SCREEN_H

#include <SDL3/SDL.h>

class Screen {
public:
    virtual ~Screen() {}
    virtual void handle_events(SDL_Event& event) = 0;
    virtual void update(float deltaTime) = 0;
    virtual void render(SDL_Renderer* renderer) = 0;
};

#endif