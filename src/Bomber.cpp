#include "Bomber.h"
#include "BomberComponents.h"
#include "Controller.h"
#include "ClanBomber.h"
#include "GameContext.h"

// ===== CONSTRUCTOR / DESTRUCTOR =====

Bomber::Bomber(int _x, int _y, COLOR _color, Controller* _controller, GameContext& context) 
    : GameObject(_x, _y, &context), color(_color), controller(_controller) {
    
    // Attach controller
    if (controller) {
        controller->attach(this);
    }
    
    // Initialize components using modern C++17 patterns with GameContext dependency injection
    movement_component = std::make_unique<BomberMovementComponent>(this, &context);
    combat_component = std::make_unique<BomberCombatComponent>(this, &context);
    animation_component = std::make_unique<BomberAnimationComponent>(this, &context);
    lifecycle_component = std::make_unique<BomberLifecycleComponent>(this, &context);
    
    // Set initial properties
    cur_dir = DIR_RIGHT;
    
    // Configure animation component with color
    animation_component->set_texture_from_color(static_cast<int>(color));
    
    SDL_Log("Bomber: Created modern component-based bomber at (%d,%d) with color %d", _x, _y, static_cast<int>(color));
}

Bomber::~Bomber() {
    // Components are automatically cleaned up via unique_ptr
    SDL_Log("Bomber: Destroyed bomber with modern component cleanup");
}

// ===== CORE GAME LOOP =====

void Bomber::act(float deltaTime) {
    // MODERN ARCHITECTURE: Delegate to specialized components
    // Each component handles its own responsibility
    
    // Update controller first
    if (controller) {
        controller->update();
    }
    
    // Update all components in proper order
    if (lifecycle_component) {
        lifecycle_component->update(deltaTime);
    }
    
    if (movement_component) {
        movement_component->update(deltaTime);
        movement_component->handle_controller_input(controller, deltaTime);
    }
    
    if (combat_component) {
        combat_component->update(deltaTime);
        combat_component->handle_controller_input(controller, deltaTime);
    }
    
    if (animation_component) {
        animation_component->update(deltaTime);
    }
}

void Bomber::show() {
    // Delegate rendering to animation component
    // Handle invincibility flickering
    if (animation_component && animation_component->is_invincible()) {
        // Flicker effect during invincibility
        static float flicker_timer = 0.0f;
        flicker_timer += 0.1f;
        if (static_cast<int>(flicker_timer * 10) % 2 == 0) {
            GameObject::show(); // Only show every other frame
        }
    } else {
        GameObject::show(); // Normal rendering
    }
}

// ===== COMPONENT DELEGATION API =====

// Death system (delegated to combat component)
void Bomber::die() {
    if (combat_component) combat_component->die();
}

bool Bomber::is_dead() const {
    return combat_component ? combat_component->is_dead() : false;
}

// Lives system (delegated to lifecycle component)
void Bomber::set_lives(int lives) {
    if (lifecycle_component) lifecycle_component->set_lives(lives);
}

int Bomber::get_lives() const {
    return lifecycle_component ? lifecycle_component->get_lives() : 0;
}

void Bomber::lose_life() {
    if (lifecycle_component) lifecycle_component->lose_life();
}

bool Bomber::has_lives() const {
    return lifecycle_component ? lifecycle_component->has_lives() : false;
}

// Respawn system (delegated to lifecycle component)
void Bomber::respawn() {
    if (lifecycle_component) lifecycle_component->respawn();
}

bool Bomber::is_respawning() const {
    return lifecycle_component ? lifecycle_component->is_respawning() : false;
}

void Bomber::set_invincible(bool inv) {
    if (animation_component) animation_component->set_invincible(inv);
}

bool Bomber::is_invincible() const {
    return animation_component ? animation_component->is_invincible() : false;
}

// Team management (delegated to lifecycle component)
void Bomber::set_team(int team) {
    if (lifecycle_component) lifecycle_component->set_team(team);
}

int Bomber::get_team() const {
    return lifecycle_component ? lifecycle_component->get_team() : 0;
}

// Name management (delegated to lifecycle component)
void Bomber::set_name(const std::string& name) {
    if (lifecycle_component) lifecycle_component->set_name(name);
}

std::string Bomber::get_name() const {
    return lifecycle_component ? lifecycle_component->get_name() : "Unknown";
}

// Number management (delegated to lifecycle component)
void Bomber::set_number(int number) {
    if (lifecycle_component) lifecycle_component->set_number(number);
}

int Bomber::get_number() const {
    return lifecycle_component ? lifecycle_component->get_number() : 0;
}

// Animation (delegated to movement component)
void Bomber::fly_to(int target_x, int target_y, float duration_ms) {
    // MODERN ONLY: Use BomberMovementComponent, block GameObject legacy system
    if (movement_component) {
        movement_component->fly_to(target_x, target_y, duration_ms);
    }
    
    // CRITICAL: Block GameObject::flying state to prevent interference
    flying = false;  // Ensure GameObject doesn't think it's flying
}

bool Bomber::can_move() const {
    return movement_component ? movement_component->can_move() : false;
}

// Bomb mechanics (delegated to combat component)
void Bomber::place_bomb() {
    if (combat_component) combat_component->place_bomb();
}

void Bomber::throw_bomb() {
    if (combat_component) combat_component->throw_bomb();
}

bool Bomber::can_place_bomb() const {
    return combat_component ? combat_component->can_place_bomb() : false;
}

// Power-up effects (delegated to movement/combat components)
void Bomber::inc_speed(int amount) {
    if (movement_component) {
        int current_speed = movement_component->get_speed();
        movement_component->set_speed(current_speed + amount);
    }
}

void Bomber::dec_speed(int amount) {
    if (movement_component) {
        int current_speed = movement_component->get_speed();
        movement_component->set_speed(std::max(30, current_speed - amount));
    }
}

int Bomber::get_power() const {
    return combat_component ? combat_component->get_power() : 1;
}

void Bomber::inc_power(int amount) {
    if (combat_component) combat_component->inc_power(amount);
}

// Bomb management (delegated to combat component)
int Bomber::get_max_bombs() const {
    return combat_component ? combat_component->get_max_bombs() : 1;
}

void Bomber::inc_max_bombs(int amount) {
    if (combat_component) combat_component->inc_max_bombs(amount);
}

int Bomber::get_current_bombs() const {
    return combat_component ? combat_component->get_current_bombs() : 0;
}

void Bomber::inc_current_bombs() {
    if (combat_component) combat_component->inc_current_bombs();
}

void Bomber::dec_current_bombs() {
    if (combat_component) combat_component->dec_current_bombs();
}

// Special abilities (delegated to combat component)
bool Bomber::can_kick() const {
    return combat_component ? combat_component->can_kick : false;
}

bool Bomber::can_throw() const {
    return combat_component ? combat_component->can_throw : false;
}

void Bomber::set_can_kick(bool kick) {
    if (combat_component) combat_component->can_kick = kick;
}

void Bomber::set_can_throw(bool throw_ability) {
    if (combat_component) combat_component->can_throw = throw_ability;
}