#include "Map.h"
#include "MapTile_Wall.h"
#include "MapTile_Ground.h"
#include "ClanBomber.h"
#include <algorithm>
#include <random>
#include <SDL3/SDL.h>

Map::Map(ClanBomberApplication* _app) {
    app = _app;
    width = 20; // 800 / 40
    height = 15; // 600 / 40
    current_map_index = 0;
    total_maps = 1; // For now, just one default map
    init_default_bomber_positions();
}

Map::~Map() {
    for (int i = 0; i < width; ++i) {
        for (int j = 0; j < height; ++j) {
            delete tiles[i][j];
        }
    }
}

void Map::load() {
    tiles.resize(width);
    for (int i = 0; i < width; ++i) {
        tiles[i].resize(height);
    }

    for (int i = 0; i < width; ++i) {
        for (int j = 0; j < height; ++j) {
            if (i == 0 || j == 0 || i == width - 1 || j == height - 1) {
                tiles[i][j] = new MapTile_Wall(i * 40, j * 40, app);
            } else {
                tiles[i][j] = new MapTile_Ground(i * 40, j * 40, app);
            }
        }
    }
}

void Map::show() {
    for (int i = 0; i < width; ++i) {
        for (int j = 0; j < height; ++j) {
            tiles[i][j]->show();
        }
    }
}

MapTile* Map::get_tile(int tx, int ty) {
    if (tx >= 0 && tx < width && ty >= 0 && ty < height) {
        return tiles[tx][ty];
    }
    return nullptr;
}

void Map::load_random_valid() {
    load(); // For now, just load the default map
}

void Map::load_next_valid(int map_nr) {
    if (map_nr >= 0) {
        current_map_index = map_nr;
    }
    load(); // For now, just load the default map
}

void Map::act() {
    // No-op for now
}

void Map::refresh_holes() {
    // No-op for now - would handle animated map elements
}

bool Map::any_valid_map() {
    return total_maps > 0;
}

int Map::get_map_count() {
    return total_maps;
}

std::string Map::get_name() {
    return "Default Map";
}

std::string Map::get_author() {
    return "ClanBomber SDL3";
}

CL_Vector Map::get_bomber_pos(int nr) {
    if (nr >= 0 && nr < (int)bomber_positions.size()) {
        return bomber_positions[nr];
    }
    // Return default position if out of bounds
    return CL_Vector(2, 2);
}

void Map::randomize_bomber_positions() {
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(bomber_positions.begin(), bomber_positions.end(), g);
}

void Map::init_default_bomber_positions() {
    bomber_positions.clear();
    
    // Standard bomber start positions (in tile coordinates)
    bomber_positions.push_back(CL_Vector(2, 2));   // Top-left
    bomber_positions.push_back(CL_Vector(17, 2));  // Top-right  
    bomber_positions.push_back(CL_Vector(2, 12));  // Bottom-left
    bomber_positions.push_back(CL_Vector(17, 12)); // Bottom-right
    bomber_positions.push_back(CL_Vector(9, 2));   // Top-center
    bomber_positions.push_back(CL_Vector(9, 12));  // Bottom-center
    bomber_positions.push_back(CL_Vector(2, 7));   // Left-center
    bomber_positions.push_back(CL_Vector(17, 7));  // Right-center
}
