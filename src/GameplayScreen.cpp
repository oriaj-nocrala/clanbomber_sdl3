#include "GameplayScreen.h"
#include "Bomber.h"
#include "Timer.h"
#include "Controller_Keyboard.h"
#include "GameConfig.h"
#include "Controller.h"
#include "AudioMixer.h"
#include "Resources.h"
#include <algorithm>
#include <set>
#include <vector>
#include <string>

GameplayScreen::GameplayScreen(ClanBomberApplication* app) : app(app) {
    SDL_Log("GameplayScreen::GameplayScreen() - Loading game configuration...");
    GameConfig::load(); // Load game configuration before initializing
    
    // Clear any pending keyboard events to prevent menu input bleeding into gameplay
    SDL_PumpEvents();
    SDL_FlushEvents(SDL_EVENT_KEY_DOWN, SDL_EVENT_KEY_UP);
    
    init_game();
    next_state = GameState::GAMEPLAY;
}

GameplayScreen::~GameplayScreen() {
    deinit_game();
}

GameState GameplayScreen::get_next_state() {
    return next_state;
}

void GameplayScreen::init_game() {
    frame_count = 0;
    frame_time = 0;
    fps = 0;
    pause_game = false;
    show_fps = false;
    
    // Initialize controller activation delay (wait for fly-to animations to complete)
    controller_activation_timer = 2.0f; // 2 second delay to allow animations to finish
    controllers_activated = false;
    
    // Initialize victory/defeat state
    game_over = false;
    victory_achieved = false;
    game_over_timer = 0.0f;
    winning_team = 0;
    winning_player = "";
    
    // Initialize gore delay
    gore_delay_timer = 0.0f;
    checking_victory = false;

    app->map = new Map(app);
    if (!app->map->any_valid_map()) {
        SDL_Log("No valid maps found.");
    }

    if (GameConfig::get_random_map_order()) {
        app->map->load_random_valid();
    } else {
        if (GameConfig::get_start_map() > app->map->get_map_count() - 1) {
            GameConfig::set_start_map(app->map->get_map_count() - 1);
        }
        app->map->load_next_valid(GameConfig::get_start_map());
    }

    if (GameConfig::get_random_positions()) {
        app->map->randomize_bomber_positions();
    }

    int j = 0;
    for (int i = 0; i < 8; i++) {
        if (GameConfig::bomber[i].is_enabled()) {
            CL_Vector pos = app->map->get_bomber_pos(j++);
            Controller* controller = Controller::create(static_cast<Controller::CONTROLLER_TYPE>(GameConfig::bomber[i].get_controller()));
            if (!controller) {
                SDL_Log("Failed to create controller for bomber %d, skipping", i);
                continue;
            }
            
            SDL_Log("Creating bomber %d: controller=%d, pos=(%f,%f) -> (%d,%d)", 
                   i, GameConfig::bomber[i].get_controller(), pos.x, pos.y, (int)(pos.x * 40), (int)(pos.y * 40));
            
            // Create bomber at temporary position (off-screen or center)
            int temp_x = 400 - i * 20;
            int temp_y = 300 - i * 20;
            Bomber* bomber = new Bomber(temp_x, temp_y, static_cast<Bomber::COLOR>(GameConfig::bomber[i].get_skin()), controller, app);
            bomber->set_name(GameConfig::bomber[i].get_name());
            bomber->set_team(GameConfig::bomber[i].get_team());
            bomber->set_number(i);
            bomber->set_lives(3); // Start with 3 lives
            app->bomber_objects.push_back(bomber);
            
            // Start fly-to animation to final position
            bomber->fly_to((int)(pos.x*40), (int)(pos.y*40), 1000 + i * 200); // 1 second + stagger
            
            // Delay controller activation to prevent menu input bleeding
            if (bomber->get_controller()) {
                bomber->get_controller()->deactivate(); // Start deactivated
                // Will be activated after a short delay in update()
            }
            
            // Set appropriate Z-order for visual layering
            bomber->z = 10 + i;
        }
    }

    // Remove teams with only one player
    int team_count[] = {0, 0, 0, 0};
    for (int team = 0; team < 4; team++) {
        for (auto const& bomber : app->bomber_objects) {
            if (bomber->get_team() - 1 == team) {
                team_count[team]++;
            }
        }
    }
    for (auto const& bomber : app->bomber_objects) {
        if (bomber->get_team() != 0) {
            if (team_count[bomber->get_team() - 1] == 1) {
                bomber->set_team(0);
            }
        }
    }
}

