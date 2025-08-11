#ifndef GAMEPLAYSCREEN_H
#define GAMEPLAYSCREEN_H

#include "Screen.h"
#include "ClanBomber.h"
#include "Map.h"

class GameplayScreen : public Screen {
public:
    GameplayScreen(ClanBomberApplication* app);
    ~GameplayScreen();

    void handle_events(SDL_Event& event) override;
    void update(float deltaTime) override;
    void render(SDL_Renderer* renderer) override;

private:
    void init_game();
    void deinit_game();
    void act_all();
    void show_all();
    void delete_some();

    ClanBomberApplication* app;
    bool pause_game;
    bool show_fps;
    int fps;
    int frame_count;
    float frame_time;
};

#endif