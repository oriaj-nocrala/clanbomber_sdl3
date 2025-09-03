#include "Map.h"
#include "MapTile.h"
#include "TileEntity.h"
#include "MapTile_Pure.h"
#include "MapEntry.h"
#include "GameContext.h"
#include "CoordinateSystem.h"
#include <algorithm>
#include <random>
#include <filesystem>
#include <SDL3/SDL.h>

// Import CoordinateConfig constants for refactoring Phase 1
static constexpr int TILE_SIZE = CoordinateConfig::TILE_SIZE;

Map::Map(GameContext* _context) {
    context = _context;
    current_map_index = 0;
    current_map = nullptr;
    
    // Initialize both arrays
    for (int x = 0; x < MAP_WIDTH; x++) {
        for (int y = 0; y < MAP_HEIGHT; y++) {
            maptiles[x][y] = nullptr;  // Legacy compatibility
            tile_entities[x][y] = nullptr;  // NEW: TileEntity storage
        }
    }
    
    enumerate_maps();
}

Map::~Map() {
    clear();
    
    // Clean up map list
    for (auto& map_entry : map_list) {
        delete map_entry;
    }
    map_list.clear();
}

void Map::enumerate_maps() {
    map_list.clear();
    
    std::filesystem::path maps_dir = "data/maps";
    if (!std::filesystem::exists(maps_dir)) {
        SDL_Log("Maps directory not found: %s", maps_dir.string().c_str());
        return;
    }
    
    for (const auto& entry : std::filesystem::directory_iterator(maps_dir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".map") {
            MapEntry* map_entry = new MapEntry(entry.path().string());
            if (map_entry->load()) {
                map_list.push_back(map_entry);
            } else {
                delete map_entry;
            }
        }
    }
    
    if (map_list.empty()) {
        SDL_Log("No valid maps found");
    } else {
        SDL_Log("Found %zu maps", map_list.size());
        current_map = map_list[0];
    }
}

void Map::load() {
    if (current_map) {
        reload();
    } else if (!map_list.empty()) {
        current_map = map_list[0];
        reload();
    }
}

void Map::clear() {
    // Clear legacy tiles
    for (int x = 0; x < MAP_WIDTH; x++) {
        for (int y = 0; y < MAP_HEIGHT; y++) {
            if (maptiles[x][y]) {
                delete maptiles[x][y];
                maptiles[x][y] = nullptr;
            }
            
            // Clear TileEntity (registered with LifecycleManager, don't delete directly)
            if (tile_entities[x][y]) {
                // TileEntity cleanup handled by LifecycleManager
                tile_entities[x][y] = nullptr;
            }
        }
    }
}

void Map::reload() {
    if (!current_map) return;
    
    clear();
    
    SDL_Log("Map: Loading with NEW TileEntity architecture");
    
    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            char tile_char = current_map->get_data(x, y);
            
            MapTile_Pure::TILE_TYPE tile_type;
            
            switch (tile_char) {
                case '0': case '1': case '2': case '3':
                case '4': case '5': case '6': case '7':
                case ' ':
                    tile_type = MapTile_Pure::GROUND;
                    break;
                case '*':
                case '-':
                    tile_type = MapTile_Pure::WALL;
                    break;
                case '+':
                    tile_type = MapTile_Pure::BOX;
                    break;
                case 'R':
                    // Random box
                    tile_type = (rand() % 3) ? MapTile_Pure::BOX : MapTile_Pure::GROUND;
                    break;
                default:
                    tile_type = MapTile_Pure::GROUND;
                    break;
            }
            
            // Create NEW architecture: MapTile_Pure + TileEntity
            MapTile_Pure* tile_data = MapTile_Pure::create(tile_type, x, y);
            
            if (tile_type == MapTile_Pure::BOX) {
                // Use specialized TileEntity_Box for box tiles
                tile_entities[x][y] = new TileEntity_Box(tile_data, context);
            } else {
                // Use base TileEntity for other tiles
                tile_entities[x][y] = new TileEntity(tile_data, context);
            }
            
            // Register TileEntity with GameContext (LifecycleManager + render list)
            if (context) {
                context->register_object(tile_entities[x][y]);
            }
            
            // Legacy compatibility: also create old MapTile
            maptiles[x][y] = MapTile::create(
                tile_type == MapTile_Pure::GROUND ? MapTile::GROUND :
                tile_type == MapTile_Pure::WALL ? MapTile::WALL :
                MapTile::BOX, 
                x*TILE_SIZE, y*TILE_SIZE, context);
        }
    }
    
    SDL_Log("Map: Created %d TileEntities with new architecture", MAP_WIDTH * MAP_HEIGHT);
}

