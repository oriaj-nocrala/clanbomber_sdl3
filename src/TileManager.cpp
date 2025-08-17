#include "TileManager.h"
#include "GameContext.h"
#include "Map.h"
#include "MapTile.h"
#include "TileEntity.h"
#include "LifecycleManager.h"
#include "Bomb.h"
#include "Bomber.h"
#include <SDL3/SDL.h>
#include <cassert>

TileManager::TileManager(GameContext* context) : context(context) {
    SDL_Log("TileManager: Initialized intelligent tile coordination system");
}

void TileManager::set_context(GameContext* new_context) {
    context = new_context;
    SDL_Log("TileManager: GameContext set to %p", context);
}

TileManager::~TileManager() {
    SDL_Log("TileManager: Shutdown complete");
}

// === COORDINACIÓN PRINCIPAL ===

void TileManager::update_tiles(float deltaTime) {
    if (!context || !context->get_map()) return;
    
    // PASO 1: Coordinar con LifecycleManager PRIMERO
    coordinate_with_lifecycle_manager();
    
    // PASO 2: Procesar tiles que están muriendo
    process_dying_tiles();
    
    // PASO 3: Procesar tiles que están listos para reemplazo
    process_dead_tiles();
    
    // PASO 4: Actualizar tiles individuales
    handle_tile_updates();
}

void TileManager::handle_tile_updates() {
    iterate_all_tiles([this](MapTile* tile, int x, int y) {
        if (tile) {
            update_single_tile(tile, x, y);
        }
    });
}

// === COORDINACIÓN CON LIFECYCLE ===

void TileManager::coordinate_with_lifecycle_manager() {
    if (!context->get_lifecycle_manager()) {
        SDL_Log("TileManager: ERROR - No LifecycleManager available for coordination");
        return;
    }
    
    // TileManager es el ÚNICO autorizado para coordinar con LifecycleManager
    // Esto elimina la coordinación cruzada que nos ha estado jodiendo
    context->get_lifecycle_manager()->update_states(0.016f); // ~60 FPS delta
}

void TileManager::process_dying_tiles() {
    iterate_all_tiles([this](MapTile* tile, int x, int y) {
        if (!tile || !context->get_lifecycle_manager()) return;
        
        LifecycleManager::ObjectState state = context->get_lifecycle_manager()->get_tile_state(tile);
        if (state == LifecycleManager::ObjectState::DYING) {
            SDL_Log("TileManager: Tile at (%d,%d) is dying - monitoring", x, y);
            // Tile está en animación de muerte - solo monitorear
        }
    });
}

void TileManager::process_dead_tiles() {
    iterate_all_tiles([this](MapTile* tile, int x, int y) {
        if (!tile || !context->get_lifecycle_manager()) return;
        
        LifecycleManager::ObjectState state = context->get_lifecycle_manager()->get_tile_state(tile);
        if (state == LifecycleManager::ObjectState::DELETED) {
            SDL_Log("TileManager: Tile at (%d,%d) ready for replacement - executing", x, y);
            replace_tile_when_ready(x, y, MapTile::GROUND);
        }
    });
}

// === GESTIÓN DE TILES ===

void TileManager::request_tile_destruction(int map_x, int map_y) {
    if (!is_valid_position(map_x, map_y)) return;
    
    SDL_Log("TileManager: Processing destruction request for tile at (%d,%d)", map_x, map_y);
    
    bool tile_destroyed = false;
    Bomb* bomb_to_explode = nullptr;
    
    // DUAL ARCHITECTURE COORDINATION: Handle legacy MapTile first
    MapTile* legacy_tile = context->get_map()->get_tile(map_x, map_y);
    if (legacy_tile && legacy_tile->is_burnable()) {
        SDL_Log("TileManager: Destroying legacy MapTile at (%d,%d)", map_x, map_y);
        legacy_tile->destroy();
        tile_destroyed = true;
        if (legacy_tile->bomb && !bomb_to_explode) {
            bomb_to_explode = legacy_tile->bomb;
        }
    }
    
    // Handle new TileEntity (only if legacy wasn't destroyed to avoid duplicate effects)
    if (!tile_destroyed) {
        TileEntity* tile_entity = context->get_map()->get_tile_entity(map_x, map_y);
        if (tile_entity && !tile_entity->is_destroyed() && tile_entity->is_destructible()) {
            SDL_Log("TileManager: Destroying TileEntity at (%d,%d)", map_x, map_y);
            tile_entity->destroy();
            tile_destroyed = true;
            if (tile_entity->get_bomb() && !bomb_to_explode) {
                bomb_to_explode = tile_entity->get_bomb();
            }
        }
    }
    
    // Handle bomb explosion (avoid duplicate bomb explosions)
    if (bomb_to_explode) {
        bomb_to_explode->explode_delayed();
    }
    
    if (tile_destroyed) {
        SDL_Log("TileManager: Destruction completed for tile at (%d,%d)", map_x, map_y);
    } else {
        SDL_Log("TileManager: No destructible tile found at (%d,%d)", map_x, map_y);
    }
}

