#ifndef TILEMANAGER_H
#define TILEMANAGER_H

#include <functional>
#include <vector>
#include "UtilsCL_Vector.h"

class MapTile;
class GameObject;
class Bomb;
class Bomber;
class ClanBomberApplication;
class LifecycleManager;

/**
 * TileManager: Coordinador Inteligente entre Map, MapTiles y LifecycleManager
 * 
 * RESPONSABILIDADES:
 * - Coordinar destrucción y reemplazo de tiles
 * - Gestionar objetos en posiciones del mapa
 * - Interfaz limpia entre sistemas
 * - ELIMINAR coordinación cruzada entre Map/MapTile/LifecycleManager
 */
class TileManager {
public:
    TileManager(ClanBomberApplication* app);
    ~TileManager();

    // === COORDINACIÓN PRINCIPAL ===
    void update_tiles(float deltaTime);
    void handle_tile_updates();
    
    // === GESTIÓN DE TILES ===
    void request_tile_destruction(int map_x, int map_y);
    void replace_tile_when_ready(int map_x, int map_y, int new_tile_type);
    bool is_tile_ready_for_replacement(MapTile* tile);
    
    // === QUERY INTELIGENTE ===
    bool is_position_walkable(int map_x, int map_y);
    bool is_position_blocked(int map_x, int map_y);
    MapTile* get_tile_at(int map_x, int map_y);
    
    // === GESTIÓN DE OBJETOS EN TILES ===
    void register_bomb_at(int map_x, int map_y, Bomb* bomb);
    void unregister_bomb_at(int map_x, int map_y);
    Bomb* get_bomb_at(int map_x, int map_y);
    
    void register_bomber_at(int map_x, int map_y, Bomber* bomber);
    void unregister_bomber_at(int map_x, int map_y);
    Bomber* get_bomber_at(int map_x, int map_y);
    bool has_bomber_at(int map_x, int map_y);
    
    // === COORDINACIÓN CON LIFECYCLE ===
    void coordinate_with_lifecycle_manager();
    void process_dying_tiles();
    void process_dead_tiles();
    
    // === UTILITIES ===
    void iterate_all_tiles(std::function<void(MapTile*, int, int)> callback);
    std::vector<MapTile*> get_destructible_tiles_in_radius(int center_x, int center_y, int radius);

private:
    ClanBomberApplication* app;
    
    // === COORDINACIÓN INTERNA ===
    void update_single_tile(MapTile* tile, int map_x, int map_y);
    void handle_tile_destruction_request(int map_x, int map_y);
    void perform_tile_replacement(int map_x, int map_y, int new_tile_type);
    
    // === VALIDACIÓN ===
    bool is_valid_position(int map_x, int map_y) const;
    
    // === NO COORDINATION CROSSING! ===
    // TileManager es el ÚNICO que habla con Map, MapTiles y LifecycleManager
    // Nadie más debe coordinar entre estos sistemas
};

#endif