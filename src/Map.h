#ifndef MAP_H
#define MAP_H

#include <vector>
#include <string>
#include "UtilsCL_Vector.h"

#define MAP_WIDTH 20
#define MAP_HEIGHT 15

class MapTile;
class MapEntry;
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
    
    // === GRID MANAGEMENT (Pure Grid Manager) ===
    MapTile* get_tile(int tx, int ty);
    void set_tile(int tx, int ty, MapTile* tile);  // Para TileManager
    CL_Vector get_bomber_pos(int nr);
    
    bool any_valid_map();
    int get_map_count();
    std::string get_name();
    std::string get_author();

private:
    ClanBomberApplication* app;
    MapTile* maptiles[MAP_WIDTH][MAP_HEIGHT];
    std::vector<MapEntry*> map_list;
    MapEntry* current_map;
    int current_map_index;
    
    void enumerate_maps();
    void clear();
    void reload();
};

#endif