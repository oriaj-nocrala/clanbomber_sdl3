#include "MapTile.h"
#include "MapTile_Ground.h"
#include "MapTile_Wall.h"
#include "MapTile_Box.h"
#include "ClanBomber.h"

MapTile::MapTile(int x, int y, ClanBomberApplication* app) : GameObject(x, y, app) {
    blocking = false;
    destructible = false;
    bomb = nullptr;
    bomber = nullptr;
}

MapTile::~MapTile() {
}

MapTile* MapTile::create(MAPTILE_TYPE type, int x, int y, ClanBomberApplication* app) {
    switch (type) {
        case GROUND:
            return new MapTile_Ground(x, y, app);
        case WALL:
            return new MapTile_Wall(x, y, app);
        case BOX:
            return new MapTile_Box(x, y, app);
        case NONE:
        default:
            return new MapTile_Ground(x, y, app); // Default to ground
    }
}

void MapTile::act() {
    // Base implementation - do nothing
}

void MapTile::show() {
    GameObject::show();
}

void MapTile::destroy() {
    // Base implementation - do nothing
}