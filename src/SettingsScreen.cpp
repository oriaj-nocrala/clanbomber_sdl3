#include "SettingsScreen.h"
#include "Resources.h"
#include "GameConfig.h"
#include "Controller.h"
#include <string.h>

SettingsScreen::SettingsScreen(SDL_Renderer* renderer, TTF_Font* font)
    : renderer(renderer), font(font), selected_item(0), selected_player(0), next_state(GameState::SETTINGS) {
    menu_items.push_back("Player Setup");
    menu_items.push_back("Game Options"); 
    menu_items.push_back("Graphics Options");
    menu_items.push_back("Back to Main Menu");
}

SettingsScreen::~SettingsScreen() {
}

void SettingsScreen::handle_events(SDL_Event& event) {
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
                    case 0: // Player Setup - handled in render for now
                        break;
                    case 1: // Game Options
                        break;
                    case 2: // Graphics Options
                        break;
                    case 3: // Back to Main Menu
                        next_state = GameState::MAIN_MENU;
                        break;
                }
                break;
            case SDLK_ESCAPE:
                next_state = GameState::MAIN_MENU;
                break;
        }
    }
    
    handle_player_setup_input(event);
}

void SettingsScreen::update(float deltaTime) {
}

void SettingsScreen::render(SDL_Renderer* renderer) {
    // Render title
    SDL_Color title_color = {255, 255, 255, 255};
    SDL_Surface* title_surface = TTF_RenderText_Solid(font, "SETTINGS", strlen("SETTINGS"), title_color);
    SDL_Texture* title_texture = SDL_CreateTextureFromSurface(renderer, title_surface);
    
    float title_width, title_height;
    SDL_GetTextureSize(title_texture, &title_width, &title_height);
    SDL_FRect title_rect = {400 - title_width / 2.0f, 50, title_width, title_height};
    SDL_RenderTexture(renderer, title_texture, NULL, &title_rect);
    
    SDL_DestroySurface(title_surface);
    SDL_DestroyTexture(title_texture);
    
    // Render main menu
    int y = 120;
    for (int i = 0; i < menu_items.size(); ++i) {
        SDL_Color color = (i == selected_item) ? SDL_Color{255, 255, 0, 255} : SDL_Color{255, 255, 255, 255};
        std::string item_text = (i == selected_item) ? "> " + menu_items[i] + " <" : "  " + menu_items[i] + "  ";
        
        SDL_Surface* surface = TTF_RenderText_Solid(font, item_text.c_str(), strlen(item_text.c_str()), color);
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        
        float width, height;
        SDL_GetTextureSize(texture, &width, &height);
        SDL_FRect dst_rect = {400 - width / 2.0f, (float)y, width, height};
        
        SDL_RenderTexture(renderer, texture, NULL, &dst_rect);
        
        SDL_DestroySurface(surface);
        SDL_DestroyTexture(texture);
        
        y += 35;
    }
    
    // Render specific content based on selection
    if (selected_item == 0) {
        render_player_setup();
    } else if (selected_item == 1) {
        render_game_options(); 
    }
    
    // Render instructions
    SDL_Color instructions_color = {150, 150, 150, 255};
    SDL_Surface* instructions_surface = TTF_RenderText_Solid(font, "UP/DOWN: Navigate | ENTER: Select | ESC: Back", strlen("UP/DOWN: Navigate | ENTER: Select | ESC: Back"), instructions_color);
    SDL_Texture* instructions_texture = SDL_CreateTextureFromSurface(renderer, instructions_surface);
    
    float inst_width, inst_height;
    SDL_GetTextureSize(instructions_texture, &inst_width, &inst_height);
    SDL_FRect inst_rect = {400 - inst_width / 2.0f, 550, inst_width, inst_height};
    SDL_RenderTexture(renderer, instructions_texture, NULL, &inst_rect);
    
    SDL_DestroySurface(instructions_surface);
    SDL_DestroyTexture(instructions_texture);
}

GameState SettingsScreen::get_next_state() const {
    return next_state;
}

