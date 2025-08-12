#include "Map.h"
#include "MapTile.h"
#include "MapEntry.h"
#include "ClanBomber.h"
#include <algorithm>
#include <random>
#include <filesystem>
#include <SDL3/SDL.h>

Map::Map(ClanBomberApplication* _app) {
    app = _app;
    current_map_index = 0;
    current_map = nullptr;
    
    // Initialize maptiles array
    for (int x = 0; x < MAP_WIDTH; x++) {
        for (int y = 0; y < MAP_HEIGHT; y++) {
            maptiles[x][y] = nullptr;
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
    for (int x = 0; x < MAP_WIDTH; x++) {
        for (int y = 0; y < MAP_HEIGHT; y++) {
            if (maptiles[x][y]) {
                delete maptiles[x][y];
                maptiles[x][y] = nullptr;
            }
        }
    }
}

void Map::reload() {
    if (!current_map) return;
    
    clear();
    
    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            char tile_char = current_map->get_data(x, y);
            
            switch (tile_char) {
                case '0': case '1': case '2': case '3':
                case '4': case '5': case '6': case '7':
                case ' ':
                    maptiles[x][y] = MapTile::create(MapTile::GROUND, x*40, y*40, app);
                    break;
                case '*':
                    maptiles[x][y] = MapTile::create(MapTile::WALL, x*40, y*40, app);
                    break;
                case '+':
                    maptiles[x][y] = MapTile::create(MapTile::BOX, x*40, y*40, app);
                    break;
                case 'R':
                    // Random box
                    if (rand() % 3) {
                        maptiles[x][y] = MapTile::create(MapTile::BOX, x*40, y*40, app);
                    } else {
                        maptiles[x][y] = MapTile::create(MapTile::GROUND, x*40, y*40, app);
                    }
                    break;
                case '-':
                    maptiles[x][y] = MapTile::create(MapTile::WALL, x*40, y*40, app);
                    break;
                default:
                    maptiles[x][y] = MapTile::create(MapTile::GROUND, x*40, y*40, app);
                    break;
            }
        }
    }
}

void Map::show() {
    for (int x = 0; x < MAP_WIDTH; x++) {
        for (int y = 0; y < MAP_HEIGHT; y++) {
            if (maptiles[x][y]) {
                maptiles[x][y]->show();
            }
        }
    }
}

MapTile* Map::get_tile(int tx, int ty) {
    if (tx >= 0 && tx < MAP_WIDTH && ty >= 0 && ty < MAP_HEIGHT) {
        return maptiles[tx][ty];
    }
    return nullptr;
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
    // Update animated map tiles and handle destroyed boxes
    for (int x = 0; x < MAP_WIDTH; x++) {
        for (int y = 0; y < MAP_HEIGHT; y++) {
            if (maptiles[x][y]) {
                maptiles[x][y]->act();
                
                // Replace destroyed boxes with ground tiles
                if (maptiles[x][y]->delete_me) {
                    SDL_Log("Map::act() replacing tile at (%d,%d) with ground (was type %d)", 
                            x, y, maptiles[x][y]->get_tile_type());
                    delete maptiles[x][y];
                    maptiles[x][y] = MapTile::create(MapTile::GROUND, x*40, y*40, app);
                }
            }
        }
    }
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
