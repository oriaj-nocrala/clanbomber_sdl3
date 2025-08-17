#include "MapTile_Pure.h"
#include "Bomb.h"
#include "Bomber.h"
#include <SDL3/SDL.h>

// === BASE MAPTILE_PURE ===

MapTile_Pure::MapTile_Pure(TILE_TYPE type, int grid_x, int grid_y) 
    : type(type), grid_x(grid_x), grid_y(grid_y) {
    blocking = false;
    destructible = false;
    sprite_nr = 0;
    bomb = nullptr;
    bomber = nullptr;
    
    SDL_Log("MapTile_Pure: Created %s tile at grid (%d,%d)", 
           type == GROUND ? "GROUND" : type == WALL ? "WALL" : type == BOX ? "BOX" : "UNKNOWN",
           grid_x, grid_y);
}

MapTile_Pure::~MapTile_Pure() {
    // Pure tile - no special cleanup needed
}

MapTile_Pure* MapTile_Pure::create(TILE_TYPE type, int grid_x, int grid_y) {
    switch (type) {
        case GROUND:
            return new MapTile_Ground_Pure(grid_x, grid_y);
        case WALL:
            return new MapTile_Wall_Pure(grid_x, grid_y);
        case BOX:
            return new MapTile_Box_Pure(grid_x, grid_y);
        case NONE:
        default:
            return new MapTile_Ground_Pure(grid_x, grid_y); // Default to ground
    }
}

// === GROUND TILE ===

MapTile_Ground_Pure::MapTile_Ground_Pure(int grid_x, int grid_y) 
    : MapTile_Pure(GROUND, grid_x, grid_y) {
    blocking = false;
    destructible = false;
    sprite_nr = 0; // Ground sprite (matches legacy MapTile_Ground)
}

// === WALL TILE ===

MapTile_Wall_Pure::MapTile_Wall_Pure(int grid_x, int grid_y) 
    : MapTile_Pure(WALL, grid_x, grid_y) {
    blocking = true;
    destructible = false;
    sprite_nr = 1; // Wall sprite (matches legacy MapTile_Wall)
}

// === BOX TILE ===

MapTile_Box_Pure::MapTile_Box_Pure(int grid_x, int grid_y) 
    : MapTile_Pure(BOX, grid_x, grid_y) {
    blocking = true;
    destructible = true;
    sprite_nr = 10; // Box sprite
}

void MapTile_Box_Pure::on_destruction_request() {
    SDL_Log("MapTile_Box_Pure: Destruction requested at grid (%d,%d)", grid_x, grid_y);
    // Pure tile just logs - actual destruction handling done by TileEntity
}