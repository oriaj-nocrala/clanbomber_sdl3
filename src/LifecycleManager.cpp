#include "LifecycleManager.h"
#include "GameObject.h"
#include "MapTile.h"
#include "MapTile_Box.h"
#include "TileEntity.h"
#include <algorithm>
#include <SDL3/SDL.h>

LifecycleManager::LifecycleManager() {
    SDL_Log("LifecycleManager: Initialized unified object lifecycle system");
}

LifecycleManager::~LifecycleManager() {
    clear_all();
    SDL_Log("LifecycleManager: Shutdown complete");
}

void LifecycleManager::register_object(GameObject* obj) {
    if (!obj) return;
    
    // Check if already registered
    if (find_managed_object(obj)) {
        SDL_Log("LifecycleManager: Object %p already registered", obj);
        return;
    }
    
    managed_objects.emplace_back(obj);
    SDL_Log("LifecycleManager: Registered object %p (total: %zu)", obj, managed_objects.size());
}

void LifecycleManager::register_tile(MapTile* tile, int map_x, int map_y) {
    if (!tile) return;
    
    // Check if already registered
    if (find_managed_tile(tile)) {
        SDL_Log("LifecycleManager: Tile %p already registered", tile);
        return;
    }
    
    managed_tiles.emplace_back(tile, map_x, map_y);
    SDL_Log("LifecycleManager: Registered tile %p at (%d,%d) (total: %zu)", tile, map_x, map_y, managed_tiles.size());
}

void LifecycleManager::register_tile_entity(TileEntity* tile_entity) {
    if (!tile_entity) return;
    
    // TileEntity hereda de GameObject, así que lo registramos como objeto normal
    register_object(tile_entity);
    SDL_Log("LifecycleManager: Registered TileEntity %p as GameObject", tile_entity);
}

void LifecycleManager::mark_for_destruction(GameObject* obj) {
    if (!obj) return;
    
    ManagedObject* managed = find_managed_object(obj);
    if (!managed) {
        SDL_Log("LifecycleManager: Cannot mark unregistered object %p for destruction", obj);
        return;
    }
    
    if (managed->state == ObjectState::ACTIVE) {
        managed->state = ObjectState::DYING;
        managed->state_timer = 0.0f;
        SDL_Log("LifecycleManager: Object %p marked for destruction (ACTIVE → DYING)", obj);
        
        if (managed->on_state_change) {
            managed->on_state_change();
        }
    }
}

void LifecycleManager::mark_tile_for_destruction(MapTile* tile, MapTile* replacement) {
    if (!tile) return;
    
    ManagedTile* managed = find_managed_tile(tile);
    if (!managed) {
        SDL_Log("LifecycleManager: Cannot mark unregistered tile %p for destruction", tile);
        return;
    }
    
    if (managed->state == ObjectState::ACTIVE) {
        managed->state = ObjectState::DYING;
        managed->state_timer = 0.0f;
        managed->replacement = replacement;
        SDL_Log("LifecycleManager: Tile %p at (%d,%d) marked for destruction (ACTIVE → DYING)", 
                tile, managed->map_x, managed->map_y);
    }
}

void LifecycleManager::mark_tile_entity_for_destruction(TileEntity* tile_entity) {
    if (!tile_entity) return;
    
    // TileEntity hereda de GameObject, usa el método estándar
    mark_for_destruction(tile_entity);
    SDL_Log("LifecycleManager: TileEntity %p marked for destruction", tile_entity);
}

void LifecycleManager::update_states(float deltaTime) {
    // Update all object states
    for (auto& managed : managed_objects) {
        if (managed.state != ObjectState::DELETED) {
            update_object_state(managed, deltaTime);
        }
    }
    
    // Update all tile states  
    for (auto& managed : managed_tiles) {
        if (managed.state != ObjectState::DELETED) {
            update_tile_state(managed, deltaTime);
        }
    }
}

