#ifndef BOMBER_H
#define BOMBER_H

#include "GameObject.h"
#include "Controller.h"
#include <string>
#include <algorithm>

class Bomber : public GameObject {
public:
    typedef enum {
        RED = 0,
        BLUE = 1,
        YELLOW = 2,
        GREEN = 3,
        CYAN = 4,
        ORANGE = 5,
        PURPLE = 6,
        BROWN = 7,
    } COLOR;

    Bomber(int _x, int _y, COLOR _color, Controller* _controller, ClanBomberApplication *_app);
    
    void act(float deltaTime) override;

    COLOR get_color() const { return color; }
    ObjectType get_type() const override { return BOMBER; }

    // Team management
    void set_team(int team) { bomber_team = team; }
    int get_team() const { return bomber_team; }
    
    // Name management
    void set_name(const std::string& name) { bomber_name = name; }
    std::string get_name() const { return bomber_name; }
    
    // Number management
    void set_number(int number) { bomber_number = number; }
    int get_number() const { return bomber_number; }

    // Controller access
    Controller* get_controller() { return controller; }
    
    // Power-up effects
    void inc_speed(int amount) { speed += amount; }
    void dec_speed(int amount) { speed = std::max(30, speed - amount); }
    int get_power() const { return power; }
    void inc_power(int amount) { power += amount; }
    
public:
    bool can_kick;

protected:
    float anim_count;
    COLOR color;
    Controller* controller;
    float bomb_cooldown;
    int power;
    int bomber_team;
    int bomber_number;
    std::string bomber_name;
};

#endif
