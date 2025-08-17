#ifndef GAMEPLAYSCREEN_H
#define GAMEPLAYSCREEN_H

#include "Screen.h"
#include "ClanBomber.h"
#include "Map.h"
#include "GameState.h"

class GameSystems;

class GameplayScreen : public Screen {
public:
    GameplayScreen(ClanBomberApplication* app);
    ~GameplayScreen();

    void handle_events(SDL_Event& event) override;
    void update(float deltaTime) override;
    void render(SDL_Renderer* renderer) override;

    GameState get_next_state();

private:
    void init_game();
    void deinit_game();
    void act_all();
    void show_all();
    void delete_some();
    void update_audio_listener();
    void check_victory_conditions();
    void render_victory_screen();
    
    // Victory/defeat state
    bool game_over;
    bool victory_achieved;
    float game_over_timer;
    int winning_team;
    std::string winning_player;
    
    // Gore delay before checking victory
    float gore_delay_timer;
    bool checking_victory;
    
    // Controller activation delay
    float controller_activation_timer;
    bool controllers_activated;

    ClanBomberApplication* app;
    bool pause_game;
    bool show_fps;
    int fps;
    int frame_count;
    float frame_time;
    GameState next_state;
    
    // Add GameSystems
    GameSystems* game_systems;
};

#endif