void LifecycleManager::update_object_state(ManagedObject& managed, float deltaTime) {
    managed.state_timer += deltaTime;
    
    switch (managed.state) {
        case ObjectState::ACTIVE:
            // Normal operation - check if object marked itself for deletion
            if (managed.object->delete_me) {
                managed.state = ObjectState::DYING;
                managed.state_timer = 0.0f;
                SDL_Log("LifecycleManager: Object %p self-marked for destruction", managed.object);
            }
            break;
            
        case ObjectState::DYING:
            // Allow object to perform death animation
            // Most objects don't have death animations, so transition quickly
            if (managed.state_timer >= 0.1f) {  // 0.1s default dying time
                managed.state = ObjectState::DEAD;
                managed.state_timer = 0.0f;
                SDL_Log("LifecycleManager: Object %p death animation complete (DYING → DEAD)", managed.object);
            }
            break;
            
        case ObjectState::DEAD:
            // Ready for cleanup
            managed.state = ObjectState::DELETED;
            SDL_Log("LifecycleManager: Object %p ready for deletion (DEAD → DELETED)", managed.object);
            break;
            
        case ObjectState::DELETED:
            // Will be cleaned up by cleanup_dead_objects()
            break;
    }
}

void LifecycleManager::update_tile_state(ManagedTile& managed, float deltaTime) {
    managed.state_timer += deltaTime;
    
    switch (managed.state) {
        case ObjectState::ACTIVE:
            // Normal operation - check if tile marked itself for deletion
            if (managed.tile->delete_me) {
                managed.state = ObjectState::DYING;
                managed.state_timer = 0.0f;
                SDL_Log("LifecycleManager: Tile %p at (%d,%d) self-marked for destruction", 
                        managed.tile, managed.map_x, managed.map_y);
            }
            break;
            
        case ObjectState::DYING:
            {
                // Allow tile to perform destruction animation
                // For MapTile_Box, this is the fragment animation (0.5s)
                float dying_duration = 0.5f;  // Default destruction animation time
                
                // Special handling for different tile types
                if (dynamic_cast<MapTile_Box*>(managed.tile)) {
                    dying_duration = 0.5f;  // Box fragment animation
                }
                
                if (managed.state_timer >= dying_duration) {
                    managed.state = ObjectState::DEAD;
                    managed.state_timer = 0.0f;
                    SDL_Log("LifecycleManager: Tile %p destruction animation complete (DYING → DEAD)", managed.tile);
                }
                break;
            }
            
        case ObjectState::DEAD:
            // Ready for cleanup and replacement
            managed.state = ObjectState::DELETED;
            SDL_Log("LifecycleManager: Tile %p ready for replacement (DEAD → DELETED)", managed.tile);
            break;
            
        case ObjectState::DELETED:
            // Will be cleaned up by cleanup_dead_objects()
            break;
    }
}

void LifecycleManager::cleanup_dead_objects() {
    // Remove deleted objects from tracking (don't delete the objects themselves!)
    auto it = std::remove_if(managed_objects.begin(), managed_objects.end(),
        [](const ManagedObject& managed) {
            if (managed.state == ObjectState::DELETED) {
                SDL_Log("LifecycleManager: Removing object %p from tracking (deletion handled elsewhere)", managed.object);
                return true;
            }
            return false;
        });
    
    size_t objects_removed = std::distance(it, managed_objects.end());
    managed_objects.erase(it, managed_objects.end());
    
    if (objects_removed > 0) {
        SDL_Log("LifecycleManager: Removed %zu objects from tracking", objects_removed);
    }
    
    // Remove deleted tiles from tracking (don't delete the tiles themselves!)
    auto tile_it = std::remove_if(managed_tiles.begin(), managed_tiles.end(),
        [](const ManagedTile& managed) {
            if (managed.state == ObjectState::DELETED) {
                SDL_Log("LifecycleManager: Removing tile %p at (%d,%d) from tracking", 
                        managed.tile, managed.map_x, managed.map_y);
                return true;
            }
            return false;
        });
    
    size_t tiles_removed = std::distance(tile_it, managed_tiles.end());
    managed_tiles.erase(tile_it, managed_tiles.end());
    
    if (tiles_removed > 0) {
        SDL_Log("LifecycleManager: Removed %zu tiles from tracking", tiles_removed);
    }
}

