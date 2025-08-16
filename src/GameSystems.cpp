#include "GameSystems.h"
#include "GameContext.h"
#include "GameObject.h"
#include "Bomber.h"
#include "Timer.h"
#include <SDL3/SDL.h>

GameSystems::GameSystems(GameContext* context) 
    : context(context)
    , objects_ref(nullptr)
    , bombers_ref(nullptr) {
    SDL_Log("GameSystems: Initialized modular game systems");
}

GameSystems::~GameSystems() {
    SDL_Log("GameSystems: Shutdown complete");
}

void GameSystems::update_all_systems(float deltaTime) {
    // Cap delta time to prevent issues with large time steps
    const float max_delta = 1.0f / 30.0f; // 30 FPS minimum
    if (deltaTime > max_delta) {
        deltaTime = max_delta;
    }
    
    // Smooth delta time to prevent jitter
    static float avg_delta = 1.0f / 60.0f;
    avg_delta = avg_delta * 0.9f + deltaTime * 0.1f;
    deltaTime = avg_delta;
    
    update_input_system(deltaTime);
    update_ai_system(deltaTime);
    update_physics_system(deltaTime);
    update_collision_system(deltaTime);
    update_animation_system(deltaTime);
}

void GameSystems::render_all_systems() {
    render_world();
    render_effects();
    render_ui();
}

void GameSystems::update_input_system(float deltaTime) {
    // TODO: Extract input processing from individual objects
}

void GameSystems::update_physics_system(float deltaTime) {
    // Update object physics/movement
    if (!objects_ref) return;
    
    for (auto& obj : *objects_ref) {
        if (obj && !obj->delete_me) {
            obj->act(deltaTime);
        }
    }
}

void GameSystems::update_ai_system(float deltaTime) {
    // Update bomber AI and behaviors
    if (!bombers_ref) return;
    
    for (auto& bomber : *bombers_ref) {
        if (bomber && !bomber->delete_me) {
            bomber->act(deltaTime);
        }
    }
}

void GameSystems::update_collision_system(float deltaTime) {
    // TODO: Extract collision detection logic
}

void GameSystems::update_animation_system(float deltaTime) {
    // TODO: Extract animation logic
}

void GameSystems::render_world() {
    // TODO: Extract rendering logic
}

void GameSystems::render_effects() {
    // TODO: Extract effects rendering
}

void GameSystems::render_ui() {
    // TODO: Extract UI rendering
}

void GameSystems::register_object(GameObject* obj) {
    // TODO: Implement proper object registration
}

void GameSystems::register_bomber(Bomber* bomber) {
    // TODO: Implement proper bomber registration
}

void GameSystems::cleanup_destroyed_objects() {
    // TODO: Extract cleanup logic from GameplayScreen
}