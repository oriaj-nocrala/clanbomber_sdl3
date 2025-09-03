#include "ParticleSystem.h"
#include "Timer.h"
#include "Resources.h"
#include "AudioMixer.h"
#include <cmath>
#include <algorithm>

ParticleSystem::ParticleSystem(int _x, int _y, ParticleType type, GameContext* context) 
    : GameObject(_x, _y, context), particle_type(type), random_gen(std::random_device{}()), random_dist(-1.0f, 1.0f) {
    
    emission_timer = 0.0f;
    emission_rate = 60.0f; // 60 particles per second
    continuous_emission = false;
    system_lifetime = 0.0f;
    max_lifetime = 3.0f; // System lasts 3 seconds
    
    z = Z_EXPLOSION; // Same layer as explosions
    
    // Initialize based on particle type
    switch (type) {
        case EXPLOSION_SPARKS:
            emit_explosion_sparks();
            max_lifetime = 1.5f;
            break;
        case DUST_CLOUDS:
            emit_dust_cloud();
            continuous_emission = true;
            emission_rate = 30.0f;
            max_lifetime = 2.0f;
            break;
        case FIRE_PARTICLES:
            emit_fire_particles();
            continuous_emission = true;
            emission_rate = 40.0f;
            max_lifetime = 2.5f;
            break;
        case SMOKE_TRAILS:
            emit_smoke_trail();
            continuous_emission = true;
            emission_rate = 20.0f;
            max_lifetime = 3.0f;
            break;
    }
}

ParticleSystem::~ParticleSystem() {
}

void ParticleSystem::reset_for_pool() {
    // Clear all particles and reset system state
    particles.clear();
    emission_timer = 0.0f;
    system_lifetime = 0.0f;
    delete_me = false;
    
    // Reset random generator
    random_gen.seed(std::random_device{}());
}

void ParticleSystem::reinitialize(int _x, int _y, ParticleType type, GameContext* context) {
    // First reset for pool to clear state
    reset_for_pool();
    
    // Reinitialize all members as if constructed fresh
    x = _x;
    y = _y;
    particle_type = type;
    set_game_context(context); // Update context reference
    
    emission_timer = 0.0f;
    emission_rate = 60.0f;
    continuous_emission = false;
    system_lifetime = 0.0f;
    max_lifetime = 3.0f;
    
    z = Z_EXPLOSION; // Same layer as explosions
    
    // Initialize based on particle type (same logic as constructor)
    switch (type) {
        case EXPLOSION_SPARKS:
            emit_explosion_sparks();
            max_lifetime = 1.5f;
            break;
        case DUST_CLOUDS:
            emit_dust_cloud();
            continuous_emission = true;
            emission_rate = 30.0f;
            max_lifetime = 2.0f;
            break;
        case FIRE_PARTICLES:
            emit_fire_particles();
            continuous_emission = true;
            emission_rate = 40.0f;
            max_lifetime = 2.5f;
            break;
        case SMOKE_TRAILS:
            emit_smoke_trail();
            continuous_emission = true;
            emission_rate = 20.0f;
            max_lifetime = 3.0f;
            break;
    }
}

void ParticleSystem::act(float deltaTime) {
    system_lifetime += deltaTime;
    
    // Update existing particles
    update_particles(deltaTime);
    
    // Emit new particles if continuous
    if (continuous_emission && system_lifetime < max_lifetime * 0.7f) {
        emission_timer += deltaTime;
        if (emission_timer > (1.0f / emission_rate)) {
            switch (particle_type) {
                case DUST_CLOUDS:
                    emit_dust_cloud(3);
                    break;
                case FIRE_PARTICLES:
                    emit_fire_particles(4);
                    break;
                case SMOKE_TRAILS:
                    emit_smoke_trail(2);
                    break;
                default:
                    break;
            }
            emission_timer = 0.0f;
        }
    }
    
    // Remove system when lifetime expires and no particles remain
    if (system_lifetime > max_lifetime && particles.empty()) {
        delete_me = true;
    }
}

void ParticleSystem::show() {
    render_particles();
}

void ParticleSystem::update_particles(float deltaTime) {
    for (auto it = particles.begin(); it != particles.end();) {
        Particle& p = *it;
        
        // Update life
        p.life -= deltaTime;
        
        if (p.life <= 0.0f) {
            it = particles.erase(it);
            continue;
        }
        
        // Update position
        p.x += p.vel_x * deltaTime;
        p.y += p.vel_y * deltaTime;
        
        // Apply gravity
        p.vel_y += p.gravity * deltaTime;
        
        // Apply drag
        p.vel_x *= (1.0f - p.drag * deltaTime);
        p.vel_y *= (1.0f - p.drag * deltaTime);
        
        // Update alpha based on remaining life
        float life_ratio = p.life / p.max_life;
        p.a = (Uint8)(255.0f * life_ratio);
        
        // Size can change over time
        if (particle_type == SMOKE_TRAILS) {
            p.size += 0.5f * deltaTime; // Smoke expands
        }
        
        ++it;
    }
}

void ParticleSystem::render_particles() {
    // TODO
}

