#ifndef GPU_PARTICLE_SYSTEM_H
#define GPU_PARTICLE_SYSTEM_H

#include "GameObject.h"
#include "GPURenderer.h"

class GPUParticleSystem : public GameObject {
public:
    GPUParticleSystem(int _x, int _y, ParticleType type, GPURenderer* renderer, ClanBomberApplication* app);
    ~GPUParticleSystem();
    
    void act(float deltaTime) override;
    void show() override;
    ObjectType get_type() const override { return EXPLOSION; }
    
    void emit_gpu_explosion(int count = 50);
    void emit_gpu_blood(int count = 30);
    void emit_gpu_sparks(int count = 25);

private:
    GPURenderer* gpu_renderer;
    ParticleType particle_type;
    float emission_timer;
    float system_lifetime;
    float max_lifetime;
    bool continuous_emission;
    int particles_emitted;
    int max_particles_per_system;
};

#endif