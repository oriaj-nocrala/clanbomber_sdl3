#ifndef MAP_H
#define MAP_H

#include <vector>

#define MAP_WIDTH 20
#define MAP_HEIGHT 15

class MapTile;
class ClanBomberApplication;

class Map {
public:
    Map(ClanBomberApplication* app);
    ~Map();

    void load();
    void show();
    MapTile* get_tile(int tx, int ty);

private:
    ClanBomberApplication* app;
    std::vector<std::vector<MapTile*>> tiles;
    int width;
    int height;
};

#endif