void ParticleSystem::create_particle(float px, float py, float vel_x, float vel_y, 
                                   float life, float size, Uint8 r, Uint8 g, Uint8 b) {
    if (particles.size() < 200) { // Limit for performance
        Particle p;
        p.x = px;
        p.y = py;
        p.vel_x = vel_x;
        p.vel_y = vel_y;
        p.life = life;
        p.max_life = life;
        p.size = size;
        p.r = r;
        p.g = g;
        p.b = b;
        p.a = 255;
        p.gravity = 200.0f; // Default gravity
        p.drag = 0.5f; // Default drag
        
        particles.push_back(p);
    }
}

void ParticleSystem::emit_explosion_sparks(int count) {
    for (int i = 0; i < count; i++) {
        float angle = random_dist(random_gen) * M_PI * 2.0f;
        float velocity = 100.0f + random_dist(random_gen) * 150.0f;
        
        float vel_x = std::cos(angle) * velocity;
        float vel_y = std::sin(angle) * velocity - 50.0f; // Slight upward bias
        
        float offset_x = random_dist(random_gen) * 10.0f;
        float offset_y = random_dist(random_gen) * 10.0f;
        
        // Orange/yellow sparks
        Uint8 r = 255;
        Uint8 g = 150 + random_dist(random_gen) * 105;
        Uint8 b = 0;
        
        create_particle(x + offset_x, y + offset_y, vel_x, vel_y, 
                       0.5f + random_dist(random_gen) * 0.5f, 
                       2.0f + random_dist(random_gen) * 2.0f, r, g, b);
    }
}

void ParticleSystem::emit_dust_cloud(int count) {
    for (int i = 0; i < count; i++) {
        float angle = random_dist(random_gen) * M_PI * 2.0f;
        float velocity = 30.0f + random_dist(random_gen) * 40.0f;
        
        float vel_x = std::cos(angle) * velocity;
        float vel_y = std::sin(angle) * velocity;
        
        float offset_x = random_dist(random_gen) * 15.0f;
        float offset_y = random_dist(random_gen) * 15.0f;
        
        // Brown/gray dust
        Uint8 gray = 100 + random_dist(random_gen) * 50;
        
        Particle p;
        p.x = x + offset_x;
        p.y = y + offset_y;
        p.vel_x = vel_x;
        p.vel_y = vel_y;
        p.life = 1.5f + random_dist(random_gen) * 1.0f;
        p.max_life = p.life;
        p.size = 3.0f + random_dist(random_gen) * 2.0f;
        p.r = gray + 20;
        p.g = gray;
        p.b = gray - 20;
        p.a = 255;
        p.gravity = 50.0f; // Less gravity for dust
        p.drag = 1.5f; // More drag
        
        particles.push_back(p);
    }
}

void ParticleSystem::emit_fire_particles(int count) {
    for (int i = 0; i < count; i++) {
        float angle = random_dist(random_gen) * M_PI * 0.5f - M_PI * 0.25f; // Upward bias
        float velocity = 60.0f + random_dist(random_gen) * 80.0f;
        
        float vel_x = std::cos(angle) * velocity;
        float vel_y = std::sin(angle) * velocity - 80.0f; // Strong upward motion
        
        float offset_x = random_dist(random_gen) * 8.0f;
        float offset_y = random_dist(random_gen) * 8.0f;
        
        // Fire colors (red to yellow)
        Uint8 r = 255;
        Uint8 g = 100 + random_dist(random_gen) * 155;
        Uint8 b = random_dist(random_gen) * 50;
        
        Particle p;
        p.x = x + offset_x;
        p.y = y + offset_y;
        p.vel_x = vel_x;
        p.vel_y = vel_y;
        p.life = 0.8f + random_dist(random_gen) * 0.7f;
        p.max_life = p.life;
        p.size = 2.5f + random_dist(random_gen) * 1.5f;
        p.r = r;
        p.g = g;
        p.b = b;
        p.a = 255;
        p.gravity = -50.0f; // Negative gravity (buoyancy)
        p.drag = 0.8f;
        
        particles.push_back(p);
    }
}

void ParticleSystem::emit_smoke_trail(int count) {
    for (int i = 0; i < count; i++) {
        float angle = random_dist(random_gen) * M_PI * 0.3f - M_PI * 0.15f; // Slight upward bias
        float velocity = 20.0f + random_dist(random_gen) * 30.0f;
        
        float vel_x = std::cos(angle) * velocity;
        float vel_y = std::sin(angle) * velocity - 40.0f; // Upward motion
        
        float offset_x = random_dist(random_gen) * 12.0f;
        float offset_y = random_dist(random_gen) * 12.0f;
        
        // Gray smoke
        Uint8 gray = 60 + random_dist(random_gen) * 40;
        
        Particle p;
        p.x = x + offset_x;
        p.y = y + offset_y;
        p.vel_x = vel_x;
        p.vel_y = vel_y;
        p.life = 2.0f + random_dist(random_gen) * 1.5f;
        p.max_life = p.life;
        p.size = 4.0f + random_dist(random_gen) * 3.0f;
        p.r = gray;
        p.g = gray;
        p.b = gray;
        p.a = 255;
        p.gravity = -20.0f; // Slight buoyancy
        p.drag = 0.3f; // Low drag
        
        particles.push_back(p);
    }
}