void TileManager::replace_tile_when_ready(int map_x, int map_y, int new_tile_type) {
    if (!is_valid_position(map_x, map_y) || !context->get_map()) return;
    
    perform_tile_replacement(map_x, map_y, new_tile_type);
}

bool TileManager::is_tile_ready_for_replacement(MapTile* tile) {
    if (!tile || !context->get_lifecycle_manager()) return false;
    
    LifecycleManager::ObjectState state = context->get_lifecycle_manager()->get_tile_state(tile);
    return state == LifecycleManager::ObjectState::DELETED;
}

// === QUERY INTELIGENTE ===

bool TileManager::is_position_walkable(int map_x, int map_y) {
    if (!is_valid_position(map_x, map_y)) return false;
    
    MapTile* tile = get_tile_at(map_x, map_y);
    if (!tile) return false;
    
    // Un tile es caminable si NO está bloqueando
    return !tile->is_blocking();
}

bool TileManager::is_position_blocked(int map_x, int map_y) {
    return !is_position_walkable(map_x, map_y);
}

MapTile* TileManager::get_tile_at(int map_x, int map_y) {
    if (!is_valid_position(map_x, map_y) || !context->get_map()) return nullptr;
    
    return context->get_map()->get_tile(map_x, map_y);
}

// === DUAL ARCHITECTURE OPTIMIZED QUERIES ===

bool TileManager::is_tile_blocking_at(int map_x, int map_y) {
    if (!is_valid_position(map_x, map_y) || !context->get_map()) return false;
    
    MapTile* legacy_tile = context->get_map()->get_tile(map_x, map_y);
    if (legacy_tile) {
        return legacy_tile->is_blocking();
    }
    
    TileEntity* tile_entity = context->get_map()->get_tile_entity(map_x, map_y);
    if (tile_entity) {
        return tile_entity->is_blocking();
    }
    
    return false;
}

bool TileManager::has_bomb_at(int map_x, int map_y) {
    if (!is_valid_position(map_x, map_y) || !context->get_map()) return false;
    
    MapTile* legacy_tile = context->get_map()->get_tile(map_x, map_y);
    if (legacy_tile) {
        return legacy_tile->bomb != nullptr;
    }
    
    TileEntity* tile_entity = context->get_map()->get_tile_entity(map_x, map_y);
    if (tile_entity) {
        return tile_entity->get_bomb() != nullptr;
    }
    
    return false;
}

bool TileManager::is_tile_destructible_at(int map_x, int map_y) {
    if (!is_valid_position(map_x, map_y) || !context->get_map()) return false;
    
    MapTile* legacy_tile = context->get_map()->get_tile(map_x, map_y);
    if (legacy_tile) {
        return legacy_tile->is_destructible();
    }
    
    TileEntity* tile_entity = context->get_map()->get_tile_entity(map_x, map_y);
    if (tile_entity) {
        return tile_entity->is_destructible();
    }
    
    return false;
}

// === GESTIÓN DE OBJETOS EN TILES ===

void TileManager::register_bomb_at(int map_x, int map_y, Bomb* bomb) {
    MapTile* tile = get_tile_at(map_x, map_y);
    if (tile && bomb) {
        tile->set_bomb(bomb);
        SDL_Log("TileManager: Registered bomb %p at (%d,%d)", bomb, map_x, map_y);
    }
}

void TileManager::unregister_bomb_at(int map_x, int map_y) {
    MapTile* tile = get_tile_at(map_x, map_y);
    if (tile) {
        tile->set_bomb(nullptr);
        SDL_Log("TileManager: Unregistered bomb at (%d,%d)", map_x, map_y);
    }
}

void TileManager::unregister_bomb_at(int map_x, int map_y, Bomb* bomb) {
    MapTile* tile = get_tile_at(map_x, map_y);
    if (tile && tile->get_bomb() == bomb) {
        tile->set_bomb(nullptr);
        SDL_Log("TileManager: Unregistered bomb %p at (%d,%d) with safety check", bomb, map_x, map_y);
    } else if (tile && tile->get_bomb() != bomb) {
        SDL_Log("WARNING: TileManager: Attempted to unregister bomb %p at (%d,%d) but found different bomb %p", 
                bomb, map_x, map_y, tile->get_bomb());
    }
}