void GameplayScreen::deinit_game() {
    for (auto& obj : app->objects) {
        delete obj;
    }
    app->objects.clear();

    for (auto& bomber : app->bomber_objects) {
        delete bomber;
    }
    app->bomber_objects.clear();

    delete app->map;
    app->map = nullptr;

    // if (observer) {
    //     delete observer;
    //     observer = nullptr;
    // }
}

void GameplayScreen::handle_events(SDL_Event& event) {
    if (event.type == SDL_EVENT_KEY_DOWN) {
        switch (event.key.key) {
            case SDLK_P:
                pause_game = !pause_game;
                break;
            case SDLK_F1:
                show_fps = !show_fps;
                break;
        }
    }
}

void GameplayScreen::update(float deltaTime) {
    if (pause_game) {
        return;
    }

    // Handle controller activation delay
    if (!controllers_activated) {
        controller_activation_timer -= deltaTime;
        if (controller_activation_timer <= 0.0f) {
            controllers_activated = true;
            // Activate all bomber controllers
            for (auto& bomber : app->bomber_objects) {
                if (bomber && bomber->get_controller()) {
                    bomber->get_controller()->activate();
                }
            }
            SDL_Log("Controllers activated after delay");
        }
    }

    // Update 3D audio listener position based on active players
    update_audio_listener();

    // observer->act();
    // if (observer->end_of_game_requested()) {
    //     // Handle end of game
    // }

    delete_some();
    act_all();
    
    // Handle gore delay and victory checking
    if (!game_over) {
        // Check if we need to start gore delay
        bool any_bombers_just_died = false;
        for (auto& bomber : app->bomber_objects) {
            if (bomber && bomber->is_dead() && !bomber->delete_me) {
                any_bombers_just_died = true;
                break;
            }
        }
        
        if (any_bombers_just_died && !checking_victory) {
            // Start gore delay timer
            checking_victory = true;
            gore_delay_timer = 2.0f; // 2 seconds to enjoy the gore
            SDL_Log("Starting gore delay...");
        }
        
        if (checking_victory) {
            gore_delay_timer -= deltaTime;
            if (gore_delay_timer <= 0.0f) {
                checking_victory = false;
                check_victory_conditions();
            }
        } else if (!any_bombers_just_died) {
            // No recent deaths, check victory immediately
            check_victory_conditions();
        }
    } else {
        game_over_timer += deltaTime;
        // Return to menu after 8 seconds (more time to enjoy victory)
        if (game_over_timer > 8.0f) {
            SDL_Log("Game over timer expired, should return to menu");
            next_state = GameState::MAIN_MENU;
        }
    }

    frame_time += Timer::time_elapsed();
    if (frame_time > 2) {
        fps = (int)(frame_count / frame_time + 0.5f);
        frame_time = 0;
        frame_count = 0;
    }
    frame_count++;
}

void GameplayScreen::update_audio_listener() {
    if (app->bomber_objects.empty()) return;
    
    // Position audio listener at the center of all active players
    float total_x = 0.0f, total_y = 0.0f;
    int active_count = 0;
    
    for (auto& bomber : app->bomber_objects) {
        if (bomber && !bomber->delete_me) {
            total_x += bomber->get_x();
            total_y += bomber->get_y();
            active_count++;
        }
    }
    
    if (active_count > 0) {
        AudioPosition listener_pos(total_x / active_count, total_y / active_count, 0.0f);
        AudioMixer::set_listener_position(listener_pos);
    }
}

void GameplayScreen::render(SDL_Renderer* renderer) {
    show_all();

    if (pause_game) {
        // Render pause message
    }

    if (show_fps) {
        // Render FPS
    }
}

