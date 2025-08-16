#ifndef PARTICLEEFFECTSMANAGER_H
#define PARTICLEEFFECTSMANAGER_H

#include <vector>
#include <memory>

class ClanBomberApplication;
class GPUAcceleratedRenderer;

enum class EffectType {
    BOX_DESTRUCTION,
    EXPLOSION,
    SMOKE,
    DEBRIS
};

struct EffectRequest {
    EffectType type;
    float x, y;
    float intensity;
    int tile_type;
    
    EffectRequest(EffectType t, float x_pos, float y_pos, float intens = 1.0f, int tile = 0) 
        : type(t), x(x_pos), y(y_pos), intensity(intens), tile_type(tile) {}
};

class ParticleEffectsManager {
public:
    ParticleEffectsManager(ClanBomberApplication* app);
    ~ParticleEffectsManager();
    
    void request_effect(const EffectRequest& request);
    void update(float deltaTime);
    void render();
    
    void create_box_destruction_effect(float x, float y, float intensity = 1.0f);
    void create_explosion_effect(float x, float y, float intensity = 1.0f);
    
private:
    ClanBomberApplication* app;
    std::vector<EffectRequest> pending_effects;
    
    void process_box_destruction(float x, float y, float intensity);
    void process_explosion(float x, float y, float intensity);
};

#endif