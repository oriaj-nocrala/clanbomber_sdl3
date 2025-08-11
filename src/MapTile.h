#ifndef MAPTILE_H
#define MAPTILE_H

#include "GameObject.h"

class Bomb; // Forward declaration

class MapTile : public GameObject {
public:
    enum TILE_TYPE {
        GROUND,
        WALL,
        BOX
    };

    MapTile(int x, int y, ClanBomberApplication* app) : GameObject(x, y, app) {
        bomb = nullptr;
    }

    // Implement GameObject's get_type
    ObjectType get_type() const override { return MAPTILE; }

    // Add a new method for the specific tile type
    virtual TILE_TYPE get_tile_type() const = 0;

    virtual bool is_blocking() const { return false; }
    virtual bool is_burnable() const { return false; }
    virtual bool has_bomber() const { return false; }

    Bomb* bomb;
};

#endif
