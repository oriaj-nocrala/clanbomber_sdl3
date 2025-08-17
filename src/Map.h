#ifndef MAP_H
#define MAP_H

#include <vector>
#include <string>
#include "UtilsCL_Vector.h"

#define MAP_WIDTH 20
#define MAP_HEIGHT 15

class MapTile;
class TileEntity;
class MapTile_Pure;
class MapEntry;
class GameContext;
class Bomber;

class Map {
public:
    Map(GameContext* context);
    ~Map();

    void load();
    void load_random_valid();
    void load_next_valid(int map_nr = -1);
    void show();
    void act();
    void refresh_holes();
    void randomize_bomber_positions();
    
    // === GRID MANAGEMENT (Pure Grid Manager) ===
    MapTile* get_tile(int tx, int ty);  // Legacy compatibility
    TileEntity* get_tile_entity(int tx, int ty);  // NEW: TileEntity access
    void set_tile(int tx, int ty, MapTile* tile);  // Legacy compatibility
    void set_tile_entity(int tx, int ty, TileEntity* tile_entity);  // NEW: TileEntity support
    void clear_tile_entity_at(int tx, int ty);  // NEW: Clear TileEntity pointer (for use-after-free fix)
    CL_Vector get_bomber_pos(int nr);
    
    bool any_valid_map();
    int get_map_count();
    std::string get_name();
    std::string get_author();

private:
    GameContext* context;
    
    // NUEVO: Dual storage para transición
    MapTile* maptiles[MAP_WIDTH][MAP_HEIGHT];  // Legacy storage
    TileEntity* tile_entities[MAP_WIDTH][MAP_HEIGHT];  // NEW: TileEntity storage
    std::vector<MapEntry*> map_list;
    MapEntry* current_map;
    int current_map_index;
    
    void enumerate_maps();
    void clear();
    void reload();
};

#endif