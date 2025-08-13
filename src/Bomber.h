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
    void show() override; // Override to handle invincibility flickering

    COLOR get_color() const { return color; }
    void die(); // Kill the bomber
    bool is_dead() const { return dead; }
    ObjectType get_type() const override { return BOMBER; }
    
    // Lives system
    void set_lives(int lives) { remaining_lives = lives; }
    int get_lives() const { return remaining_lives; }
    void lose_life() { if (remaining_lives > 0) remaining_lives--; }
    bool has_lives() const { return remaining_lives > 0; }
    
    // Respawn system
    void respawn(); 
    bool is_respawning() const { return respawning; }
    void set_invincible(bool inv) { invincible = inv; }
    bool is_invincible() const { return invincible; }

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
    
    // Animation
    void fly_to(int target_x, int target_y, float duration_ms);
    
    // Power-up effects
    void inc_speed(int amount) { speed += amount; }
    void dec_speed(int amount) { speed = std::max(30, speed - amount); }
    int get_power() const { return power; }
    void inc_power(int amount) { power += amount; }
    
public:
    bool can_kick;
    bool dead;
    
    // Lives and respawn
    int remaining_lives;
    bool respawning;
    bool invincible;
    float respawn_timer;
    float invincible_timer;
    
    // Flight animation
    bool flying;
    float flight_timer;
    float flight_duration;
    int start_x, start_y;
    int target_x, target_y;

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
