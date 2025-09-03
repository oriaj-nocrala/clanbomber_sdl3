#ifndef PARTICLE_SYSTEM_H
#define PARTICLE_SYSTEM_H

#include "GameObject.h"
#include <vector>
#include <random>

struct Particle {
    float x, y;
    float vel_x, vel_y;
    float life, max_life;
    float size;
    Uint8 r, g, b, a;
    float gravity;
    float drag;
};

enum ParticleType {
    EXPLOSION_SPARKS,
    DUST_CLOUDS,
    FIRE_PARTICLES,
    SMOKE_TRAILS
};

class ParticleSystem : public GameObject {
public:
    ParticleSystem(int _x, int _y, ParticleType type, GameContext* context);
    ~ParticleSystem();
    
    void act(float deltaTime) override;
    void show() override;
    ObjectType get_type() const override { return EXPLOSION; } // Reuse explosion type
    void reset_for_pool() override;
    
    // ObjectPool support - reinitializes object with new parameters
    void reinitialize(int _x, int _y, ParticleType type, GameContext* context);
    
    void emit_explosion_sparks(int count = 30);
    void emit_dust_cloud(int count = 20);
    void emit_fire_particles(int count = 25);
    void emit_smoke_trail(int count = 15);

private:
    std::vector<Particle> particles;
    ParticleType particle_type;
    float emission_timer;
    float emission_rate;
    bool continuous_emission;
    float system_lifetime;
    float max_lifetime;
    
    std::mt19937 random_gen;
    std::uniform_real_distribution<float> random_dist;
    
    void update_particles(float deltaTime);
    void render_particles();
    void create_particle(float x, float y, float vel_x, float vel_y, 
                        float life, float size, Uint8 r, Uint8 g, Uint8 b);
};

#endif