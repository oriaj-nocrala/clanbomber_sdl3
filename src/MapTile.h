#ifndef MAPTILE_H
#define MAPTILE_H

#include "GameObject.h"

class Bomb; // Forward declaration
class Bomber; // Forward declaration

class MapTile : public GameObject {
public:
    enum MAPTILE_TYPE {
        NONE,
        GROUND, 
        WALL,
        BOX,
        ICE,
        ARROW,
        TRAP
    };

    MapTile(int x, int y, ClanBomberApplication* app);
    virtual ~MapTile();

    // Implement GameObject's get_type
    ObjectType get_type() const override { return MAPTILE; }

    // Static factory method
    static MapTile* create(MAPTILE_TYPE type, int x, int y, ClanBomberApplication* app);

    virtual void act();
    virtual void show() override;
    virtual void destroy();

    virtual bool is_blocking() const { return blocking; }
    virtual bool is_destructible() const { return destructible; }
    virtual bool is_burnable() const { return destructible; }
    virtual bool has_bomb() const { return bomb != nullptr; }
    virtual bool has_bomber() const { return bomber != nullptr; }
    
    // Compatibility method for legacy code
    virtual int get_tile_type() const { 
        if (blocking && !destructible) return WALL;
        if (destructible) return BOX;
        return GROUND;
    }

    void set_bomb(Bomb* _bomb) { bomb = _bomb; }
    Bomb* get_bomb() { return bomb; }
    
    void set_bomber(Bomber* _bomber) { bomber = _bomber; }
    Bomber* get_bomber() { return bomber; }

public:
    Bomb* bomb;
    Bomber* bomber;

protected:
    bool blocking;
    bool destructible;
};

#endif
