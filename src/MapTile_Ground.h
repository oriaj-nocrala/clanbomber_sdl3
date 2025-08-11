#ifndef MAPTILE_GROUND_H
#define MAPTILE_GROUND_H

#include "MapTile.h"

class MapTile_Ground : public MapTile {
public:
    MapTile_Ground(int x, int y, ClanBomberApplication* app);

    TILE_TYPE get_tile_type() const override { return GROUND; }
};

#endif