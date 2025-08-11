#ifndef SETTINGSSCREEN_H
#define SETTINGSSCREEN_H

#include "Screen.h"
#include <SDL3_ttf/SDL_ttf.h>
#include <vector>
#include <string>
#include "GameState.h"

class SettingsScreen : public Screen {
public:
    SettingsScreen(SDL_Renderer* renderer, TTF_Font* font);
    ~SettingsScreen();

    void handle_events(SDL_Event& event) override;
    void update(float deltaTime) override;
    void render(SDL_Renderer* renderer) override;

    GameState get_next_state() const;

private:
    SDL_Renderer* renderer;
    TTF_Font* font;
    std::vector<std::string> menu_items;
    int selected_item;
    GameState next_state;
};

#endif