Bomb* TileManager::get_bomb_at(int map_x, int map_y) {
    MapTile* tile = get_tile_at(map_x, map_y);
    return tile ? tile->get_bomb() : nullptr;
}

void TileManager::register_bomber_at(int map_x, int map_y, Bomber* bomber) {
    MapTile* tile = get_tile_at(map_x, map_y);
    if (tile && bomber) {
        tile->set_bomber(bomber);
        SDL_Log("TileManager: Registered bomber %p at (%d,%d)", bomber, map_x, map_y);
    }
}

void TileManager::unregister_bomber_at(int map_x, int map_y) {
    MapTile* tile = get_tile_at(map_x, map_y);
    if (tile) {
        tile->set_bomber(nullptr);
        SDL_Log("TileManager: Unregistered bomber at (%d,%d)", map_x, map_y);
    }
}

Bomber* TileManager::get_bomber_at(int map_x, int map_y) {
    MapTile* tile = get_tile_at(map_x, map_y);
    return tile ? tile->get_bomber() : nullptr;
}

bool TileManager::has_bomber_at(int map_x, int map_y) {
    return get_bomber_at(map_x, map_y) != nullptr;
}

// === UTILITIES ===

void TileManager::iterate_all_tiles(std::function<void(MapTile*, int, int)> callback) {
    if (!context->get_map() || !callback) return;
    
    for (int x = 0; x < MAP_WIDTH; x++) {
        for (int y = 0; y < MAP_HEIGHT; y++) {
            MapTile* tile = context->get_map()->get_tile(x, y);
            callback(tile, x, y);
        }
    }
}

std::vector<MapTile*> TileManager::get_destructible_tiles_in_radius(int center_x, int center_y, int radius) {
    std::vector<MapTile*> result;
    
    for (int dx = -radius; dx <= radius; dx++) {
        for (int dy = -radius; dy <= radius; dy++) {
            int x = center_x + dx;
            int y = center_y + dy;
            
            if (is_valid_position(x, y)) {
                MapTile* tile = get_tile_at(x, y);
                if (tile && tile->is_destructible()) {
                    result.push_back(tile);
                }
            }
        }
    }
    
    return result;
}

// === COORDINACIÓN INTERNA ===

void TileManager::update_single_tile(MapTile* tile, int map_x, int map_y) {
    if (!tile) return;
    
    // IMPORTANTE: Solo TileManager coordina - NO coordinación cruzada
    tile->act();
    
    // Si el tile se marcó para destrucción, lo procesaremos en el próximo update
    if (tile->delete_me) {
        SDL_Log("TileManager: Tile at (%d,%d) marked for destruction", map_x, map_y);
    }
}

void TileManager::handle_tile_destruction_request(int map_x, int map_y) {
    MapTile* tile = get_tile_at(map_x, map_y);
    if (!tile) return;
    
    SDL_Log("TileManager: Handling destruction request for tile at (%d,%d)", map_x, map_y);
    request_tile_destruction(map_x, map_y);
}

void TileManager::perform_tile_replacement(int map_x, int map_y, int new_tile_type) {
    if (!context->get_map()) return;
    
    SDL_Log("TileManager: Replacing tile at (%d,%d) with type %d", map_x, map_y, new_tile_type);
    
    // Eliminar tile anterior
    MapTile* old_tile = get_tile_at(map_x, map_y);
    if (old_tile) {
        delete old_tile;
    }
    
    // Crear nuevo tile
    MapTile* new_tile = MapTile::create(
        static_cast<MapTile::MAPTILE_TYPE>(new_tile_type),
        map_x * 40, map_y * 40, context
    );
    
    // Actualizar grid usando el setter
    context->get_map()->set_tile(map_x, map_y, new_tile);
    
    SDL_Log("TileManager: Tile replacement complete at (%d,%d)", map_x, map_y);
}

// === VALIDACIÓN ===

bool TileManager::is_valid_position(int map_x, int map_y) const {
    // Debug assertions for development builds
    assert(MAP_WIDTH > 0 && MAP_HEIGHT > 0);
    assert(map_x >= -1000 && map_x <= 1000); // Sanity check for extreme values
    assert(map_y >= -1000 && map_y <= 1000);
    
    return map_x >= 0 && map_x < MAP_WIDTH && map_y >= 0 && map_y < MAP_HEIGHT;
}