void GameplayScreen::act_all() {
    float deltaTime = Timer::time_elapsed();
    
    // Cap delta time to prevent issues with large time steps
    const float max_delta = 1.0f / 30.0f; // 30 FPS minimum
    if (deltaTime > max_delta) {
        deltaTime = max_delta;
    }
    
    // Smooth delta time to prevent jitter
    static float avg_delta = 1.0f / 60.0f;
    avg_delta = avg_delta * 0.9f + deltaTime * 0.1f;
    deltaTime = avg_delta;
    
    if (app->map != nullptr) {
        app->map->act();
    }

    // Update all objects with consistent delta time
    for (auto& obj : app->objects) {
        if (obj && !obj->delete_me) {
            obj->act(deltaTime);
        }
    }

    for (auto& bomber : app->bomber_objects) {
        if (bomber && !bomber->delete_me) {
            bomber->act(deltaTime);
        }
    }
}

void GameplayScreen::delete_some() {
    app->objects.remove_if([](GameObject* obj) {
        if (obj->delete_me) {
            delete obj;
            return true;
        }
        return false;
    });

    app->bomber_objects.remove_if([](Bomber* bomber) {
        if (bomber->delete_me) {
            delete bomber;
            return true;
        }
        return false;
    });
}

void GameplayScreen::show_all() {
    std::vector<GameObject*> draw_list;
    for(auto const& value: app->objects) {
        draw_list.push_back(value);
    }
    for(auto const& value: app->bomber_objects) {
        draw_list.push_back(value);
    }

    std::sort(draw_list.begin(), draw_list.end(), [](GameObject* go1, GameObject* go2) {
        return go1->get_z() < go2->get_z();
    });

    if (app->map != nullptr) {
        app->map->refresh_holes();
    }

    // Always draw map first as background
    if (app->map != nullptr) {
        app->map->show();
    }
    
    // Draw all game objects in Z-order
    for (auto& obj : draw_list) {
        if (obj != nullptr && !obj->delete_me) {
            obj->show();
        }
    }
    
    // Show victory/defeat overlay
    if (game_over) {
        render_victory_screen();
    }
}

void GameplayScreen::check_victory_conditions() {
    if (game_over) return;
    
    std::vector<Bomber*> alive_bombers;
    std::set<int> alive_teams;
    
    // Count alive bombers and their teams
    for (auto& bomber : app->bomber_objects) {
        if (bomber && !bomber->delete_me && !bomber->is_dead() && bomber->has_lives()) {
            alive_bombers.push_back(bomber);
            if (bomber->get_team() > 0) {
                alive_teams.insert(bomber->get_team());
            }
        }
    }
    
    // Check victory conditions
    if (alive_bombers.empty()) {
        // No one left - draw
        if (!game_over) { // Only trigger once
            game_over = true;
            victory_achieved = false;
            winning_player = "Draw!";
            
            // Play game over sound (only once) - with error protection
            AudioPosition center_pos(400, 300, 0.0f);
            if (!AudioMixer::play_sound_3d("time_over", center_pos, 800.0f)) {
                SDL_Log("Failed to play time_over sound - continuing without audio");
            }
            SDL_Log("Game Over: Draw!");
        }
        
    } else if (alive_bombers.size() == 1 && alive_teams.size() <= 1) {
        // Single winner or single team remaining
        if (!game_over) { // Only trigger once
            game_over = true;
            victory_achieved = true;
            
            Bomber* winner = alive_bombers[0];
            if (winner->get_team() > 0) {
                winning_team = winner->get_team();
                winning_player = "Team " + std::to_string(winning_team) + " Wins!";
            } else {
                winning_player = winner->get_name() + " Wins!";
            }
            
            // Play victory sound (only once) - with error protection
            AudioPosition winner_pos(winner->get_x(), winner->get_y(), 0.0f);
            if (!AudioMixer::play_sound_3d("winlevel", winner_pos, 800.0f)) {
                SDL_Log("Failed to play winlevel sound - continuing without audio");
            }
            SDL_Log("Game Over: %s", winning_player.c_str());
        }
    }
}

void GameplayScreen::render_victory_screen() {
    SDL_Renderer* renderer = Resources::get_renderer();
    if (!renderer) return;
    
    // Semi-transparent overlay
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 128);
    SDL_FRect overlay = {0.0f, 0.0f, 800.0f, 600.0f};
    SDL_RenderFillRect(renderer, &overlay);
    
    // TODO: Render victory/defeat text using TTF fonts
    // For now just change background color to indicate game over
    if (victory_achieved) {
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 64); // Green tint for victory
    } else {
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 64); // Red tint for defeat/draw
    }
    SDL_RenderFillRect(renderer, &overlay);
}