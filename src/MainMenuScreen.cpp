#include "MainMenuScreen.h"
#include "Resources.h"
#include <string.h>

MainMenuScreen::MainMenuScreen(SDL_Renderer* renderer, TTF_Font* font)
    : renderer(renderer), font(font), selected_item(0), next_state(GameState::MAIN_MENU) {
    menu_items.push_back("Start Game");
    menu_items.push_back("Settings");
    menu_items.push_back("Quit");
}

MainMenuScreen::~MainMenuScreen() {
    // Font is managed by the Game class
}

void MainMenuScreen::handle_events(SDL_Event& event) {
    if (event.type == SDL_EVENT_KEY_DOWN) {
        switch (event.key.key) {
            case SDLK_UP:
                selected_item = (selected_item > 0) ? selected_item - 1 : menu_items.size() - 1;
                break;
            case SDLK_DOWN:
                selected_item = (selected_item < menu_items.size() - 1) ? selected_item + 1 : 0;
                break;
            case SDLK_RETURN:
                if (selected_item == 0) {
                    next_state = GameState::GAMEPLAY;
                } else if (selected_item == 1) {
                    next_state = GameState::SETTINGS;
                } else if (selected_item == 2) {
                    next_state = GameState::QUIT;
                }
                break;
        }
    }
}

void MainMenuScreen::update(float deltaTime) {
    // Nothing to update for now
}

void MainMenuScreen::render(SDL_Renderer* renderer) {
    int y = 200;
    for (int i = 0; i < menu_items.size(); ++i) {
        SDL_Color color = (i == selected_item) ? SDL_Color{255, 255, 0, 255} : SDL_Color{255, 255, 255, 255};
        SDL_Surface* surface = TTF_RenderText_Solid(font, menu_items[i].c_str(), strlen(menu_items[i].c_str()), color);
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        
        float width, height;
        SDL_GetTextureSize(texture, &width, &height);
        SDL_FRect dst_rect = {400 - width / 2.0f, (float)y, (float)width, (float)height};
        
        SDL_RenderTexture(renderer, texture, NULL, &dst_rect);
        
        SDL_DestroySurface(surface);
        SDL_DestroyTexture(texture);
        
        y += 50;
    }
}

GameState MainMenuScreen::get_next_state() const {
    return next_state;
}