void SettingsScreen::render_player_setup() {
    SDL_Color header_color = {200, 200, 255, 255};
    SDL_Surface* header_surface = TTF_RenderText_Solid(font, "PLAYER SETUP", strlen("PLAYER SETUP"), header_color);
    SDL_Texture* header_texture = SDL_CreateTextureFromSurface(renderer, header_surface);
    
    float header_width, header_height;
    SDL_GetTextureSize(header_texture, &header_width, &header_height);
    SDL_FRect header_rect = {400 - header_width / 2.0f, 260, header_width, header_height};
    SDL_RenderTexture(renderer, header_texture, NULL, &header_rect);
    
    SDL_DestroySurface(header_surface);
    SDL_DestroyTexture(header_texture);
    
    // Display players
    int y = 300;
    for (int i = 0; i < 4; i++) { // Show first 4 players for now
        SDL_Color player_color = GameConfig::bomber[i].is_enabled() ? 
            SDL_Color{0, 255, 0, 255} : SDL_Color{150, 150, 150, 255};
            
        std::string player_text = "Player " + std::to_string(i + 1) + ": ";
        if (GameConfig::bomber[i].is_enabled()) {
            player_text += GameConfig::bomber[i].get_name() + " (" + 
                          get_controller_name(GameConfig::bomber[i].get_controller()) + ") " +
                          get_team_name(GameConfig::bomber[i].get_team());
        } else {
            player_text += "DISABLED";
        }
        
        if (i == selected_player && selected_item == 0) {
            player_text = "> " + player_text + " <";
            player_color = {255, 255, 0, 255};
        }
        
        SDL_Surface* player_surface = TTF_RenderText_Solid(font, player_text.c_str(), strlen(player_text.c_str()), player_color);
        SDL_Texture* player_texture = SDL_CreateTextureFromSurface(renderer, player_surface);
        
        float player_width, player_height;
        SDL_GetTextureSize(player_texture, &player_width, &player_height);
        SDL_FRect player_rect = {50, (float)y, player_width, player_height};
        SDL_RenderTexture(renderer, player_texture, NULL, &player_rect);
        
        SDL_DestroySurface(player_surface);
        SDL_DestroyTexture(player_texture);
        
        y += 30;
    }
}

void SettingsScreen::render_game_options() {
    SDL_Color header_color = {200, 255, 200, 255};
    SDL_Surface* header_surface = TTF_RenderText_Solid(font, "GAME OPTIONS", strlen("GAME OPTIONS"), header_color);
    SDL_Texture* header_texture = SDL_CreateTextureFromSurface(renderer, header_surface);
    
    float header_width, header_height;
    SDL_GetTextureSize(header_texture, &header_width, &header_height);
    SDL_FRect header_rect = {400 - header_width / 2.0f, 260, header_width, header_height};
    SDL_RenderTexture(renderer, header_texture, NULL, &header_rect);
    
    SDL_DestroySurface(header_surface);
    SDL_DestroyTexture(header_texture);
    
    // Display game options
    int y = 300;
    std::vector<std::string> options = {
        "Points to win: " + std::to_string(GameConfig::get_points_to_win()),
        "Round time: " + std::to_string(GameConfig::get_round_time()) + " seconds",
        "Random positions: " + std::string(GameConfig::get_random_positions() ? "ON" : "OFF"),
        "Random map order: " + std::string(GameConfig::get_random_map_order() ? "ON" : "OFF")
    };
    
    for (const auto& option : options) {
        SDL_Color option_color = {255, 255, 255, 255};
        SDL_Surface* option_surface = TTF_RenderText_Solid(font, option.c_str(), strlen(option.c_str()), option_color);
        SDL_Texture* option_texture = SDL_CreateTextureFromSurface(renderer, option_surface);
        
        float option_width, option_height;
        SDL_GetTextureSize(option_texture, &option_width, &option_height);
        SDL_FRect option_rect = {50, (float)y, option_width, option_height};
        SDL_RenderTexture(renderer, option_texture, NULL, &option_rect);
        
        SDL_DestroySurface(option_surface);
        SDL_DestroyTexture(option_texture);
        
        y += 30;
    }
}

void SettingsScreen::handle_player_setup_input(SDL_Event& event) {
    if (selected_item != 0) return; // Only handle input when Player Setup is selected
    
    if (event.type == SDL_EVENT_KEY_DOWN) {
        switch (event.key.key) {
            case SDLK_LEFT:
                selected_player = (selected_player > 0) ? selected_player - 1 : 3;
                break;
            case SDLK_RIGHT:
                selected_player = (selected_player < 3) ? selected_player + 1 : 0;
                break;
            case SDLK_SPACE:
                // Toggle player enabled/disabled
                if (GameConfig::bomber[selected_player].is_enabled()) {
                    GameConfig::bomber[selected_player].disable();
                } else {
                    GameConfig::bomber[selected_player].enable();
                }
                break;
        }
    }
}

std::string SettingsScreen::get_controller_name(int controller_type) {
    switch (controller_type) {
        case Controller::KEYMAP_1: return "Keys1";
        case Controller::KEYMAP_2: return "Keys2"; 
        case Controller::KEYMAP_3: return "Keys3";
        default: return "None";
    }
}

std::string SettingsScreen::get_team_name(int team) {
    switch (team) {
        case 0: return "No Team";
        case 1: return "Red Team";
        case 2: return "Blue Team";
        case 3: return "Green Team";
        case 4: return "Yellow Team";
        default: return "Team " + std::to_string(team);
    }
}
