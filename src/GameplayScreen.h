#ifndef GAMEPLAYSCREEN_H
#define GAMEPLAYSCREEN_H

#include "Screen.h"
#include "ClanBomber.h"
#include "Controller_Keyboard.h"
#include "Map.h"

class GameplayScreen : public Screen {
public:
    GameplayScreen(ClanBomberApplication* app);
    ~GameplayScreen();

    void handle_events(SDL_Event& event) override;
    void update(float deltaTime) override;
    void render(SDL_Renderer* renderer) override;

private:
    ClanBomberApplication* app;
    Controller_Keyboard* controller;
};

#endif