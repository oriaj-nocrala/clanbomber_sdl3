#include "ParticleEffectsManager.h"
#include "ClanBomber.h"
#include "GPUAcceleratedRenderer.h"
#include "Resources.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <cmath>

ParticleEffectsManager::ParticleEffectsManager(ClanBomberApplication* app) 
    : app(app) {
    SDL_Log("ParticleEffectsManager: Initialized centralized effects system");
}

ParticleEffectsManager::~ParticleEffectsManager() {
    SDL_Log("ParticleEffectsManager: Shutdown complete");
}

void ParticleEffectsManager::request_effect(const EffectRequest& request) {
    pending_effects.push_back(request);
}

void ParticleEffectsManager::update(float deltaTime) {
    for (const auto& effect : pending_effects) {
        switch (effect.type) {
            case EffectType::BOX_DESTRUCTION:
                process_box_destruction(effect.x, effect.y, effect.intensity);
                break;
            case EffectType::EXPLOSION:
                process_explosion(effect.x, effect.y, effect.intensity);
                break;
            default:
                break;
        }
    }
    pending_effects.clear();
}

void ParticleEffectsManager::render() {
}

void ParticleEffectsManager::create_box_destruction_effect(float x, float y, float intensity) {
    request_effect(EffectRequest(EffectType::BOX_DESTRUCTION, x, y, intensity));
}

void ParticleEffectsManager::create_explosion_effect(float x, float y, float intensity) {
    request_effect(EffectRequest(EffectType::EXPLOSION, x, y, intensity));
}

void ParticleEffectsManager::process_box_destruction(float x, float y, float intensity) {
    if (!app || !app->gpu_renderer) return;
    
    GLuint gl_texture = Resources::get_gl_texture("maptile_box");
    if (!gl_texture) return;
    
    try {
        app->gpu_renderer->begin_batch(GPUAcceleratedRenderer::TILE_FRAGMENTATION);
        
        const int num_fragments = static_cast<int>(12 * intensity);
        const float base_force = 35.0f * intensity;
        
        for (int i = 0; i < num_fragments; i++) {
            float angle = (float)i / num_fragments * 6.28318f;
            angle += (i * 127 + 31) % 100 / 100.0f - 0.5f;
            
            float fragment_scale = 0.2f + (i % 3) * 0.1f;
            float explosion_force = base_force + (i % 5) * 10.0f;
            
            float fragment_x = x + cos(angle) * explosion_force * 0.6f;
            float fragment_y = y + sin(angle) * explosion_force * 0.6f;
            
            float fragment_alpha = 0.8f - (i % 4) * 0.1f;
            
            float color[3] = {1.0f, 1.0f, 1.0f};
            app->gpu_renderer->add_sprite(
                fragment_x, fragment_y, 
                fragment_scale * 20, fragment_scale * 20,
                gl_texture, color
            );
        }
        
        app->gpu_renderer->end_batch();
        
    } catch (const std::exception& e) {
        SDL_Log("ParticleEffectsManager: Error creating box destruction effect: %s", e.what());
    }
}

void ParticleEffectsManager::process_explosion(float x, float y, float intensity) {
}