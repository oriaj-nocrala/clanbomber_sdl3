#include "MainMenuScreen.h"
#include "Resources.h"
#include "TextRenderer.h"
#include "GameContext.h"
#include "RenderingFacade.h"
#include <string.h>

MainMenuScreen::MainMenuScreen(TextRenderer* text_renderer, GameContext* game_context)
    : selected_item(0), next_state(GameState::MAIN_MENU), text_renderer(text_renderer), game_context(game_context) {
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
    if (!text_renderer) {
        return; // Can't render without text renderer
    }
    
    // Colors for text
    SDL_Color title_color = {255, 255, 255, 255};      // White
    SDL_Color selected_color = {255, 255, 0, 255};     // Yellow
    SDL_Color normal_color = {200, 200, 200, 255};     // Light gray
    SDL_Color instructions_color = {150, 150, 150, 255}; // Dark gray
    
    // Get RenderingFacade from GameContext
    RenderingFacade* facade = game_context ? game_context->get_rendering_facade() : nullptr;
    
    // Render title (centered at x=400 for 800px wide screen)
    text_renderer->draw_text_centered(facade, "CLANBOMBER", "big", 400, 100, title_color);
    
    // Render version (centered)
    text_renderer->draw_text_centered(facade, "SDL3 Modern Edition", "small", 400, 140, normal_color);
    
    // Render menu items (centered)
    float y = 220;
    for (int i = 0; i < menu_items.size(); ++i) {
        SDL_Color color = (i == selected_item) ? selected_color : normal_color;
        
        // Add selection indicator
        std::string item_text = (i == selected_item) ? "> " + menu_items[i] + " <" : "  " + menu_items[i] + "  ";
        
        text_renderer->draw_text_centered(facade, item_text, "big", 400, y, color);
        
        y += 40;
    }
    
    // Render instructions (centered)
    text_renderer->draw_text_centered(facade, "Use UP/DOWN arrows to navigate, ENTER to select", "small", 400, 550, instructions_color);
}

GameState MainMenuScreen::get_next_state() const {
    return next_state;
}
