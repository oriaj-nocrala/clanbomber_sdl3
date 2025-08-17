#include "GameContext.h"
#include "LifecycleManager.h"
#include "TileManager.h"
#include "ParticleEffectsManager.h"
#include "Map.h"
#include "GPUAcceleratedRenderer.h"
#include "TextRenderer.h"
#include "GameObject.h"

GameContext::GameContext(LifecycleManager* lifecycle,
                         TileManager* tiles,
                         ParticleEffectsManager* effects,
                         Map* map,
                         GPUAcceleratedRenderer* renderer,
                         TextRenderer* text)
    : lifecycle_manager(lifecycle)
    , tile_manager(tiles)
    , particle_effects(effects)
    , map(map)
    , gpu_renderer(renderer)
    , text_renderer(text) {
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
}

void GameContext::register_object(GameObject* obj) const {
    if (lifecycle_manager && obj) {
        lifecycle_manager->register_object(obj);
    }
}