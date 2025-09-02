#include "MainMenuScreen.h"
#include "Resources.h"
#include "TextRenderer.h"
#include "GameContext.h"
#include "RenderingFacade.h"
#include "Controller_Joystick.h"
#include <string.h>

MainMenuScreen::MainMenuScreen(TextRenderer* text_renderer, GameContext* game_context)
    : selected_item(0), next_state(GameState::MAIN_MENU), text_renderer(text_renderer), game_context(game_context), menu_joystick(nullptr) {
    menu_items.push_back("Local Game");
    menu_items.push_back("Player Setup");
    menu_items.push_back("Game Options");
    menu_items.push_back("Graphics Options");
    menu_items.push_back("Help");
    menu_items.push_back("Credits");
    menu_items.push_back("Quit");
    
    // Create joystick controller for menu navigation if available
    if (Controller_Joystick::get_joystick_count() > 0) {
        menu_joystick = new Controller_Joystick(0);
        menu_joystick->activate();
        SDL_Log("MainMenuScreen: Created joystick controller for menu navigation");
    }
}

MainMenuScreen::~MainMenuScreen() {
    // Font is managed by the Game class
    if (menu_joystick) {
        delete menu_joystick;
        menu_joystick = nullptr;
    }
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
    // Update joystick input for menu navigation
    if (menu_joystick) {
        menu_joystick->update();
        
        // Handle joystick navigation with debouncing
        static float navigation_cooldown = 0.0f;
        if (navigation_cooldown > 0.0f) {
            navigation_cooldown -= deltaTime;
        }
        
        if (navigation_cooldown <= 0.0f) {
            bool moved = false;
            
            if (menu_joystick->is_up()) {
                selected_item = (selected_item > 0) ? selected_item - 1 : menu_items.size() - 1;
                moved = true;
            } else if (menu_joystick->is_down()) {
                selected_item = (selected_item < menu_items.size() - 1) ? selected_item + 1 : 0;
                moved = true;
            }
            
            if (moved) {
                navigation_cooldown = 0.2f; // 200ms cooldown to prevent rapid navigation
            }
            
            // Handle joystick selection (bomb button acts as Enter)
            if (menu_joystick->is_bomb()) {
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
                navigation_cooldown = 0.5f; // Longer cooldown for selection
            }
        }
    }
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
    std::string instructions = "Use UP/DOWN arrows or joystick to navigate, ENTER or A button to select";
    text_renderer->draw_text_centered(facade, instructions, "small", 400, 550, instructions_color);
}

GameState MainMenuScreen::get_next_state() const {
    return next_state;
}
