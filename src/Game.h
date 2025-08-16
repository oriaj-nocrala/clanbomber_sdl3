#ifndef GAME_H
#define GAME_H

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <vector>
#include "GameObject.h"
#include "Map.h"
#include "Controller_Keyboard.h"
#include "GameState.h"
#include "Screen.h"
#include "GPUAcceleratedRenderer.h"

class Game {
public:
    Game();
    ~Game();

    void run();

private:
    void handle_events();
    void update(float deltaTime);
    void render();

    void start_game();
    void change_screen(GameState next_state);

    SDL_Window* window;
    SDL_Renderer* renderer;
    bool running;
    Screen* current_screen;

    // Game-specific objects
    ClanBomberApplication app;
    Controller_Keyboard* controller;
};

#endif