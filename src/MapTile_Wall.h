#ifndef MAPTILE_WALL_H
#define MAPTILE_WALL_H

#include "MapTile.h"

class MapTile_Wall : public MapTile {
public:
    MapTile_Wall(int x, int y, GameContext* context);
    ~MapTile_Wall();

    bool is_blocking() const override { return true; }
};

#endif