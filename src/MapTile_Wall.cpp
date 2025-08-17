#include "MapTile_Wall.h"
#include "Resources.h"

MapTile_Wall::MapTile_Wall(int x, int y, GameContext* context) : MapTile(x, y, context) {
    texture_name = "maptiles";
    sprite_nr = 1; // Assuming 1 is the wall sprite in maptiles.png
    blocking = true;
    destructible = false;
}

MapTile_Wall::~MapTile_Wall() {
}
