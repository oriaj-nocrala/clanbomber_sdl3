#ifndef BOMB_H
#define BOMB_H

#include "GameObject.h"

class Bomber;

class Bomb : public GameObject {
public:
    Bomb(int _x, int _y, int _power, Bomber* _owner, ClanBomberApplication* app);

    ~Bomb();

    void act(float deltaTime) override;
    void explode();
    void explode_delayed();
    void kick(Direction dir);

    ObjectType get_type() const override { return BOMB; }

private:
    float countdown;
    int power;
    Bomber* owner;
};

#endif