LifecycleManager::ObjectState LifecycleManager::get_object_state(GameObject* obj) const {
    const ManagedObject* managed = find_managed_object(obj);
    return managed ? managed->state : ObjectState::DELETED;
}

LifecycleManager::ObjectState LifecycleManager::get_tile_state(MapTile* tile) const {
    const ManagedTile* managed = find_managed_tile(tile);
    return managed ? managed->state : ObjectState::DELETED;
}

LifecycleManager::ObjectState LifecycleManager::get_tile_entity_state(TileEntity* tile_entity) const {
    // TileEntity hereda de GameObject, usa el método estándar
    return get_object_state(tile_entity);
}

bool LifecycleManager::is_dying_or_dead(GameObject* obj) const {
    ObjectState state = get_object_state(obj);
    return state == ObjectState::DYING || state == ObjectState::DEAD;
}

bool LifecycleManager::is_dying_or_dead(MapTile* tile) const {
    ObjectState state = get_tile_state(tile);
    return state == ObjectState::DYING || state == ObjectState::DEAD;
}

bool LifecycleManager::is_dying_or_dead(TileEntity* tile_entity) const {
    // TileEntity hereda de GameObject, usa el método estándar
    return is_dying_or_dead(static_cast<GameObject*>(tile_entity));
}

void LifecycleManager::clear_all() {
    SDL_Log("LifecycleManager: Clearing all managed objects and tiles");
    
    // Clean up all objects
    for (const auto& managed : managed_objects) {
        delete managed.object;
    }
    managed_objects.clear();
    
    // Clear tiles (don't delete - Map owns them)
    managed_tiles.clear();
}

size_t LifecycleManager::get_active_object_count() const {
    return std::count_if(managed_objects.begin(), managed_objects.end(),
        [](const ManagedObject& managed) {
            return managed.state == ObjectState::ACTIVE;
        });
}

size_t LifecycleManager::get_active_tile_count() const {
    return std::count_if(managed_tiles.begin(), managed_tiles.end(),
        [](const ManagedTile& managed) {
            return managed.state == ObjectState::ACTIVE;
        });
}

// Private helper methods
LifecycleManager::ManagedObject* LifecycleManager::find_managed_object(GameObject* obj) {
    auto it = std::find_if(managed_objects.begin(), managed_objects.end(),
        [obj](const ManagedObject& managed) {
            return managed.object == obj;
        });
    return it != managed_objects.end() ? &(*it) : nullptr;
}

LifecycleManager::ManagedTile* LifecycleManager::find_managed_tile(MapTile* tile) {
    auto it = std::find_if(managed_tiles.begin(), managed_tiles.end(),
        [tile](const ManagedTile& managed) {
            return managed.tile == tile;
        });
    return it != managed_tiles.end() ? &(*it) : nullptr;
}

const LifecycleManager::ManagedObject* LifecycleManager::find_managed_object(GameObject* obj) const {
    auto it = std::find_if(managed_objects.begin(), managed_objects.end(),
        [obj](const ManagedObject& managed) {
            return managed.object == obj;
        });
    return it != managed_objects.end() ? &(*it) : nullptr;
}

const LifecycleManager::ManagedTile* LifecycleManager::find_managed_tile(MapTile* tile) const {
    auto it = std::find_if(managed_tiles.begin(), managed_tiles.end(),
        [tile](const ManagedTile& managed) {
            return managed.tile == tile;
        });
    return it != managed_tiles.end() ? &(*it) : nullptr;
}