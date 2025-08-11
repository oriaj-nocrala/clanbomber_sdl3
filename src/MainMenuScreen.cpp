#include "MainMenuScreen.h"
#include "Resources.h"
#include <string.h>

MainMenuScreen::MainMenuScreen(SDL_Renderer* renderer, TTF_Font* font)
    : renderer(renderer), font(font), selected_item(0), next_state(GameState::MAIN_MENU) {
    menu_items.push_back("Local Game");
    menu_items.push_back("Player Setup");
    menu_items.push_back("Game Options");
    menu_items.push_back("Graphics Options");
    menu_items.push_back("Help");
    menu_items.push_back("Credits");
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
                switch (selected_item) {
                    case 0: // Local Game
                        next_state = GameState::GAMEPLAY;
                        break;
                    case 1: // Player Setup
                        next_state = GameState::SETTINGS;
                        break;
                    case 2: // Game Options
                        next_state = GameState::SETTINGS;
                        break;
                    case 3: // Graphics Options
                        next_state = GameState::SETTINGS;
                        break;
                    case 4: // Help
                        // TODO: Implement help screen
                        break;
                    case 5: // Credits
                        // TODO: Implement credits screen
                        break;
                    case 6: // Quit
                        next_state = GameState::QUIT;
                        break;
                }
                break;
        }
    }
}

void MainMenuScreen::update(float deltaTime) {
    // Nothing to update for now
}

void MainMenuScreen::render(SDL_Renderer* renderer) {
    // Render title
    SDL_Color title_color = {255, 255, 255, 255};
    SDL_Surface* title_surface = TTF_RenderText_Solid(font, "CLANBOMBER", strlen("CLANBOMBER"), title_color);
    SDL_Texture* title_texture = SDL_CreateTextureFromSurface(renderer, title_surface);
    
    float title_width, title_height;
    SDL_GetTextureSize(title_texture, &title_width, &title_height);
    SDL_FRect title_rect = {400 - title_width / 2.0f, 100, title_width, title_height};
    SDL_RenderTexture(renderer, title_texture, NULL, &title_rect);
    
    SDL_DestroySurface(title_surface);
    SDL_DestroyTexture(title_texture);
    
    // Render version
    SDL_Color version_color = {200, 200, 200, 255};
    SDL_Surface* version_surface = TTF_RenderText_Solid(font, "SDL3 Modern Edition", strlen("SDL3 Modern Edition"), version_color);
    SDL_Texture* version_texture = SDL_CreateTextureFromSurface(renderer, version_surface);
    
    float version_width, version_height;
    SDL_GetTextureSize(version_texture, &version_width, &version_height);
    SDL_FRect version_rect = {400 - version_width / 2.0f, 140, version_width, version_height};
    SDL_RenderTexture(renderer, version_texture, NULL, &version_rect);
    
    SDL_DestroySurface(version_surface);
    SDL_DestroyTexture(version_texture);

    // Render menu items
    int y = 220;
    for (int i = 0; i < menu_items.size(); ++i) {
        SDL_Color color = (i == selected_item) ? SDL_Color{255, 255, 0, 255} : SDL_Color{255, 255, 255, 255};
        
        // Add selection indicator
        std::string item_text = (i == selected_item) ? "> " + menu_items[i] + " <" : "  " + menu_items[i] + "  ";
        
        SDL_Surface* surface = TTF_RenderText_Solid(font, item_text.c_str(), strlen(item_text.c_str()), color);
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        
        float width, height;
        SDL_GetTextureSize(texture, &width, &height);
        SDL_FRect dst_rect = {400 - width / 2.0f, (float)y, width, height};
        
        SDL_RenderTexture(renderer, texture, NULL, &dst_rect);
        
        SDL_DestroySurface(surface);
        SDL_DestroyTexture(texture);
        
        y += 40;
    }
    
    // Render instructions
    SDL_Color instructions_color = {150, 150, 150, 255};
    SDL_Surface* instructions_surface = TTF_RenderText_Solid(font, "Use UP/DOWN arrows to navigate, ENTER to select", strlen("Use UP/DOWN arrows to navigate, ENTER to select"), instructions_color);
    SDL_Texture* instructions_texture = SDL_CreateTextureFromSurface(renderer, instructions_surface);
    
    float inst_width, inst_height;
    SDL_GetTextureSize(instructions_texture, &inst_width, &inst_height);
    SDL_FRect inst_rect = {400 - inst_width / 2.0f, 550, inst_width, inst_height};
    SDL_RenderTexture(renderer, instructions_texture, NULL, &inst_rect);
    
    SDL_DestroySurface(instructions_surface);
    SDL_DestroyTexture(instructions_texture);
}

GameState MainMenuScreen::get_next_state() const {
    return next_state;
}
