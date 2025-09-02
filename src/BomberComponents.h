#ifndef BOMBER_COMPONENTS_H
#define BOMBER_COMPONENTS_H

#include <string>
#include "ClanBomber.h" // For Direction enum

// Forward declarations
class ClanBomberApplication;
class Controller;
class GameObject;
class GameContext;
class Bomb;

/**
 * BomberComponents: Modular component system for Bomber entities
 * 
 * ARCHITECTURE PATTERN: Composition over Inheritance
 * - Separate concerns into focused components
 * - Enable dependency injection via GameContext
 * - Make code more testable and maintainable
 * - Follow modern C++17 practices
 */

// ===== MOVEMENT COMPONENT =====

class BomberMovementComponent {
public:
    BomberMovementComponent(GameObject* owner, GameContext* context);
    
    // Core movement logic
    void update(float deltaTime);
    void handle_controller_input(Controller* controller, float deltaTime);
    
    // Movement properties
    void set_speed(int new_speed) { speed = new_speed; }
    int get_speed() const { return speed; }
    
    // Flight animation system
    void fly_to(int target_x, int target_y, float duration_ms);
    bool is_flying() const { return flying; }
    
    // Movement state
    bool can_move() const;
    
private:
    GameObject* owner;
    GameContext* context;
    
    // Movement properties
    int speed = 60; // Reduced from 90 - calibrated for deltaTime (was designed for ~60fps frame-based)
    Direction last_direction = DIR_DOWN; // Track last movement direction
    
    // Flight animation
    bool flying = false;
    float flight_timer = 0.0f;
    float flight_duration = 0.0f;
    int start_x = 0, start_y = 0;
    int target_x = 0, target_y = 0;
    
    // Internal movement logic
    void update_flight_animation(float deltaTime);
};

// ===== COMBAT COMPONENT =====

class BomberCombatComponent {
public:
    BomberCombatComponent(GameObject* owner, GameContext* context);
    
    // Core combat logic
    void update(float deltaTime);
    void handle_controller_input(Controller* controller, float deltaTime);
    
    // Bomb mechanics
    void place_bomb();
    void throw_bomb();
    bool can_place_bomb() const;
    
    // Power management
    int get_power() const { return power; }
    void set_power(int new_power) { power = new_power; }
    void inc_power(int amount) { power += amount; }
    
    // Bomb capacity
    int get_max_bombs() const { return max_bombs; }
    void set_max_bombs(int bombs) { max_bombs = bombs; }
    void inc_max_bombs(int amount) { max_bombs += amount; }
    
    int get_current_bombs() const { return current_bombs; }
    void inc_current_bombs() { current_bombs++; }
    void dec_current_bombs() { if (current_bombs > 0) current_bombs--; }
    
    // Special abilities
    bool can_kick = false;
    bool can_throw = false;
    
    // Death system
    void die();
    bool is_dead() const { return dead; }
    
    // Bomb escape system  
    bool can_ignore_bomb_collision(Bomb* bomb) const;
    
private:
    GameObject* owner;
    GameContext* context;
    
    // Combat properties
    int power = 1;
    int max_bombs = 1;
    int current_bombs = 0;
    float bomb_cooldown = 0.0f;
    
    // Bomb throwing mechanics
    float bomb_hold_timer = 0.0f;
    bool bomb_button_held = false;
    static constexpr float THROW_HOLD_TIME = 0.3f; // 300ms hold to throw
    
    // Death state
    bool dead = false;
    
    // BOMB ESCAPE SYSTEM: Allow bomber to move while on top of placed bomb
    Bomb* bomb_standing_on = nullptr;     // Reference to bomb bomber is currently on top of
    bool has_left_bomb_tile = false;      // Track if bomber has left the bomb tile
    
    // Internal combat logic
    void update_bomb_cooldown(float deltaTime);
    void update_bomb_throwing(float deltaTime);
    void update_bomb_escape_status(); // Track bomber position relative to bomb
};

// ===== ANIMATION COMPONENT =====

class BomberAnimationComponent {
public:
    BomberAnimationComponent(GameObject* owner, GameContext* context);
    
    // Core animation logic
    void update(float deltaTime);
    
    // Animation state
    void set_texture_from_color(int color);
    void update_animation_frame(float deltaTime, Direction direction);
    void set_standing_sprite(Direction direction); // Set sprite for standing still
    
    // Invincibility effects
    void set_invincible(bool inv) { invincible = inv; }
    bool is_invincible() const { return invincible; }
    void update_invincibility_effects(float deltaTime);
    
private:
    GameObject* owner;
    GameContext* context;
    
    // Animation state
    float anim_count = 0.0f;
    
    // Movement tracking for animation
    int last_x = 0;
    int last_y = 0;
    
    // Invincibility rendering
    bool invincible = false;
    float invincible_timer = 0.0f;
    
    // Internal animation logic
    void choose_texture_for_color(int color);
};

// ===== LIFECYCLE COMPONENT =====

class BomberLifecycleComponent {
public:
    BomberLifecycleComponent(GameObject* owner, GameContext* context);
    
    // Core lifecycle logic
    void update(float deltaTime);
    
    // Lives system
    void set_lives(int lives) { remaining_lives = lives; }
    int get_lives() const { return remaining_lives; }
    void lose_life() { if (remaining_lives > 0) remaining_lives--; }
    bool has_lives() const { return remaining_lives > 0; }
    
    // Respawn system
    void respawn();
    bool is_respawning() const { return respawning; }
    
    // Team and identity
    void set_team(int team) { bomber_team = team; }
    int get_team() const { return bomber_team; }
    void set_name(const std::string& name) { bomber_name = name; }
    std::string get_name() const { return bomber_name; }
    void set_number(int number) { bomber_number = number; }
    int get_number() const { return bomber_number; }
    
private:
    GameObject* owner;
    GameContext* context;
    
    // Lives and respawn
    int remaining_lives = 3;
    bool respawning = false;
    float respawn_timer = 0.0f;
    
    // Team and identity
    int bomber_team = 0;
    int bomber_number = 0;
    std::string bomber_name = "Bomber";
    
    // Internal lifecycle logic
    void update_respawn_timer(float deltaTime);
};

#endif