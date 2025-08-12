#ifndef EXPLOSION_H
#define EXPLOSION_H

#include "GameObject.h"

class Bomber;

class Explosion : public GameObject {
public:
    enum {
        EXPLODE_LEFT    = 0,
        EXPLODE_H       = 1,
        EXPLODE_RIGHT   = 2,
        EXPLODE_UP      = 3,
        EXPLODE_V       = 4,
        EXPLODE_DOWN    = 5,
        EXPLODE_X       = 6, // Center
    };

    Explosion(int _x, int _y, int _power, Bomber* _owner, ClanBomberApplication* app);

    void act(float deltaTime) override;
    void show() override;

    ObjectType get_type() const override { return EXPLOSION; }

private:
    void draw_part(int x, int y, int sprite_nr);
    void detonate_other_bombs();
    void kill_bombers();
    void explode_corpses();

    Bomber* owner;
    int power;
    float detonation_period;

    int length_up, length_down, length_left, length_right;
};

#endif
