#ifndef MAPTILE_BOX_H
#define MAPTILE_BOX_H

#include "MapTile.h"

class MapTile_Box : public MapTile {
public:
    MapTile_Box(int _x, int _y, ClanBomberApplication* _app);
    ~MapTile_Box();

    void act() override;
    void show() override;
    void destroy() override;

private:
    bool destroyed;
    float destroy_animation;
};

#endif