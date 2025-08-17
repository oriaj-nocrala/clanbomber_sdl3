#ifndef CORPSEPART_H
#define CORPSEPART_H

#include "GameObject.h"
#include <vector>
#include <cmath>

struct Vector2D {
    float x, y;
    Vector2D(float _x = 0.0f, float _y = 0.0f) : x(_x), y(_y) {}
    Vector2D operator+(const Vector2D& v) const { return Vector2D(x + v.x, y + v.y); }
    Vector2D operator-(const Vector2D& v) const { return Vector2D(x - v.x, y - v.y); }
    Vector2D operator*(float s) const { return Vector2D(x * s, y * s); }
    float magnitude() const { return std::sqrt(x*x + y*y); }
    Vector2D normalized() const { float m = magnitude(); return m > 0 ? *this * (1.0f/m) : Vector2D(); }
};

struct BloodTrail {
    Vector2D position;
    float life;
    float size;
    Uint8 alpha;
};

class CorpsePart : public GameObject {
public:
    CorpsePart(int _x, int _y, int part_type, float velocity_x, float velocity_y, float explosion_force, GameContext* context);
    ~CorpsePart();

    void act(float deltaTime) override;
    void show() override;
    
    ObjectType get_type() const override { return CORPSE_PART; }

private:
    // Newtonian physics
    Vector2D position;
    Vector2D velocity;
    Vector2D acceleration;
    float mass; // Different body parts have different masses
    float drag_coefficient;
    float restitution; // Bounciness factor
    
    // Visceral effects
    std::vector<BloodTrail> blood_trails;
    float blood_emission_timer;
    float blood_emission_rate;
    
    // Advanced physics
    float air_density;
    float surface_area; // For drag calculation
    float moment_of_inertia;
    float angular_velocity;
    float angular_drag;
    float rotation;
    
    // Gore state
    float lifetime;
    float max_lifetime;
    int part_sprite;
    bool is_resting;
    float rest_timer;
    float viscosity_factor;
    
    // Physics methods
    void apply_force(const Vector2D& force);
    void apply_drag();
    void apply_gravity();
    void handle_collisions(float deltaTime);
    void update_blood_trail(float deltaTime);
    void emit_blood();
    
    // Visceral methods
    void render_blood_trails();
    float get_part_mass(int part_type);
    float get_part_surface_area(int part_type);
};

#endif