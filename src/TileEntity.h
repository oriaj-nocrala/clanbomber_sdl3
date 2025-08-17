#ifndef TILEENTITY_H
#define TILEENTITY_H

#include "GameObject.h"
#include "MapTile_Pure.h"

/**
 * TileEntity: GameObject que representa un tile en el mundo
 * 
 * ARQUITECTURA LIMPIA:
 * - TileEntity hereda de GameObject (para LifecycleManager, rendering, etc.)
 * - Contiene MapTile_Pure* para datos del tile (composición > herencia)
 * - Separación clara: GameObject comportamiento vs Tile datos
 */
class TileEntity : public GameObject {
public:
    TileEntity(MapTile_Pure* tile_data, ClanBomberApplication* app);
    virtual ~TileEntity();

    // === IMPLEMENT GAMEOBJECT ===
    ObjectType get_type() const override { return MAPTILE; }
    void act(float deltaTime) override;
    void show() override;

    // === TILE DATA ACCESS ===
    MapTile_Pure* get_tile_data() const { return tile_data; }
    MapTile_Pure::TILE_TYPE get_tile_type() const { return tile_data ? tile_data->get_type() : MapTile_Pure::NONE; }

    // === TILE PROPERTIES (DELEGATED) ===
    bool is_blocking() const { 
        if (!tile_data || tile_data == (MapTile_Pure*)0xffffffffffffffff) {
            SDL_Log("ERROR: TileEntity::is_blocking() - corrupted tile_data pointer: %p", tile_data);
            return false;
        }
        return tile_data->is_blocking(); 
    }
    bool is_destructible() const { 
        if (!tile_data || tile_data == (MapTile_Pure*)0xffffffffffffffff) {
            SDL_Log("ERROR: TileEntity::is_destructible() - corrupted tile_data pointer: %p (TileEntity at %p)", tile_data, this);
            return false;
        }
        return tile_data->is_destructible(); 
    }
    bool is_burnable() const { 
        if (!tile_data || tile_data == (MapTile_Pure*)0xffffffffffffffff) {
            SDL_Log("ERROR: TileEntity::is_burnable() - corrupted tile_data pointer: %p", tile_data);
            return false;
        }
        return tile_data->is_burnable(); 
    }
    bool has_bomb() const { return tile_data ? tile_data->has_bomb() : false; }
    bool has_bomber() const { return tile_data ? tile_data->has_bomber() : false; }

    // === OBJECT MANAGEMENT (DELEGATED) ===
    void set_bomb(Bomb* bomb) { if (tile_data) tile_data->set_bomb(bomb); }
    Bomb* get_bomb() const { return tile_data ? tile_data->get_bomb() : nullptr; }
    void set_bomber(Bomber* bomber) { if (tile_data) tile_data->set_bomber(bomber); }
    Bomber* get_bomber() const { return tile_data ? tile_data->get_bomber() : nullptr; }

    // === DESTRUCTION SYSTEM ===
    virtual void destroy();
    void spawn_extra();

    // === GRID POSITION (SHADOWS BASE CLASS METHODS) ===
    int get_map_x() const { return tile_data ? tile_data->get_grid_x() : 0; }
    int get_map_y() const { return tile_data ? tile_data->get_grid_y() : 0; }
    
    // === DESTRUCTION STATE ===
    bool is_destroyed() const { return destroyed; }

protected:
    MapTile_Pure* tile_data;  // Composición: datos puros del tile

    // === DESTRUCTION ANIMATION ===
    bool destroyed;
    float destroy_animation;
    
    // === PARTICLE RATE LIMITING ===
    static float last_particle_emission_time;  // Shared rate limiter for all tiles
    static const float PARTICLE_EMISSION_COOLDOWN; // Minimum time between particle emissions
    
    void update_destruction_animation(float deltaTime);
    void render_destruction_effects();
};

// === TILE ENTITIES ESPECÍFICOS ===

class TileEntity_Box : public TileEntity {
public:
    TileEntity_Box(MapTile_Pure* tile_data, ClanBomberApplication* app);
    
    void act(float deltaTime) override;
    void show() override;
    void destroy() override;

private:
    void render_fragmentation_effects();
};

#endif