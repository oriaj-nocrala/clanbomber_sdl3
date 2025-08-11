#include "MapTile_Box.h"
#include "ClanBomber.h"
#include "Resources.h"
#include "Timer.h"

MapTile_Box::MapTile_Box(int _x, int _y, ClanBomberApplication* _app) : MapTile(_x, _y, _app) {
    texture_name = "maptiles";
    sprite_nr = 1; // Box tile
    destroyed = false;
    destroy_animation = 0.0f;
    blocking = true;
    destructible = true;
}

MapTile_Box::~MapTile_Box() {
}

void MapTile_Box::act() {
    if (destroyed) {
        destroy_animation += Timer::time_elapsed();
        if (destroy_animation > 0.5f) {
            delete_me = true;
        }
    }
}

void MapTile_Box::show() {
    if (!destroyed) {
        MapTile::show();
    }
    // When destroyed, don't render anything (becomes passable ground)
}

void MapTile_Box::destroy() {
    if (!destroyed) {
        destroyed = true;
        blocking = false;
        destroy_animation = 0.0f;
        
        // TODO: Create explosion effect
        // TODO: Maybe drop a random extra item
    }
}