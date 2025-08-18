#include "GameLogic.h"
#include "Bomb.h"
#include "Explosion.h"
#include "Extra.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <vector>

GameLogic::GameLogic(GameContext* context) 
    : game_context(context), is_paused(false), frame_counter(0) {
    if (!game_context) {
        SDL_Log("ERROR: GameLogic initialized with null GameContext");
    }
}

void GameLogic::update_frame(float deltaTime) {
    if (!game_context) return;
    
    frame_counter++;
    
    // Skip game logic if paused
    if (is_paused) {
        return;
    }
    
    // Execute game logic in order
    update_all_objects(deltaTime);
    cleanup_deleted_objects();
    
    // Log statistics every 600 frames (10 seconds at 60 FPS)
    if (frame_counter % 600 == 0) {
        log_frame_statistics();
    }
}

void GameLogic::update_all_objects(float deltaTime) {
    if (!game_context) return;
    
    const auto& objects = game_context->get_object_lists();
    
    // Update all active objects
    for (auto& obj : objects) {
        if (should_skip_object_update(obj)) continue;
        
        try {
            obj->act(deltaTime);
        } catch (const std::exception& e) {
            SDL_Log("ERROR: Exception in object update: %s", e.what());
            // Mark object for deletion to prevent further errors
            obj->delete_me = true;
        }
    }
}

void GameLogic::render_all_objects() {
    if (!game_context) return;
    
    const auto& objects = game_context->get_object_lists();
    
    // OPTIMIZED: Collect all objects for Z-order sorting (matches legacy show_all behavior)
    std::vector<GameObject*> draw_list;
    for (auto& obj : objects) {
        if (!obj || obj->delete_me) continue;
        draw_list.push_back(obj);
    }
    
    // Sort by Z-order for proper layering (matches legacy show_all behavior)
    std::sort(draw_list.begin(), draw_list.end(), [](GameObject* go1, GameObject* go2) {
        return go1->get_z() < go2->get_z();
    });
    
    // Render all visible objects in Z-order
    for (auto& obj : draw_list) {
        try {
            obj->show();
        } catch (const std::exception& e) {
            SDL_Log("ERROR: Exception in object rendering: %s", e.what());
        }
    }
}

void GameLogic::cleanup_deleted_objects() {
    if (!game_context || !game_context->get_lifecycle_manager()) return;
    
    // Let LifecycleManager handle the cleanup
    game_context->get_lifecycle_manager()->cleanup_dead_objects();
}

GameObject* GameLogic::find_object_by_id(int object_id) const {
    // TODO: Implement proper object ID system in GameObject class
    // For now, this functionality is disabled
    (void)object_id; // Suppress unused parameter warning
    return nullptr;
}

Bomber* GameLogic::find_bomber_by_id(int bomber_id) const {
    // TODO: Implement proper object ID system in GameObject class
    // For now, this functionality is disabled
    (void)bomber_id; // Suppress unused parameter warning
    return nullptr;
}

size_t GameLogic::count_active_objects() const {
    if (!game_context) return 0;
    
    const auto& objects = game_context->get_object_lists();
    
    return std::count_if(objects.begin(), objects.end(),
        [](const GameObject* obj) {
            return obj && !obj->delete_me;
        });
}

void GameLogic::clear_all_objects() {
    if (!game_context || !game_context->get_lifecycle_manager()) return;
    
    SDL_Log("GameLogic: Clearing all game objects");
    game_context->get_lifecycle_manager()->clear_all();
}

void GameLogic::reset_game_state() {
    SDL_Log("GameLogic: Resetting game state");
    
    // Clear all objects
    clear_all_objects();
    
    // Reset internal state
    is_paused = false;
    frame_counter = 0;
}

GameLogic::GameStats GameLogic::get_game_statistics() const {
    GameStats stats;
    
    if (!game_context) return stats;
    
    const auto& objects = game_context->get_object_lists();
    
    for (const auto& obj : objects) {
        if (!obj || obj->delete_me) continue;
        
        stats.total_objects++;
        
        switch (obj->get_type()) {
            case GameObject::BOMBER:
                stats.active_bombers++;
                break;
            case GameObject::BOMB:
                stats.active_bombs++;
                break;
            case GameObject::EXPLOSION:
                stats.active_explosions++;
                break;
            case GameObject::EXTRA:
                stats.active_extras++;
                break;
            default:
                // Other object types
                break;
        }
    }
    
    return stats;
}

void GameLogic::log_frame_statistics() const {
    auto stats = get_game_statistics();
    
    SDL_Log("GameLogic Stats - Frame: %llu, Objects: %zu (Bombers: %zu, Bombs: %zu, Explosions: %zu, Extras: %zu)",
        static_cast<unsigned long long>(frame_counter),
        stats.total_objects,
        stats.active_bombers, 
        stats.active_bombs,
        stats.active_explosions,
        stats.active_extras);
}

bool GameLogic::should_skip_object_update(GameObject* obj) const {
    return !obj || obj->delete_me;
}