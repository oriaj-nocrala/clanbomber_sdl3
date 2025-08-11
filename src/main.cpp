#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include "Resources.h"
#include "GameObject.h"
#include "ClanBomber.h"
#include "Bomber.h"
#include "Controller_Keyboard.h"
#include "Timer.h"
#include "Map.h"
#include "Audio.h"

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        SDL_Log("Unable to initialize SDL: %s", SDL_GetError());
        return 1;
    }

    if (!TTF_Init()) {
        SDL_Log("Unable to initialize SDL_ttf: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("ClanBomber Modern", 800, 600, SDL_WINDOW_RESIZABLE);
    if (!window) {
        SDL_Log("Unable to create window: %s", SDL_GetError());
        TTF_Quit();

        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
    if (!renderer) {
        SDL_Log("Unable to create renderer: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        TTF_Quit();

        SDL_Quit();
        return 1;
    }
    
    // Enable VSync for smooth gameplay
    SDL_SetRenderVSync(renderer, 1);

    Resources::init(renderer);
    Timer::init();
    Audio::init();

    ClanBomberApplication app;
    app.map = new Map(&app);
    app.map->load();

    Controller_Keyboard* controller = new Controller_Keyboard(0);
    Bomber* bomber = new Bomber(100, 100, Bomber::RED, controller, &app);
    app.objects.push_back(bomber);

    bool running = true;
    while (running) {
        Timer::tick();
        
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        app.map->show();

        for (auto& obj : app.objects) {
            obj->act(Timer::time_elapsed());
            obj->show();
        }

        app.objects.remove_if([](GameObject* obj){
            if (obj->delete_me) {
                delete obj;
                return true;
            }
            return false;
        });

        SDL_RenderPresent(renderer);
    }

    delete app.map;
    delete controller;
    Audio::shutdown();
    Resources::shutdown();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    TTF_Quit();
    
    SDL_Quit();

    return 0;
}
