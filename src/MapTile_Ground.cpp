#include "MapTile_Ground.h"
#include "Resources.h"

MapTile_Ground::MapTile_Ground(int x, int y, GameContext* context) : MapTile(x, y, context) {
    texture_name = "maptiles";
    sprite_nr = 0; // Assuming 0 is the ground sprite in maptiles.png
    blocking = false;
    destructible = false;
}

MapTile_Ground::~MapTile_Ground() {
}