void Map::show() {
    // NEW ARCHITECTURE: TileEntities render themselves through GameObject system
    // No need to render here - TileEntities are in app->objects and render automatically
    
    // Legacy compatibility: still show old tiles if needed
    for (int x = 0; x < MAP_WIDTH; x++) {
        for (int y = 0; y < MAP_HEIGHT; y++) {
            if (maptiles[x][y] && !tile_entities[x][y]) {
                // Only show legacy tiles if no TileEntity exists
                maptiles[x][y]->show();
            }
        }
    }
}

// === PURE GRID MANAGER FUNCTIONS ===

MapTile* Map::get_tile(int tx, int ty) {
    if (tx >= 0 && tx < MAP_WIDTH && ty >= 0 && ty < MAP_HEIGHT) {
        return maptiles[tx][ty];
    }
    return nullptr;
}

TileEntity* Map::get_tile_entity(int tx, int ty) {
    if (tx >= 0 && tx < MAP_WIDTH && ty >= 0 && ty < MAP_HEIGHT) {
        return tile_entities[tx][ty];
    }
    return nullptr;
}

void Map::set_tile(int tx, int ty, MapTile* tile) {
    if (tx < 0 || tx >= MAP_WIDTH || ty < 0 || ty >= MAP_HEIGHT) {
        SDL_Log("Map::set_tile() - Invalid position (%d,%d)", tx, ty);
        return;
    }
    
    SDL_Log("Map: Setting legacy tile at (%d,%d) to %p", tx, ty, tile);
    maptiles[tx][ty] = tile;
}

void Map::set_tile_entity(int tx, int ty, TileEntity* tile_entity) {
    if (tx < 0 || tx >= MAP_WIDTH || ty < 0 || ty >= MAP_HEIGHT) {
        SDL_Log("Map::set_tile_entity() - Invalid position (%d,%d)", tx, ty);
        return;
    }
    
    SDL_Log("Map: Setting TileEntity at (%d,%d) to %p", tx, ty, tile_entity);
    tile_entities[tx][ty] = tile_entity;
}

void Map::clear_tile_entity_at(int tx, int ty) {
    if (tx < 0 || tx >= MAP_WIDTH || ty < 0 || ty >= MAP_HEIGHT) {
        SDL_Log("Map::clear_tile_entity_at() - Invalid position (%d,%d)", tx, ty);
        return;
    }
    
    if (tile_entities[tx][ty]) {
        SDL_Log("Map: Clearing TileEntity pointer at (%d,%d) - was %p", tx, ty, tile_entities[tx][ty]);
        tile_entities[tx][ty] = nullptr;
    }
}

void Map::load_random_valid() {
    if (map_list.empty()) return;
    
    current_map_index = rand() % map_list.size();
    current_map = map_list[current_map_index];
    reload();
}

void Map::load_next_valid(int map_nr) {
    if (map_list.empty()) return;
    
    if (map_nr >= 0 && map_nr < (int)map_list.size()) {
        current_map_index = map_nr;
    } else {
        current_map_index = (current_map_index + 1) % map_list.size();
    }
    
    current_map = map_list[current_map_index];
    reload();
}

void Map::act() {
    // Map is now a PURE GRID MANAGER - NO coordination logic!
    // TileManager handles ALL coordination between systems
    
    // Map only provides grid access - that's it!
    // NO MORE coordination crossing here!
}

void Map::refresh_holes() {
    // No-op for now - would handle animated holes/ground effects
}

bool Map::any_valid_map() {
    return !map_list.empty();
}

int Map::get_map_count() {
    return (int)map_list.size();
}

std::string Map::get_name() {
    if (current_map) {
        return current_map->get_name();
    }
    return "No Map";
}

std::string Map::get_author() {
    if (current_map) {
        return current_map->get_author();
    }
    return "Unknown";
}

CL_Vector Map::get_bomber_pos(int nr) {
    if (current_map) {
        return current_map->get_bomber_pos(nr);
    }
    // Default positions if no map
    switch (nr) {
        case 0: return CL_Vector(2, 2);
        case 1: return CL_Vector(17, 2);
        case 2: return CL_Vector(2, 12);
        case 3: return CL_Vector(17, 12);
        case 4: return CL_Vector(9, 2);
        case 5: return CL_Vector(9, 12);
        case 6: return CL_Vector(2, 7);
        case 7: return CL_Vector(17, 7);
        default: return CL_Vector(2, 2);
    }
}

void Map::randomize_bomber_positions() {
    if (current_map) {
        // The positions are shuffled in MapEntry, this is just a placeholder
        // In the original, this would modify the bomber position order
    }
}
