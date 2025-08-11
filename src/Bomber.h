#ifndef BOMBER_H
#define BOMBER_H

#include "GameObject.h"
#include "Controller.h"

class Bomber : public GameObject {
public:
    typedef enum {
        RED = 0,
        BLUE = 1,
        YELLOW = 2,
        GREEN = 3,
        RED2 = 4,
        BLUE2 = 5,
        YELLOW2 = 6,
        GREEN2 = 7,
    } COLOR;

    Bomber(int _x, int _y, COLOR _color, Controller* _controller, ClanBomberApplication *_app);
    
    void act(float deltaTime) override;

    ObjectType get_type() const override { return BOMBER; }

protected:
    float anim_count;
    COLOR color;
    Controller* controller;
    float bomb_cooldown;
    int power;
};

#endif
