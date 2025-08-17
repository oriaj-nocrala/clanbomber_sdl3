#include "GameplayScreen.h"
#include "Bomber.h"
#include "Timer.h"
#include "Controller_Keyboard.h"
#include "GameConfig.h"
#include "Controller.h"
#include "AudioMixer.h"
#include "Resources.h"
#include "Extra.h"
#include "TileManager.h"
#include "GameSystems.h"
#include "TileEntity.h"
#include "GameContext.h"
#include <algorithm>
#include <set>
#include <vector>
#include <string>

GameplayScreen::GameplayScreen(ClanBomberApplication* app) : app(app), game_systems(nullptr) {
    SDL_Log("GameplayScreen::GameplayScreen() - Loading game configuration...");
    GameConfig::load(); // Load game configuration before initializing
    
    // Clear any pending keyboard events to prevent menu input bleeding into gameplay
    SDL_PumpEvents();
    SDL_FlushEvents(SDL_EVENT_KEY_DOWN, SDL_EVENT_KEY_UP);
    
    init_game();
    next_state = GameState::GAMEPLAY;
}

GameplayScreen::~GameplayScreen() {
    if (game_systems) {
        delete game_systems;
        game_systems = nullptr;
    }
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

    // Initialize GameContext first (without map)
    app->initialize_game_context();
    
    // CRITICAL FIX: Connect GameContext to rendering lists so TileEntity objects are rendered
    if (app->game_context) {
        app->game_context->set_object_lists(&app->objects);
        SDL_Log("GameplayScreen: Connected GameContext to rendering lists");
    }
    
    app->map = new Map(app->game_context);
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
    
    // Now set the map in GameContext
    if (app->game_context && app->map) {
        app->game_context->set_map(app->map);
    }

    int j = 0;
    for (int i = 0; i < 8; i++) {
        SDL_Log("Bomber %d: enabled=%d, controller=%d", i, GameConfig::bomber[i].is_enabled(), GameConfig::bomber[i].get_controller());
        if (GameConfig::bomber[i].is_enabled()) {
            CL_Vector pos = app->map->get_bomber_pos(j++);
            int controller_type = GameConfig::bomber[i].get_controller();
            SDL_Log("Creating controller type %d for bomber %d", controller_type, i);
            Controller* controller = Controller::create(static_cast<Controller::CONTROLLER_TYPE>(controller_type));
            if (!controller) {
                SDL_Log("Failed to create controller for bomber %d, skipping", i);
                continue;
            }
            
            SDL_Log("Creating bomber %d: controller=%d, pos=(%f,%f) -> (%d,%d)", 
                   i, GameConfig::bomber[i].get_controller(), pos.x, pos.y, (int)(pos.x * 40), (int)(pos.y * 40));
            
            // Create bomber at temporary position (off-screen or center)
            int temp_x = 400 - i * 20;
            int temp_y = 300 - i * 20;
            Bomber* bomber = new Bomber(temp_x, temp_y, static_cast<Bomber::COLOR>(GameConfig::bomber[i].get_skin()), controller, app->game_context);
            bomber->set_name(GameConfig::bomber[i].get_name());
            bomber->set_team(GameConfig::bomber[i].get_team());
            bomber->set_number(i);
            bomber->set_lives(3); // Start with 3 lives
            app->bomber_objects.push_back(bomber);
            
            // Register bomber with GameContext for proper lifecycle management
            if (app->game_context) {
                app->game_context->register_object(bomber);
            }
            
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

    // Power-ups now spawn naturally when destroying boxes!
    // Includes positive effects: BOMB, FLAME, SPEED, KICK, GLOVE, SKATE
    // And negative effects: DISEASE (constipation), KOKS (speed), VIAGRA (sticky bombs)

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
    
    // Initialize GameSystems after GameContext is ready
    if (app->game_context) {
        game_systems = new GameSystems(app->game_context);
        game_systems->set_object_references(&app->objects, &app->bomber_objects);
        game_systems->init_all_systems();
        SDL_Log("GameSystems initialized in GameplayScreen");
    } else {
        SDL_Log("WARNING: GameContext not available, using legacy act_all()");
    }
}

void GameplayScreen::deinit_game() {
    // CRITICAL FIX: Do NOT delete objects directly here!
    // This was causing use-after-free during shutdown because LifecycleManager
    // still held references to objects that GameplayScreen was deleting.
    //
    // ARCHITECTURE DECISION: LifecycleManager has exclusive responsibility for object deletion
    // GameplayScreen only clears its references, doesn't delete the objects
    
    SDL_Log("GameplayScreen: deinit_game() - clearing references (LifecycleManager will handle deletion)");
    
    // Clear references without deleting - LifecycleManager will handle cleanup
    app->objects.clear();
    app->bomber_objects.clear();

    // Map deletion is safe as it's not managed by LifecycleManager
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

    // CLEAN ARCHITECTURE: Each manager handles its own responsibility
    
    // TileManager coordinates ALL tile-related logic (eliminates coordination crossing!)
    if (app->tile_manager) {
        app->tile_manager->update_tiles(deltaTime);
    }
    
    delete_some();  // Clean up objects marked as DELETED by LifecycleManager
    
    // Use GameSystems if available, fallback to legacy
    if (game_systems) {
        game_systems->update_all_systems(deltaTime);
    } else {
        act_all();  // Legacy fallback
    }
    
    // Final cleanup of dead objects
    if (app->lifecycle_manager) {
        app->lifecycle_manager->cleanup_dead_objects();
    }
    
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
    
    // Map is now a PURE GRID MANAGER - no act() needed!
    // TileManager handles all coordination in update() above

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
    // ARCHITECTURE FIX: LifecycleManager handles ALL deletion
    // GameplayScreen only removes references from its lists
    app->objects.remove_if([this](GameObject* obj) {
        LifecycleManager::ObjectState state = app->lifecycle_manager->get_object_state(obj);
        if (state == LifecycleManager::ObjectState::DELETED) {
            SDL_Log("GameplayScreen: Removing object %p from render list (LifecycleManager will delete)", obj);
            
            // CRITICAL: Clear Map grid pointer for TileEntity before LifecycleManager deletes it
            if (obj->get_type() == GameObject::MAPTILE && app->map) {
                TileEntity* tile_entity = static_cast<TileEntity*>(obj);
                int map_x = tile_entity->get_map_x();
                int map_y = tile_entity->get_map_y();
                SDL_Log("GameplayScreen: Clearing Map grid pointer for TileEntity at (%d,%d)", map_x, map_y);
                app->map->clear_tile_entity_at(map_x, map_y);
            }
            
            // DON'T DELETE - LifecycleManager owns the object lifecycle
            return true;  // Remove from list only
        }
        return false;
    });

    app->bomber_objects.remove_if([this](Bomber* bomber) {
        LifecycleManager::ObjectState state = app->lifecycle_manager->get_object_state(bomber);
        if (state == LifecycleManager::ObjectState::DELETED) {
            SDL_Log("GameplayScreen: Removing bomber %p from render list (LifecycleManager will delete)", bomber);
            // DON'T DELETE - LifecycleManager owns the object lifecycle
            return true;  // Remove from list only
        }
        return false;
    });
}

void GameplayScreen::show_all() {
    // Clear is now handled by Game.cpp
    
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

// TODO
void GameplayScreen::render_victory_screen() {
}