#ifndef MAPTILE_WALL_H
#define MAPTILE_WALL_H

#include "MapTile.h"

class MapTile_Wall : public MapTile {
public:
    MapTile_Wall(int x, int y, ClanBomberApplication* app);

    TILE_TYPE get_tile_type() const override { return WALL; }
    bool is_blocking() const override { return true; }
};

#endif