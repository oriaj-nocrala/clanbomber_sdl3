#ifndef MAP_H
#define MAP_H

#include <vector>
#include <string>
#include "UtilsCL_Vector.h"

#define MAP_WIDTH 20
#define MAP_HEIGHT 15

class MapTile;
class ClanBomberApplication;
class Bomber;

class Map {
public:
    Map(ClanBomberApplication* app);
    ~Map();

    void load();
    void load_random_valid();
    void load_next_valid(int map_nr = -1);
    void show();
    void act();
    void refresh_holes();
    void randomize_bomber_positions();
    
    MapTile* get_tile(int tx, int ty);
    CL_Vector get_bomber_pos(int nr);
    
    bool any_valid_map();
    int get_map_count();
    std::string get_name();
    std::string get_author();

private:
    ClanBomberApplication* app;
    std::vector<std::vector<MapTile*>> tiles;
    std::vector<CL_Vector> bomber_positions;
    int width;
    int height;
    int current_map_index;
    int total_maps;
    
    void init_default_bomber_positions();
};

#endif