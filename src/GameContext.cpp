#include "GameContext.h"
#include "LifecycleManager.h"
#include "TileManager.h"
#include "ParticleEffectsManager.h"
#include "Map.h"
#include "GPUAcceleratedRenderer.h"
#include "TextRenderer.h"
#include "GameObject.h"
#include "SpatialPartitioning.h"
#include "RenderingFacade.h"
#include "CoordinateSystem.h"
#include <SDL3/SDL.h>

// Import CoordinateConfig constants for refactoring Phase 1
static constexpr int TILE_SIZE = CoordinateConfig::TILE_SIZE;

GameContext::GameContext(LifecycleManager* lifecycle,
                         TileManager* tiles,
                         ParticleEffectsManager* effects,
                         Map* map,
                         GPUAcceleratedRenderer* renderer,
                         TextRenderer* text,
                         RenderingFacade* facade)
    : lifecycle_manager(lifecycle)
    , tile_manager(tiles)
    , particle_effects(effects)
    , map(map)
    , gpu_renderer(nullptr) // REMOVED: Legacy renderer - handled by RenderingFacade
    , text_renderer(text)
    , spatial_grid(nullptr)
    , rendering_facade(facade)
    , render_objects(nullptr) {
    
    // Initialize spatial grid for collision optimization
    spatial_grid = new SpatialGrid(TILE_SIZE); // TILE_SIZE pixels = tile size
    SDL_Log("GameContext: Created SpatialGrid with %d-pixel cells", TILE_SIZE);
    
    // ARCHITECTURE FIX: Set up LifecycleManager coordination
    if (lifecycle_manager) {
        lifecycle_manager->set_game_context(this);
        SDL_Log("GameContext: Coordinated with LifecycleManager for proper cleanup");
    }
    
    // Initialize rendering facade if not provided
    if (!rendering_facade) {
        rendering_facade = new RenderingFacade();
        SDL_Log("GameContext: Created default RenderingFacade");
        
        // TODO: Initialize RenderingFacade properly - needs SDL_Window reference
        // For now, we'll initialize it when needed in the render loop
    }
}

GameContext::~GameContext() {
    delete spatial_grid;
    spatial_grid = nullptr;
    SDL_Log("GameContext: Cleaned up SpatialGrid");
    
    delete rendering_facade;
    rendering_facade = nullptr;
    SDL_Log("GameContext: Cleaned up RenderingFacade");
}

bool GameContext::is_position_blocked(int map_x, int map_y) const {
    return tile_manager ? tile_manager->is_tile_blocking_at(map_x, map_y) : false;
}

bool GameContext::has_bomb_at(int map_x, int map_y) const {
    return tile_manager ? tile_manager->has_bomb_at(map_x, map_y) : false;
}

bool GameContext::is_position_walkable(int map_x, int map_y) const {
    return tile_manager ? tile_manager->is_position_walkable(map_x, map_y) : false;
}

void GameContext::request_destruction_effect(float x, float y, float intensity) const {
    if (particle_effects) {
        particle_effects->create_box_destruction_effect(x, y, intensity);
    }
}

void GameContext::mark_for_destruction(GameObject* obj) const {
    if (lifecycle_manager && obj) {
        lifecycle_manager->mark_for_destruction(obj);
    }
    
    // Remove from spatial systems immediately
    remove_from_spatial_systems(obj);
}

void GameContext::remove_from_spatial_systems(GameObject* obj) const {
    // COLLISION FIX: Remove from SpatialGrid when destroying objects
    if (spatial_grid && obj) {
        spatial_grid->remove_object(obj);
        SDL_Log("GameContext: Removed object %p (type=%d) from SpatialGrid", obj, obj->get_type());
    }
}

void GameContext::register_object(GameObject* obj) const {
    if (lifecycle_manager && obj) {
        lifecycle_manager->register_object(obj);
    }
    
    // NOTE: render_objects list is managed by the object creator using proper smart pointer ownership
    
    // COLLISION FIX: Also add to SpatialGrid for optimized collision detection
    if (spatial_grid && obj) {
        spatial_grid->add_object(obj);
        SDL_Log("GameContext: Added object %p (type=%d) to SpatialGrid at (%d,%d)", 
                obj, obj->get_type(), obj->get_x(), obj->get_y());
    }
}

void GameContext::set_object_lists(std::list<std::unique_ptr<GameObject>>* objects) {
    render_objects = objects;
    SDL_Log("GameContext: Render objects list set to %p", render_objects);
}

void GameContext::set_map(Map* new_map) {
    map = new_map;
    SDL_Log("GameContext: Map set to %p", map);
}

void GameContext::update_object_position_in_spatial_grid(GameObject* obj, float old_x, float old_y) const {
    if (spatial_grid && obj) {
        PixelCoord old_position(old_x, old_y);
        spatial_grid->update_object_position(obj, old_position);
    }
}