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
    if (!app) return;
    
    // TEMPORARILY DISABLED: Particle effects - will be migrated to RenderingFacade
    SDL_Log("ParticleEffectsManager: Box destruction effect disabled during renderer migration");
    
    /*
    GLuint gl_texture = Resources::get_gl_texture("maptile_box");
    if (!gl_texture) return;
    
    try {
        // Will be migrated to use RenderingFacade instead of direct GPU renderer
        
    } catch (const std::exception& e) {
        SDL_Log("ParticleEffectsManager: Error creating box destruction effect: %s", e.what());
    }
    */
}

void ParticleEffectsManager::process_explosion(float x, float y, float intensity) {
}