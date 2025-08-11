#include "MapTile_Ground.h"
#include "Resources.h"

MapTile_Ground::MapTile_Ground(int x, int y, ClanBomberApplication* app) : MapTile(x, y, app) {
    texture_name = "maptiles";
    sprite_nr = 0; // Assuming 0 is the ground sprite in maptiles.png
}
