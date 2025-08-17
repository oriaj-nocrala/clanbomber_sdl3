#ifndef BOMB_H
#define BOMB_H

#include "GameObject.h"

class Bomber;
class GameContext;

class Bomb : public GameObject {
public:
    Bomb(int _x, int _y, int _power, Bomber* _owner, GameContext* context);

    ~Bomb();

    void act(float deltaTime) override;
    void explode();
    void explode_delayed();
    void kick(Direction dir);
    void stop();

    ObjectType get_type() const override { return BOMB; }

private:
    float countdown;
    int power;
    Bomber* owner;
    float anim_timer;
    int base_sprite;
};

#endif