#ifndef MAPTILE_PURE_H
#define MAPTILE_PURE_H

class Bomb; // Forward declaration
class Bomber; // Forward declaration

/**
 * MapTile_Pure: Datos puros de tile sin herencia GameObject
 * 
 * FILOSOFIA: Un tile es información del mapa, NO un objeto de juego
 * - Sin delete_me, sin act(), sin show(), sin herencia
 * - Solo propiedades y comportamiento de tile
 * - Composición en lugar de herencia para máxima claridad
 */
class MapTile_Pure {
public:
    enum TILE_TYPE {
        NONE,
        GROUND, 
        WALL,
        BOX,
        ICE,
        ARROW,
        TRAP
    };

    // === CONSTRUCTOR PURO ===
    MapTile_Pure(TILE_TYPE type, int grid_x, int grid_y);
    virtual ~MapTile_Pure();

    // === FACTORY METHOD ===
    static MapTile_Pure* create(TILE_TYPE type, int grid_x, int grid_y);

    // === PROPIEDADES DE TILE (PURAS) ===
    TILE_TYPE get_type() const { return type; }
    bool is_blocking() const { return blocking; }
    bool is_destructible() const { return destructible; }
    bool is_burnable() const { return destructible; }
    
    // === POSICIÓN EN GRID ===
    int get_grid_x() const { return grid_x; }
    int get_grid_y() const { return grid_y; }
    int get_pixel_x() const { return grid_x * 40; }
    int get_pixel_y() const { return grid_y * 40; }

    // === OBJETOS EN ESTE TILE ===
    void set_bomb(Bomb* bomb) { this->bomb = bomb; }
    Bomb* get_bomb() const { return bomb; }
    bool has_bomb() const { return bomb != nullptr; }
    
    void set_bomber(Bomber* bomber) { this->bomber = bomber; }
    Bomber* get_bomber() const { return bomber; }
    bool has_bomber() const { return bomber != nullptr; }

    // === SPRITE INFO ===
    int get_sprite_number() const { return sprite_nr; }
    void set_sprite_number(int sprite) { sprite_nr = sprite; }

    // === COMPORTAMIENTO ESPECÍFICO POR TIPO ===
    virtual bool can_be_destroyed() const { return destructible; }
    virtual void on_destruction_request() {} // Override en subclases
    
    // Compatibility method para código legacy
    int get_tile_type() const { 
        switch (type) {
            case WALL: return 2;
            case BOX: return 3;
            case GROUND: 
            default: return 1;
        }
    }

protected:
    // === DATOS PUROS DEL TILE ===
    TILE_TYPE type;
    int grid_x, grid_y;  // Posición en grid del mapa
    bool blocking;
    bool destructible;
    int sprite_nr;

    // === OBJETOS PRESENTES ===
    Bomb* bomb;
    Bomber* bomber;
};

// === TIPOS ESPECÍFICOS DE TILE ===

class MapTile_Ground_Pure : public MapTile_Pure {
public:
    MapTile_Ground_Pure(int grid_x, int grid_y);
};

class MapTile_Wall_Pure : public MapTile_Pure {
public:
    MapTile_Wall_Pure(int grid_x, int grid_y);
};

class MapTile_Box_Pure : public MapTile_Pure {
public:
    MapTile_Box_Pure(int grid_x, int grid_y);
    bool can_be_destroyed() const override { return true; }
    void on_destruction_request() override;
};

#endif