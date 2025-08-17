#ifndef BOMBER_H
#define BOMBER_H

#include "GameObject.h"
#include "Controller.h"
#include "BomberComponents.h"
#include <string>
#include <memory>

// Forward declarations
class BomberMovementComponent;
class BomberCombatComponent;
class BomberAnimationComponent;
class BomberLifecycleComponent;

/**
 * Bomber: Modern component-based bomber entity
 * 
 * ARCHITECTURE: Composition over Inheritance
 * - Uses specialized components for different responsibilities
 * - Integrates with GameContext for dependency injection
 * - Follows modern C++17 patterns
 */
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

    Bomber(int _x, int _y, COLOR _color, Controller* _controller, class GameContext* context);
    ~Bomber();
    
    void act(float deltaTime) override;
    void show() override; // Override to handle invincibility flickering

    COLOR get_color() const { return color; }
    ObjectType get_type() const override { return BOMBER; }
    
    // === COMPONENT-BASED API ===
    
    // Death system (delegated to combat component)
    void die();
    bool is_dead() const;
    
    // Lives system (delegated to lifecycle component)
    void set_lives(int lives);
    int get_lives() const;
    void lose_life();
    bool has_lives() const;
    
    // Respawn system (delegated to lifecycle component)
    void respawn(); 
    bool is_respawning() const;
    void set_invincible(bool inv);
    bool is_invincible() const;

    // Team management (delegated to lifecycle component)
    void set_team(int team);
    int get_team() const;
    
    // Name management (delegated to lifecycle component)
    void set_name(const std::string& name);
    std::string get_name() const;
    
    // Number management (delegated to lifecycle component)
    void set_number(int number);
    int get_number() const;

    // Controller access
    Controller* get_controller() { return controller; }
    
    // Animation (delegated to movement component)
    void fly_to(int target_x, int target_y, float duration_ms);
    bool can_move() const;
    // bool is_flying() const;  // Use modern system only (don't override since not virtual)
    
    // Bomb mechanics (delegated to combat component)
    void place_bomb();
    void throw_bomb();
    bool can_place_bomb() const;
    
    // Power-up effects (delegated to movement/combat components)
    void inc_speed(int amount);
    void dec_speed(int amount);
    int get_power() const;
    void inc_power(int amount);
    
    // Bomb management (delegated to combat component)
    int get_max_bombs() const;
    void inc_max_bombs(int amount);
    int get_current_bombs() const;
    void inc_current_bombs();
    void dec_current_bombs();
    
    // Special abilities (delegated to combat component)
    bool can_kick() const;
    bool can_throw() const;
    void set_can_kick(bool kick);
    void set_can_throw(bool throw_ability);
    
    // === COMPONENT ACCESS (for inter-component communication) ===
    std::unique_ptr<BomberAnimationComponent> animation_component;
    
private:
    // === COMPONENT-BASED ARCHITECTURE ===
    std::unique_ptr<BomberMovementComponent> movement_component;
    std::unique_ptr<BomberCombatComponent> combat_component;
    std::unique_ptr<BomberLifecycleComponent> lifecycle_component;
    
    // Core properties
    COLOR color;
    Controller* controller;
};

#endif
