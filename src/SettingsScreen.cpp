#include "SettingsScreen.h"
#include "Resources.h"
#include <string.h>

SettingsScreen::SettingsScreen(SDL_Renderer* renderer, TTF_Font* font)
    : renderer(renderer), font(font), selected_item(0), next_state(GameState::SETTINGS) {
    menu_items.push_back("Back");
}

SettingsScreen::~SettingsScreen() {
}

void SettingsScreen::handle_events(SDL_Event& event) {
    if (event.type == SDL_EVENT_KEY_DOWN) {
        switch (event.key.key) {
            case SDLK_RETURN:
                if (selected_item == 0) {
                    next_state = GameState::MAIN_MENU;
                }
                break;
        }
    }
}

void SettingsScreen::update(float deltaTime) {
}

void SettingsScreen::render(SDL_Renderer* renderer) {
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

GameState SettingsScreen::get_next_state() const {
    return next_state;
}
